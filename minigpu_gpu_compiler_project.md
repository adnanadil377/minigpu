# MiniGPU: Build an Educational GPU Compiler from Scratch

> **Project type:** Compiler engineering / systems programming\
> **Primary language:** C++17 or C++20\
> **Target:** A small, documented subset of NVIDIA PTX (with an optional
> PTX-like simulator backend)\
> **Estimated timeline:** 10--14 weeks at a steady part-time pace;
> longer if you are new to C++ or compiler design\
> **Goal:** Demonstrate the core skills relevant to GPU compiler,
> compiler infrastructure, and systems software roles.

------------------------------------------------------------------------

## 1. Project summary

Build **MiniGPU**, a small compiler for a deliberately limited C-like
kernel language. A user writes a kernel in the MiniGPU language; the
compiler parses it, builds an intermediate representation (IR), performs
optimizations, analyzes variable lifetimes, allocates registers, selects
target instructions, and emits low-level GPU code.

The point is not to recreate NVIDIA's production compiler. The point is
to build a complete, testable compiler pipeline whose stages are
understandable and whose correctness can be demonstrated.

### The compiler pipeline

``` text
MiniGPU source
    |
    v
Lexer and parser
    |
    v
Abstract syntax tree (AST)
    |
    v
Semantic analysis and type checking
    |
    v
Intermediate representation (IR)
    |
    v
Control-flow graph and analysis
    |
    v
Optimization passes
    |
    v
Instruction selection
    |
    v
Liveness analysis and register allocation
    |
    v
PTX code generation
    |
    v
PTX validation / optional GPU execution
```

### What a successful first release does

A user can compile a small kernel such as vector addition, inspect the
AST and IR, enable or disable optimization passes, inspect
register-allocation decisions, and generate documented low-level output.
If a compatible NVIDIA GPU and CUDA toolkit are available, the generated
PTX can be assembled and the kernel can be executed and checked against
a reference result.

------------------------------------------------------------------------

## 2. Why this project is relevant

GPU compiler engineering involves much more than writing CUDA kernels.
It involves understanding how source programs are represented,
transformed, optimized, and mapped onto a target architecture.

MiniGPU gives you practical work in:

-   C++ software design, testing, debugging, and build systems.
-   Lexing, parsing, ASTs, semantic analysis, and type checking.
-   Intermediate representations, basic blocks, and control-flow graphs.
-   Data-flow analysis and compiler optimization.
-   Instruction selection and target-specific code generation.
-   Liveness analysis, interference graphs, register allocation, and
    spilling.
-   GPU execution concepts, PTX, memory operations, and performance
    measurement.
-   Reproducible experiments and technical documentation.

This is a portfolio project, not a guarantee of employment. Its value
comes from correctness, depth, clear explanations, and the ability to
discuss design trade-offs in an interview.

------------------------------------------------------------------------

## 3. Scope: what to build first

Keep the language small. A restricted, working compiler is better than a
large language with many unfinished features.

### Version 0.1: minimum viable compiler

Implement:

1.  Integer and floating-point literals.
2.  Local variables and simple assignments.
3.  Arithmetic operators: `+`, `-`, `*`, and, if needed, `/`.
4.  Comparisons such as `<`, `<=`, `==`, and `!=`.
5.  `if` statements and `while` or `for` loops.
6.  Kernel parameters and a few built-in functions such as
    `thread_id_x()`.
7.  A typed AST and basic semantic checks.
8.  A three-address-code IR.
9.  Constant folding and dead-code elimination.
10. A small target instruction set and readable assembly output.
11. Automated tests for every stage.

### Version 0.2: compiler analysis and optimization

Add:

-   Basic blocks and a control-flow graph (CFG).
-   Constant propagation.
-   Common-subexpression elimination.
-   Liveness analysis.
-   An interference graph.
-   Graph-coloring register allocation.
-   Register spilling when the available register budget is
    insufficient.
-   Before-and-after IR dumps for optimization passes.

### Version 0.3: GPU-oriented backend

Add:

