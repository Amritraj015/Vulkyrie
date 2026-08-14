# Vulkyrie Memory Subsystem — Architecture & Tooling

## Context

Vulkyrie currently has **no memory subsystem**. Every allocation goes straight through
`new`/`delete`, `std` containers, `Scope<T>`/`Ref<T>` (`vlkypch.h`), and third-party libraries
(GLFW, assimp, OpenAL, ImGui, STB, glm) with **no accounting whatsoever**. There is no way to
answer "how much memory is Physics using vs. Rendering?", no leak detection, no budgets, and no
visibility into allocation hot spots. `main.cpp` even carries a commented-out global
`operator new` override under `VULKYRIE_DEBUG` — the intent was already there, unbuilt.

The goal is a first-class **Memory subsystem** that (1) tracks memory usage **per engine
subsystem** (Physics, Rendering, Audio, Core, …), (2) ships the must-have tooling around it
(live HUD, leak detection, budgets, snapshot/export, hot-spot capture), and (3) provides reusable
allocation building blocks — all with a tiered overhead model so it can stay on in release at
near-zero cost while offering deep tracking in debug. This is a design document for later
execution, not an implementation task.

---

## Current-state findings (what the codebase gives us to build on)

- **Single auto-globbed static lib.** `engine/CMakeLists.txt` builds via
  `file(GLOB_RECURSE ENGINE_SOURCES src/*.cpp)`, so a new `engine/src/memory/` module compiles
  automatically — no source-list maintenance.
- **PCH-driven vocabulary.** `engine/include/vlkypch.h` is force-included into every TU
  (`target_precompile_headers`) and defines `Scope`/`Ref`/`CreateScope`/`CreateRef`, `VE_INLINE`,
  `VE_API`, `VE_DELETE_*`. It is the single point where global memory macros/vocabulary belong.
- **Existing tooling pattern to mirror.** `engine/include/debug/profiler.h` is a `GetSingleton()`
  Meyers singleton, `std::mutex`-guarded, compile-gated by a `VLKY_PROFILE` flag, with an RAII
  `Timer` and `VLKY_PROFILE_SCOPE`/`_FUNCTION` macros writing Chrome-tracing JSON. The memory
  system should mirror this shape (singleton tracker + RAII scope + macros + compile gate) so it
  feels native.
- **Init/lifecycle hooks.** `main.cpp` initializes `Logger` first, then `CreateApplication()`,
  then wraps `Run()` in `VLKY_PROFILE_BEGIN/END_SESSION`. `Logger::InitializeLogger` (static init)
  and `Application::GetSingleton()` show the established bootstrap/singleton conventions. The
  memory tracker must initialize **before** any subsystem allocates and report at shutdown.
- **Existing global-override seam.** The commented-out `operator new` in `main.cpp` (guarded by
  `VULKYRIE_DEBUG`, a compile def set for Debug in `engine/CMakeLists.txt`) is exactly where a
  global heap hook belongs.
- **Subsystems** are sibling module directories: `core, renderer, audio, physics, input, events,
  networking, materials` (+ `platform`). **`Engine` (`core/engine.h`) no longer exists** — it was
  deleted, and `Application` (`core/application.h`) is the sole lifecycle owner. Read every mention of
  `Engine` below as `Application`. It owns `Platform` and `Renderer` via smart pointers, and the design
  target of subsystem-owned lifetimes is unchanged. `PhysicsWorld` still isn't wired in (today it's
  constructed at the example/game layer); that wiring is expected. This means the
  attribution/lifecycle model can be **centralized in the owner**: as it constructs each
  subsystem it registers it with the tracker, and it can wrap each subsystem's per-frame `Update`
  in a `VE_MEMORY_SCOPE(<tag>)` from one place rather than relying on each subsystem to tag itself.
- **Editor is ImGui-based** (`editor/src/vulkyrie_layer_ui.cpp`, layer-stack overlays) — the host
  for a real-time memory HUD/graphs panel.
- **Threading is coming.** The physics performance plan introduces a job system; the tracker must
  be **thread-safe and low-contention** (per-thread scope stacks, atomic counters) from day one.
