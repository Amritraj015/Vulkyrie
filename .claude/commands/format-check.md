---
description: Check clang-format and clang-tidy compliance on files changed from main
argument-hint: [base-ref]
---

Check code style and static-analysis compliance for the current changes, without modifying any files.

Base ref to diff against: $ARGUMENTS (default to `main` if not given).

1. Get the changed C/C++ files: `git diff --name-only <base-ref>...HEAD -- '*.cpp' '*.h'`, plus
   `git diff --name-only HEAD -- '*.cpp' '*.h'` for uncommitted changes. De-duplicate the list.
2. For each file, run `clang-format --dry-run --Werror <file>` (uses the repo's `.clang-format`) and report which
   files would be reformatted. Do not run `clang-format -i` to auto-fix without the user's explicit go-ahead, since
   it rewrites files in place.
3. If `compile_commands.json` exists at the repo root (it's auto-exported by the build when
   `VULKYRIE_EXPORT_COMPILE_COMMANDS=ON`), run `clang-tidy <file>` for each changed file per the repo's `.clang-tidy`
   config and summarize the findings.
4. Report a concise per-file pass/fail summary, not the raw tool output.
