# C++ Build Toolchain Fundamentals

This document explains core C++ compilation and build concepts in the context of Personal Quant Lab.

## Overview: From Source to Executable

```
Source Code (.cpp, .h)
        ↓
   Preprocessor
        ↓
  Translation Unit
        ↓
    Compiler
        ↓
  Object Files (.obj)
        ↓
    Linker
        ↓
  Executable (.exe)
```

When you run `cmake --build build`, the build system executes this pipeline for every file in your project.

---

## Stage 1: Preprocessing & Translation Units

### Translation Unit
A **translation unit** is a single `.cpp` file plus all headers it includes (directly or indirectly). The preprocessor expands this into a complete source tree, which is then compiled to a single `.obj` file.

Example: When compiling `src/main.cpp`:
1. Preprocessor reads `#include <iostream>`
2. Copies the entire `iostream` header into the translation unit
3. Preprocessor expands any `#define` macros
4. Result: A large intermediate file (never saved to disk by default, but this is what the compiler sees)

**Key point:** Each `.cpp` file becomes exactly one translation unit and exactly one `.obj` file. Headers are NOT compiled directly; they're included into `.cpp` files.

### Header Files
A **header** is a file (typically `.h` or `.hpp`) that declares types, functions, classes, and constants. Headers are `#include`-ed into `.cpp` files to share declarations.

Example:
```cpp
// myclass.h
class MyClass {
public:
    void doSomething();
};

// main.cpp
#include "myclass.h"  // <- Header is inserted here by preprocessor
int main() {
    MyClass obj;
    obj.doSomething();
}
```

Headers should declare, not define. Definitions belong in `.cpp` files (with rare exceptions like inline functions and templates).

---

## Stage 2: Compilation

### Compiler
A **compiler** is a program that translates human-readable C++ code into machine-readable object code. On Windows, we use MSVC (Microsoft Visual C++). On Linux, we use GCC or Clang.

The compiler's job:
1. Parse C++ syntax
2. Type-check function calls and expressions
3. Optimize the code
4. Emit object code (`.obj` files)

For example, when compiling this code:
```cpp
int add(int a, int b) {
    return a + b;
}
```

The compiler generates machine instructions that add two integers and return the result. On x86-64, this might be:
```
mov rax, rdi
add rax, rsi
ret
```

**Compiler flags in PQL:**
- MSVC: `/W4 /WX` → treat all warnings as errors, pedantic checking
- GCC/Clang: `-Wall -Wextra -Wpedantic -Werror` → equivalent strictness

The compiler does NOT resolve external symbol references (like calling a function defined in another `.cpp` file). That's the linker's job.

---

## Stage 3: Linking

### Linker
A **linker** is a program that combines multiple object files (`.obj`) into a single executable (`.exe`) or library. It resolves symbol references, manages memory layout, and embeds dependencies.

Example: If `main.cpp` calls `add()` from `math.cpp`:
1. Compiler translates `main.cpp` to `main.obj` with an undefined reference to `add`
2. Compiler translates `math.cpp` to `math.obj` with a defined symbol `add`
3. Linker reads both `.obj` files, finds the definition of `add` in `math.obj`, and replaces the undefined reference in `main.obj` with the correct memory address
4. Result: A single `.exe` with all symbols resolved

The linker also:
- Combines all read-only data (strings, constants)
- Allocates memory for global and static variables
- Creates import/export tables for external libraries
- Generates debug information (if requested)

---

## CMake and Targets

### CMake Target
A **CMake target** is a logical entity that represents something to build: an executable, a library, or a test. CMake doesn't compile; it generates the instructions for Ninja (or make, or Visual Studio) to compile.

Example:
```cmake
# CMakeLists.txt
add_executable(pql_app main.cpp)  # This creates a target named "pql_app"
target_compile_options(pql_app PRIVATE -Wall)  # Add compile options to this target
target_link_libraries(pql_app PRIVATE mylib)  # Link this target against mylib
```

When you run `cmake --build build`, CMake has already generated Ninja build files. Ninja then:
1. Compiles `main.cpp` with the specified options
2. Links the result against `mylib`
3. Produces `pql_app.exe`

**Targets isolate build rules:** Each target can have different compiler flags, linked libraries, and dependencies. This is how we enable sanitizers for some targets and disable them for others, or link different tests against different libraries.

### Target Properties
A target can have many properties:
- **Source files:** Which `.cpp`/`.c` files to compile
- **Compiler flags:** Warnings, optimization level, language standard
- **Linked libraries:** Other targets or system libraries to link against
- **Include directories:** Where to find headers
- **Compile definitions:** `-D` macros to pass to the compiler

In PQL, we use target properties to:
- Enforce C++20 on all targets
- Apply sanitizers selectively
- Enable warnings and treat them as errors

