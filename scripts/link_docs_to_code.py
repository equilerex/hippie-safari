# scripts/link_docs_to_code.py
import os
import re
import json
from pathlib import Path

def get_cpp_symbols_and_files(root_dir):
    """
    Scans src/ and include/ to find C++ filenames and declared classes/structs/enums.
    """
    symbols = set()
    files = set()
    
    # Regex to find class, struct, and enum declarations
    class_re = re.compile(r'\b(?:class|struct)\s+([A-Za-z0-9_]+)\b')
    enum_re = re.compile(r'\benum\s+(?:class\s+)?([A-Za-z0-9_]+)\b')
    
    scan_dirs = ['src', 'include']
    for d in scan_dirs:
        dir_path = Path(root_dir) / d
        if not dir_path.exists():
            continue
            
        for file_path in dir_path.rglob('*'):
            if file_path.is_file() and file_path.suffix in ['.h', '.cpp', '.hpp', '.c']:
                # Track the file name itself (both full name and without extension)
                files.add(file_path.name)
                files.add(file_path.stem)
                
                try:
                    content = file_path.read_text(encoding='utf-8', errors='ignore')
                    # Find class/struct names
                    for match in class_re.finditer(content):
                        symbol = match.group(1)
                        if symbol not in ('alignas', 'void', 'int', 'bool', 'char', 'float'):
                            symbols.add(symbol)
                    # Find enum names
                    for match in enum_re.finditer(content):
                        symbol = match.group(1)
                        symbols.add(symbol)
                except Exception as e:
                    print(f"Warning: Could not read {file_path}: {e}")
                    
    return list(symbols), list(files)

def parse_markdown_for_links(root_dir, cpp_symbols, cpp_files):
    """
    Scans docs/ and root README.md for mentions of C++ symbols and filenames.
    """
    docs_to_code = {}
    code_to_docs = {}
    
    # Find markdown files
    md_files = []
    docs_dir = Path(root_dir) / 'docs'
    if docs_dir.exists():
        md_files.extend(docs_dir.rglob('*.md'))
        
    readme_path = Path(root_dir) / 'README.md'
    if readme_path.exists():
        md_files.append(readme_path)
        
    # Combine symbols and filenames to search for
    all_targets = set(cpp_symbols + cpp_files)
    
    # We want to match whole words or backticked expressions
    # To do this efficiently, we can check for exact word matches using boundary regex
    for md_path in md_files:
        try:
            content = md_path.read_text(encoding='utf-8', errors='ignore')
            relative_path = os.path.relpath(md_path, root_dir).replace('\\', '/')
            
            referenced = set()
            
            # 1. Search for items in backticks e.g., `SystemManagerImpl`
            backtick_matches = re.findall(r'`([A-Za-z0-9_.-]+)`', content)
            for match in backtick_matches:
                # Strip extension to match file stems or symbols
                stem = Path(match).stem
                if match in all_targets:
                    referenced.add(match)
                elif stem in all_targets:
                    referenced.add(stem)
                    
            # 2. Search for any exact word boundary match of our targets in the doc
            for target in all_targets:
                if target in referenced:
                    continue
                # Simple boundary check
                pattern = r'\b' + re.escape(target) + r'\b'
                if re.search(pattern, content):
                    referenced.add(target)
                    
            if referenced:
                sorted_refs = sorted(list(referenced))
                docs_to_code[relative_path] = sorted_refs
                
                for ref in sorted_refs:
                    if ref not in code_to_docs:
                        code_to_docs[ref] = []
                    code_to_docs[ref].append(relative_path)
                    
        except Exception as e:
            print(f"Warning: Could not read markdown {md_path}: {e}")
            
    return docs_to_code, code_to_docs

def main():
    root_dir = Path(__file__).resolve().parent.parent
    output_dir = root_dir / 'graphify-out'
    output_dir.mkdir(parents=True, exist_ok=True)
    
    print("Scanning C++ codebase for symbols...")
    symbols, files = get_cpp_symbols_and_files(root_dir)
    print(f"Found {len(symbols)} C++ symbols and {len(files)} file references.")
    
    print("Parsing documentation files...")
    docs_to_code, code_to_docs = parse_markdown_for_links(root_dir, symbols, files)
    
    output_data = {
        "docs_to_code": docs_to_code,
        "code_to_docs": code_to_docs,
        "meta": {
            "total_documents_linked": len(docs_to_code),
            "total_symbols_linked": len(code_to_docs)
        }
    }
    
    output_file = output_dir / 'docs_to_code_map.json'
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(output_data, f, indent=2, ensure_ascii=False)
        
    print(f"Successfully generated doc-to-code mapping: {output_file}")
    print(f"Linked {len(docs_to_code)} markdown files with C++ code symbols.")

if __name__ == "__main__":
    main()