- **Third-party allocation is significant and hookable:** ImGui (`SetAllocatorFunctions`), GLFW
  3.4 (`glfwInitAllocator`), STB (`STBI_MALLOC`/`STBI_FREE` defines), assimp (custom IO/allocator),
  OpenAL. These can be attributed to subsystem buckets rather than vanishing into "untracked".

---

## Locked decisions

1. **Attribution:** global `operator new`/`delete` override + a thread-local "current subsystem"
   scope-tag stack → total coverage of `std`, `Scope`/`Ref`, and most third-party allocations.
2. **Scope:** tracking **plus** a reusable allocator toolkit (linear/arena, pool, stack, freelist)
   whose allocators auto-report to the tracker.
3. **Overhead:** tiered — always-on lightweight per-subsystem atomic counters (release-safe) +
   opt-in deep per-allocation records/leak-detection/callstacks in debug.
4. **Third-party:** hook ImGui / GLFW / STB / assimp / OpenAL into subsystem buckets now.

---

## Architecture

### Module layout (`engine/{include,src}/memory/`)

New sibling module, auto-compiled by the existing `GLOB_RECURSE`. Representative files:

- `memory/memory_tag.h` — `enum class MemoryTag : u8` of subsystems (Physics, Rendering, Audio,
  Core, Input, Events, Networking, Materials, Platform, Assets, Editor, GpuVram, ThirdParty,
  Untagged) + a `constexpr` name table. Fixed, small, array-indexable.
- `memory/memory_tracker.{h,cpp}` — the singleton core (counters + scope stack + deep table).
- `memory/global_new_delete.cpp` — the global `operator new`/`delete` replacements (one TU).
- `memory/memory_scope.h` — `VE_MEMORY_SCOPE(tag)` RAII + compile gates; **included from
  `vlkypch.h`** so the vocabulary is engine-wide.
- `memory/allocators/{linear_arena,pool_allocator,stack_allocator,free_list_allocator,
  tracked_std_allocator}.h(+.cpp)` — the toolkit.
- `memory/memory_budget.h`, `memory/memory_report.{h,cpp}` — budgets, snapshots, export.
- `memory/thirdparty_hooks.cpp` — installs the third-party allocator hooks.
- `editor/src/memory_panel.{h,cpp}` — the ImGui HUD.

### Core: `MemoryTracker` (mirror `Profiler`'s shape)

`GetSingleton()` Meyers singleton, but its **bookkeeping never routes through the overridden
`operator new`** (it uses raw `malloc`/an untracked allocator) to avoid infinite recursion.

- **Always-on tier** — `std::array<SubsystemCounters, N>` indexed by `MemoryTag`. Each holds
  `std::atomic<i64>` for `currentBytes`, `liveAllocations`, `totalAllocated`, `totalFreed`, and a
  CAS-updated `peakBytes`. Relaxed atomics → ~nanoseconds per alloc/free. **`constinit` / static
  storage** so counters are valid before any dynamic init (globals that allocate pre-`main` are
  still counted).
- **Attribution** — a `thread_local` small tag stack. `VE_MEMORY_SCOPE(MemoryTag::Physics){…}`
  pushes/pops; the override reads the top (or `Untagged`). Per-thread stacks make attribution
  correct under the coming job system with zero contention.
- **Deep tier** (debug, `VE_MEMORY_DEEP_TRACKING`) — pointer→`AllocationRecord{size, tag, thread,
  timestamp, optional callstack}` in a **sharded** hash map (lock striped by pointer hash) using the
  untracked allocator. Powers leak detection, top-N sites, and per-category breakdown. Callstacks
  gated behind `VE_MEMORY_CALLSTACKS` (backtrace on Linux / StackWalk on Windows) due to cost.

### Global `operator new`/`delete` override (header-prefix technique)

Override **all** forms (throwing/nothrow, aligned, array, sized-delete). Each `new` over-allocates
a small aligned header `{size, tag, magic}`, returns the payload pointer, and adds `size` to the
current tag's counters; each `delete` reads the header to subtract from the **correct** bucket in
O(1) — so attribution is exact in **release too**, independent of the deep table. Deep tracking, when
on, additionally records the pointer.

