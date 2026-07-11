# scripts/serena_sync_memories.py
import os
import re
import json
import hashlib
import shutil
import subprocess
import urllib.request
import urllib.error
from pathlib import Path

def get_file_hash(file_path):
    """
    Returns the MD5 hash of a file.
    """
    hasher = hashlib.md5()
    try:
        with open(file_path, 'rb') as f:
            buf = f.read()
            hasher.update(buf)
        return hasher.hexdigest()
    except Exception:
        return None

def load_env(root_dir):
    """
    Loads environment variables from the root .env file.
    """
    env = {}
    env_path = Path(root_dir) / '.env'
    if env_path.exists():
        try:
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
        except Exception as e:
            print(f"Warning: Could not read .env: {e}")
    return env

def select_model(env_model):
    """
    Queries local Ollama models and selects a valid downloaded one, falling back gracefully.
    """
    default_fallback = 'qwen2.5-coder:7b'
    try:
        res = subprocess.run(['ollama', 'list'], capture_output=True, text=True)
        if res.returncode != 0:
            return env_model or default_fallback
            
        models = []
        for line in res.stdout.split('\n')[1:]: # Skip header
            parts = line.strip().split()
            if parts:
                models.append(parts[0])
                
        if not models:
            return env_model or default_fallback
            
        # If the requested env model is already downloaded, use it
        if env_model:
            # Check for exact match or suffix match (e.g. qwen2.5-coder:7b vs qwen2.5-coder:7b-instruct)
            for m in models:
                if m == env_model or m.startswith(env_model + ":") or env_model.startswith(m + ":"):
                    return m
                    
        # Check standard fallbacks in order of preference
        fallbacks = [default_fallback, 'qwen2.5-coder:1.5b-base', 'qwen2.5-coder:1.5b', 'qwen2.5-coder:32b']
        for fb in fallbacks:
            for m in models:
                if m.startswith(fb):
                    return m
                    
        # Look for any model containing coder or qwen
        for m in models:
            if 'coder' in m.lower() or 'qwen' in m.lower():
                return m
                
        # Default to first available local model
        return models[0]
    except Exception:
        return env_model or default_fallback

def call_ollama(api_base, model, system_prompt, user_prompt):
    """
    Calls Ollama's OpenAI-compatible endpoint using urllib (no external dependencies).
    """
    url = f"{api_base.rstrip('/')}/chat/completions"
    headers = {
        "Content-Type": "application/json"
    }
    payload = {
        "model": model,
        "messages": [
            {"role": "system", "content": system_prompt},
            {"role": "user", "content": user_prompt}
        ],
        "temperature": 0.2
    }
    req = urllib.request.Request(
        url,
        data=json.dumps(payload).encode('utf-8'),
        headers=headers,
        method='POST'
    )
    try:
        with urllib.request.urlopen(req, timeout=30) as response:
            res_data = json.loads(response.read().decode('utf-8'))
            return res_data['choices'][0]['message']['content'].strip()
    except urllib.error.URLError as e:
        print(f"Connection error to Ollama ({url}): {e}")
        return None
    except Exception as e:
        print(f"Error communicating with Ollama: {e}")
        return None

def extract_classes_from_header(file_path):
    """
    Extracts class/struct names and their declarations from a C++ header.
    """
    try:
        content = Path(file_path).read_text(encoding='utf-8', errors='ignore')
        # Simple extraction of class declarations and surrounding comments
        class_matches = re.findall(r'(\/\*\*[\s\S]*?\*\/)?\s*\b(?:class|struct)\s+([A-Za-z0-9_]+)\b', content)
        classes = []
        for doc, name in class_matches:
            if name not in ('alignas', 'void', 'int', 'bool', 'char', 'float'):
                classes.append((name, doc.strip() if doc else ""))
        return classes
    except Exception:
        return []

