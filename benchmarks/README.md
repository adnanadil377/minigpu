# MiniGPU Benchmarks & Performance Evaluation

This directory contains evaluation benchmarks, reproducible metric scripts, input datasets, and compilation performance results for the MiniGPU compiler pipeline.

## Benchmark Metrics

MiniGPU tracks multiple empirical compiler performance indicators across all supported kernels:
1. **Instruction Count**: IR instruction counts before and after optimization passes.
2. **Compilation Latency**: Wall-clock compilation time from source parsing through AST, IR, optimization passes, liveness analysis, register allocation, and PTX generation.
3. **Register Pressure**: Physical register count required by the graph-coloring allocator and spill frequency.
4. **Target Code Size**: Number of emitted PTX assembly instructions.

## Running Benchmarks

Ensure the project is built:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Run the benchmark suite:
```bash
python3 scripts/run_benchmarks.py
```

Results are printed in a structured table and saved as JSON in `benchmarks/results/benchmark_report.json`.

## Methodology & Reproducibility

- **Platform**: Apple Silicon / macOS / Linux x86_64
- **Precision**: 32-bit single precision IEEE 754 floats and 32-bit signed integers
- **Target PTX Version**: 7.0 (sm_75 architecture)
- **Register Budget**: Default 32 physical registers
