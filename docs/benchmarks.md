# MiniGPU Benchmarking & Evaluation Methodology

## 1. Overview
Compiler benchmarking evaluates the efficiency of both the compilation process and the generated code.

## 2. Experimental Metrics

The `scripts/run_benchmarks.py` harness tracks:
1. **Instruction Count**: Total number of IR instructions before vs. after optimization.
2. **Compilation Time (ms)**: Time taken to parse, optimize, analyze, allocate registers, and emit PTX.
3. **Physical Register Usage**: Number of registers allocated under a budget of $K=32$.
4. **Spill Count**: Number of registers spilled to local frame memory.

## 3. Benchmark Results

| Kernel | Unoptimized IR | Optimized IR | Instruction Reduction | Compilation Latency | Physical Registers | Spills |
|---|---|---|---|---|---|---|
| `vector_add` | 30 | 30 | 0.0% | ~3.1 ms | 7 | 0 |
| `scale` | 27 | 27 | 0.0% | ~2.8 ms | 7 | 0 |
| `matmul` | 67 | 67 | 0.0% | ~2.7 ms | 11 | 0 |

## 4. Hardware Execution (Optional CUDA GPU Path)
When an NVIDIA GPU and CUDA toolkit are present, generated `.ptx` files can be assembled using `ptxas` or loaded using the CUDA Driver API (`cuModuleLoad`, `cuLaunchKernel`) to benchmark GPU kernel execution time against CUDA C++ baselines.
