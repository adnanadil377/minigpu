# MiniGPU Language Specification

## 1. Syntax & Lexical Conventions

MiniGPU source files (`.mgpu`) define GPU compute kernels using a C-family syntax.

### Keywords
- Declarations: `kernel`, `int`, `float`, `bool`, `void`
- Control Flow: `if`, `else`, `while`, `for`, `return`, `break`, `continue`
- GPU Built-ins:
  - `thread_id_x()`, `thread_id_y()`, `thread_id_z()`
  - `block_id_x()`, `block_id_y()`, `block_id_z()`
  - `block_dim_x()`, `block_dim_y()`, `block_dim_z()`
  - `grid_dim_x()`, `grid_dim_y()`, `grid_dim_z()`

### Types
- `int`: 32-bit signed two's complement integer
- `float`: 32-bit IEEE 754 single-precision floating point
- `bool`: 1-bit boolean (`true` or `false`)
- `void`: only as kernel return type
- `T*`: 64-bit GPU global memory pointer pointing to element type `T` (e.g. `float*`, `int*`)

### Comments
- Single line: `// text to end of line`
- Block: `/* multi-line comment */`

## 2. Expressions & Operator Precedence

| Precedence | Operators | Associativity | Description |
|---|---|---|---|
| 1 (highest) | `()`, `[]`, `x++`, `x--` | Left-to-right | Grouping, indexing, postfix step |
| 2 | `-` (unary), `!` | Right-to-left | Arithmetic negation, logical NOT |
| 3 | `*`, `/`, `%` | Left-to-right | Multiplicative |
| 4 | `+`, `-` | Left-to-right | Additive |
| 5 | `<`, `<=`, `>`, `>=` | Left-to-right | Relational comparisons |
| 6 | `==`, `!=` | Left-to-right | Equality comparisons |
| 7 | `&&` | Left-to-right | Logical AND |
| 8 | `\|\|` | Left-to-right | Logical OR |
| 9 (lowest) | `=`, `+=`, `-=`, `*=`, `/=` | Right-to-left | Assignment |

## 3. Kernel Declarations

Kernels must begin with `kernel` and have an implicit return type of `void`.
```c
kernel vector_add(float* A, float* B, float* C, int N) {
    int i = thread_id_x();
    if (i < N) {
        C[i] = A[i] + B[i];
    }
}
```