-   Explicit kernel parameters and thread-index built-ins.
-   Global memory loads and stores.
-   Integer and floating-point instructions.
-   Conditional branches and labels.
-   PTX emission for the supported language subset.
-   Validation with NVIDIA's assembler/toolchain when available.
-   Optional execution and correctness tests on an NVIDIA GPU.

### Version 0.4: portfolio-quality release

Add:

-   Matrix multiplication and additional test kernels.
-   Benchmark scripts and reproducible result files.
-   A compiler-pass visualizer or clear textual dumps.
-   Design documents and documented limitations.
-   Continuous integration (CI).
-   Sanitizer runs and robust diagnostics.
-   A final demo showing the full compilation pipeline.

------------------------------------------------------------------------

## 4. Language design

Create a small language called **MiniGPU**. Its syntax can resemble C,
but do not claim full C compatibility.

### Example source program

``` c
kernel vector_add(float* A, float* B, float* C, int N) {
    int i = thread_id_x();

    if (i < N) {
        C[i] = A[i] + B[i];
    }
}
```

This is an illustrative syntax. You must define pointer, array, type,
memory, and thread-indexing semantics precisely before compiling it.

### Initial language features

  Feature          Initial support
  ---------------- -------------------------------------------------------------
  Types            `int`, `float`, `bool`, and `void` where appropriate
  Variables        Local variables and kernel parameters
  Expressions      Arithmetic, comparisons, logical operators
  Statements       Assignment, blocks, `if`, loops, `return`
  Memory           Explicitly defined array indexing and load/store operations
  Built-ins        `thread_id_x()` initially; additional dimensions later
  Functions        Start with kernels only; helper functions are optional
  Error handling   Line/column diagnostics for syntax and type errors

### Language rules to specify

Document the following before the backend becomes complex:

-   Whether implicit type conversions are allowed.
-   Integer width and signedness.
-   Floating-point behavior and whether fast-math transformations are
    allowed.
-   Array and pointer semantics.
-   What happens when a thread index is out of bounds.
-   Which memory spaces are supported.
-   Whether loops can contain `break` or `continue`.
-   How kernel parameters are represented in the target.
-   Which constructs are unsupported and how the compiler reports them.

Do not silently accept syntax or semantics that you have not
implemented.

------------------------------------------------------------------------

## 5. Compiler architecture and implementation requirements

### Module A --- Lexer

**Purpose:** Convert source text into tokens.

Implement:

-   Identifiers and keywords.
-   Integer and floating-point literals.
-   Operators and punctuation.
-   Whitespace and comments.
-   Source locations on tokens.
-   Diagnostics for invalid characters and malformed literals.

**Deliverable:** A token stream that can be printed for debugging.

**Tests:** Empty source, comments, valid tokens, malformed numbers,
unknown characters, and line/column positions.

### Module B --- Parser and AST

**Purpose:** Convert tokens into a structured syntax tree.

Implement:

-   Expression parsing with operator precedence.
-   Variable declarations and assignments.
-   Blocks and conditionals.
-   Loop syntax.
-   Kernel declarations and parameters.
-   Syntax-error recovery where practical.

A recursive-descent parser is a reasonable first choice.

**Deliverable:** An AST printer that produces a readable representation.

**Tests:** Operator precedence, nested expressions, missing delimiters,
invalid statements, and nested control flow.

### Module C --- Semantic analysis and type checking

**Purpose:** Reject programs that are syntactically valid but
semantically invalid.

Implement:

-   Symbol tables and scopes.
-   Declaration-before-use checks.
-   Duplicate declaration checks.
-   Type checking for expressions and assignments.
-   Function/kernel parameter checks.
-   Validation of built-in functions.
-   Array/pointer checks according to your language specification.

**Deliverable:** Typed AST nodes or a separate typed representation,
plus useful diagnostics.

**Tests:** Unknown variables, invalid operands, assignment type
mismatches, invalid indexing, and scope errors.

### Module D --- Intermediate representation (IR)

**Purpose:** Represent program operations in a form that is easier to
optimize and lower.

