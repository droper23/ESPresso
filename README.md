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
By default, ESPresso runs the **bytecode VM** (currently minimal feature set).

```bash
./build/ESPresso examples/program.espr
```

### 3. VM Minimal Example
The VM currently supports literals, arithmetic/logic, globals, `if`, `while`, and `print`. Here is a small VM-compatible program:

```espresso
print("Arithmetic:", 10 + 20 * 2)

var x = 1
x = x + 2
print("x =", x)

var k = 3
while k > 0 {
    print("k =", k)
    k = k - 1
}

if 3 < 5 {
    print("ok")
}
```

Run it with:

```bash
./build/ESPresso examples/test.espr
```

To run the **AST interpreter** (full language support), pass `--interp`:

```bash
./build/ESPresso --interp examples/program.espr
```

To print every VM opcode as it executes, pass `--trace`:

```bash
./build/ESPresso --trace examples/test.espr
```

---

## Language Features & Syntax

### Variables & Constants
Use `var` for mutable variables and `const` for immutable ones.
```espresso
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
```espresso
if x > 100 {
    print("Large")
} else if x > 50 {
    print("Medium")
} else {
    print("Small")
}
```

#### Modern Loops
```espresso
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
```espresso
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
```espresso
var list = [10, 20, 30]
print(list[1])  # 20
list[0] = 5     # Mutation
list[2] *= 2    # Augmented assign on index
```

---

## How It Works

ESPresso now has two execution paths:

1.  **Bytecode VM (default)** — compiles AST to a `Chunk` and executes on a stack-based VM. This is currently minimal (literals, arithmetic/logic, globals, `if`, `while`, `print`).
2.  **AST Interpreter (`--interp`)** — full language support for arrays, `for` loops, functions, and closures.

1.  **Lexing (`lexer.c`)**: The source code is broken down into small pieces called **Tokens** (e.g., `var`, `identifier`, `+`, `number`).
2.  **Parsing (`parser.c`)**: Tokens are organized into a tree structure called an **Abstract Syntax Tree (AST)**. This defines the hierarchy and rules of your code.
3.  **Evaluating (`eval.c`)**: The interpreter "walks" through the tree, executing each node's logic step-by-step using an **Environment (`env.c`)** to store variables.

### Bytecode VM Status
The VM is the default runtime, but it is intentionally limited for now. For full language support, use the interpreter via `--interp`.

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

## Contributing
ESPresso is under active development. Feel free to explore the code and suggest improvements!
