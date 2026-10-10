# MiniGPU Intermediate Representation (IR) Design

## 1. Overview
MiniGPU uses a typed three-address code (3AC) intermediate representation.

An `IRModule` consists of one or more `IRFunction` definitions. Each function contains an entry `IRBasicBlock` followed by labeled basic blocks.

## 2. Invariants
- **Single Terminator**: Every basic block ends with exactly one terminator instruction (`br`, `br_cond`, or `ret`).
- **Def-Before-Use**: Every virtual register operand must be defined prior to consumption.
- **Explicit Labels**: Control-flow targets are explicitly named basic block labels.

## 3. Instruction Set Summary

| Opcode | Syntax | Description |
|---|---|---|
| `alloca` | `%v = alloca <type>` | Allocates a local stack slot |
| `load` | `%v = load %ptr` | Loads value from address `%ptr` |
| `store` | `store %val, %ptr` | Stores value `%val` into address `%ptr` |
| `gep` | `%v = gep %base, %idx` | Address calculation: `%base + %idx * sizeof(elem)` |
| `add`, `sub`, `mul`, `sdiv`, `srem` | `%v = add %a, %b` | 32-bit signed integer arithmetic |
| `fadd`, `fsub`, `fmul`, `fdiv` | `%v = fadd %a, %b` | 32-bit floating point arithmetic |
| `icmp_<cond>` | `%v = icmp_slt %a, %b` | Integer comparison producing boolean |
| `fcmp_<cond>` | `%v = fcmp_olt %a, %b` | Float comparison producing boolean |
| `and`, `or`, `xor` | `%v = and %a, %b` | Boolean and bitwise logical operations |
| `special_reg` | `%v = special_reg <name>` | Loads GPU hardware intrinsic (`thread_id_x`, etc.) |
| `copy` | `%v = copy %src` | Register copy / constant assignment |
| `br` | `br <label>` | Unconditional branch |
| `br_cond` | `br_cond %c, <then>, <else>` | Conditional branch based on `%c` |
| `ret` | `ret` | Kernel function return |