Start with a three-address-code IR. Each instruction should perform one
simple operation and define at most one result where practical.

Example:

``` text
%0 = call thread_id_x
%1 = load_param N
%2 = icmp_slt %0, %1
branch %2, bb_then, bb_exit

bb_then:
%3 = load A[%0]
%4 = load B[%0]
%5 = fadd %3, %4
store C[%0], %5
jump bb_exit

bb_exit:
return
```

The exact syntax is your design choice. The example is illustrative, not
a complete target-specific IR.

Implement:

-   Virtual registers or SSA-style values.
-   Constants and arithmetic operations.
-   Loads and stores.
-   Comparisons.
-   Branches and labels/basic blocks.
-   Returns.
-   An IR verifier that checks definitions, operands, types, and branch
    targets.

**Design decision:** A non-SSA three-address IR is simpler to begin
with. An SSA representation can be added later. If you choose SSA,
document how control-flow merges and phi nodes work.

**Deliverables:** IR data structures, parser-to-IR lowering, a readable
IR printer, and an IR verifier.

### Module E --- Control-flow graph and analyses

**Purpose:** Make control flow explicit so optimizations and register
allocation can reason about the program.

Implement:

-   Basic-block construction.
-   Successor and predecessor lists.
-   Reachability analysis.
-   Unreachable-block removal.
-   A CFG text or Graphviz/DOT dump.
-   Use/definition sets for each basic block.

**Deliverable:** A CFG dump for a kernel with branches and loops.

**Tests:** Straight-line code, if/else, nested branches, loops,
unreachable blocks, and malformed branch targets.

### Module F --- Optimization passes

Build passes independently. Each pass should have tests and should
preserve program behavior.

#### Required passes

1.  **Constant folding:** Evaluate safe constant expressions at compile
    time.
2.  **Constant propagation:** Replace uses with known constants when
    valid.
3.  **Dead-code elimination:** Remove unused computations and
    unreachable blocks.
4.  **Algebraic simplification:** Apply safe rules such as `x + 0 -> x`
    where the language's semantics permit it.
5.  **Common-subexpression elimination:** Reuse equivalent computations
    when their operands and memory assumptions make it safe.

#### Later optional passes

-   Copy propagation.
-   Strength reduction.
-   Loop-invariant code motion.
-   Simple induction-variable analysis.
-   Instruction scheduling.

Be careful with floating-point transformations. Reordering operations
can change results, so document the chosen numerical semantics and do
not apply unsafe transformations by default.

**Deliverables:**

-   A pass manager.
-   A way to run selected passes.
-   IR dumps before and after each pass.
-   Tests that demonstrate both the transformation and correctness.
-   An optimization-off mode for comparisons.

### Module G --- Liveness analysis

**Purpose:** Determine where virtual-register values are live.

Implement standard backward data-flow analysis over the CFG.

For each basic block, compute:

-   `USE`: values used before being defined in the block.
-   `DEF`: values defined in the block.
-   `LIVE_IN`: values live on entry.
-   `LIVE_OUT`: values live on exit.

Iterate until the data-flow equations reach a fixed point:

``` text
LIVE_OUT[B] = union(LIVE_IN[S]) for each successor S of B
LIVE_IN[B]  = USE[B] union (LIVE_OUT[B] - DEF[B])
```

Then compute instruction-level live ranges or live sets for register
allocation.

**Deliverables:** A liveness dump and tests covering branches, loops,
and values that are live across blocks.

### Module H --- Register allocation

**Purpose:** Map virtual registers to a limited number of physical
registers.

Implement:

1.  An interference graph in which simultaneously live values interfere.
2.  A graph-coloring allocation algorithm.
3.  A configurable register budget.
4.  Detection of allocation failures.
5.  A basic spilling strategy that stores selected values to local
    stack/frame slots and reloads them when needed.
6.  A verifier that checks conflicting values do not share a register.

Start with a simple algorithm such as simplify/select graph coloring. It
need not match the complexity of a production compiler allocator.

**Experiments to demonstrate:**

