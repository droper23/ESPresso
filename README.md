<p align="center">
  <img src="./assets/ESPresso_real.png" width="800" alt="ESPresso Hero">
</p>

# ESPresso ☕️

ESPresso is a lightweight, modern scripting language designed specifically for the ESP32 and other embedded systems. It combines C-like performance with the expressive power of modern high-level languages.

## Quick Start

### 1. Compile the Interpreter
You need a C compiler (like `clang` or `gcc`). Run the following command in your terminal:

```bash
clang -std=c11 -Wall -Wextra -o ESPresso main.c lexer.c parser.c eval.c env.c value.c
```

### 2. Run a Script
Create a file named `hello.espr`:
```espresso
print("Hello from ESPresso!")
```

Then run it:
```bash
./ESPresso hello.espr
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

ESPresso currently operates as an **AST-Walking Interpreter**.

1.  **Lexing (`lexer.c`)**: The source code is broken down into small pieces called **Tokens** (e.g., `var`, `identifier`, `+`, `number`).
2.  **Parsing (`parser.c`)**: Tokens are organized into a tree structure called an **Abstract Syntax Tree (AST)**. This defines the hierarchy and rules of your code.
3.  **Evaluating (`eval.c`)**: The interpreter "walks" through the tree, executing each node's logic step-by-step using an **Environment (`env.c`)** to store variables.

### The Future: Bytecode VM
I am currently working on transitioning ESPresso to a **Stack-based Bytecode VM**. This will make ESPresso significantly faster and more memory-efficient by compiling the AST into a flat list of instructions (bytecode) instead of walking a tree.

---

## Project Structure

- `lexer.h/c`: Tokenizes raw source text.
- `parser.h/c`: Validates syntax and builds the AST.
- `ast.h`: Defines the structure of the AST nodes.
- `eval.h/c`: Executes the AST logic.
- `value.h/c`: Manages ESPresso data types (Strings, Arrays, numbers).
- `env.h/c`: Handles variable scoping and environments.
- `main.c`: The entry point that ties everything together.

---

## Contributing
ESPresso is under active development. Feel free to explore the code and suggest improvements!
