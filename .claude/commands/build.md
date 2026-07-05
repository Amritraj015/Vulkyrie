---
description: Configure and build a Vulkyrie CMake preset (default: clang-all-debug)
argument-hint: [preset-name]
---

Configure and build the project using CMake presets.

Preset to use: $ARGUMENTS (default to `clang-all-debug` if no preset is given — see the preset naming scheme
in CLAUDE.md, e.g. `gcc-tests-release`, `msvc-all-debug`).

Steps:
1. Run `cmake --preset <preset>` to configure (safe to skip if the build directory is already configured and
   nothing relevant to configuration changed).
2. Run `cmake --build --preset <preset>`.
3. Report any compiler errors/warnings concisely. This project builds with `-Werror`/`/WX`, so any warning is a
   build failure, not just noise to skim past.
