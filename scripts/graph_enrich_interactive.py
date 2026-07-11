# scripts/graph_enrich_interactive.py
import sys
import os
import re
import time
import subprocess
import threading
import json
from pathlib import Path

def find_ollama_log():
    """
    Finds the Ollama server.log file depending on the OS.
    """
    # Windows
    if os.name == 'nt':
        log_path = Path.home() / 'AppData' / 'Local' / 'Ollama' / 'server.log'
        if log_path.exists():
            return log_path
    # macOS
    else:
        log_path = Path.home() / 'Library' / 'Logs' / 'Ollama' / 'server.log'
        if log_path.exists():
            return log_path
            
    return None

def monitor_log(log_path, stop_event):
    """
    Tails the Ollama log file and prints parsed generation stats in real-time.
    """
    try:
        # Open log and seek to end
        with open(log_path, 'r', encoding='utf-8', errors='ignore') as f:
            f.seek(0, os.SEEK_END)
            
            prompt_process_re = re.compile(r'prompt processing, n_tokens =\s*(\d+),\s*progress =\s*([\d.]+)')
            timing_re = re.compile(r'n_decoded =\s*(\d+),\s*tg =\s*([\d.]+)\s*t/s')
            cache_re = re.compile(r'cached n_tokens =\s*(\d+)')
            
            last_status = ""
            while not stop_event.is_set():
                line = f.readline()
                if not line:
                    time.sleep(0.2)
                    continue
                
                # Check for prompt processing progress
                prompt_match = prompt_process_re.search(line)
                if prompt_match:
                    tokens = prompt_match.group(1)
                    progress = float(prompt_match.group(2)) * 100
                    status = f"  [Ollama] Processing prompt: {progress:.1f}% ({tokens} tokens)"
                    if status != last_status:
                        sys.stdout.write(f"\r{status:<80}")
                        sys.stdout.flush()
                        last_status = status
                    continue
                
                # Check for token generation speed
                timing_match = timing_re.search(line)
                if timing_match:
                    decoded = timing_match.group(1)
                    speed = timing_match.group(2)
                    status = f"  [Ollama] Generating: {decoded} tokens ({speed} t/s)"
                    if status != last_status:
                        sys.stdout.write(f"\r{status:<80}")
                        sys.stdout.flush()
                        last_status = status
                    continue
                
                # Check for cache hits
                cache_match = cache_re.search(line)
                if cache_match:
                    tokens = cache_match.group(1)
                    status = f"  [Ollama] Prompt cache hit! ({tokens} tokens)"
                    if status != last_status:
                        sys.stdout.write(f"\r{status:<80}")
                        sys.stdout.flush()
                        last_status = status
                    continue

    except Exception as e:
        # Fail silently for monitoring thread
        pass

def update_savings(input_tokens, output_tokens, output_dir):
    """
    Updates the local_llm_savings.json log file and prints a fun savings report.
    """
    savings_file = Path(output_dir) / 'local_llm_savings.json'
    
    # Rates per 1M tokens (Claude 3.5 Sonnet vs GPT-4o)
    claude_input_rate = 3.00 / 1_000_000
    claude_output_rate = 15.00 / 1_000_000
    gpt_input_rate = 5.00 / 1_000_000
    gpt_output_rate = 15.00 / 1_000_000
    
    this_claude = (input_tokens * claude_input_rate) + (output_tokens * claude_output_rate)
    this_gpt = (input_tokens * gpt_input_rate) + (output_tokens * gpt_output_rate)
    
    data = None
    if savings_file.exists():
        try:
            data = json.loads(savings_file.read_text(encoding='utf-8'))
        except Exception:
            pass
            
    if not data:
        data = {
            "total_runs": 0,
            "total_input_tokens": 0,
            "total_output_tokens": 0,
            "total_savings_usd": 0.0,
            "claude_equivalent_cost": 0.0,
            "gpt_equivalent_cost": 0.0,
            "history": []
        }
        
    data["total_runs"] += 1
    data["total_input_tokens"] += input_tokens
    data["total_output_tokens"] += output_tokens
    data["claude_equivalent_cost"] += this_claude
    data["gpt_equivalent_cost"] += this_gpt
    data["total_savings_usd"] += this_claude # Default to Claude 3.5 Sonnet
    
    from datetime import datetime, timezone
    data["history"].append({
        "date": datetime.now(timezone.utc).isoformat(),
        "input_tokens": input_tokens,
        "output_tokens": output_tokens,
        "claude_cost": this_claude,
        "gpt_cost": this_gpt
    })
    
    try:
        savings_file.parent.mkdir(parents=True, exist_ok=True)
        savings_file.write_text(json.dumps(data, indent=2), encoding='utf-8')
    except Exception as e:
        print(f"Warning: Could not write savings file: {e}")
        
    # Print the fun savings report
    sys.stdout.write("\n" + "="*80 + "\n")
    sys.stdout.write("💰 LOCAL LLM COST SAVINGS REPORT (Claude 3.5 Sonnet Benchmark)\n")
    sys.stdout.write("="*80 + "\n")
    sys.stdout.write(f"  This Run:\n")
    sys.stdout.write(f"    - Input Tokens:          {input_tokens:,}\n")
    sys.stdout.write(f"    - Output Tokens:         {output_tokens:,}\n")
    sys.stdout.write(f"    - Cloud Cost Equivalent: ${this_claude:.4f}\n")
    sys.stdout.write(f"    - Local Ollama Cost:     $0.0000\n")
    sys.stdout.write(f"    - Savings:               ${this_claude:.4f} ✅\n\n")
    sys.stdout.write(f"  Accumulated Savings (All Runs):\n")
    sys.stdout.write(f"    - Total Runs:            {data['total_runs']}\n")
    sys.stdout.write(f"    - Total Tokens:          {data['total_input_tokens'] + data['total_output_tokens']:,}\n")
    sys.stdout.write(f"    - Total Savings:         ${data['total_savings_usd']:.4f} 💵\n")
    sys.stdout.write("="*80 + "\n\n")
    sys.stdout.flush()