def main():
    root_dir = Path(__file__).resolve().parent.parent
    
    # 1. Sync .ai/memory files to .serena/memories
    ai_mem_dir = root_dir / '.ai' / 'memory'
    serena_mem_dir = root_dir / '.serena' / 'memories'
    serena_mem_dir.mkdir(parents=True, exist_ok=True)
    
    print("Syncing .ai/memory files to Serena memories...")
    if ai_mem_dir.exists():
        for f in ai_mem_dir.glob('*.md'):
            dest = serena_mem_dir / f.name
            try:
                shutil.copy2(f, dest)
                print(f"  Copied: {f.name} -> .serena/memories/{f.name}")
            except Exception as e:
                print(f"  Failed to copy {f.name}: {e}")
                
    # 2. Setup Ollama context from env
    env = load_env(root_dir)
    api_base = env.get('OPENAI_BASE_URL') or env.get('OPENAI_API_BASE') or 'http://localhost:11434/v1'
    
    # Determine the model name by inspecting local Ollama list
    requested_model = env.get('GRAPHIFY_MODEL')
    model = select_model(requested_model)
    print(f"Using Ollama model for C++ summaries: {model}")
    
    # 3. Load incremental cache
    cache_path = root_dir / 'graphify-out' / 'serena_sync_cache.json'
    cache = {}
    if cache_path.exists():
        try:
            cache = json.loads(cache_path.read_text(encoding='utf-8'))
        except Exception:
            pass
            
    # 4. Scan C++ headers for classes to summarize
    include_dir = root_dir / 'include'
    headers = list(include_dir.glob('*.h')) + list(include_dir.glob('*.hpp'))
    src_dir = root_dir / 'src'
    if src_dir.exists():
        headers.extend(list(src_dir.glob('*.h')) + list(src_dir.glob('*.hpp')))
        
    class_summaries = {}
    # Load previous summaries if they exist
    summaries_file = serena_mem_dir / 'cpp_module_summaries.md'
    existing_summaries_content = ""
    if summaries_file.exists():
        try:
            existing_summaries_content = summaries_file.read_text(encoding='utf-8')
            # Extract previous summaries using regex
            matches = re.finditer(r'### ([A-Za-z0-9_]+)\n\n([\s\S]*?)(?=\n\n### |\Z)', existing_summaries_content)
            for m in matches:
                class_summaries[m.group(1)] = m.group(2).strip()
        except Exception:
            pass
            
    has_changes = False
    system_prompt = (
        "You are an expert C++ software architect. "
        "Your task is to write a concise, single-paragraph functional summary of a C++ class based on its name and its header declaration. "
        "Do not include code snippets, greetings, or explanations. Start directly with the summary."
    )
    
    for h in headers:
        h_hash = get_file_hash(h)
        rel_path = os.path.relpath(h, root_dir).replace('\\', '/')
        
        # Check cache
        if cache.get(rel_path) == h_hash:
            continue
            
        classes = extract_classes_from_header(h)
        if not classes:
            cache[rel_path] = h_hash
            continue
            
        print(f"Analyzing {h.name} for Serena context summaries...")
        for name, doc in classes:
            user_prompt = f"Write a functional summary for the C++ class '{name}' in the file '{h.name}'.\n"
            if doc:
                user_prompt += f"Class documentation comment:\n{doc}\n"
                
            try:
                header_snippet = h.read_text(encoding='utf-8', errors='ignore')
                # Keep first 150 lines of header to avoid overflowing context
                header_lines = header_snippet.split('\n')[:150]
                user_prompt += f"\nHeader file declaration:\n```cpp\n" + '\n'.join(header_lines) + "\n```"
            except Exception:
                pass
                
            summary = call_ollama(api_base, model, system_prompt, user_prompt)
            if summary:
                class_summaries[name] = summary
                print(f"  Generated summary for class: {name}")
                has_changes = True
                
        cache[rel_path] = h_hash
        
    # Write summaries to .serena/memories/cpp_module_summaries.md
    if has_changes or not summaries_file.exists():
        content = (
            "# C++ Module and Class Summaries\n\n"
            "This file provides functional summaries of the C++ classes and structures "
            "composing the Hippy Safari firmware, generated via local Ollama.\n\n"
        )
        for name in sorted(class_summaries.keys()):
            content += f"### {name}\n\n{class_summaries[name]}\n\n"
            
        try:
            summaries_file.write_text(content, encoding='utf-8')
            print(f"Saved: {summaries_file.name} -> .serena/memories/{summaries_file.name}")
        except Exception as e:
            print(f"Error saving summaries file: {e}")
            
    # Save incremental cache
    try:
        cache_path.parent.mkdir(parents=True, exist_ok=True)
        cache_path.write_text(json.dumps(cache, indent=2), encoding='utf-8')
    except Exception as e:
        print(f"Warning: Could not save cache: {e}")
        
    print("Serena context update completed.")

if __name__ == "__main__":
    main()
