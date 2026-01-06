# Extended PL/0 Compiler

An advanced compiler and interpreter for the Extended PL/0 language, built with C++17. This project features a robust CLI, an optional Qt5-based GUI, and support for modern PL/0 extensions including arrays, for-loops, and heap allocation.

## Features

- **Core Compiler**:
  - **Lexer**: Efficient tokenization with detailed source tracking.
  - **Parser**: Recursive descent parser with comprehensive error recovery.
  - **Semantic Analysis**: Symbol table management and scope handling.
  - **Code Generation**: Emits P-Code for the virtual machine.
  - **Interpreter**: Stack-based VM to execute the generated P-Code.
  - **Optimizer**: Multi-pass optimization (Constant Folding, Dead Code Elimination, Strength Reduction).

- **Language Extensions**:
  - **Data Structures**: Support for single and multi-dimensional arrays.
  - **Control Flow**: `if-then-else`, `while-do`, `repeat-until`, and `for-loop` constructs.
  - **Memory Management**: Explicit heap allocation/deallocation (`new`/`free`).
  - **Procedures**: Support for nested procedures and recursion.

- **Tools**:
  - **Smart CLI**: Intelligent file resolution and colorful diagnostics.
  - **Debug Modes**: View Tokens, AST, Symbol Table, and P-Code dumps.
  - **Trace Mode**: Step-by-step P-Code execution tracing.
  - **Test Runner**: Built-in batch testing framework.
  - **GUI**: Qt5-based IDE with syntax highlighting and compiler visualization.

## EBNF Grammar

### PL/0 BNF Description (Extended Backus-Naur Form)

```bnf
<prog> → program <id>; <block>
<block> → [<condecl>][<vardecl>][<proc>]<body>
<condecl> → const <const>{, <const>};
<const> → <id> := <integer>
<vardecl> → var <vardef>{, <vardef>};
<vardef> → <id> [ : (integer | pointer) | [ <integer> ] ]
<proc> → procedure <id>([<id>{, <id>}]); <block>; { <proc> }
<body> → begin <statement>{; <statement>} end
<statement> → <id> [ [ <exp> ] ] := <exp>
            | * <exp> := <exp>
            | if <lexp> then <statement> [else <statement>]
            | while <lexp> do <statement>
            | for <id> := <exp> (to | downto) <exp> do <statement>
            | call <id>([<exp>{, <exp>}])
            | <body>
            | read (<id>{, <id>})
            | write (<exp>{, <exp>})
            | new (<id>, <exp>)
            | delete (<id>)
<lexp> → <exp> <lop> <exp> | odd <exp>
<exp> → [+|-]<term>{<aop><term>}
<term> → <factor>{<mop><factor>}
<factor> → <id> [ [ <exp> ] ] | <integer> | (<exp>) | & <id> [ [ <exp> ] ] | * <factor>
<lop> → = | <> | < | <= | > | >=
<aop> → + | -
<mop> → * | / | %
<id> → l{l|d}   (Note: l for letter)
<integer> → d{d}

Legend:
<prog>: Program; <block>: Block; <condecl>: Constant Declaration; <const>: Constant; 
<vardecl>: Variable Declaration; <proc>: Procedure; <body>: Compound Statement; 
<statement>: Statement; <exp>: Expression; <lexp>: Condition; <term>: Term; 
<factor>: Factor; <aop>: Additive Operator; <mop>: Multiplicative Operator; 
<lop>: Relational Operator.
```

## Prerequisites

- **C++ Compiler**: C++17 compatible (GCC, Clang, or MSVC)
- **CMake**: Version 3.16 or higher
- **Qt5** (Optional): Required only for building the GUI (`pl0gui`).
  - Components: `Qt5Core`, `Qt5Widgets`

## Build Instructions

Clone the repository and build using CMake:

```bash
mkdir build
cd build
cmake ..
make
```

### Build Options

- `-DCMAKE_BUILD_TYPE=Debug`: Build with debug symbols.
- `-DCMAKE_BUILD_TYPE=Release`: Build with optimizations (default).

If Qt5 is found on your system, the GUI application `pl0gui` will be built automatically. If not found, only the CLI `pl0c` will be built.

## Usage

### Command Line Interface (CLI)

The `pl0c` executable is the main entry point for the compiler.

**Basic Compilation and Execution:**

```bash
./pl0c source.pl0
```

**Show Generated P-Code:**

```bash
./pl0c source.pl0 --code
```

**Full Debug Output:**

```bash
./pl0c source.pl0 --all
```

**Available Options:**

| Option | Description |
|--------|-------------|
| `-h, --help` | Display help message |
| `-v, --version` | Display version information |
| `--tokens` | Print lexer token sequence |
| `--ast` | Print abstract syntax tree |
| `--sym` | Print symbol table |
| `--code` | Print generated P-Code instructions |
| `--all` | Enable all debug outputs |
| `--trace` | Trace P-Code execution step by step |
| `--no-run` | Compile only, do not execute |
| `--no-color` | Disable colored output |
| `--test [dir]` | Run batch tests (default: `test/`) |
| `-O, --optimize` | Enable experimental optimizations |
| `-d, --debug` | Enter interactive debug mode (break, step, run) |

### Graphical User Interface (GUI)

If built, run the GUI application:

```bash
./pl0gui
```

The GUI provides:
- Code editor with syntax highlighting.
- Real-time visualization of Tokens, Symbol Table, and P-Code.
- Console output for execution results.

## Testing

The project includes a comprehensive test suite in the `test/` directory, categorized by component (Lexer, Parser, etc.) and expectation (Correct, Error).

**Run all tests:**

```bash
./pl0c --test
```

**Run specific test category:**

```bash
./pl0c --test test/parser
```

**Test Directory Structure:**
- `correct/`: Files strictly obeying the grammar.
- `error/`: Files containing specific errors for testing recovery/reporting.

## Documentation

Detailed documentation is available in the `doc/` directory:
- System Design and Architecture
- Usage Guide
- Test Reports
- `Extension_Plan.md`: detailed plan for future extensions.
- `GUI_Debugger_Integration_Guide.md`: guide for GUI debugger.
- `Test_Report.md`: comprehensive test results.

## License

This project is licensed under the MIT License.
Copyright (c) 2025.
