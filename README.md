# Bismuth Compiler

> Bismuth Compiler — a hobby compiler written in C++ and LLVM, from lexer to object file.

This compiler does not aim to be a good tool to compile C++ or to replace any other existing program.
It is only a stand-alone project built as a personal learning project, to explore compiler design and LLVM from the ground up.

**Work in Progress**

Go to the "Quick Start" section to try the compiler, or read the code and let us know what you think ;)

---

## Overview

Bismuth is a personal compiler written from scratch in C++, using LLVM as backend code-generator. The project is built by a self-taught programmer, with the goal of publicly showcasing the code and gaining hands-on experience building a complete compiler — from tokenization to the production of an object file.

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
- LLVM 18.1.8
- CMake ≥ 3.16
- C++20 compiler (MinGW/UCRT64)

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

third-party/
├── mingw/
└── lld-libs/
```

**Installed release structure**
```
<prefix>/
bin/            
├── bismuth.exe             # the compiler executable 
|                           # there will also be included mingw runtime dlls
├── lld-link.exe
└── libs/                   # lld dependencies
    └── lib files (.a, .dll, .o)  
```

---

## Roadmap / TODO

The language is being expanded, with the goal of gradually moving toward a syntax closer to C++. Contributions and suggestions are welcome.
The main goal of this project its just the exploration of compilers for personal fun.

---

## License

Bismuth is licensed under the MIT License. See the [LICENSE](LICENSE) file for the full license text.

---

### Attribution

If you use, modify, or distribute Bismuth or a derivative work, please preserve the original copyright notice and clearly acknowledge Bismuth as the original project.

For example:

> This project is based on Bismuth Compiler by Leonardo Bersellini.
> Original project: https://github.com/leonardo-bersellini/MyCompiler

This attribution is requested to preserve the connection with the original Bismuth project and is in addition to the requirements of the MIT License.

---

### Third-party software

Bismuth uses third-party software, including:

- **LLVM** — Apache License 2.0 with LLVM Exceptions
- **MSYS2** — packages are distributed under their respective licenses

Third-party software remains subject to its own license terms.

---
