# Renderer Plan Docs — Retarget onto the Concepts-Based Backend Architecture

## Context

The renderer pivoted. Instead of a virtual-dispatch RHI (`IRenderDevice`/`ICommandList`), the tree now
builds a **compile-time-polymorphic backend**: one virtual seam at `Renderer`, everything below it
monomorphized through `RendererImpl<B>` where `B` is a backend trait struct checked by the
`RendererBackend` concept. That work landed across `0a2d7c1` → `26b3e1a` (all 2026-08-13).

The planning docs did not keep up, and they are wrong in two different ways:

1. **`vulkan-renderer-architecture.md`** already describes the new design, but narrates itself against
   "the original plan" in 11 places — a document that **no longer exists anywhere**. It was overwritten
   in place, recoverable only via `git show 353ec61`. A reader cannot resolve any of those references.
   It also defers to "the phase-0 documents" ~8 times.
2. **The two phase-0 docs describe the superseded design as if it were current.** They specify a
   different `Renderer` seam (`PrepareForWindowCreation`/`Initialize`/`Shutdown`/`BeginFrame`/`EndFrame`),
   plan hand-written `VulkanRenderer`/`OpenGLRenderer` classes that `RendererImpl<B>` exists precisely to
   replace, put Vulkan at `src/renderer/vulkan/` (actual: `src/renderer/backends/vulkan/`), name
   `IRenderDevice` as the Phase-1 seam, and open by asserting `Renderer` is "a literally empty class"
   and `Engine` is dead code — `core/engine.h` is not in the tree at all.

Intended outcome: every doc under `.claude/plans/` describes the architecture that exists, with no
dangling references to removed documents, removed types, or removed paths.

## Decisions taken

- **Phase-0 docs merge into one**, retargeted rather than rewritten in parallel. They overlap heavily —
  both open with the same stale context section — and their genuinely valuable content (Vulkan bring-up
  sequencing, synchronization model, swapchain recreation) is design, not process.
- **Vulkan first; OpenGL stays minimal.** `OpenGLBackend` is a conformance stub that keeps the concept
  honest; it does not chase features. The current symmetric stubbing was scaffolding, not a commitment.
  The capability flags already encode the asymmetry (GL all-`false`, Vulkan all-`true`). See the build
  finding below — "minimal" currently means *nonexistent*, which the docs must say plainly.
- **All three referring docs get real updates**, not just link repairs.

---

## Work

### 1. Rewrite `vulkan-renderer-architecture.md` to stand alone

Make it describe the architecture as it is, with no archaeology. Concretely:

- **Delete every "the original plan" / "unchanged from" / "supersedes" framing** (lines 5–11, 29, 139,
  145, 152, 177, 205, 247, 251, 258, 272, 280). State each decision on its own merits. The "Why concepts,
  not virtuals" section (159–184) is the strongest content in the doc — keep it, but drop its comparison
  to the dead virtual-RHI proposal and argue it against the alternative directly.
- **Fold the phase-0 deferrals inward.** Eight passages hand off to "the phase-0 documents"; after the
  merge there is exactly one, so these become single links or short inline statements.
- **Resolve locked decision #5** (currently "Open, not locked") to Vulkan-first / GL-minimal, and record
  what follows: don't invest in `OpenGLContext::Create*` beyond keeping downstream targets alive;
  `if constexpr` on the capability constants is the sanctioned divergence mechanism inside
  `RendererImpl<B>`; the `static_assert(RendererBackend<OpenGLBackend>)` cross-check stays.
- **Fix one factual error:** the doc says `PipelineCache<B>` owns the `ShaderCompiler`. `Device<B>`
  ([device.h](engine/include/renderer/backends/device.h)) owns it directly, alongside the caches.
