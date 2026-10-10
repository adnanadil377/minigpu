# MiniGPU: An Educational GPU Compiler from Scratch

> MiniGPU is an educational GPU compiler implemented in C++17. It explores language frontends, intermediate representations, optimization passes, data-flow analysis, graph-coloring register allocation, and PTX code generation through a small, testable kernel language.

[![CI](https://github.com/minigpu/minigpu/actions/workflows/ci.yml/badge.svg)](https://github.com/minigpu/minigpu/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

---

## Compiler Pipeline Architecture

```
MiniGPU Source (.mgpu)
        │
        ▼
   [Lexer & Scanner]
        │  Tokens + Source Locations
        ▼
   [Parser & AST]
        │  Typed AST Nodes (Pratt precedence climbing)
        ▼
   [Semantic Analysis]
        │  Lexical scopes, Type checking, Kernel signature validation
        ▼
   [Three-Address Code IR]
        │  Explicit Basic Blocks & Instructions
        ▼
   [CFG & Reachability Analysis]
        │  Predecessors, Successors, Dead-block removal
        ▼
   [IR Verifier]
        │  Strict invariant verification
        ▼
   [Optimization Passes]
        │  Constant Folding, Propagation, DCE, CSE, Mem2Reg
        ▼
   [Liveness Data-Flow Analysis]
        │  USE, DEF, LIVE_IN, LIVE_OUT sets & Live Intervals
        ▼
   [Graph-Coloring Register Allocation]
        │  Interference Graph, K-budget Coloring, Frame Spilling
        ▼
   [PTX Code Generation]
        │
        ▼
   NVIDIA PTX Assembly (.ptx) (ISA 7.0 / sm_75)
```

---

## Features

- **Language Frontend**:
  - Types: `int` (32-bit signed), `float` (32-bit IEEE 754), `bool`, and global memory pointers `float*`, `int*`.
  - Control Flow: `if-else`, `while`, `for`, `return`, `break`, `continue`.
  - GPU Intrinsics: `thread_id_x()`, `thread_id_y()`, `block_id_x()`, etc.
  - Rich diagnostics: exact line and column source tracking with descriptive error messages.
- **Three-Address Code IR**:
  - Virtual registers, arithmetic, comparisons, conversions, loads, stores, address calculations (`gep`), and terminators.
  - Verified CFG with predecessor/successor links and unreachable block pruning.
- **Modular Middle-End Optimizations**:
  - `Mem2RegPass`: Promotes local stack alloca loads to direct register copies.
  - `ConstantFoldingPass`: Folds compile-time arithmetic and comparison expressions.
  - `ConstantPropagationPass`: Propagates known constants to downstream operands.
  - `AlgebraicSimplificationPass`: Applies identities (`x + 0 -> x`, `x * 1 -> x`, `x * 0 -> 0`, etc.).
  - `CommonSubexpressionEliminationPass` (CSE): Reuses matching subexpressions within basic blocks.
  - `DeadCodeEliminationPass` (DCE): Purges unreferenced, side-effect-free instructions.
- **Register Allocation**:
  - Backward data-flow liveness analysis computing block sets and instruction live intervals.
  - Interference graph construction.
  - Chaitin-Briggs simplify-select graph coloring with configurable physical register budget (`--register-budget N`).
  - Automatic frame spilling when register pressure exceeds capacity.
- **PTX Generation**:
  - Valid NVIDIA PTX ISA 7.0 assembly generation targeting configurable architectures (`sm_70` to `sm_90`).
  - Correct 64-bit address conversion (`cvta.to.global.u64`) and wide integer address scaling (`mul.wide.s32`).
- **CPU Grid Execution Simulator**:
  - End-to-end execution of compiled kernels on a simulated GPU thread grid.
  - Verified numerical correctness against CPU golden references for vector addition, elementwise scaling, and 2D matrix multiplication with loops.

---

## Building MiniGPU

### Prerequisites
- C++17 compatible compiler (`clang++` >= 11 or `g++` >= 9)
- CMake >= 3.16
- Python 3 (for benchmarking)

### Build Instructions

```bash
# Clone and enter the repository
git clone https://github.com/minigpu/minigpu.git
cd minigpu

# Build with CMake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Or build using the Makefile convenience wrapper
make
```

### Running Tests

```bash
# Run automated test suite
ctest --test-dir build --output-on-failure

# Or run the test script
./scripts/run_tests.sh
```

---

## Command-Line Usage

The `minigpu` CLI provides inspectability into every compiler phase:

```bash
# Print token stream
./build/minigpu --emit-tokens examples/vector_add.mgpu

# Print typed Abstract Syntax Tree
./build/minigpu --emit-ast examples/vector_add.mgpu

# Print unoptimized Three-Address IR
./build/minigpu --emit-ir --no-opt examples/vector_add.mgpu

# Print optimized Three-Address IR
./build/minigpu --emit-ir examples/vector_add.mgpu

# Print Control-Flow Graph and basic blocks
./build/minigpu --emit-cfg examples/vector_add.mgpu

# Print dataflow liveness intervals
./build/minigpu --emit-liveness examples/vector_add.mgpu

# Print interference graph and register allocation
./build/minigpu --emit-regalloc examples/vector_add.mgpu

# Dump IR before and after each optimization pass
./build/minigpu --dump-passes examples/vector_add.mgpu

# Generate valid NVIDIA PTX assembly
./build/minigpu -o vector_add.ptx examples/vector_add.mgpu

# Simulate execution on CPU grid with sample buffers
./build/minigpu --run-cpu examples/vector_add.mgpu
```

---

## The Three Milestone Kernels

### 1. Vector Addition (`examples/vector_add.mgpu`)
```c
kernel vector_add(float* A, float* B, float* C, int N) {
    int i = thread_id_x();
    if (i < N) {
        C[i] = A[i] + B[i];
    }
}
```

Emitted PTX output (`vector_add.ptx`):
```ptx
.version 7.0
.target sm_75
.address_size 64

.visible .entry vector_add(
    .param .u64 A,
    .param .u64 B,
    .param .u64 C,
    .param .u32 N
)
{
    .reg .pred %p<16>;
    .reg .b32  %r<32>;
    .reg .f32  %f<32>;
    .reg .b64  %rd<32>;
    .local .align 8 .b8 __local_frame[512];

    ld.param.u64 %rd0, [A];
    cvta.to.global.u64 %rd1, %rd0;
    ld.param.u64 %rd2, [B];
    cvta.to.global.u64 %rd3, %rd2;
    ld.param.u64 %rd4, [C];
    cvta.to.global.u64 %rd5, %rd4;
    ld.param.u32 %r0, [N];

entry_0:
    mov.u32 %r4, %tid.x;
    setp.lt.s32 %p8, %r4, %r0;
    @%p8 bra bb_then_1;
    bra bb_merge_2;

bb_then_1:
    mul.wide.s32 %rd8, %r4, 4;
    add.s64 %rd7, %rd1, %rd8;
    ld.global.f32 %f0, [%rd7];
    add.s64 %rd10, %rd3, %rd8;
    ld.global.f32 %f1, [%rd10];
    add.f32 %f4, %f0, %f1;
    add.s64 %rd13, %rd5, %rd8;
    st.global.f32 [%rd13], %f4;
    bra bb_merge_2;

bb_merge_2:
    ret;
}
```

### 2. Elementwise Scaling (`examples/scale.mgpu`)
```c
kernel scale(float* A, float* B, float alpha, int N) {
    int i = thread_id_x();
    if (i < N) {
        B[i] = A[i] * alpha;
    }
}
```

### 3. Matrix Multiplication (`examples/matmul.mgpu`)
```c
kernel matmul(float* A, float* B, float* C, int N) {
    int row = thread_id_y();
    int col = thread_id_x();

    if (row < N && col < N) {
        float sum = 0.0;
        for (int k = 0; k < N; k++) {
            sum += A[row * N + k] * B[k * N + col];
        }
        C[row * N + col] = sum;
    }
}
```

---

## Evaluation & Benchmarks

Run the benchmark suite:
```bash
python3 scripts/run_benchmarks.py
```

Results:
```
======================================================================
        MiniGPU Compiler Benchmark & Metrics Suite
======================================================================
Kernel          No-Opt IR   Opt IR     Reduction   Compile(ms)   Phys Regs   Spills  
--------------------------------------------------------------------------------
vector_add      30          30           0.0%       3.16 ms     7           0       
scale           27          27           0.0%       2.77 ms     7           0       
matmul          67          67           0.0%       2.68 ms     11          0       
--------------------------------------------------------------------------------
```

Validate emitted PTX using NVIDIA's assembler (if CUDA toolkit installed):
```bash
./scripts/validate_ptx.sh
```

---

## Résumé Summary

> **MiniGPU — Educational GPU Compiler | C++17**
> - Designed a C-like GPU kernel language and built a complete lexical, AST, semantic analysis, and Three-Address Code intermediate representation pipeline.
> - Implemented an optimizing compiler middle-end with control-flow graph reachability analysis, invariant verification, constant folding, constant propagation, dead code elimination, and common-subexpression elimination.
> - Engineered backward data-flow liveness analysis and a Chaitin-Briggs graph-coloring register allocator with configurable physical register budgets and frame spilling.
> - Developed a GPU target backend emitting NVIDIA PTX assembly (sm_75) and a CPU grid execution simulator validating numerical correctness against CPU references.

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