-   Allocation with a generous register budget.
-   Allocation with a small register budget.
-   How register pressure changes the assignment.
-   When and how spills are introduced.
-   The trade-off between register usage and extra memory operations.

**Deliverables:** Interference-graph output, register assignment
reports, spill reports, and tests for successful allocation and
spilling.

### Module I --- Instruction selection and target model

**Purpose:** Translate IR operations into target instructions.

Define a small target instruction model before generating text. Include:

-   Integer arithmetic.
-   Floating-point arithmetic.
-   Comparisons and predicates.
-   Loads and stores.
-   Branches and labels.
-   Kernel parameters.
-   Thread-index access.
-   Register and operand types.

You can initially use a PTX-like teaching assembly to test the backend
independently. Label it clearly as custom output, not real PTX. Then
implement actual PTX syntax for a supported subset.

**Deliverables:** Instruction-selection rules, target-instruction data
structures, and tests that assert the selected instructions for
representative IR operations.

### Module J --- PTX code generation

**Purpose:** Emit valid PTX for the subset you have implemented.

Implement:

-   Module and kernel declarations.
-   Parameter declarations.
-   Register declarations.
-   Type-correct arithmetic instructions.
-   Global memory access for supported types.
-   Comparisons, predicates, branches, and labels.
-   Correct thread-index mapping.
-   Required address calculations.
-   Consistent symbol naming and formatting.

Validate generated PTX using the appropriate NVIDIA toolchain, such as
`ptxas`, when available. Do not assume any arbitrary PTX string is
valid; instruction syntax, types, addressing, and target versions
matter.

**Deliverables:** Generated `.ptx` files, validation scripts, and a
clear list of supported target features.

### Module K --- Execution and correctness

**Purpose:** Verify that compiled programs produce correct results.

If you have a compatible NVIDIA GPU and CUDA toolkit:

-   Assemble or load generated PTX using a supported route.
-   Run small kernels.
-   Compare outputs against a simple CPU reference.
-   Test boundary cases, including zero-length inputs if supported,
    small sizes, non-multiples of the execution width, and bounds
    conditions.
-   Report compilation failures separately from runtime failures.

If GPU hardware is unavailable, continue with CPU-side compiler tests
and PTX validation where possible. You can also build an optional
interpreter for the custom IR, but be clear that IR simulation does not
prove the generated PTX executes correctly.

**Deliverables:** End-to-end correctness tests and reproducible
instructions for running them.

------------------------------------------------------------------------

## 6. Three milestone kernels

Implement the kernels in this order. The source examples use the
proposed MiniGPU syntax and may need adjustment as you define the
language.

### Kernel 1 --- Vector addition

``` c
kernel vector_add(float* A, float* B, float* C, int N) {
    int i = thread_id_x();
    if (i < N) {
        C[i] = A[i] + B[i];
    }
}
```

Demonstrates:

-   Parsing and type checking.
-   Thread-index built-in handling.
-   Bounds checks and conditional branches.
-   Memory loads and stores.
-   Floating-point addition.
-   Basic instruction selection and PTX emission.

This is the minimum end-to-end milestone.

### Kernel 2 --- Elementwise scaling

``` c
kernel scale(float* A, float* B, float alpha, int N) {
    int i = thread_id_x();
    if (i < N) {
        B[i] = A[i] * alpha;
    }
}
```

Demonstrates:

-   Scalar parameters.
-   Multiplication.
-   Constant propagation and simplification opportunities.
-   Register lifetime and allocation.
-   A second end-to-end correctness case.

This is elementwise scaling, not a sum reduction.

### Kernel 3 --- Matrix multiplication