def main():
    # Pass all arguments to graphify extract
    args = sys.argv[1:]
    
    root_dir = Path(__file__).resolve().parent.parent
    output_dir = root_dir / 'graphify-out'
    
    # Locate Ollama log file
    log_path = find_ollama_log()
    
    # Construct command: run the locally installed graphify command
    cmd = ['graphify', 'extract'] + args
    
    # Inject PYTHONUNBUFFERED=1 to ensure Python outputs immediately
    env = os.environ.copy()
    env['PYTHONUNBUFFERED'] = '1'
    
    # Load .env file variables to pass to graphify
    env_path = root_dir / '.env'
    if env_path.exists():
        with open(env_path, 'r', encoding='utf-8', errors='ignore') as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                match = re.match(r'^(?:export\s+)?([\w.-]+)\s*=\s*(.*)$', line)
                if match:
                    key = match.group(1)
                    val = match.group(2).strip()
                    if (val.startswith('"') and val.endswith('"')) or (val.startswith("'") and val.endswith("'")):
                        val = val[1:-1]
                    env[key] = val
    
    # Start Ollama log monitor if available
    stop_event = threading.Event()
    monitor_thread = None
    if log_path:
        monitor_thread = threading.Thread(target=monitor_log, args=(log_path, stop_event), daemon=True)
        monitor_thread.start()
        
    print(f"Executing: {' '.join(cmd)}")
    if log_path:
        print(f"Monitoring Ollama server logs at: {log_path}")
    else:
        print("Ollama server.log not found, progress monitoring disabled.")
        
    try:
        # Start the graphify subprocess
        # We capture stdout to parse token counts and errors
        # We inherit stderr so tqdm progress bar writes directly to TTY
        process = subprocess.Popen(
            cmd,
            env=env,
            stdout=subprocess.PIPE,
            stderr=None,
            text=True,
            bufsize=1
        )
        
        # Read and print stdout line by line
        input_tokens = 0
        output_tokens = 0
        model_error_detected = False
        
        for line in process.stdout:
            # If we were printing an Ollama progress status line, print a carriage return to clear it
            if monitor_thread and monitor_thread.is_alive():
                sys.stdout.write("\r" + " " * 80 + "\r")
                sys.stdout.flush()
            sys.stdout.write(line)
            sys.stdout.flush()
            
            # Parse token count
            # Example: [graphify extract] tokens: 17,443 in / 7,274 out, est. cost (~ollama): $0.0000
            token_match = re.search(r'tokens:\s*([\d,]+)\s*in\s*/\s*([\d,]+)\s*out', line)
            if token_match:
                input_tokens = int(token_match.group(1).replace(',', ''))
                output_tokens = int(token_match.group(2).replace(',', ''))
                
            # Check for model not found errors
            if not model_error_detected and ("not found" in line.lower() or "not_found_error" in line.lower()):
                model_error_detected = True
            
        process.wait()
        
        # If the process completed successfully and we got tokens, log the savings
        if process.returncode == 0 and (input_tokens > 0 or output_tokens > 0):
            update_savings(input_tokens, output_tokens, output_dir)
            
        # If the process failed, check if the model is missing from Ollama
        elif process.returncode != 0:
            model_name = "qwen2.5-coder:7b"
            if '--model' in args:
                idx = args.index('--model')
                if idx + 1 < len(args):
                    model_name = args[idx+1]
            
            try:
                res = subprocess.run(['ollama', 'list'], capture_output=True, text=True)
                if res.returncode == 0 and model_name not in res.stdout:
                    sys.stdout.write("\n" + "="*80 + "\n")
                    sys.stdout.write(f"ERROR: Model '{model_name}' was not found in your local Ollama setup.\n")
                    sys.stdout.write(f"Please install it by running the following command in your terminal:\n\n")
                    sys.stdout.write(f"  ollama pull {model_name}\n")
                    sys.stdout.write("="*80 + "\n\n")
                    sys.stdout.flush()
            except Exception:
                pass
        
    except KeyboardInterrupt:
        print("\nAborting extraction...")
        if 'process' in locals():
            process.terminate()
    finally:
        # Stop log monitor
        stop_event.set()
        if monitor_thread:
            monitor_thread.join(timeout=1.0)
        print() # Clear line

if __name__ == "__main__":
    main()
