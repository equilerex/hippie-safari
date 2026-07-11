# AI-Assisted Development & Workflow Experiments Guide

This project serves as a testbed for cost-efficient, local, and deterministic AI developer workflows. It demonstrates how to combine local LLMs (via Ollama) with static analysis tools (Graphify, Serena) and git hooks to empower coding agents while maintaining speed, zero API costs, and code privacy.

---

## 🏗️ Workflow Architecture Layers

The AI development workflow is divided into four distinct layers:

```mermaid
graph TD
    A[C++ Codebase & Docs] --> B[Structural Layer: Tree-Sitter]
    A --> C[Documentation Linker: Regex]
    A --> D[Semantic Layer: Local LLM]
    A --> E[Agent Context Layer: Serena]
    
    B -->|npm run graph:update| B1[graphify-out/graph.json]
    C -->|npm run graph:link| C1[graphify-out/docs_to_code_map.json]
    D -->|npm run graph:enrich| D1[graphify-out/local_llm_savings.json]
    E -->|npm run serena:sync| E1[.serena/memories/]
    
    subgraph Git Hooks (Pre-commit)
        B
        E
    end
```

### 1. The Structural Layer (Deterministic & Free)
* **Goal**: Maintain an up-to-date syntax and dependency tree of the codebase.
* **Tool**: `graphify update`
* **How it works**: Uses Tree-Sitter to parse symbols, imports, and definitions in milliseconds without calling any LLMs.
* **Automation**: Configured in `.husky/pre-commit` to scan staged C++ files and update the structural index as a silent, fast (<2s) side effect of committing code.

### 2. The Documentation Mapping Layer (Regex & Free)
* **Goal**: Establish relationships between technical documentation and source files.
* **Tool**: `scripts/link_docs_to_code.py`
* **How it works**: Scans C++ header declarations for symbols and searches markdown docs for matching terms, creating a JSON dictionary lookup at `graphify-out/docs_to_code_map.json`.
* **Value**: Enables LLM agents to instantly locate which doc corresponds to a given class/struct without needing to read every markdown file.

### 3. The Semantic Enrichment Layer (LLM-driven & On-demand)
* **Goal**: Summarize system modules, detect architectural communities, and extract complex relationships.
* **Tool**: `graphify extract` wrapped by `scripts/graph_enrich_interactive.py`.
* **How it works**: Feeds parsed file chunks to a local Ollama model (e.g. `qwen2.5-coder:7b` or `qwen2.5-coder:32b`) using the OpenAI-compatible client endpoint to extract high-level semantic meaning.
* **Features**:
  * **Interactive Tailing**: Reads Ollama's `server.log` to stream prompt processing percentages and generation speeds to the console while preserving the native terminal progress bar.
  * **Cost Tracker**: Captures the processed token count and logs accumulated "cloud cost savings" inside `graphify-out/local_llm_savings.json` based on Claude 3.5 Sonnet and GPT-4o prices.

### 4. The Agent Context Layer (Serena MCP)
* **Goal**: Feed facts, rules, and class summaries directly to active IDE agents.
* **Tool**: `serena` wrapped by `scripts/serena_sync_memories.py`.
* **How it works**: Copies `.ai/memory/` markdown files to `.serena/memories/` and utilizes Ollama to write concise 1-paragraph summaries of C++ class declarations into `.serena/memories/cpp_module_summaries.md` using file-hash caching for incremental updates.

### 5. Instruction Files Synchronization Layer (Multi-LLM)
* **Goal**: Keep all IDE agent instruction files synced to a single master file, while appending LLM-specific customizations.
* **Tool**: `scripts/sync_agent_docs.py`.
* **How it works**: Treats [agents-source.md](file:///d:/ESP32%20projects/hippie%20safari/agents-source.md) in the root as the single master source. It parses the block-separator syntax (e.g., `<!-- BLOCK: COMMON -->`, `<!-- BLOCK: CLAUDE -->`, etc.) and compiles custom target outputs to `CLAUDE.md`, `GEMINI.md`, and `.github/copilot-instructions.md`.
* **Automation**: Configured in `.husky/pre-commit` to automatically run whenever `agents-source.md` is modified.

---

## 🛠️ CLI Reference Commands

These commands are configured inside `package.json` with Node wrappers to run shell-independently on Windows (CMD, PowerShell, Git Bash) and Unix:

| Command | Action | Frequency | Cost |
|---------|--------|-----------|------|
| `npm run graph:update` | Updates C++ file structural nodes and AST relationships | Automatic (Pre-commit on code change) | \$0.00 |
| `npm run serena:sync` | Syncs `.ai` facts & generates Ollama class summaries for Serena | Automatic (Pre-commit on code change) | \$0.00 (Local LLM) |
| `npm run agents:sync` | Compiles block sections from `agents-source.md` to target files | Automatic (Pre-commit on doc change) | \$0.00 |
| `npm run graph:link` | Generates the C++ symbol-to-document mapping JSON file | Manual (On-demand) | \$0.00 |
| `npm run graph:enrich` | Semantic extraction using the fast `qwen2.5-coder:7b` | Manual (On-demand) | \$0.00 (Local LLM) |
| `npm run graph:enrich:32b` | Semantic extraction using the heavy `qwen2.5-coder:32b` | Manual (On-demand) | \$0.00 (Local LLM) |

---

## 🧪 Future Experiment Ideas

This setup is highly flexible and can be used to run various AI tooling experiments:
1. **Model Benchmarking**: Compare the semantic extraction accuracy and speed of various local models (e.g. `llama3.1:8b`, `deepseek-coder:6.7b`, or `qwen2.5-coder:7b`) by changing the model parameters.
2. **Context Budgets**: Test how limiting Serena's `symbol_info_budget` or token window impacts agent behavior when editing deep C++ structures.
3. **Automated Commit Message Generator**: Add a pre-commit script that queries Ollama with a diff of staged files to propose a structured commit message.
4. **Local Linting / Code Review**: Write an on-demand review script that sends diffs of changed C++ files to Ollama to detect memory leaks, uninitialized pointers, or concurrency issues.
