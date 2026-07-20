# Bismuth Compiler

> Bismuth Compiler — a hobby compiler written in C++ with Qt and LLVM, from lexer to object file.

This compiler does not aim to be a good tool to compile C++ or to replace any other existing program.
It is only a stand-alone project built as a personal learning project, to explore compiler design and LLVM from the ground up.

**Work in Progress**

Go to the "Quick Start" section to try the compiler, or read the code and let us know what you think ;)

---

## Overview

Bismuth is a personal compiler written from scratch in C++, using Qt as a support framework and LLVM as the code generation backend. The project is built by a self-taught programmer, with the goal of publicly showcasing the code and gaining hands-on experience building a complete compiler — from tokenization to the production of an object file.

It is not intended as a production-ready language, but as a learning path through compiler architecture, syntactic/semantic analysis, and code generation via LLVM.

---

## Pipeline

```
 ┌─────────┐     ┌─────────┐     ┌─────────┐     ┌───────────────────┐     ┌─────────┐     ┌─────────────┐     ┌────────────┐
 │  Lexer  │ --> │ Parser  │ --> │   AST   │ --> │  SemanticAnalyzer │ --> │ Codegen │ --> │ Object file │ --> │ Executable │
 └─────────┘     └─────────┘     └─────────┘     └───────────────────┘     └─────────┘     └─────────────┘     └────────────┘
```

- **Lexer** — turns the source code into a sequence of tokens
- **Parser** — builds the AST from the tokens, checking syntax
- **AST** — tree representation of the program, Expr/Stmt nodes
- **SemanticAnalyzer** — checks types, scopes, and the language's semantic rules
- **Codegen** — translates the AST into LLVM IR
- **Object file** — the IR is compiled into an object file through the LLVM backend

📖 In-depth documentation: see the documented code in the project.

---

## Language features

**Supported statements**
- Variable declaration and assignment
- Scope blocks
- `if` / `elif` / `else`
- `for`, `while`
- `break`, `continue`
- Function declaration, `return`

**Supported expressions**
- Arithmetic, comparison, and logical operators
- Literals (int, double, string, char, bool)
- Variables and function calls
- Parenthesized expressions

---

## Requirements

**To use the compiler** (pre-built release)
- No particular requirements — the necessary linker is already included in the with all the lib files

**To build from source / contribute**
- Qt6
- LLVM 18.1.8
- CMake ≥ 3.16
- C++17 compiler (MinGW/UCRT64)

---

## Quick Start


**Download the compiled code from the latest release**

Go to the release section and select a version-release, then download and exctract the packages to use the compiler.

From this repository: `Releases` > `Bismuth Vx.x.x` > `Download zip`


---

## CLI usage

| Flag | Function |
|------|----------|
| `-o <file>` | Compiles and produces an object file (`.obj`) |
| `-e <file>` | Compiles, link and produces an exe file (`.exe`) |
| `--llvm-ir` | Prints the generated LLVM IR to stdout |
| `--verbose` | Shows detailed information during execution |

---

## Project structure

**Development structure**
```
compiler/
├── codegen/
├── constants/
├── driver/
├── errorlog/
├── lexer/
├── parser/
└── semantics/
```

**Installed release structure**
```
<prefix>/
bin/            
├── bismuth.exe             # the compiler executable
├── Qt6 dll                 # qt dll and dependencies
|
└── lld/                    # contains the linker and all of its dependencies
    ├── lld-link.exe
    └── libs/
        └── lib files (.a, .dll, .o)  
```

---

## Roadmap / TODO

The language is being expanded, with the goal of gradually moving toward a syntax closer to C++. Contributions and suggestions are welcome.
The main goal of this project its just the exploration of compilers for personal fun.

---

## Licence and Third-Part Tools

This project has no licence as it is just a personal project for fun.

Jet it uses third-part software and tools, such as:
- Qt6 from Qt Group
- Ucrt64 from Msys2 project

These components remain subject to their respective original licenses.
This is a personal, non-commercial project distributed free of charge and provided without any warranty of any kind.

If you desire to create a different project based on the code of this repository, feel free to do it. If you want, we would be very happy if you mentioned this code as starting point or inspiration for your own project.

---
