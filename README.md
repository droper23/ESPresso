<p align="center">
  <img src="./assets/ESPresso_real.png" width="800" alt="ESPresso Hero">
</p>

# ESPresso ☕️

ESPresso is a lightweight, modern scripting language designed specifically for the ESP32 and other embedded systems. It combines C-like performance with the expressive power of modern high-level languages.

## Quick Start

### 1. Build
You need a C compiler (like `clang` or `gcc`) and CMake.

```bash
cmake -S . -B build
cmake --build build
```

### 2. Run
By default, ESPresso runs the **bytecode VM**.

```bash
./build/ESPresso examples/program.espr
```

To run the **AST interpreter**, pass `--interp`:

```bash
./build/ESPresso --interp examples/program.espr
```

To print every VM opcode as it executes, pass `--trace`:

```bash
./build/ESPresso --trace examples/program.espr
```

To disassemble the compiled bytecode before execution, pass `--disasm`:

```bash
./build/ESPresso --disasm examples/program.espr
```

You can combine flags:

```bash
./build/ESPresso --trace --disasm examples/program.espr
```

To benchmark interpreter vs VM in-process, use `--bench` and optionally `--runs`:

```bash
./build/ESPresso --bench --runs 100 examples/bench.espr
```

---

## Language Features & Syntax

### Variables & Constants
Use `var` for mutable variables and `const` for immutable ones.
```javascript
var score = 100
const name = "ESPresso"

score += 50  # OK
name = "JavaScript" # Error!
```

### Data Types
- **Numbers**: Integers (`42`) and Floats (`3.14`)
- **Strings**: `"Hello World"` with `{interpolation}`
- **Booleans**: `true` and `false`
- **Null**: Represents nothingness (`null`)
- **Arrays**: Dynamic collections `[1, 2, 3]`

### Control Flow

#### If Statements
```javascript
if x > 100 {
    print("Large")
} else if x > 50 {
    print("Medium")
} else {
    print("Small")
}
```

#### Modern Loops
```javascript
# Range-based for loops
for var i in 0..5 {
    print("i is {i}")
}

# Standard while loops
var k = 3
while k > 0 {
    print(k)
    k -= 1
}
```

### Functions & Closures
First-class functions with full closure support
```javascript
fn makeMultiplier(factor) {
    fn multiply(n) {
        return n * factor
    }
    return multiply
}

const double = makeMultiplier(2)
print(double(10)) # 20
```

### Arrays
```javascript
var list = [10, 20, 30]
print(list[1])  # 20
list[0] = 5     # Mutation
list[2] *= 2    # Augmented assign on index
```

---

## How It Works

ESPresso has two execution paths:

1. **Bytecode VM (default)** — compiles AST to a `Chunk` and executes on a stack-based VM.
2. **AST Interpreter (`--interp`)** — executes the AST directly.

Core pipeline stages:
- **Lexing (`lexer.c`)**: turns source into tokens.
- **Parsing (`parser.c`)**: builds the AST.
- **Evaluating (`eval.c`)**: executes the AST using an environment (`env.c`).

### Bytecode VM Status
The VM is the default runtime and supports the features exercised in `examples/program.espr`.

---

## Testing

### Run All Tests
```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

### Targeted Test Scripts
All scripts live in `tests/` and accept the binary path and one or more program files.

```bash
# Interpreter and VM suites
tests/run_interp_suite.sh ./build/ESPresso examples/program.espr
tests/run_vm_suite.sh ./build/ESPresso examples/program.espr

# Parse error smoke tests (should not crash)
tests/run_parse_errors.sh ./build/ESPresso \
  examples/parse_errors/unclosed_string.espr \
  examples/parse_errors/missing_brace.espr \
  examples/parse_errors/bad_for.espr

# Trace + disasm output validation
tests/run_trace_disasm.sh ./build/ESPresso examples/program.espr

# Sanitizers / leak checks
tests/run_asan.sh ./build/ESPresso examples/program.espr
tests/run_valgrind.sh ./build/ESPresso examples/program.espr
tests/run_leaks.sh ./build/ESPresso examples/program.espr
```

### Benchmark (Interpreter vs VM)
To compare interpreter and VM performance, run:

```bash
tests/run_bench.sh ./build/ESPresso examples/bench.espr
```

To change the number of runs (default is 20), pass a third argument:

```bash
tests/run_bench.sh ./build/ESPresso examples/bench.espr 100
```

This runs the same program in both modes and prints timing results and speedup.

You can also benchmark directly with the binary:

```bash
./build/ESPresso --bench --runs 100 examples/bench.espr
```

---

## Project Structure

- `src/lexer.h/c`: Tokenizes raw source text.
- `src/parser.h/c`: Validates syntax and builds the AST.
- `src/ast.h`: Defines the structure of the AST nodes.
- `src/eval.h/c`: Executes the AST logic.
- `src/value.h/c`: Manages ESPresso data types (Strings, Arrays, numbers).
- `src/env.h/c`: Handles variable scoping and environments.
- `src/main.c`: The entry point that ties everything together.

---

## Status

Current:
- VM is the default runtime and passes the full `examples/program.espr` suite.
- Interpreter and VM both have line/column error reporting with readable traces.
- ASan and leaks tests pass in CI-style runs.

Next:
- Tighten parse-error tests to assert specific error messages and positions.
- Expand benchmarks to cover closures, arrays, and range-heavy workloads.
- Add a small standard library surface (core helpers like `len`, `typeOf`, `clockMs`).

Phase Status (Condensed):
- [x] Phase 0 — Stabilize Core
- [~] Phase 1 — VM Parity (in progress)
- [ ] Phase 2 — Reliability/Determinism
- [ ] Phase 3 — ESP‑32 Integration
- [ ] Phase 4 — Language Ergonomics
- [ ] Phase 5 — Ecosystem and Adoption

## Contributing
ESPresso is under active development. Feel free to explore the code and suggest improvements!