- **Known integration risk (must handle):** a global `operator new` replacement living in a *static
  library* can be dropped by the linker if nothing references its object file. Mitigate by
  force-linking that TU (`--whole-archive` / `/WHOLEARCHIVE` for the object, or a referenced
  anchor symbol, or define the overrides in each executable target). Call this out in the build.
- **Sanitizer coexistence:** a `VE_MEMORY_DISABLE_GLOBAL_NEW` flag compiles the override out so
  ASan/Valgrind/heaptrack builds don't fight it. The counter API still works via the toolkit.

### Allocator toolkit (`memory/allocators/`)

Each allocator takes a `MemoryTag`, is alignment-aware, follows CLAUDE.md style
(`VE_DELETE_MOVE_AND_COPY`, Doxygen), and **reports its reserved + in-use bytes** to the tracker as
a distinct "reserved pool" so pools don't double-count against per-alloc heap:

- `LinearArena` (bump/frame allocator; `Reset()` per frame, tracks high-water) — directly serves the
  physics plan's per-frame arenas.
- `PoolAllocator` / fixed-block pool — uniform objects (contacts, tree nodes).
- `StackAllocator` — LIFO scoped scratch.
- `FreeListAllocator` — general purpose within a reserved block.
- `TrackedStdAllocator<T, Tag>` — STL-compatible adapter so hot `std::vector`/`unordered_map` (e.g.
  in physics/renderer) attribute precisely and can optionally be arena-backed.

### Third-party attribution (`thirdparty_hooks.cpp`)

Install at the correct point in each lib's lifecycle:

- **ImGui** → `ImGui::SetAllocatorFunctions(..., userdata=Rendering)` before `CreateContext`.
- **GLFW 3.4+** → `glfwInitAllocator(...)` before `glfwInit` → `Platform`. (Verify vcpkg GLFW
  version; fall back to scope-based attribution if <3.4.)
- **STB** → define `STBI_MALLOC/REALLOC/FREE` in the stb-impl TU → `Assets`.
- **assimp / OpenAL** — these mostly allocate through C++ `new` / their own paths; wrap importer and
  audio init/usage in `VE_MEMORY_SCOPE(Assets|Audio)` so the global override buckets them. Document
  per-lib what is directly hookable vs. scope-attributed vs. genuinely external.

### Tooling (the "must-haves")

- **Live ImGui HUD** (`editor/src/memory_panel.cpp`, a layer overlay like `vulkyrie_layer_ui`):
  per-subsystem current/peak bars, a per-frame history sparkline (ring buffer sampled on the main
  thread), budget bars (green/amber/red), live allocation counts, top-N allocation sites (deep
  tier), and buttons to capture a snapshot / dump a full report.
- **Budgets** (`memory_budget.h`): per-subsystem soft (→ `VWARN`) and hard (→ `VASSERT`/log)
  thresholds, checked against the cheap counters.
- **Snapshots & export** (`memory_report.h`): capture per-subsystem current/peak/count to
  JSON/CSV; **memory diff** between two snapshots (regression comparisons across runs).
- **Leak detection**: at shutdown the deep table must be empty; otherwise report each outstanding
  allocation with tag/size/callstack. Wire a **CI leak-gate** test.
- **Chrome-tracing memory timeline**: emit periodic counter events (`"ph":"C"`) into the **existing
  `Profiler` JSON stream** so memory graphs appear alongside the profiler timeline in
  chrome://tracing — reuses `debug/profiler.h` rather than a second exporter.

### Lifecycle integration

- **Init first.** In `main.cpp`, call `MemorySystem::Initialize()` at the very top (before
  `CreateApplication()`), mirroring the `Logger`/`VLKY_PROFILE_BEGIN_SESSION` bootstrap. (Cheap
  counters already work pre-init via static storage; Initialize sets budgets, opens the report
  sink, and installs third-party hooks at their proper lifecycle points.)
- **Engine-owned subsystems drive tagging.** As `Engine` constructs each subsystem it registers it
  with the tracker, and `Engine`'s frame loop wraps each subsystem's `Update`/render call in a
  `VE_MEMORY_SCOPE(<tag>)` centrally — so attribution follows engine ownership and no subsystem has
  to tag itself. Once `PhysicsWorld` is wired into `Engine`, its step is tagged the same way.
