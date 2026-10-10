# MiniGPU Compiler Architecture

## Architectural Pipeline

```
MiniGPU Source (.mgpu)
        │
        ▼
   [Lexer & Scanner]
        │  Tokens + Source Locations
        ▼
   [Parser & AST]
        │  Typed AST Nodes
        ▼
   [Semantic Analyzer]
        │  Scope checking, Type verification, Built-in checks
        ▼
   [IR Lowering (IRGen)]
        │  Three-Address Code IR Module
        ▼
   [CFG & Reachability Analysis]
        │  Basic Blocks, Predecessors, Successors
        ▼
   [IR Verifier]
        │  Invariant verification
        ▼
   [Pass Manager & Optimization]
        │  Constant Folding, Propagation, DCE, CSE, Mem2Reg
        ▼
   [Liveness Data-Flow Analysis]
        │  USE, DEF, LIVE_IN, LIVE_OUT, Live Intervals
        ▼
   [Graph-Coloring Register Allocator]
        │  Interference Graph, K-budget Coloring, Spilling
        ▼
   [PTX Code Generator]
        │
        ▼
   NVIDIA PTX Assembly (.ptx)
```

## Module Directory Breakdown

- `include/minigpu/`: Public header APIs for compiler data structures, AST, IR, passes, and backend.
- `src/frontend/`: Lexical analysis (`lexer.cpp`), token definitions (`token.cpp`), AST printer (`ast.cpp`), and recursive-descent parser (`parser.cpp`).
- `src/semantic/`: Scoped symbol tables and static semantic validation (`semantic_analysis.cpp`).
- `src/ir/`: Intermediate representation data structures (`ir.cpp`), AST lowering (`ir_builder.cpp`), and invariant verification (`ir_verifier.cpp`).
- `src/analysis/`: Control-flow graph analysis (`cfg.cpp`) and backward liveness analysis (`liveness.cpp`).
- `src/passes/`: Pass Manager and modular optimization passes (`optimizer.cpp`).
- `src/backend/`: Target architectures (`target.cpp`), graph-coloring register allocator (`regalloc.cpp`), and PTX assembly generation (`codegen.cpp`).
- `src/simulator/`: CPU GPU-grid execution engine (`simulator.cpp`).
- `src/driver/`: Command-line compiler entry point (`main.cpp`).
