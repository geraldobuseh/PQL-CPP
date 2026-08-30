# C++20 Build Toolchain Pipeline

## Complete Build Flow

```
CMakeLists.txt
      ↓
    CMake (generate build graph)
      ↓
    Ninja (build executor)
      ↓
    MSVC (C++ compiler)
      ↓
  object files
      ↓
   Linker (resolve symbols + link libraries)
      ↓
 pql.exe (executable)
```

---

## Stage 1: Source to Translation Unit

**Input**: `main.cpp` (human-readable C++)

**Process**: Preprocessing
- Each `.cpp` file becomes an independent translation unit
- Preprocessor handles `#include`, `#define`, conditional compilation

---

## Stage 2: Compilation

**Input**: Translation unit (preprocessed C++)  
**Output**: `main.obj` (object code)

**Compiler**: MSVC
- Translates C++20 source into machine code
- Applies compiler flags (`/W4`, `/WX`, C++20 standard)
- Generates object files

---

## Stage 3: Linking

**Input**: Object files + libraries  
**Output**: `pql.exe` (executable)

**Process**: Linker
- Resolves symbols across all object files
- Links external libraries
- Creates final executable binary

---

## Build System: CMake

**Role**: Does NOT compile C++

**Responsibility**: Describes the build graph

```
CMakeLists.txt  →  CMake  →  Ninja build files
```

---

## Build Executor: Ninja

**Role**: Executes the build graph

**Process**: Reads CMake-generated build graph and invokes MSVC compiler and linker

---

## Key Concepts

| Tool | Role |
|------|------|
| **CMake** | "What to build" (declarative) |
| **Ninja** | "How to build" (execution engine) |
| **MSVC** | "Compile C++ to object code" (compiler) |
| **Linker** | "Connect objects and libraries" (final assembly) |