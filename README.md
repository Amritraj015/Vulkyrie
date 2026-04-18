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
  - [Vulkyrie Editor](#vulkyrie-editor)
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
- **Ninja** build system (required for CMake presets) ([Can be downloaded from here](https://github.com/ninja-build/ninja/wiki/Pre-built-Ninja-packages))
- **vcpkg** with the `VCPKG_ROOT` environment variable set, since the presets use `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`

On Windows with **MSVC**, run CMake from an **x64 Developer PowerShell** or **x64 Developer Command Prompt** so `cl.exe`, the Windows SDK, and the STL headers are available to the preset.

If you prefer not to use Ninja, you can build without presets as described [here](#building-without-presets).

### Build Options

The project supports several CMake options to control what gets built:

- `VULKYRIE_BUILD_EXAMPLES` - Build example applications (default: ON)
- `VULKYRIE_BUILD_EDITOR` - Build the Vulkyrie editor application (default: ON)
- `VULKYRIE_BUILD_CLI` - Build the vulky-cli tool (default: ON)
- `VULKYRIE_BUILD_TESTS` - Build tests (default: ON)
- `VULKYRIE_EXPORT_COMPILE_COMMANDS` - Export compile_commands.json (default: ON)

### Building with CMake Presets (Recommended)

The project includes configure and build presets for multiple compilers and target sets. **All presets use the Ninja generator**, and the configure preset name matches the build preset name.

General usage:

```bash
cmake --preset <preset-name>
cmake --build --preset <preset-name>
```

#### System Default Compiler

- Debug presets: `all-debug`, `examples-debug`, `cli-debug`, `tests-debug`
- Release presets: `all-release`, `examples-release`, `cli-release`, `tests-release`
- `all-*` presets build the engine, editor, examples, CLI, and tests

#### GCC

##### Linux

- Debug presets: `gcc-all-debug`, `gcc-examples-debug`, `gcc-cli-debug`, `gcc-tests-debug`
- Release presets: `gcc-all-release`, `gcc-examples-release`, `gcc-cli-release`, `gcc-tests-release`

##### Windows (MinGW)

- Debug presets: `gcc-all-debug-win`, `gcc-examples-debug-win`, `gcc-cli-debug-win`, `gcc-tests-debug-win`
- Release presets: `gcc-all-release-win`, `gcc-examples-release-win`, `gcc-cli-release-win`, `gcc-tests-release-win`

#### Clang

- Debug presets: `clang-all-debug`, `clang-examples-debug`, `clang-cli-debug`, `clang-tests-debug`
- Release presets: `clang-all-release`, `clang-examples-release`, `clang-cli-release`, `clang-tests-release`

#### MSVC (Windows)

- Debug presets: `msvc-all-debug`, `msvc-examples-debug`, `msvc-cli-debug`, `msvc-tests-debug`
- Release presets: `msvc-all-release`, `msvc-examples-release`, `msvc-cli-release`, `msvc-tests-release`
- Use these presets from an x64 Visual Studio developer shell

Example:

```powershell
cmake --preset msvc-all-debug
cmake --build --preset msvc-all-debug
```

### Building without Presets

If you prefer not to use presets (does not require Ninja):

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build
```

On PowerShell, use `$env:VCPKG_ROOT`. On Windows Command Prompt, use `%VCPKG_ROOT%`.

## Running the Applications

Executables are written under `build/<preset-name>/...`. On Windows, use the `.exe` suffix.

### Vulkyrie Editor

```bash
build/<preset-name>/editor/editor
```

### Sandbox Application

```bash
build/<preset-name>/examples/sandbox/sandbox
```

### Asteroids Application

```bash
build/<preset-name>/examples/asteroids/asteroids
```

Examples:

- `build/clang-all-debug/examples/sandbox/sandbox`
- `build/gcc-all-debug-win/examples/asteroids/asteroids.exe`
- `build/msvc-all-debug/editor/editor.exe`
