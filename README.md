# Bismuth Compiler

> Bismuth Compiler — a hobby compiler written in C++ with LLVM, from lexer to executable.

Bismuth is a standalone compiler written in C++, using LLVM as its backend, developed as a personal project to explore compiler design from the ground up.

**Work in Progress**

Go to the "Quick Start" section to try the compiler, or read the code and let me know what you think ;)

---

## Overview

Bismuth is a compiler written from scratch in C++, using LLVM as its backend for code generation. The project is built with the goal of publicly showcasing the code and gaining hands-on experience building a complete compiler — from tokenization to the production of an object file.

The project is continuously evolving, becoming progressively more complete and realistic while exploring compiler architecture, semantic analysis, and code generation with LLVM.

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
- **Object file** — LLVM compiles the generated IR into target-specific machine code and produces an object file.
- **Executable** — the object file is linked with the required runtime and linker components to produce an executable.

📖 In-depth documentation: see the documented code in the project.

---

## Language features

**Supported statements**
- Variable declaration and assignment
- Scope blocks
- `if` / `elif` / `else`
- `for`, `while`
- `break`, `continue`
- `switch`
- `const`, const declarations and parameters
- Function declaration, `return`
- `arrays`, complex types and static arrays

**Supported expressions**
- Arithmetic, comparison, and logical operators
- Literals (int, double, string, char, bool and arrays)
- Variables and function calls
- Parenthesized expressions
- Access to an array element

---

## Requirements

**To use the compiler** (pre-built release)

- No particular requirements — the release package includes the required linker and runtime libraries
- Windows 64-bit is the only supported os

**To build from source / contribute**
- LLVM 18.1.8
- CMake ≥ 3.16
- C++20 compiler
- MinGW/UCRT64 toolchain

---

## Quick Start

Choose one of the following options:

### • Download the compiled code from the latest release

Go to the release section and select the latest version, then download and extract the packages to use the compiler.

From this repository: `Releases` > `Bismuth Vx.x.x` > `bismuth-Vx.x.x-win64.zip`


### • Build from source

If you prefer to build this project from source, you can use CMake.

Download the repository source code from: `Code` > `Download zip`

Then follow those steps:

```bash
# clone the repository
git clone https://github.com/leonardo-bersellini/MyCompiler 
cd mycompiler

# then build with cmake
cmake -S . -B build -DLLVM_DIR="/path/to/llvm-install/lib/cmake/llvm"
cmake --build build
```

> Note: LLVM is not included in this repository. You need to have LLVM 18.1.8
> installed on your system and provide its CMake path through `LLVM_DIR`.

---

## CLI usage

| Flag | Function |
|------|----------|
| `-o <file>` | Compiles and produces an object file (`.obj`) |
| `-e <file>` | Compiles, links and produces an exe file (`.exe`) |
| `--llvm-ir` | Prints the generated LLVM IR to stdout |
| `--verbose` | Shows detailed information during execution |
| `--no-generation` | Compiles but does not generate any output file |
| `--hide-warnings` | Hide collected warnings |

---

## Testing

The project includes a test suite based on [Catch2](https://github.com/catchorg/Catch2) (v3), 
covering the Lexer, Parser, and other compiler components.

Tests are enabled by default in **Debug** builds and disabled in **Release**, so that building 
just the final executable doesn't require fetching/compiling Catch2.

The tests executable will be found in the bin directory with the other compiled files and can be launched using the Catch2 flags. (es: *bismuth_tests -r console -s*, which will print all stout of the tests in the console mode).

### Build and run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DLLVM_DIR="/path/to/llvm-install/lib/cmake/llvm"
cmake --build build
ctest --test-dir build
```

To enable/disable tests explicitly, regardless of build type:

```bash
cmake -B build -DBUILD_TESTING=ON   # or OFF
```

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

tests/
└── test_***.cpp

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

## Roadmap 

The language is continuously evolving, with new features and improvements being added over time. The long-term direction is to gradually move toward a more complete and realistic language and compiler, with syntax increasingly inspired by C++.

Contributions, suggestions, and feedback are welcome.

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
- **Catch2** — Boost Software License 1.0

Third-party software remains subject to its own license terms.

---
