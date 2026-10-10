#!/usr/bin/env python3
import subprocess
import time
import json
import os
import re

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MINIGPU_BIN = os.path.join(ROOT_DIR, "build", "minigpu")
EXAMPLES_DIR = os.path.join(ROOT_DIR, "examples")
RESULTS_DIR = os.path.join(ROOT_DIR, "benchmarks", "results")

os.makedirs(RESULTS_DIR, exist_ok=True)

KERNELS = [
    ("vector_add", os.path.join(EXAMPLES_DIR, "vector_add.mgpu")),
    ("scale", os.path.join(EXAMPLES_DIR, "scale.mgpu")),
    ("matmul", os.path.join(EXAMPLES_DIR, "matmul.mgpu"))
]

def count_ir_instructions(ir_text):
    count = 0
    for line in ir_text.splitlines():
        line = line.strip()
        if not line or line.startswith(";") or line.startswith("function") or line.endswith(":") or line == "}":
            continue
        count += 1
    return count

def parse_regalloc_stats(ra_text):
    budget = 32
    used = 0
    spills = 0
    m_used = re.search(r"Physical Registers Used:\s+(\d+)", ra_text)
    if m_used:
        used = int(m_used.group(1))
    m_spills = re.search(r"Spill Count:\s+(\d+)", ra_text)
    if m_spills:
        spills = int(m_spills.group(1))
    return {"phys_used": used, "spills": spills}

def run_benchmarks():
    print("=" * 70)
    print("        MiniGPU Compiler Benchmark & Metrics Suite")
    print("=" * 70)

    if not os.path.exists(MINIGPU_BIN):
        print(f"Error: MiniGPU binary not found at {MINIGPU_BIN}. Please build first.")
        return

    results = []

    for name, path in KERNELS:
        if not os.path.exists(path):
            continue

        # 1. Unoptimized IR
        t0 = time.perf_counter()
        res_no_opt = subprocess.run([MINIGPU_BIN, "--emit-ir", "--no-opt", path],
                                    capture_output=True, text=True, check=True)
        no_opt_time = (time.perf_counter() - t0) * 1000.0
        no_opt_insts = count_ir_instructions(res_no_opt.stdout)

        # 2. Optimized IR
        t0 = time.perf_counter()
        res_opt = subprocess.run([MINIGPU_BIN, "--emit-ir", path],
                                 capture_output=True, text=True, check=True)
        opt_time = (time.perf_counter() - t0) * 1000.0
        opt_insts = count_ir_instructions(res_opt.stdout)

        # 3. Register allocation stats
        res_ra = subprocess.run([MINIGPU_BIN, "--emit-regalloc", path],
                                capture_output=True, text=True, check=True)
        ra_stats = parse_regalloc_stats(res_ra.stdout)

        # Instruction reduction
        reduction = 0.0
        if no_opt_insts > 0:
            reduction = ((no_opt_insts - opt_insts) / no_opt_insts) * 100.0

        item = {
            "kernel": name,
            "no_opt_insts": no_opt_insts,
            "opt_insts": opt_insts,
            "inst_reduction_pct": round(reduction, 1),
            "no_opt_compile_ms": round(no_opt_time, 2),
            "opt_compile_ms": round(opt_time, 2),
            "phys_registers": ra_stats["phys_used"],
            "spills": ra_stats["spills"]
        }
        results.append(item)

    # Print table
    header = f"{'Kernel':<15} {'No-Opt IR':<11} {'Opt IR':<10} {'Reduction':<11} {'Compile(ms)':<13} {'Phys Regs':<11} {'Spills':<8}"
    print(header)
    print("-" * 80)
    for r in results:
        line = f"{r['kernel']:<15} {r['no_opt_insts']:<11} {r['opt_insts']:<10} {r['inst_reduction_pct']:>5.1f}%     {r['opt_compile_ms']:>6.2f} ms     {r['phys_registers']:<11} {r['spills']:<8}"
        print(line)
    print("-" * 80)

    # Save to JSON
    out_file = os.path.join(RESULTS_DIR, "benchmark_report.json")
    with open(out_file, "w") as f:
        json.dump(results, f, indent=2)
    print(f"\nDetailed metrics saved to: {out_file}")

if __name__ == "__main__":
    run_benchmarks()