``` c
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

Demonstrates:

-   Loops and loop control flow.
-   Integer address calculations.
-   Repeated memory operations.
-   Longer live ranges and register pressure.
-   Opportunities for loop optimizations.

Begin with a simple, correct implementation. Tiling, shared memory,
vectorization, and tensor-core instructions are optional future work,
not requirements for the first release.

------------------------------------------------------------------------

## 7. Suggested repository structure

``` text
minigpu/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── .gitignore
├── .github/
│   └── workflows/
│       └── ci.yml
├── docs/
│   ├── project-spec.md
│   ├── architecture.md
│   ├── language-spec.md
│   ├── ir-design.md
│   ├── optimization-passes.md
│   ├── register-allocation.md
│   ├── backend-and-ptx.md
│   ├── testing.md
│   └── benchmarks.md
├── examples/
│   ├── vector_add.mgpu
│   ├── scale.mgpu
│   └── matmul.mgpu
├── include/
│   └── minigpu/
│       ├── token.hpp
│       ├── lexer.hpp
│       ├── ast.hpp
│       ├── parser.hpp
│       ├── types.hpp
│       ├── semantic_analysis.hpp
│       ├── ir.hpp
│       ├── ir_verifier.hpp
│       ├── cfg.hpp
│       ├── optimizer.hpp
│       ├── liveness.hpp
│       ├── regalloc.hpp
│       ├── target.hpp
│       └── codegen.hpp
├── src/
│   ├── frontend/
│   ├── semantic/
│   ├── ir/
│   ├── analysis/
│   ├── passes/
│   ├── backend/
│   └── driver/
├── tests/
│   ├── lexer/
│   ├── parser/
│   ├── semantic/
│   ├── ir/
│   ├── passes/
│   ├── regalloc/
│   ├── codegen/
│   └── integration/
├── scripts/
│   ├── run_tests.sh
│   ├── validate_ptx.sh
│   └── run_benchmarks.py
└── benchmarks/
    ├── inputs/
    ├── results/
    └── README.md
```

This is a target structure, not a requirement to create every file on
day one. Add directories as modules become real.

------------------------------------------------------------------------

## 8. Command-line interface

Design a simple command-line compiler. For example:

``` bash
# Build the compiler
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run tests
ctest --test-dir build --output-on-failure

# Print tokens
./build/minigpu --emit-tokens examples/vector_add.mgpu

# Print the AST
./build/minigpu --emit-ast examples/vector_add.mgpu

# Print IR before optimization
./build/minigpu --emit-ir --no-opt examples/vector_add.mgpu

# Print optimized IR
./build/minigpu --emit-ir examples/vector_add.mgpu

# Print control-flow graph
./build/minigpu --emit-cfg examples/vector_add.mgpu

# Print register allocation
./build/minigpu --emit-regalloc examples/vector_add.mgpu

