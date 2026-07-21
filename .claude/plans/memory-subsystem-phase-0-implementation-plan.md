# Memory Subsystem — Phase 0 Implementation Plan

## Context

Vulkyrie has **no memory accounting** today: every allocation flows through `new`/`delete`,
`std` containers, `Scope`/`Ref`, and third-party libs with zero visibility. We cannot answer
"how much memory is Physics using vs. Rendering?", have no leak detection, and no budgets.

This plan implements **Phase 0** of the [memory subsystem architecture](memory-subsystem-architecture.md)
— the baseline that answers *"who uses what"* — and, per the [roadmap](roadmap.md), does it
**first** to de-risk the one real build hazard early: a global `operator new`/`delete`
replacement defined inside the `engine` **static** library can be silently dropped by the linker.

Phase 0 delivers: `MemoryTag` (already stubbed) → an always-on cheap-tier `MemoryTracker`
(per-subsystem atomic counters) → a header-prefix global `operator new`/`delete` override that
attributes every allocation in **release too** → `VE_MEMORY_SCOPE(tag)` in the PCH → a shutdown
report → light scope-sprinkling at subsystem entry points → a Catch2 reconciliation test.

**Decisions locked with the user:**
- **Linker-drop mitigation:** anchor symbol exported by `global_new_delete.cpp`, ODR-used from an
  `[[maybe_unused]] inline` reference in `memory_scope.h` (force-included by `vlkypch.h`). Portable
  across gcc/clang/msvc; transparently covers editor/examples/cli **and the Catch2 `tests` binary**
  (which gets `main` from Catch2, not from `engine/src/main.cpp`) with no per-target CMake edits.
- **Step scope:** full Phase 0 in one deliverable.

## Current state (what already exists)

- [engine/include/memory/memory_tag.h](engine/include/memory/memory_tag.h) — X-macro `VE_MEMORY_TAGS`
  + `enum class MemoryTag : u8` with 14 tags (Physics … Untagged). **Keep the X-macro.**
- [engine/include/memory/memory_tracker.h](engine/include/memory/memory_tracker.h) — a stub with a
  `static constexpr std::array<std::string, 14>` name table. **Rewrite** (`std::string` is the wrong
  element type; use `std::string_view`).
- No `engine/src/memory/` directory yet. Nothing anywhere references the memory module (verified).

## Patterns to mirror

