# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

Vulkyrie is a C++26 game engine (work in progress) with its own ECS, a custom rigid-body physics engine, and an
OpenGL renderer (Vulkan backend planned). The repo builds several targets on top of the `engine` static library:
`editor` (ImGui-based), `examples/sandbox` and `examples/asteroids`, `vulky-cli`, `runtime`, `tests` (Catch2), and
`benchmarks` (Catch2 microbenchmarks).

## Build commands

This repo ships slash-command skills that wrap the workflows below — prefer them: `/build [preset]` (configure +
build, default `clang-all-debug`), `/test [preset] [name-or-tag]` (build + run the Catch2 suite), and
`/format-check [base-ref]` (clang-format + clang-tidy on files changed from `main`, read-only). The details below
apply when running the underlying commands directly.

Building requires **CMake 4.2+**, a C++26 compiler (GCC/Clang/MSVC), **Ninja**, and **vcpkg** with `VCPKG_ROOT` set
(presets reference `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`). Dependencies (glfw3, glad, glm, imgui,
assimp, openal-soft, stb, catch2) are resolved via `vcpkg.json`/`Dependencies.cmake`.

### Presets (recommended)

```bash
cmake --preset <preset-name>
cmake --build --preset <preset-name>
```

Preset names follow `[<compiler>-]<target-set>-<config>[-win]`:
- Compiler prefix: none (system default), `gcc-`, `clang-`, `msvc-` (MSVC presets must be run from an x64 Developer
  shell; `-win` variants are GCC/MinGW on Windows).
- Target set: `all` (engine + editor + examples + CLI + tests + benchmarks), `examples`, `cli`, `tests`,
  `benchmarks`.
- Config: `debug`, `release`.

Day-to-day local development normally uses **`clang-all-debug`** (this is the default VS Code build task).

