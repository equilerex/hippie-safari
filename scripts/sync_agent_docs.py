# scripts/sync_agent_docs.py
import os
import re
import shutil
from pathlib import Path

def main():
    root_dir = Path(__file__).resolve().parent.parent
    source_file = root_dir / 'agents-source.md'
    
    if not source_file.exists():
        print(f"Error: {source_file.name} not found in the root directory!")
        return
        
    source_content = source_file.read_text(encoding='utf-8')
    
    # Extract blocks using regex
    # Matches: <!-- BLOCK: NAME --> content <!-- ENDBLOCK: NAME -->
    block_pattern = re.compile(r'<!-- BLOCK:\s*(\w+)\s*-->([\s\S]*?)<!-- ENDBLOCK:\s*\1\s*-->')
    blocks = {}
    
    for match in block_pattern.finditer(source_content):
        name = match.group(1).upper()
        content = match.group(2).strip()
        blocks[name] = content
        
    common_content = blocks.get('COMMON', '')
    if not common_content:
        print("Warning: COMMON block was not found or is empty in agents-source.md")
        
    targets = {
        'CLAUDE.md': {
            'dest': root_dir / 'CLAUDE.md',
            'extra': blocks.get('CLAUDE', ''),
            'title': '# Claude Instructions'
        },
        'GEMINI.md': {
            'dest': root_dir / 'GEMINI.md',
            'extra': blocks.get('GEMINI', ''),
            'title': '# Gemini Instructions'
        },
        '.github/copilot-instructions.md': {
            'dest': root_dir / '.github' / 'copilot-instructions.md',
            'extra': blocks.get('COPILOT', ''),
            'title': '# GitHub Copilot Instructions'
        }
    }
    
    print("Syncing agent instruction files from agents-source.md...")
    for name, cfg in targets.items():
        dest = cfg['dest']
        extra = cfg['extra']
        # Build compiled file content
        content = ""
        if cfg.get('title'):
            content += cfg['title'] + "\n\n"
        content += common_content
        content += "\n\n"
        
        if extra:
            content += extra
            content += "\n"
            print(f"  Synced: {name} (with custom extras)")
        else:
            print(f"  Synced: {name} (generic)")
            
        try:
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_text(content, encoding='utf-8')
        except Exception as e:
            print(f"  Error writing to {name}: {e}")
            
    # 4. Clean up old files/directories
    old_agents = root_dir / 'AGENTS.md'
    if old_agents.exists():
        try:
            old_agents.unlink()
            print(f"Removed old root file: {old_agents.name}")
        except Exception as e:
            print(f"Failed to remove {old_agents.name}: {e}")
            
    old_instructions_dir = root_dir / '.ai' / 'instructions'
    if old_instructions_dir.exists():
        try:
            shutil.rmtree(old_instructions_dir)
            print("Removed old instructions template directory (.ai/instructions/)")
        except Exception as e:
            print(f"Failed to remove {old_instructions_dir.name}: {e}")
            
    print("Agent instructions sync completed successfully.")

if __name__ == '__main__':
    main()