- **Singleton/compile-gate/RAII/macros:** [engine/include/debug/profiler.h](engine/include/debug/profiler.h)
  — `GetSingleton()`, `#define VLKY_PROFILE 0` gate, RAII `Timer`, `VLKY_PROFILE_SCOPE_LINE(name,__LINE__)`
  token-paste, macros defined outside the namespace with `#if/#else`. `VE_MEMORY_SCOPE` mirrors this shape.
  **Deviation:** counters live in `constinit` static storage, **not** a lazily-constructed Meyers
  singleton, so pre-`main` allocations (from other globals' dynamic init) are still counted.
- **Auto-compiled module:** [engine/CMakeLists.txt](engine/CMakeLists.txt) uses
  `file(GLOB_RECURSE ... src/*.cpp CONFIGURE_DEPENDS)` → new `engine/src/memory/*.cpp` compiles
  with **no CMakeLists edit**. `VULKYRIE_DEBUG` is a `PUBLIC` compile def for the Debug config.
- **PCH vocabulary:** [engine/include/vlkypch.h](engine/include/vlkypch.h) force-included into every TU;
  `u8/i64/f32`, `VE_INLINE`, `VE_DELETE_MOVE_AND_COPY`, `Scope`/`Ref` live here.
- **Bootstrap:** [engine/src/main.cpp](engine/src/main.cpp) — Logger init → `CreateApplication()` →
  `VLKY_PROFILE_BEGIN_SESSION` → `Run()` → `VLKY_PROFILE_END_SESSION`. Carries the dead commented-out
  `operator new` at lines 3–8 (remove it). This is the engine-provided `main` shared by
  editor/sandbox/asteroids. `runtime/` and `vulky-cli/` have their own `main.cpp`.
- **Frame loop:** [engine/src/core/application.cpp](engine/src/core/application.cpp) `Run()` already
  brackets work in `{ VLKY_PROFILE_SCOPE("...") ... }` blocks — the exact model for `VE_MEMORY_SCOPE`.
- **Tests:** [tests/CMakeLists.txt](tests/CMakeLists.txt) globs `tests/src/*.cpp`, links `engine` +
  `Catch2::Catch2WithMain`. Mirror layout: new file `tests/src/memory/memory_tracker_tests.cpp`.

---

## Design

### 1. `memory/memory_tag.h` (revise existing, PCH-safe)

Keep the `VE_MEMORY_TAGS(X)` X-macro and `enum class MemoryTag : u8`. Add, generated from the same
macro:
- `constexpr u8 MemoryTagCount` (count the X entries; e.g. `= []{ u8 n=0; #define X(_) ++n; ... }()`
  or a fixed `static_assert`'d constant).
- `constexpr std::string_view MemoryTagName(MemoryTag)` backed by a
  `static constexpr std::array<std::string_view, MemoryTagCount>` (fixes the `std::string` stub).

**Make it self-contained to break the PCH include cycle** (see §7): include `<cstdint>`, `<array>`,
`<string_view>` and declare the enum as `enum class MemoryTag : std::uint8_t`. **Do not** include
`vlkypch.h` here (the current stub does — remove it), because `vlkypch.h` will include this header.

### 2. `memory/memory_scope.h` (new — lives in the PCH chain, kept light)

The always-included vocabulary header. Depends only on `memory_tag.h` + `<array>`. **Must not**
pull in `memory_tracker.h`/`<atomic>` (keep the PCH lean; the scope stack only tracks the *current
tag*, never counters).

- **Thread-local scope stack** — `inline thread_local constinit` fixed-size struct
  `{ MemoryTag tags[kMaxDepth]; u32 depth; }` (e.g. `kMaxDepth = 32`). Constant-initialized →
  valid pre-`main`, allocation-free, zero-contention (per-thread). Free helpers:
  `PushMemoryTag(MemoryTag)`, `PopMemoryTag()`, `CurrentMemoryTag()` → top or `MemoryTag::Untagged`.
- **RAII `MemoryScope`** — pushes in ctor, pops in dtor; `VE_DELETE_MOVE_AND_COPY(MemoryScope)`.
- **`VE_MEMORY_SCOPE(tag)`** macro — token-paste on `__LINE__` like `VLKY_PROFILE_SCOPE_LINE`.
- **Compile gate `VE_MEMORY_TRACKING`** (default **on**, incl. release — the cheap tier is
  release-safe). When off, `VE_MEMORY_SCOPE` expands to nothing and push/pop compile out.
- **Linker anchor (the mitigation):**
  ```cpp
  namespace Vulkyrie::detail { int ForceLinkGlobalNewDelete(); // defined in global_new_delete.cpp
      [[maybe_unused]] inline int _forceLinkGlobalNewDelete = ForceLinkGlobalNewDelete(); }
  ```
  The `inline` variable's initializer ODR-uses the anchor in **every** TU that sees the PCH →
  forces `global_new_delete.o` out of `engine.a` into every executable, including the Catch2 tests
  binary (its test TUs include engine headers → `vlkypch.h` → this header). `[[maybe_unused]]`
  keeps `-Werror` happy.

### 3. `memory/memory_tracker.{h,cpp}` (new — cheap tier)

Counters in `constinit` static storage; all-static API (no lazily-constructed instance).

- `struct SubsystemCounters { std::atomic<i64> currentBytes, liveAllocations, totalAllocated,`
  `totalFreed, peakBytes; };` — `std::atomic<i64>` has a `constexpr` ctor, so a
  `constinit std::array<SubsystemCounters, MemoryTagCount>` is constant-initialized (zeroed at load).
  Define it in the `.cpp`; expose via accessor.
- `MemoryTracker::OnAllocation(MemoryTag, i64 size)` — `fetch_add` current/total/live
  (`std::memory_order_relaxed`); CAS-loop to raise `peakBytes`:
  ```cpp
  i64 cur = current.fetch_add(size, relaxed) + size;
  i64 prev = peak.load(relaxed);
  while (cur > prev && !peak.compare_exchange_weak(prev, cur, relaxed)) {}
  ```
- `MemoryTracker::OnFree(MemoryTag, i64 size)` — subtract current, add totalFreed, dec live.
- `MemoryTracker::ReportToLog()` — iterate tags in **enum order** (`0..MemoryTagCount-1`, deterministic)
  and `VINFO` a formatted table: tag name, current, peak, live count, totalAllocated/Freed.
- Casts: watch `-Wconversion`/`-Wsign-conversion` — funnel `std::size_t → i64` through explicit
  `static_cast<i64>`.

### 4. `memory/global_new_delete.cpp` (new — the override)

Includes `memory_scope.h` + `memory_tracker.h` + `<cstdlib>`/`<new>`. **Wrap the operator bodies in
`#if !defined(VE_MEMORY_DISABLE_GLOBAL_NEW)`** (sanitizer/heaptrack escape hatch); **define the anchor
unconditionally** so the PCH reference always resolves.

- **Header-prefix technique** — `struct AllocationHeader { void* base; std::size_t size;`
  `MemoryTag tag; u32 magic; };`. Store `base` explicitly so `delete` frees the true malloc pointer
  regardless of alignment padding.
  - `TrackedAlloc(size, alignment)`: `std::malloc(size + sizeof(Header) + alignment)`;
    `payload = alignUp(base + sizeof(Header), alignment)`; write the header at
    `payload - sizeof(Header)` with `tag = CurrentMemoryTag()`, `magic`;
    `MemoryTracker::OnAllocation(tag, size)`; return `payload`.
  - `TrackedFree(ptr)`: null-guard; read header at `ptr - sizeof(Header)`; `VASSERT` the magic (debug);
    `MemoryTracker::OnFree(header.tag, header.size)`; `std::free(header.base)`.
  - No recursion risk: the cheap tier only touches atomics + a thread-local — it never allocates.
- **Override all forms** (throwing + `std::nothrow` + aligned `std::align_val_t` + array, and the
  matching plain/sized/aligned/nothrow `operator delete`). Throwing `new` throws `std::bad_alloc` on
  null; nothrow returns null. Default alignment = `__STDCPP_DEFAULT_NEW_ALIGNMENT__`.
- **Anchor:** `namespace Vulkyrie::detail { int ForceLinkGlobalNewDelete() { return 0; } }` —
  outside the `VE_MEMORY_DISABLE_GLOBAL_NEW` guard.

Replacement operators are resolved at link time and replace the default globally, so **all** `new`/
`delete` across engine + statically-linked third-party libs route through them from program start —
no new/delete boundary mismatch.

### 5. `memory/memory_system.{h,cpp}` (new — lifecycle facade)

Thin facade matching the architecture doc's `MemorySystem::Initialize()`/`Shutdown()` vocabulary,
delegating to `MemoryTracker`. Phase 0 bodies: `Initialize()` logs a one-line banner (hooks/budgets
come in later phases); `Shutdown()` calls `MemoryTracker::ReportToLog()`. Counters already work
without `Initialize` (static storage), so this is lifecycle clarity, not a hard dependency.

### 6. PCH wiring — [engine/include/vlkypch.h](engine/include/vlkypch.h)

Add `#include "memory/memory_scope.h"` **after** the `u8/i64/f32` typedef block (currently lines
45–58), next to the existing `#include "debug/profiler.h"`. Placing it after the typedefs is required
so the cycle resolves (see §7).

### 7. The PCH include cycle — resolution

`vlkypch.h` → `memory_scope.h` → `memory_tag.h`. To keep a stray `#include "memory/memory_tag.h"`
(e.g. first line of a test) from breaking, **both `memory_tag.h` and `memory_scope.h` must be
self-contained** (std headers only, `std::uint8_t`, `MemoryTag`) and **must not include
`vlkypch.h`**. Only `memory_tracker.h`/`memory_system.h` (outside the PCH chain) may include
`vlkypch.h`. This removes the ordering fragility entirely.

### 8. CMake

**No `engine/CMakeLists.txt` edit needed** — `GLOB_RECURSE ... CONFIGURE_DEPENDS` auto-adds
`engine/src/memory/*.cpp`. Same for `tests` (globs `tests/src/*.cpp`). The anchor technique replaces
any whole-archive/`-u` linker flags, so no per-target link options. (If a future toolchain still drops
it, the documented fallback is `-Wl,-u,<mangled ForceLinkGlobalNewDelete>` / MSVC `/INCLUDE:` per exe.)

### 9. Bootstrap wiring — [engine/src/main.cpp](engine/src/main.cpp)

- Remove the dead commented `operator new` (lines 3–8).
- After Logger init succeeds, before `CreateApplication()`: `Vulkyrie::MemorySystem::Initialize();`
- After `VLKY_PROFILE_END_SESSION();`, before `return`: `Vulkyrie::MemorySystem::Shutdown();`
- Optionally replicate in [runtime/src/main.cpp](runtime/src/main.cpp) and
  [vulky-cli/src/main.cpp](vulky-cli/src/main.cpp). Counters/attribution work regardless; this only
  controls where the report prints.

### 10. Scope sprinkles (so Phase 0 actually attributes, not all-Untagged)

`Engine` does not own `PhysicsWorld` yet (built at the example layer) and rendering is layer-driven,
so tag at the clear per-subsystem entry points with a single `VE_MEMORY_SCOPE(...)` at function top
(RAII spans the call):

| Tag | Site |
| --- | --- |
| `Physics` | [engine/src/physics/physics_world.cpp](engine/src/physics/physics_world.cpp) `PhysicsWorld::Update` (~line 54) |
| `Audio` | [engine/src/audio/audio_system.cpp](engine/src/audio/audio_system.cpp) `AudioSystem::Update` (~line 108) + init (~line 10, `alcOpenDevice`) |
| `Assets` | [engine/src/renderer/open_gl/open_gl_model.cpp](engine/src/renderer/open_gl/open_gl_model.cpp) — around the assimp `ReadFile` import |
| `Rendering` | [engine/src/renderer/open_gl/open_gl_renderer_context.cpp](engine/src/renderer/open_gl/open_gl_renderer_context.cpp) — the per-frame draw/submit entry |

Keep it light; broad centralized tagging via `Engine` is a later phase once it owns the subsystems.

---

## File checklist

**New**
- `engine/src/memory/global_new_delete.cpp` — override + anchor definition
- `engine/src/memory/memory_tracker.cpp` — counter storage + report
- `engine/src/memory/memory_system.cpp` — Initialize/Shutdown
- `engine/include/memory/memory_scope.h` — scope stack + `VE_MEMORY_SCOPE` + anchor decl
- `engine/include/memory/memory_system.h`
- `tests/src/memory/memory_tracker_tests.cpp`

**Modify**
- `engine/include/memory/memory_tag.h` — name table + count, self-contained (drop `vlkypch.h`)
- `engine/include/memory/memory_tracker.h` — real cheap-tier API (replace stub)
- `engine/include/vlkypch.h` — `#include "memory/memory_scope.h"` after typedefs
- `engine/src/main.cpp` — remove dead snippet; `Initialize()`/`Shutdown()`
- 4 subsystem `.cpp`s above — one `VE_MEMORY_SCOPE` each

---

## Verification

1. **Build the default preset** (proves the override links + no `-Werror` regressions):
   `/build clang-all-debug`. Then a **release** build (`/build clang-all-release`) to confirm the
   cheap tier + override compile release-clean.
2. **Unit test** — `tests/src/memory/memory_tracker_tests.cpp` (`[memory]` tag), run via
   `/test clang-all-debug "[memory]"`:
   - alloc N bytes → `currentBytes`/`live` rise for the active tag; free → return to baseline
     (`totalAllocated - totalFreed == currentBytes`, `live == 0`).
   - **nested `VE_MEMORY_SCOPE`** attributes to the innermost tag; correct tag restored after scope exit.
   - `peakBytes` never decreases and equals the high-water mark.
   - A quick **multi-threaded** case (several threads alloc/free under scopes) → totals reconcile,
     no negative counters. (Sets up the Phase 1 leak-gate and the coming job system.)
   - **Linker-drop guard:** because the test binary references `MemoryTracker`, assert a `new`/`delete`
     round-trip actually moves the counters — if the anchor failed and the override were dropped,
     counters would stay flat and the test fails loudly. This is the real check that the mitigation held.
3. **End-to-end** — run `build/clang-all-debug/examples/sandbox/sandbox` (or `editor`) and confirm the
   shutdown report shows non-zero **Physics / Rendering / Audio / Assets** buckets with tracked peaks
   (not everything under `Untagged`).
4. **Sanitizer coexistence (smoke)** — a build with `-DVE_MEMORY_DISABLE_GLOBAL_NEW` compiles and runs
   (operators compile out, anchor still defined, counters flat) so ASan/Valgrind builds stay usable.
5. **Format/tidy** — `/format-check` on the changed files.

## Out of scope (later phases, do not build now)

Deep per-allocation table, leak detection, callstacks (Phase 1) · allocator toolkit
arena/pool/stack/freelist + `TrackedStdAllocator` (Phase 2) · ImGui/GLFW/STB third-party hooks
(Phase 3) · budgets, JSON/CSV export, chrome-tracing counters (Phase 4) · editor HUD (Phase 5) ·
GPU/VRAM bucket, OOM hooks, CI leak-gate (Phase 6).
