# MiniGPU Backend & NVIDIA PTX Code Generation

## 1. Target Model
MiniGPU emits syntactically valid NVIDIA Parallel Thread Execution (PTX) assembly targeting virtual architecture versions:
- `sm_70` (Volta)
- `sm_75` (Turing - default)
- `sm_80` (Ampere)
- `sm_86` (Ampere)
- `sm_90` (Hopper)

## 2. Register Classes
- `.reg .pred %p<N>`: Predicate registers (1-bit conditions and comparisons).
- `.reg .b32 %r<N>`: 32-bit registers for integer arithmetic and bit patterns.
- `.reg .f32 %f<N>`: 32-bit registers for floating-point calculations.
- `.reg .b64 %rd<N>`: 64-bit registers for pointer manipulation and memory addressing.

## 3. Emitted PTX Structure

### Header & Directives
```ptx
.version 7.0
.target sm_75
.address_size 64
```

### Parameter Conversions & Addressing
Pointers passed to kernel entries are converted to generic/global addressing via:
```ptx
ld.param.u64 %rd0, [A];
cvta.to.global.u64 %rd1, %rd0;
```

Array index calculations use wide multiplication:
```ptx
mul.wide.s32 %rd_tmp, %r_idx, 4;
add.s64 %rd_addr, %rd_base, %rd_tmp;
ld.global.f32 %f0, [%rd_addr];
```

### Predicated Execution & Control Flow
```ptx
setp.lt.s32 %p0, %r_i, %r_N;
@%p0 bra bb_then;
bra bb_merge;
```