- **Record three findings the doc does not currently mention** (as known cleanup, not as edits made here):
  - [`renderer/backends/renderer_backend.h`](engine/include/renderer/backends/renderer_backend.h) is an
    orphan: a non-template virtual `class RendererBackend` from the pre-pivot model that **name-collides
    with the new concept**. Git history shows it was `zzz_renderer_backend.h` and got renamed back into
    place by `baf75e8` while every sibling moved to `backup/*.hbak` — an accident. Belongs in `backup/`.
  - [`renderer/rhi/rhi_concepts.h`](engine/include/renderer/rhi/rhi_concepts.h) defines a **second,
    entirely unreferenced** concept set (`RHIDevice`, `ManagesHeap`, `PlacesResources`, …) modelling
    resources as free functions over handles — a competing sketch that `backend_concepts.h` diverged
    from. Nothing includes it. Needs a delete-or-reconcile call.
  - **The frame graph is not wired to the backend architecture at all.** Nothing under `frame_graph/`
    references `RendererImpl`, `RendererBackend`, or `Device<B>`; `RendererImpl<B>::Render()` is a bare
    TODO. The doc currently implies more integration than exists.
- **Qualify the "build is green" claim** (line 357). *Resolved during execution:* a real
  `/build clang-all-debug` links **every** target — engine, editor, sandbox, `vulky-cli`, tests,
  benchmarks — `-Werror` clean. An earlier reading of stale build artifacts suggested downstream targets
  were broken; that was wrong. What green does *not* mean: the pre-pivot GL renderer moved to
  `backup/*.cppbak` in `baf75e8`, both `Context` implementations are stubs, and
  `RendererImpl<B>::Render()` is a no-op the frame loop never calls. **Everything builds; nothing
  draws.** Related: ~10 `sandbox_layer_*.h` headers still `#include` retired types and survive only
  because `examples/sandbox/src/` holds a single `.cpp` that includes none of them — dead headers that
  break the moment anything includes them. The docs should say plainly that a green build proves the
  templates instantiate, not that the renderer works.

### 2. Merge the two phase-0 docs into `vulkan-renderer-phase-0-bring-up.md`

Delete [vulkan-renderer-phase-0-architecture.md](.claude/plans/vulkan-renderer-phase-0-architecture.md)
and [vulkan-renderer-phase-0-implementation-plan.md](.claude/plans/vulkan-renderer-phase-0-implementation-plan.md);
write one doc that keeps design and sequence in a single narrative.

**Carry over intact** — this is why the merge is worth more than a delete. None of it is invalidated by
the pivot, and none of it has been done:

- Bring-up/teardown order, with **surface created before physical-device selection** (present-queue
  support is an *input* to device selection).
- The per-frame-in-flight vs. per-swapchain-image resource table — `renderFinished` per image,
  `imageAvailable`/fence/command pool per frame-in-flight. Conflating them is a validation error.
- The acquire/present result matrix and the `VK_SUBOPTIMAL_KHR`-renders-anyway rule.
- The swapchain-recreation state machine (recreate at top of frame; resize only sets a flag).
- volk include topology: `vulkan_common.h` as the sole `<volk.h>` includer, one TU including both volk
  and GLFW, `vulkan_loader.h` deliberately Vulkan-type-free.
- The three cheap-now/expensive-later items: `VkAllocationCallbacks` threaded through every call,
  `SetDebugName`, per-frame command pool as a struct member.
