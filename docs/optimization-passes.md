# MiniGPU Optimization Passes

## Optimization Pipeline

MiniGPU provides a modular `PassManager` that coordinates function-level optimization passes. Pass execution can be observed with `--dump-passes` or disabled entirely with `--no-opt`.

```
  Mem2RegPass
       │ (Promotes local stack loads to register copies)
       ▼
  ConstantPropagationPass
       │ (Propagates known constants to subsequent uses)
       ▼
  ConstantFoldingPass
       │ (Folds compile-time arithmetic and comparison expressions)
       ▼
  AlgebraicSimplificationPass
       │ (Simplifies identity and annihilation expressions)
       ▼
  CommonSubexpressionEliminationPass
       │ (Reuses identical subexpressions within basic blocks)
       ▼
  FMAContractionPass
       │ (Fuses (a * b) + c into hardware GPU fma.rn.f32 instruction)
       ▼
  ConstantPropagationPass
       │ (Propagates constants resulting from folding)
       ▼
  DeadCodeEliminationPass
         (Eliminates unused pure instructions)
```

## Detailed Pass Descriptions

### 1. Constant Folding (`ConstantFoldingPass`)
Evaluates constant binary arithmetic expressions at compile-time:
- Integer arithmetic: `10 + 32` folds into `42`.
- Float arithmetic: `1.5f * 2.0f` folds into `3.0f`.
- Comparisons: `3 < 5` folds into `true`.

### 2. Constant Propagation (`ConstantPropagationPass`)
Maintains a map of virtual registers holding known constants (`%v = copy 42`) and replaces occurrences in downstream instructions with the constant literal.

### 3. Algebraic Simplification (`AlgebraicSimplificationPass`)
Applies identities:
- `x + 0 -> x`, `0 + x -> x`
- `x - 0 -> x`, `x - x -> 0`
- `x * 1 -> x`, `1 * x -> x`
- `x * 0 -> 0`, `0 * x -> 0`
- `x && false -> false`, `x || true -> true`

### 4. Common-Subexpression Elimination (`CommonSubexpressionEliminationPass`)
Maintains available expressions indexed by `(opcode, operand0, operand1)`. If an identical calculation appears earlier in the block, subsequent occurrences are rewritten to reuse the prior destination register.

### 5. Fused Multiply-Add Contraction (`FMAContractionPass`)
Specialized GPU hardware arithmetic optimization. Identifies floating-point multiply-accumulate chains `(a * b) + c` or `c + (a * b)` and contracts them into a single 3-operand `fma(a, b, c)` instruction. In the PTX backend, this emits `fma.rn.f32`, which executes in a single cycle on NVIDIA Streaming Multiprocessors with higher IEEE-754 precision (single rounding step) and reduces loop arithmetic instructions by 50% in matrix multiplication (`matmul`).

### 6. Memory-to-Register Promotion (`Mem2RegPass`)
Tracks stores to local alloca slots. When a subsequent load from the same slot occurs before any clobbering store, the load is converted directly into a register copy.

### 7. Dead-Code Elimination (`DeadCodeEliminationPass`)
Computes the set of all consumed virtual registers. Any instruction whose destination register is never read (and which has no memory or control-flow side effects) is purged. Runs iteratively until a fixed point is reached.