### Without presets

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build
```

### Build options (`cmake -D<OPTION>=OFF/ON`)

`VULKYRIE_BUILD_EXAMPLES`, `VULKYRIE_BUILD_EDITOR`, `VULKYRIE_BUILD_CLI`, `VULKYRIE_BUILD_TESTS` (all default ON),
`VULKYRIE_BUILD_BENCHMARKS` (default OFF; enabled by the `benchmarks-*` and `all-*` presets),
`VULKYRIE_EXPORT_COMPILE_COMMANDS` (default ON; copies `compile_commands.json` to the repo root for clangd/clang-tidy).

### Running built binaries

Executables land under `build/<preset-name>/...` (add `.exe` on Windows):
- `build/<preset>/editor/editor`
- `build/<preset>/examples/sandbox/sandbox`
- `build/<preset>/examples/asteroids/asteroids`
- `build/<preset>/tests/tests`

### Tests

Tests use Catch2 and are integrated with CTest (`catch_discover_tests`).

```bash
ctest --preset <preset-name>                      # run all tests via CTest
build/<preset>/tests/tests                        # run the Catch2 binary directly
build/<preset>/tests/tests "Test case name"        # run a single test case by name
build/<preset>/tests/tests "[tag]"                 # run tests by tag
build/<preset>/tests/tests --list-tests            # list all available test cases
```

Test sources live under `tests/src/`, mirroring the engine's module layout (`core/`, `physics/`, `renderer/`, ...).

### Benchmarks

Catch2 microbenchmarks in a separate `benchmarks` target, deliberately **not** registered with CTest (they are a
measuring tool, not a pass/fail gate). Sources live under `benchmarks/src/`, mirroring the same module layout, with
shared scaffolding in `benchmarks/src/support/` (`BusyWork`, `JobSystemScope`, `ScalingWorkerCounts`).

```bash
cmake --preset clang-benchmarks-release && cmake --build --preset clang-benchmarks-release
build/clang-benchmarks-release/benchmarks/benchmarks            # all benchmarks
build/clang-benchmarks-release/benchmarks/benchmarks "[jobs]"   # one subsystem
```

Always measure a Release build; `all-debug` builds the target only so it cannot rot. See `benchmarks/README.md`
for the conventions when adding benchmarks for another subsystem.

### Formatting and static analysis

- `.clang-format` (Google-based, 4-space indent, 160-column limit, `BinPackArguments/Parameters: false`).
- `.clang-tidy` is heavily configured (bugprone, cert, clang-analyzer, concurrency, cppcoreguidelines, hicpp, misc,
  modernize, performance) with specific checks disabled — consult it before assuming a check applies. It relies on
  `compile_commands.json` at the repo root (kept up to date automatically when `VULKYRIE_EXPORT_COMPILE_COMMANDS=ON`).
- Clang/GCC builds compile with `-Wall -Wextra -Wconversion -Wsign-conversion -Wpedantic -Werror`; MSVC uses `/W4 /WX`.
  Warnings are build failures, not suggestions.

## Plan mode files

Plan Mode assigns each new plan file under `.claude/plans/` an auto-generated, content-blind slug at the moment
planning starts (before the plan has any content) — this cannot be configured via CLAUDE.md or `settings.json`
(tracked upstream as [anthropics/claude-code#32118](https://github.com/anthropics/claude-code/issues/32118)).
**As the last step before calling `ExitPlanMode`, rename the plan file** (it's just a normal tracked file by then)
to a descriptive kebab-case name reflecting its actual content, e.g. `memory-subsystem-phase-0-implementation-plan.md`,
keeping it in `.claude/plans/`.

## Architecture

### Core engine conventions (`engine/include/vlkypch.h`)

Every engine translation unit is built through this precompiled header. It defines the vocabulary used throughout
the codebase:
- Sized aliases `u8/u16/u32/u64`, `i8/i16/i32/i64`, `f32/f64` — used everywhere instead of the built-in types.
- `Scope<T>` / `CreateScope<T>()` and `Ref<T>` / `CreateRef<T>()` as the `unique_ptr`/`shared_ptr` aliases.
- `VE_INLINE` (force-inline), `VE_API` (DLL import/export), and `VE_DELETE_COPY`/`VE_DELETE_MOVE`/
  `VE_DELETE_MOVE_AND_COPY(Type)` macros used on nearly every non-value class.
- All engine code lives in namespace `Vulkyrie`.

Logging (`core/logger.h`) uses `VFATAL/VERROR/VWARN/VINFO/VDEBUG/VTRACE(fmt, ...)` macros (std::format-style),
compiled out below the `VULKYRIE_LOG_LEVEL` definition. Assertions (`core/asserts.h`) use `VASSERT`/`VASSERT_EXPR`,
which log via `VERROR` and trap (`VDEBUGBREAK`); both are no-ops unless `VULKYRIE_DEBUG` is defined (Debug builds only).

### Custom ECS

`core/entity.h` / `core/entity_manager.h` implement a minimal entity system rather than a full registry: an `Entity`
packs a 48-bit index and 16-bit generation into a single `u64` (generation guards against stale handles after an
index is recycled). `EntityManager` only creates/destroys/recycles entity IDs — there is no generic component
registry. Instead, each component type has its own `*ComponentStore` (e.g. `RigidBodyComponentStore`,
`TransformComponentStore`, `ColliderComponentStore`, `JointComponentStore`, `FixedJointComponentStore`, ...) that
stores its data densely (SoA-style) and maps `Entity -> index` internally. Systems hold references to the stores
they need and iterate "active" components by index rather than querying by entity.

### Physics engine (`engine/{include,src}/physics/`)

`PhysicsWorld` owns every component store and system and drives the simulation from `Update(Timestep)`. The pipeline
per step is: collision detection (`CollisionSystem`, using a dynamic AABB tree broadphase in
`collision/broadphase/dynamic_aabb_tree.cpp` and narrowphase pair algorithms — GJK/SAT and dedicated
sphere/capsule/convex-polyhedron routines under `collision/narrowphase/`) → island creation (`Islands`) → constraint
solving → integration → sleeping-body updates.

Constraint solving follows a sequential-impulse pattern shared by contacts (`ContactSolverSystem`) and every joint
type (`constraint/` has `BallAndSocketJoint`, `FixedJoint`, `HingeJoint`, `SliderJoint`, each with a matching
`*SolverSystem` under `systems/`): `InitializeBeforeSolving()` precomputes per-step data (lever arms, mass matrices,
bias terms) once, `WarmStart()` re-applies the previous step's accumulated impulses, `SolveVelocityConstraint()` is
iterated (Gauss-Seidel) to drive relative velocities to zero, and `SolvePositionConstraint()` performs Non-Linear
Gauss-Seidel position correction for residual drift. Position correction can be configured per-world as Baumgarte
stabilization vs. split-impulse (contacts) or Baumgarte vs. NLGS (joints) — see `ContactsPositionCorrectionTechnique`
and `JointsPositionCorrectionTechnique`.

### Renderer (`engine/{include,src}/renderer/`)

The active backend is OpenGL (`src/renderer/open_gl/`), built around a frame-graph abstraction
(`include/renderer/frame_graph/`, with resource types under `frame_graph/resources/`). A `src/renderer/vulkan/`
backend exists for future work but is not the primary target yet — check its state before assuming it is functional.

### Code style

Public API surfaces use Doxygen-style `/** @brief ... */` comments (including `@param`/`@returns`), even when the
behavior seems self-explanatory from the name — match this when adding or changing public methods. Methods and
types are `PascalCase`; private/protected members use a leading-underscore `_camelCase` (no `m_` prefix). Trivial
accessors are typically `[[nodiscard]] VE_INLINE`. Classes that hold references or are otherwise non-copyable use
`VE_DELETE_MOVE_AND_COPY(TypeName);` rather than manually deleting each special member.