- Validation policy (warn, never fail) and the pure-selection-function test seam.
- Platform hardening — all three items still undone, verified in the tree:
  [vulkyrie_glfw_platform.cpp:346](engine/src/core/vulkyrie_glfw_platform.cpp#L346) hints a GL context
  unconditionally with no `GLFW_NO_API` branch, [:383](engine/src/core/vulkyrie_glfw_platform.cpp#L383)
  calls `glViewport` in the resize callback (null pointer under Vulkan — glad never loads), and
  `glfwInit()`'s return is unchecked.

**Retarget:**

| Stale | Current |
|---|---|
| `Renderer` with `PrepareForWindowCreation`/`Initialize`/`Shutdown`/`BeginFrame`/`EndFrame`/`OnResize`/`FrameResult` | [renderer.h](engine/include/renderer/renderer.h)'s `BackendType`/`QueryCapabilities`/`OnWindowResize`/`Render`/`WaitIdle`/`DeviceLost`/`GetStatistics` |
| Hand-written `VulkanRenderer : Renderer` | `RendererImpl<VulkanBackend>`, instantiated in `vulkan_renderer.cpp` |
| `src/renderer/vulkan/` | `src/renderer/backends/vulkan/` |
| `VulkanContext` as a plain aggregate passed by `const &` | A class with member functions — the concept requires method-call syntax. The pure-function decomposition (`SelectPhysicalDevice`, `ScorePhysicalDevice`, swapchain-support selection) moves *inside* those methods and stays the unit-test surface |
| `FRAMES_IN_FLIGHT = 2` constant | `B::kFramesInFlight`, already a per-backend `static constexpr` |
| Extension point "→ P1: becomes `IRenderDevice`" | `Device<B>` and the concept, which already exist |

**Drop entirely:** the demolition narrative (`RendererContext`, `_graphicsContext`, the free
`Initialize(GraphicsAPI)`, `Engine` as dead code) — all already gone or in `backup/`. Keep the
still-live `Application` bugs: `~Application()` resets `mPlatform` before `mRenderer` destructs, and
`Renderer::Create` is called with a default-constructed `DeviceCreationInfo{}`.

### 3. Update the three referring docs

- **[roadmap.md](.claude/plans/roadmap.md)** — refresh the renderer track (line 52) against the re-phased
  roadmap, and re-examine the critical path (lines 40–46, 73–76): it sequences Memory P0 → job system →
  physics/renderer, but both foundations are described as done, so the "critical path in one line" is
  now history rather than guidance.
- **[frame-graph-performance-and-correctness-plan.md](.claude/plans/frame-graph-performance-and-correctness-plan.md)**
  — add a status header marking Parts 1–5 executed and pointing at
  [frame_graph/README.md](engine/include/renderer/frame_graph/README.md) as the live source of truth.
  Every finding it raises (broken culling, missing topological sort, unreachable `GetResource`, `i32
  flags`, `void*` context, the allocation profile) is described as shipped in that README. Carry forward
  only the three open limitations the README still lists: `Import` seeds no initial usage, queues are
  carried but never scheduled, and `FrameGraphBuffer` is a stub — the last of which blocks any
  graph-managed buffer on Vulkan.
- **[shared-job-system-implementation-plan.md](.claude/plans/shared-job-system-implementation-plan.md)**
  — same treatment: status header marking it executed, pointing at the new
  [core/jobs/README.md](engine/include/core/jobs/README.md). Fix line 13's claim that the renderer plan
  "blocks Phase 4" — the dependency now runs the other way; the job system is done and
  `FrameGraph::RecordParallel` already fans out across it, waiting on a backend to record into.

### 4. Fix CLAUDE.md's renderer section

[CLAUDE.md:150-154](CLAUDE.md#L150-L154) says the active backend is OpenGL at `src/renderer/open_gl/`
with Vulkan planned at `src/renderer/vulkan/`. **Neither path exists** — `src/renderer/open_gl/` holds
only a `backup/` folder of `.cppbak` files, and Vulkan is under `backends/`. Since this file is
instructions to Claude, leaving it stale actively misdirects future sessions.

---

## Verification

Docs, so verification is consistency rather than tests:

1. **No dangling references.** `grep -rn "original plan\|IRenderDevice\|ICommandList\|src/renderer/vulkan\|src/renderer/open_gl\|core/engine.h" .claude/plans/ CLAUDE.md` returns nothing.
2. **No links to deleted files.** `grep -rn "vulkan-renderer-phase-0-architecture\|vulkan-renderer-phase-0-implementation-plan" .claude/plans/` returns nothing.
3. **Every code path named in a doc exists.** Spot-check each `engine/...` path referenced in the rewritten docs with `ls`; the phase-0 doc's "still to create" list is the intended exception.
4. **Signatures match the tree.** The `Renderer` block quoted in the architecture doc is diffed against
   [renderer.h](engine/include/renderer/renderer.h) by eye; likewise `RendererBackend` against
   [backend_concepts.h:102](engine/include/renderer/backends/backend_concepts.h#L102).
5. **One real build, for fact-checking only.** `/build clang-all-debug` to settle what actually compiles
   after the backup move, so the docs can state the downstream-target status as fact. No `engine/` code
   is edited by this work; the build is evidence, not a gate.

## Out of scope

The two orphan code artifacts (`backends/renderer_backend.h`, `rhi/rhi_concepts.h`) get **documented as
known cleanup, not deleted here** — they are code changes needing their own build verification. Same for
`RendererImpl<B>`'s member naming (`mDevice`/`mSwapchain`/`stats`), which does not follow the
leading-underscore `_camelCase` convention CLAUDE.md specifies.