# Generate PTX
./build/minigpu -o vector_add.ptx examples/vector_add.mgpu
```

These are proposed interfaces. Implement only the flags that exist and
document them in `--help`.

Useful compiler options include:

-   `--dump-passes`
-   `--no-opt`
-   `--register-budget N`
-   `--target sm_XX` once target handling is implemented
-   `--verify-ir`
-   `-o OUTPUT`

Reject unsupported options clearly rather than silently ignoring them.

------------------------------------------------------------------------

## 9. Testing strategy

A compiler needs more than a few successful examples. Test each stage in
isolation and test the whole pipeline.

### Unit tests

-   Lexer: token kinds, source positions, malformed input.
-   Parser: precedence, nesting, syntax errors.
-   Semantic analysis: scopes, types, undeclared variables.
-   IR: verifier, definitions, uses, branch targets.
-   Optimizations: expected transformation and preservation of behavior.
-   Liveness: known live-in/live-out sets.
-   Register allocation: interference and register conflicts.
-   Code generation: instruction patterns and valid output structure.

### Integration tests

Compile each example through the entire pipeline. Keep small source
files and expected outputs as test fixtures where useful.

### Differential correctness tests

Where execution is available, compare kernel outputs with a
straightforward CPU reference. Use deterministic inputs and test edge
cases. Floating-point comparisons should use an appropriate tolerance
unless exact equality is justified.

### Fuzzing and robustness

As an advanced improvement, generate random small expressions or
programs and check that the compiler either handles them correctly or
reports a controlled error. It must not crash or access invalid memory
on malformed input.

### Continuous integration

Set up CI to:

-   Build in a clean environment.
-   Run unit and integration tests.
-   Enable compiler warnings.
-   Optionally run AddressSanitizer and UndefinedBehaviorSanitizer on
    supported platforms.
-   Check formatting if you adopt a formatter.

GPU-dependent tests should be identified separately so CPU-only CI can
still validate the core compiler.

------------------------------------------------------------------------

## 10. Benchmarking and evaluation

Do not claim performance improvements without measurements.

### Compiler experiments

Measure:

-   Number of IR instructions before and after each optimization.
-   Number of virtual registers.
-   Number of allocated physical registers.
-   Number of spills.
-   Compilation time for a few representative programs.
-   Correctness results for optimized and unoptimized paths.

### GPU experiments, if hardware is available

For each supported kernel:

1.  Establish a correct reference result.
2.  Compile and validate the generated PTX.
3.  Run multiple trials with consistent input sizes.
4.  Separate compilation time from kernel execution time.
5.  Report device model, software/toolkit versions, input sizes, and
    measurement method.
6.  Compare against a clearly described baseline.
7.  Avoid claiming that the compiler is faster than NVIDIA's compiler
    unless the comparison is fair and reproducible.

A small project can still produce valuable measurements even when its
generated code is not competitive with a mature production compiler.
Explain what the data shows and why.

------------------------------------------------------------------------

## 11. Week-by-week implementation plan

This schedule assumes some C++ familiarity. If you are learning C++ and
compiler design at the same time, expect to take longer.

  -----------------------------------------------------------------------
  Weeks                   Focus                   Exit criterion
  ----------------------- ----------------------- -----------------------
  1--2                    Lexer, parser, AST      Valid programs parse;
                                                  invalid programs
                                                  produce diagnostics

  3                       Semantic analysis       Scopes and types are
                                                  checked

  4                       IR and verifier         Example kernels lower
                                                  into verified IR

  5                       CFG and data structures Branches and loops form
                                                  correct basic blocks

  6                       Basic optimization      Constant folding and
                          passes                  dead-code elimination
                                                  pass tests

  7                       Data-flow analysis      Liveness reaches a
                                                  fixed point on test
                                                  CFGs

  8                       Register allocation     Graph coloring works
                                                  and reports
                                                  conflicts/spills

  9                       Instruction selection   IR operations map to
                                                  documented target
                                                  instructions

  10                      PTX backend             A supported kernel
                                                  emits PTX that
                                                  validates when tooling
                                                  is available

  11--12                  End-to-end tests        Examples compile;
                                                  execution tests work if
                                                  hardware is available

  13                      Experiments and         Measurements and error
                          robustness              cases are documented

  14                      Documentation and       Clean build, tests,
                          release                 README, demo, and
                                                  tagged release
  -----------------------------------------------------------------------

At the end of every week, make a working commit. Do not wait until the
project is finished to put it on GitHub.

------------------------------------------------------------------------

## 12. Learning resources

Use these resources to understand the ideas behind your implementation.
Read the relevant sections as you reach each stage rather than trying to
read everything before coding.

-   **LLVM documentation:** https://llvm.org/docs/\
    Useful for compiler infrastructure, IR, passes, and backend
    concepts.

-   **NVIDIA PTX ISA:**
    https://docs.nvidia.com/cuda/parallel-thread-execution/\
    The primary reference for PTX instructions, types, addressing, and
    target details.

-   **CUDA C++ Programming Guide:**
    https://docs.nvidia.com/cuda/cuda-c-programming-guide/\
    Covers GPU execution, threads, memory hierarchy, and programming
    concepts.

-   **Crafting Interpreters:** https://craftinginterpreters.com/\
    A practical introduction to lexers, parsers, syntax trees, and
    language implementation.

-   **LLVM Kaleidoscope tutorial:** https://llvm.org/docs/tutorial/\
    A useful guided example of implementing a small language with LLVM.
    Treat it as a reference and learning aid; do not merely copy it and
    call that a custom GPU compiler.

-   **CMake documentation:** https://cmake.org/documentation/\
    Useful for organizing and building the C++ project.

------------------------------------------------------------------------

## 13. Definition of done

The project is ready to present when you can check off the following
items.

### Frontend

-   [ ] A documented MiniGPU language subset exists.
-   [ ] Lexer and parser work on valid and invalid inputs.
-   [ ] AST and type-checking stages have tests.
-   [ ] Diagnostics include useful source locations.

### Compiler core

-   [ ] The IR has a documented design.
-   [ ] An IR verifier checks important invariants.
-   [ ] Basic blocks and CFGs work for branches and loops.
-   [ ] Optimization passes are independently testable.
-   [ ] Liveness analysis is tested.
-   [ ] Register allocation handles conflicts and reports spills.

### Backend

-   [ ] Instruction selection is documented and tested.
-   [ ] Generated output matches the stated target.
-   [ ] PTX output is validated for the supported subset when the NVIDIA
    toolchain is available.
-   [ ] Unsupported features fail with clear messages.

### Correctness and engineering

-   [ ] Vector addition and scaling pass end-to-end tests.
-   [ ] Matrix multiplication works if the chosen subset supports it.
-   [ ] CPU reference comparisons are available where execution is
    possible.
-   [ ] Build and test commands work from a clean checkout.
-   [ ] CI runs the non-GPU test suite.
-   [ ] Limitations and known bugs are documented.

### Portfolio presentation

-   [ ] README includes architecture, build steps, examples, and
    limitations.
-   [ ] A design document explains at least one major trade-off.
-   [ ] Before-and-after optimization output is included.
-   [ ] Register-allocation results are included.
-   [ ] Benchmarks include methodology and environment details.
-   [ ] A short demo shows source code going through the pipeline.

------------------------------------------------------------------------

## 14. What not to do

Avoid these common mistakes:

-   Do not start with matrix multiplication before the language and IR
    work.
-   Do not implement a huge C-like language.
-   Do not confuse a custom PTX-like format with valid NVIDIA PTX.
-   Do not assume generated assembly is correct just because it looks
    plausible.
-   Do not add many optimizations before the IR verifier and tests are
    reliable.
-   Do not report speedups without reproducible measurements.
-   Do not claim production-level performance or NVIDIA compiler
    equivalence.
-   Do not spend the entire project watching tutorials. Implement, test,
    document, and explain your own design.

------------------------------------------------------------------------

## 15. Final deliverable

The final result should be a public repository named something like
`minigpu-compiler` with:

1.  A C++ compiler that builds reliably.
2.  A small, clearly specified source language.
3.  A complete frontend-to-backend pipeline for a limited subset.
4.  IR dumps and working optimization passes.
5.  Liveness analysis and register allocation.
6.  PTX generation and validation for the supported subset.
7.  Automated tests and CI.
8.  Example kernels and reproducible experiments.
9.  Technical documentation explaining architecture, limitations, and
    trade-offs.
10. A short demonstration that shows how a kernel becomes low-level
    instructions.

### Suggested README tagline

> MiniGPU is an educational GPU compiler implemented in C++. It explores
> language frontends, intermediate representations, optimization passes,
> data-flow analysis, graph-coloring register allocation, and PTX code
> generation through a small, testable kernel language.

### How to describe it on a résumé

Use only the claims that match what you have actually completed:

> **MiniGPU --- Educational GPU Compiler \| C++**
>
> -   Designed a small kernel language and implemented lexical analysis,
>     parsing, semantic checks, and a custom intermediate
>     representation.
> -   Developed compiler optimization passes and control-flow/data-flow
>     analyses with automated tests.
> -   Implemented liveness analysis, graph-coloring register allocation,
>     and basic spilling.
> -   Built a target backend for a documented subset of PTX and
>     validated generated output using available NVIDIA tooling.
> -   Evaluated compiler transformations through reproducible
>     correctness tests and performance experiments where hardware was
>     available.

**The priority is depth over breadth:** make vector addition compile
correctly first, then build each compiler stage until you can explain
and test it. A small compiler you understand thoroughly is a stronger
interview project than a large compiler you cannot debug or explain.
