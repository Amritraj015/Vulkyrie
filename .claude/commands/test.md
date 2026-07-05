---
description: Build and run the Vulkyrie Catch2 test suite, optionally filtered by name or tag
argument-hint: [preset-name] [test-name-or-tag]
---

Run the Catch2 test suite for this project.

Arguments: $ARGUMENTS

- If a preset name is given, use it; otherwise default to `clang-all-debug`.
- Make sure the `tests` target is built first: `cmake --build --preset <preset> --target tests`.
- If a test name or Catch2 tag (e.g. `[physics]`) is given alongside the preset, pass it as a filter:
  `build/<preset>/tests/tests "<filter>"`.
- Otherwise run the full suite: `build/<preset>/tests/tests`.
- To just enumerate tests without running them: `build/<preset>/tests/tests --list-tests`.
- Summarize pass/fail results; on failure, show the relevant Catch2 assertion output (expected vs. actual), not
  the full raw log.
