# Personal Quant Lab

A C++20 financial analysis system for testing investment edge through disciplined experimentation.

## Quick Start

### Prerequisites

- **CMake** 3.20 or later
- **Ninja** build system
- **C++20** capable compiler:
  - MSVC 19.30+ (Visual Studio 2022)
  - GCC 11+
  - Clang 14+

### Build Instructions

#### Configure the build:
```bash
cmake -S . -B build -G Ninja
```

This command:
- `-S .` — source directory (current)
- `-B build` — build directory
- `-G Ninja` — use Ninja generator

#### Build the project:
```bash
cmake --build build
```

#### Run tests:
```bash
ctest --test-dir build --verbose
```

### Clean Build

To remove all build artifacts:
```bash
rm -rf build        # macOS/Linux
rmdir /s build      # Windows PowerShell
```

## Project Structure

```
personal-quant-lab/
├── CMakeLists.txt          Main CMake configuration
├── README.md               This file
├── .gitignore              Git ignore rules
├── src/
│   ├── CMakeLists.txt      Source configuration
│   └── main.cpp            Minimal executable entry point
├── test/
│   ├── CMakeLists.txt      Test configuration (GoogleTest)
│   └── test_smoke.cpp      Smoke test suite
└── build/                  Generated build artifacts (after cmake)
```

## Architecture

### Build System

- **CMake** 3.20+: Modern, target-based architecture
- **Ninja**: Fast, parallel builds
- **GoogleTest**: Unit testing framework (auto-fetched via FetchContent)
- **CTest**: Test runner integration

### Compiler Configuration

**MSVC (Windows)**:
- `/W4` — Enable all warnings
- `/WX` — Treat warnings as errors
- `/wd4068` — Suppress "unknown pragma" for external deps
- `/wd4996` — Suppress deprecated warnings for stdlib

**GCC/Clang**:
- `-Wall -Wextra -Wpedantic` — Enable all warnings
- `-Werror` — Treat warnings as errors

### C++ Standard

- **C++20** (required)
- No extensions (`CMAKE_CXX_EXTENSIONS OFF`)
- Standard features only (no compiler-specific code)

## Development

### Adding New Source Files

1. Add `.cpp` files to `src/`
2. Update `src/CMakeLists.txt` to include new sources
3. Rebuild: `cmake --build build`

### Adding New Tests

1. Add test `.cpp` files to `test/`
2. Update `test/CMakeLists.txt` to register tests
3. Run tests: `ctest --test-dir build --verbose`

### Compiler Flags

All compiler flags are set in the root `CMakeLists.txt`:
- Global warning levels
- C++ standard
- Platform-specific settings

Modify these to adjust build strictness.

## Continuous Integration

When building in CI/CD:

```bash
# Fresh clone
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --verbose
```

The build will fail if:
- Any source file fails to compile
- Any warning is generated (treated as error)
- Any test fails

## PQL Architecture

Personal Quant Lab follows a disciplined design:

1. **Betting Framework** (PQL-001): Define falsifiable hypotheses
2. **Build System** (PQL-002): Production-grade C++20 foundation
3. **Portfolio Tracking** (PQL-003): Track real and paper positions
4. **Strategy Framework** (PQL-004): Run systematic strategies
5. **Backtesting** (PQL-005): Test strategies deterministically
6. **Analytics** (PQL-006): Analyze results vs. benchmarks

This ticket (PQL-002) provides the foundation for all domain code in later tickets.

## Status

**v0.1 Bootstrap**: Minimal C++20 executable + GoogleTest integration

No financial domain logic yet. See PQL-003 for portfolio tracking.

---

For questions or issues, see `.codex/AGENTS.md` for project governance.
