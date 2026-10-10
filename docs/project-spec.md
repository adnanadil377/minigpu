# MiniGPU: Project Specification

## 1. Overview
MiniGPU is an educational GPU compiler implemented in C++17. It translates a C-like GPU kernel language into NVIDIA Parallel Thread Execution (PTX) assembly code.

The pipeline comprises:
- **Frontend**: Lexer, Recursive-descent Parser, Abstract Syntax Tree (AST), Semantic Analysis & Type Checker.
- **Middle-end**: Three-Address Code Intermediate Representation (IR), Control-Flow Graph (CFG) analysis, IR Verifier, Optimization Passes (Constant Folding, Constant Propagation, Algebraic Simplification, Common-Subexpression Elimination, Dead-Code Elimination, Mem2Reg).
- **Backend**: Backward Data-flow Liveness Analysis, Interference Graph construction, Chaitin-Briggs Graph-Coloring Register Allocation with configurable budget and spilling, Target Model Instruction Selection, and valid NVIDIA PTX emission.
- **Verification & Execution**: CPU-based grid execution simulator verifying numerical correctness against CPU references.

## 2. Core Principles
1. **Verifiable Correctness**: Every stage has independent tests asserting structural and semantic invariants.
2. **Explicit Transformations**: The compiler provides CLI inspectability into every intermediate stage (`--emit-tokens`, `--emit-ast`, `--emit-ir`, `--emit-cfg`, `--emit-liveness`, `--emit-regalloc`, `--dump-passes`).
3. **Target Standard**: Generates syntactically correct PTX ISA 7.0 for target architecture `sm_75` (configurable from `sm_70` to `sm_90`).