- **Per-frame.** Sample history + emit timeline counters + reset frame arenas from the main loop
  (`engine.cpp`/`application.cpp` `Run()`), next to the existing frame update.
- **Shutdown.** Emit the report + leak check where `VLKY_PROFILE_END_SESSION()` sits in `main.cpp`.

### Threading & correctness

Relaxed atomics for counters; CAS-max for peaks; per-thread scope stacks; sharded locks for the
deep table; the tracker's own bookkeeping uses the untracked allocator to prevent recursion. This
keeps the system correct and low-contention once the physics job system lands.

---

## Additional enhancements & considerations (requested)

- **GPU / VRAM tracking** as a first-class parallel bucket (`GpuVram`): account GL buffer/texture
  allocations in the OpenGL backend now, and device memory via Vulkan/VMA budgets when that backend
  matures. A renderer HUD without VRAM is only half the picture.
- **Out-of-memory hooks:** a registerable OOM handler so subsystems can free caches / fail
  gracefully rather than crash.
- **Sanitizer / external-tool interop:** the `VE_MEMORY_DISABLE_GLOBAL_NEW` escape hatch so ASan,
  Valgrind, and heaptrack remain usable; treat the tracker as complementary, not a replacement.
- **Fragmentation & long-run metrics:** track largest-free-block / reserved-vs-used per allocator
  to catch fragmentation in long sessions.
- **Categories (2-level tags):** allow an optional sub-category within a subsystem
  (e.g. `Physics/Broadphase`, `Rendering/Textures`) surfaced in the deep tier and HUD.
- **Deterministic reporting:** report/snapshot ordering keyed by `MemoryTag` (stable), not
  iteration order — consistent with the engine's determinism direction.

---

## Suggested phasing

- **Phase 0 — Baseline (answers "who uses what"):** `MemoryTag` + `MemoryTracker` cheap tier +
  header-prefix global `new`/`delete` override + `VE_MEMORY_SCOPE` in the PCH + shutdown report.
  Sprinkle `VE_MEMORY_SCOPE` at subsystem entry points (Physics `Update`, Renderer, Audio, Assets).
- **Phase 1 — Deep tier:** records, leak detection, top-N sites, optional callstacks (debug-gated).
- **Phase 2 — Allocator toolkit:** arena/pool/stack/freelist + `TrackedStdAllocator`, self-reporting.
- **Phase 3 — Third-party hooks:** ImGui/GLFW/STB, scope-attribute assimp/OpenAL.
- **Phase 4 — Budgets + export:** budgets, JSON/CSV snapshots + diff, chrome-tracing counters.
- **Phase 5 — Editor HUD:** the ImGui memory panel.
- **Phase 6 — Enhancements:** GPU/VRAM bucket, OOM hooks, CI leak-gate, fragmentation metrics;
  adopt the arenas in the physics per-frame allocators (ties into the physics performance plan).

---

## Verification

- **Unit tests (`tests/src/memory/`, Catch2):** counters reconcile under alloc/free; nested
  `VE_MEMORY_SCOPE` attribution; arena `Reset` / pool reuse; a deliberate leak is caught; a budget
  breach warns. Add a **multi-threaded** test allocating with scopes from several threads and assert
  totals reconcile with no negative counters (`/test clang-all-debug`).
- **End-to-end:** run the sandbox/editor (`build/<preset>/examples/sandbox/sandbox`,
  `build/<preset>/editor/editor`), open the Memory HUD, confirm Physics/Rendering/Audio buckets
  populate and peaks track; capture a snapshot; confirm chrome://tracing shows memory counters
  beside the profiler trace.
- **Leak-gate:** a clean shutdown reports zero outstanding allocations; CI fails otherwise.
- **Build matrix:** verify the global-`new` override links correctly across gcc/clang/MSVC static
  linking (the whole-archive/anchor mitigation), and that `VE_MEMORY_DISABLE_GLOBAL_NEW` produces a
  working ASan build.
