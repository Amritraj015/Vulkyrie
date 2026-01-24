# Vulkyrie Game Engine

> [!WARNING]
> This project is a work in progress.

## Table of Contents

- [How to build from source](#how-to-build-from-source)
  - [Prerequisites](#prerequisites)
  - [Build Options](#build-options)
  - [Building with CMake Presets (Recommended)](#building-with-cmake-presets-recommended)
  - [Building without Presets](#building-without-presets)
- [Running the Applications](#running-the-applications)
  - [Sandbox Application](#sandbox-application)
  - [Asteroids Application](#asteroids-application)

## How to build from source

### Prerequisites

Make sure you have the following installed:

- **CMake** (version 3.27 or higher)
- A C/C++ compiler that supports C++23, such as:
  - **GCC**
  - **MSVC**
  - **Clang**
- **Ninja** build system (required for CMake presets) ([Can be downloaded from here](https://github.com/ninja-build/ninja/releases))

If you prefer not to use Ninja, you can build without presets as described [here](#building-without-presets).

### Build Options

The project supports several CMake options to control what gets built:

- `VULKYRIE_BUILD_EXAMPLES` - Build example applications (default: ON)
- `VULKYRIE_BUILD_CLI` - Build the vulky-cli tool (default: ON)
- `VULKYRIE_BUILD_TESTS` - Build tests (default: ON)
- `VULKYRIE_EXPORT_COMPILE_COMMANDS` - Export compile_commands.json (default: ON)

### Building with CMake Presets (Recommended)

The project includes CMake presets for different build configurations. **All presets use the Ninja generator.**

| Preset Name | Compiler | Targets | Configure & Build Commands |
|-------------|----------|---------|----------------------------|
| `all-debug` | System Default | All (Engine, Examples, CLI, Tests) | `cmake --preset all-debug && cmake --build --preset all-debug` |
| `examples-debug` | System Default | Examples only | `cmake --preset examples-debug && cmake --build --preset examples-debug` |
| `cli-debug` | System Default | CLI only | `cmake --preset cli-debug && cmake --build --preset cli-debug` |
| `tests-debug` | System Default | Tests only | `cmake --preset tests-debug && cmake --build --preset tests-debug` |
| `gcc-all-debug` | GCC | All (Engine, Examples, CLI, Tests) | `cmake --preset gcc-all-debug && cmake --build --preset gcc-all-debug` |
| `gcc-examples-debug` | GCC | Examples only | `cmake --preset gcc-examples-debug && cmake --build --preset gcc-examples-debug` |
| `gcc-cli-debug` | GCC | CLI only | `cmake --preset gcc-cli-debug && cmake --build --preset gcc-cli-debug` |
| `gcc-tests-debug` | GCC | Tests only | `cmake --preset gcc-tests-debug && cmake --build --preset gcc-tests-debug` |
| `clang-all-debug` | Clang | All (Engine, Examples, CLI, Tests) | `cmake --preset clang-all-debug && cmake --build --preset clang-all-debug` |
| `clang-examples-debug` | Clang | Examples only | `cmake --preset clang-examples-debug && cmake --build --preset clang-examples-debug` |
| `clang-cli-debug` | Clang | CLI only | `cmake --preset clang-cli-debug && cmake --build --preset clang-cli-debug` |
| `clang-tests-debug` | Clang | Tests only | `cmake --preset clang-tests-debug && cmake --build --preset clang-tests-debug` |

### Building without Presets

If you prefer not to use presets (does not require Ninja):

```bash
cmake -S . -B build
cmake --build build
```

## Running the Applications

### Sandbox Application

```bash
cd build/gcc-all-debug/examples/sandbox && ./sandbox
```

### Asteroids Application

```bash
cd build/gcc-all-debug/examples/asteroids && ./asteroids
```

**Note:** Adjust the build directory path based on which preset you used:

- Generic presets: `build/all-debug`, `build/examples-debug`, etc.
- GCC presets: `build/gcc-all-debug`, `build/gcc-examples-debug`, etc.
- Clang presets: `build/clang-all-debug`, `build/clang-examples-debug`, etc.

