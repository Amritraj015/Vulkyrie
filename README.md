# Vulkyrie Game Engine

> [!WARNING]
> This project is a work in progress.

## How to build from source

### Prerequisites

Make sure you have the following installed:
- **CMake** (version 3.27 or higher)
- A C/C++ compiler:
  - **GCC** v15.2.0+ (recommended for Linux)
  - **MSVC** v19+ (recommended for Windows)
  - **Clang** v20+
- **Ninja** build system ([download here](https://github.com/ninja-build/ninja/releases/tag/v1.13.2))

### Build Options

The project supports several CMake options to control what gets built:
- `VULKYRIE_BUILD_EXAMPLES` - Build example applications (default: ON)
- `VULKYRIE_BUILD_CLI` - Build the vulky-cli tool (default: ON)
- `VULKYRIE_BUILD_TESTS` - Build tests (default: ON)
- `VULKYRIE_EXPORT_COMPILE_COMMANDS` - Export compile_commands.json (default: ON)

### Building with CMake Presets (Recommended)

The project includes CMake presets for different build configurations. From the root project directory:

#### Generic Presets (uses system default compiler)

**Build all targets (engine, examples, CLI, tests):**
```bash
cmake --preset all-debug
cmake --build --preset all-debug
```

**Build only examples:**
```bash
cmake --preset examples-debug
cmake --build --preset examples-debug
```

**Build only CLI:**
```bash
cmake --preset cli-debug
cmake --build --preset cli-debug
```

**Build only tests:**
```bash
cmake --preset tests-debug
cmake --build --preset tests-debug
```

#### GCC-Specific Presets

**Build all targets with GCC:**
```bash
cmake --preset gcc-all-debug
cmake --build --preset gcc-all-debug
```

**Build only examples with GCC:**
```bash
cmake --preset gcc-examples-debug
cmake --build --preset gcc-examples-debug
```

**Build only CLI with GCC:**
```bash
cmake --preset gcc-cli-debug
cmake --build --preset gcc-cli-debug
```

**Build only tests with GCC:**
```bash
cmake --preset gcc-tests-debug
cmake --build --preset gcc-tests-debug
```

#### Clang-Specific Presets

**Build all targets with Clang:**
```bash
cmake --preset clang-all-debug
cmake --build --preset clang-all-debug
```

**Build only examples with Clang:**
```bash
cmake --preset clang-examples-debug
cmake --build --preset clang-examples-debug
```

**Build only CLI with Clang:**
```bash
cmake --preset clang-cli-debug
cmake --build --preset clang-cli-debug
```

**Build only tests with Clang:**
```bash
cmake --preset clang-tests-debug
cmake --build --preset clang-tests-debug
```

### Building without Presets

If you prefer not to use presets:
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