---

## Static Library vs. Executable

### Static Library
A **static library** is a collection of compiled object files bundled into a single `.lib` (Windows) or `.a` (Linux) file. When you link against a static library, the linker copies the relevant object code into your executable.

Example:
```cmake
add_library(pql_core STATIC core.cpp math.cpp)  # Create a static library
add_executable(pql_app main.cpp)                # Create an executable
target_link_libraries(pql_app pql_core)         # Link the executable against the library
```

Result: `pql_app.exe` contains all the code from `core.cpp` and `math.cpp` plus `main.cpp`.

Advantages:
- Organize code into modules
- Reuse libraries across multiple executables
- Easier to test individual modules

### Executable
An **executable** is the final, runnable program. It contains compiled machine code, data sections, import/export tables, and debug information (if included).

When you run an executable, the operating system:
1. Loads it into memory
2. Resolves any external dependencies (like system DLLs)
3. Starts execution at the entry point (`main()` in C++)

---

## Build System Architecture (PQL)

In Personal Quant Lab:

```
CMakeLists.txt (root)
├── Declares project, C++ standard, warnings, sanitizers
├── add_subdirectory(src)
└── add_subdirectory(test)

src/CMakeLists.txt
├── add_executable(pql_app main.cpp)
└── Apply C++20 standard to pql_app

test/CMakeLists.txt
├── Fetch GoogleTest v1.14.0
├── add_executable(pql_tests test_smoke.cpp)
├── Link against gtest_main
└── Register with CTest (add_test)
```

When you run `cmake -S . -B build -G Ninja`:
1. CMake reads root `CMakeLists.txt`
2. CMake evaluates sanitizer option
3. CMake reads `src/CMakeLists.txt` and `test/CMakeLists.txt`
4. CMake generates Ninja build graph: `build/build.ninja`

When you run `cmake --build build`:
1. Ninja reads `build/build.ninja`
2. Ninja invokes MSVC (or GCC/Clang) for each translation unit
3. Ninja invokes the linker for each target
4. Result: executables in `build/src/` and `build/test/`

---

## Putting It Together: A Complete Example

Suppose we have:

**src/math.cpp:**
```cpp
int add(int a, int b) {
    return a + b;
}
```

**src/math.h:**
```cpp
int add(int a, int b);
```

**src/main.cpp:**
```cpp
#include "math.h"
#include <iostream>

int main() {
    std::cout << "2 + 3 = " << add(2, 3) << std::endl;
    return 0;
}
```

**CMakeLists.txt:**
```cmake
cmake_minimum_required(VERSION 3.20)
project(Example)
set(CMAKE_CXX_STANDARD 20)
add_executable(example main.cpp math.cpp)
```

**Build process:**

1. **CMake phase:** Generate `build.ninja`
2. **Compiler phase (MSVC):**
   - Compile `main.cpp`: Preprocessor includes `math.h`, compiler generates `main.obj` (with undefined reference to `add`)
   - Compile `math.cpp`: Compiler generates `math.obj` (with defined symbol `add`)
3. **Linker phase:**
   - Read `main.obj` and `math.obj`
   - Resolve: `main.obj` references `add` → found in `math.obj`
   - Link `iostream` (from standard library)
   - Produce: `example.exe`
4. **Run:** Execute `example.exe` → outputs "2 + 3 = 5"

---

## Summary Table

| Concept | Role | Input | Output |
|---------|------|-------|--------|
| **Header** | Declare types, functions, classes | `.h` file | Included into `.cpp` files |
| **Translation Unit** | Single compilable unit | `.cpp` + included headers | `.obj` file |
| **Compiler** | Translate C++ to machine code | `.cpp` file + headers | `.obj` file (machine code) |
| **Object File** | Compiled code + symbols | Compiler output | `.obj` (Windows) or `.o` (Linux) |
| **Linker** | Combine object files + libraries | Multiple `.obj` files | Executable or library |
| **Static Library** | Bundled object files | Multiple `.obj` files | `.lib` (Windows) or `.a` (Linux) |
| **Executable** | Runnable program | `.obj` files + libraries | `.exe` (Windows) or ELF (Linux) |
| **CMake Target** | Logical build entity | Source files + options | Build instructions for Ninja/make |
| **Ninja** | Build executor | `build.ninja` from CMake | Compiled code via compiler/linker |

---

## References

- [C++ Standard (cppreference)](https://en.cppreference.com/)
- [CMake Documentation](https://cmake.org/cmake/help/latest/)
- [MSVC Compiler Reference](https://learn.microsoft.com/en-us/cpp/)
- [GCC Manual](https://gcc.gnu.org/onlinedocs/)