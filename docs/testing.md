# MiniGPU Testing Strategy

## 1. Test Suite Architecture

MiniGPU utilizes a comprehensive CTest and unit test architecture located under `tests/`:

1. `test_lexer`:
   - Token kinds, identifiers, decimals, hex literals, scientific notation floats.
   - Single-line comments, block comments, and source location tracking.
   - Syntax error diagnostics for malformed tokens.
2. `test_parser`:
   - Precedence climbing, statement nesting, loops, and syntax error recovery.
3. `test_semantic`:
   - Scoped symbol tables, duplicate declaration rejection, type mismatch detection, loop containment for `break`/`continue`.
4. `test_ir`:
   - Verification of 3AC IR generation, terminator completeness, and CFG reachability.
5. `test_passes`:
   - Independent verification of constant folding, algebraic simplification, and dead code elimination.
6. `test_regalloc`:
   - Liveness fixed-point convergence, interference graph construction, coloring verification, and forced spilling under small budgets.
7. `test_codegen`:
   - Emitted PTX structure, parameter directives, register declarations, and memory access instructions.
8. `test_integration`:
   - End-to-end execution of `vector_add`, `scale`, and 2D `matmul` using the built-in CPU grid simulator, asserting exact numerical equality against CPU references.

## 2. Running Tests

```bash
# Via CMake & CTest
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure

# Via convenience script
./scripts/run_tests.sh
```
