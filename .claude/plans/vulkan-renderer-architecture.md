# Vulkyrie Renderer — Architecture & Feature Plan

## Context

The goal is a **high-performance, multi-threaded Vulkan renderer** with first-class statistics and the
surrounding feature set of a modern engine, built on foundations that are reused rather than rebuilt: a
backend-agnostic **frame graph**, a **generational-handle resource model**, a **job system** for
parallel recording, and a **concepts-based RHI seam**.

The RHI is **compile-time polymorphic**. There is exactly one virtual dispatch boundary — `Renderer` —
chosen once at startup from a `GraphicsAPI` value. Everything below it is a template parameterized on a
backend trait struct and monomorphized per backend, with the `RendererBackend` concept enforcing the
interface at compile time.

This document describes that architecture, what exists, and what is left. It is the blueprint for the
whole subsystem; [vulkan-renderer-phase-0-bring-up.md](vulkan-renderer-phase-0-bring-up.md) is the
narrower slice that makes it do something real.

---

## Current state

### Built and reusable

- **`core/types/handle.h`** implements `Handle<T>` (plain index) and `GenerationalHandle<T>` (20-bit
  index + 12-bit generation packed into a `u32`), each `static_assert`ed trivially copyable with
  `sizeof == sizeof(u32)`. `renderer/rhi/rhi_types.h` builds `BufferHandle`, `TextureHandle`,
  `SamplerHandle`, `PipelineHandle`, `QueryHeapHandle`, `ShaderHandle` and `HeapHandle` on top of it.
  The frame graph reuses the same primitives.
- **The job system** (`core/jobs/`, documented in its own
  [README](engine/include/core/jobs/README.md)) is implemented: lock-free work stealing,
  dependency-graph scheduling, `parallel_for`. It uses the trampoline-dispatch pattern
  (`InvokeFn = void(*)(void*)`, inline payload, null `Destroy` for trivially-destructible captures) that
  the frame graph's own allocation-free design mirrors. This is what `RecordParallel` fans work across.
- **The frame graph** (`renderer/frame_graph/`, documented in its own
  [README](engine/include/renderer/frame_graph/README.md)) is complete and mature: a public
  `GetResource<T>(handle)` reachable from execute functors; a real topological sort (Kahn's algorithm)
  with cycle detection; `ResourceUsage`/`ResourceBarrier` structs carrying stage/access/layout/queue
  masks a backend can derive Vulkan barriers from, batched per pass; transient memory aliasing via
  `GetMemoryRequirements` + a placed `Create` overload, with a self-checking `FrameGraphAliasingReport`;
  `Record()`/`RecordParallel()`/`Submit()` split from `Execute()`; a single `FrameGraphContext` carrying
  `RenderContext`, `TransientResources` and `EmitBarriers`; and an allocation-free steady state after
  warm-up. **Do not build a second one, and do not re-plan it** — see its README for the full API.
  - Open limitations, per that README: `Import` seeds no initial usage, so the first barrier on an
    imported resource transitions from a state it may not be in; `ResourceUsage::QueueType` reaches the
    backend but nothing schedules passes onto queues; and **`FrameGraphBuffer` is a stub** whose
    `Create`/`Destroy` are commented out, so `FrameGraphTexture` is the only complete resource type.
- **`renderer/renderer.h`** is the abstract seam — the only virtual dispatch in the stack:
  ```cpp
  class Renderer {
  public:
      [[nodiscard]] static Scope<Renderer> Create(GraphicsAPI api, const DeviceCreationInfo &info);
      [[nodiscard]] virtual GraphicsAPI BackendType() const noexcept = 0;
      [[nodiscard]] virtual const DeviceCapabilities &QueryCapabilities() const = 0;
      virtual void OnWindowResize(u32 width, u32 height) = 0;
      virtual void Render() = 0;
      virtual void WaitIdle() = 0;
      [[nodiscard]] virtual bool DeviceLost() const noexcept = 0;
      [[nodiscard]] virtual const RendererStatistics &GetStatistics() const noexcept = 0;
  };
  ```
- **`renderer/backends/backend_concepts.h`** defines `RendererBackend`, composed of six sub-concepts: a
  backend trait struct must name five collaborator types (`Context`, `Queue`, `CommandList`,
  `CommandPool`, `Swapchain`), five trivially-copyable handle types (`Image`, `Buffer`, `Sampler`,
  `Pipeline`, `ShaderModule`), a set of `static constexpr` constants, and specific member-function
  shapes on `Context`/`Queue`/`CommandList`. `vulkan/vulkan_backend.h` and `open_gl/open_gl_backend.h`
  are the two trait structs, each checked by `static_assert(RendererBackend<…>)` in
  `vulkan_renderer.cpp` / `open_gl_renderer.cpp` next to the explicit
  `template class RendererImpl<…>;` instantiation.
- **`renderer/backends/device.h`** — `Device<B>` owns `B::Context`, a `ShaderCompiler`, and
  `DeletionQueue<B>`, `TransientPool<B>`, `ShaderModuleCache<B>`, `PipelineCache<B>`.
  **`src/renderer/renderer_impl.h`** — `RendererImpl<B> : Renderer` wraps one `Device<B>` and one
  `B::Swapchain`. This is the single class both backends instantiate, in place of two hand-written,
  drifting `VulkanRenderer`/`OpenGLRenderer` classes.
- **`Application`** (`core/application.h`/`.cpp`) is the sole lifecycle owner. `core/engine.h`/`.cpp` no
  longer exist in the tree. `Application::Run()` calls
  `mRenderer = Renderer::Create(mAppSettings.GraphicsSettings.API, {})` in the frame-setup path.

### Still stubs

Verified by reading every file under `engine/src/renderer/backends/`:

- `VulkanImage/Buffer/Sampler/Pipeline/ShaderModule` and their `OpenGL*` counterparts are empty structs
  — `struct VulkanImage{};`. No `Vk*` member anywhere in the tree.
- `VulkanContext`/`OpenGLContext` declare the full `RendererBackendContextOps` surface, but only the
  constructor (`(void)info;`), `WaitIdle` (empty), `QueryCapabilities` (returns a default-constructed
  `DeviceCapabilities`) and `DeviceLost` (always `false`) have bodies. Every `Create*`/`Destroy*` is
  declared and never defined — this links only because nothing ODR-uses them yet.
- `VulkanQueue`/`OpenGLQueue` and `VulkanCommandList`/`OpenGLCommandList` are declaration-only.
  `VulkanPool`/`OpenGLPool` and `VulkanSwapchain`/`OpenGLSwapchain` are empty classes with no members.
- `DeletionQueue<B>::Push`/`Collect`/`PendingCount`, `PipelineCache<B>::Get`/`PreCompile`/
  `LoadFromDisk`/`Size`, and `ShaderModuleCache<B>::Get`/`Size`/`Clear` are declared, not defined.
  `TransientPool<B>` holds two references and nothing else. `ShaderCompiler` is an empty class.
- `BufferDescriptor`, `ImageDescriptor`, `SamplerDescriptor`, `GraphicsPipelineDescriptor` and
  `ComputePipelineDescriptor` are **forward-declared only** in `rhi_types.h`, marked
  `// TODO: Finish these up.` Every `Context::Create*` signature depends on them existing for real.
- No Vulkan dependency exists: **`vulkan` is not in `vcpkg.json`**, there is no `volk`, and none of the
  instance/device/swapchain modules exist. The RHI scaffolding was built before any Vulkan bring-up.
- `RendererImpl<B>::Render()` and `OnWindowResize()` are TODO no-ops, and `GetStatistics()` returns a
  member that is never written to.

### Known defects

These are cheap to fix now and expensive once more code leans on the current shape.

1. **The build is green, but there is no render path.** `/build clang-all-debug` links every target —
   engine, editor, sandbox, `vulky-cli`, tests, benchmarks — with `-Werror` clean. What that green does
   *not* mean: the pre-pivot OpenGL renderer moved to `backup/*.cppbak` in `baf75e8`, both `Context`
   implementations are stubs, and `RendererImpl<B>::Render()` is a TODO no-op that
   `Application::Run()`'s loop never calls. The apps build and run; nothing draws.
   - The build stays green partly by accident: **~10 `examples/sandbox/src/sandbox_layer_*.h` headers
     still `#include` retired types** (`RendererContext`, `VertexArray`, `Texture2D`, …), and compile
     only because `examples/sandbox/src/` contains exactly one `.cpp` — `sandbox_entry.cpp` — which
     includes none of them. They are dead headers that will break the moment anything includes them.
     They belong in `backup/` with the code they reference, or should be ported.
   - So "OpenGL minimal" (below) is not the choice to hold a *working* fallback steady. Both backends
     are non-functional today; OpenGL simply has a backed-up implementation to draw on, and Vulkan has
     none.
2. **`Device<B>` uses more of `Context` than the concept guarantees.** `Device<B>` calls
   `GetGraphicsQueue()`, `GetTransferQueue()`, `GetComputeQueue()` and `GetHeap()`, but
   `RendererBackendContextOps` requires none of them, and `VulkanContext`/`OpenGLContext` declare none.
   This compiles only because those `Device<B>` members are never instantiated. The concept should
   require the queue accessors (and `GetHeap` under `requires(B::kUsesBindlessHeap)`), so the gap fails
   at the `static_assert` rather than at first use.
3. **`Context::CreatePipeline(Pipeline) -> bool` is almost certainly meant to be `DestroyPipeline`.** It
   sits in the destroy group in both the concept and both contexts, takes a `Pipeline` by value, and
   returns `bool` like its `Destroy*` siblings — while the real creators are
   `CreateGraphicsPipeline`/`CreateComputePipeline`. Pipelines currently have no way to be destroyed.
4. **Destruction order is inverted in `Application`.** `~Application()` explicitly calls
   `mPlatform.reset()`, destroying the GLFW window, *before* the compiler-generated member destructors
   run. Members are declared `mAppSettings, mPlatform, mRenderer, …`, so implicit order would destroy
   `mRenderer` first — correct — but the explicit reset preempts it, and `mRenderer` is destroyed after
   its window is gone. This is exactly the ordering hazard Vulkan bring-up must get right
   (surface/device before window). Fix: drop the explicit `mPlatform.reset()` and let member order do
   it, or reset `mRenderer` first.
5. **`Platform` is not API-neutral.** `vulkyrie_glfw_platform.cpp:346-347` sets
   `GLFW_CONTEXT_VERSION_MAJOR/MINOR` unconditionally with no
   `glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API)` branch, so a Vulkan window still gets a GL context
   hint. `glViewport` is unconditional in the framebuffer-resize callback (line 383) — under Vulkan glad
   never loads, so that function pointer is null and the first resize is undefined behaviour.
   `glfwInit()`'s return value is unchecked.
6. **Two orphan artifacts.** `renderer/backends/renderer_backend.h` is a pre-pivot, non-template virtual
   `class RendererBackend` that **name-collides with the concept**; git history shows it was
   `zzz_renderer_backend.h` and was renamed back into place by `baf75e8` while every sibling moved to
   `backup/*.hbak` — an accident. Nothing includes it. Separately, `renderer/rhi/rhi_concepts.h` defines
   a second, entirely unreferenced concept set (`RHIDevice`, `ManagesHeap`, `PlacesResources`, …)
   modelling resources as free functions over handles — a competing sketch `backend_concepts.h` diverged
   from. Both need a delete-or-reconcile call.

---

## Locked decisions

1. **RHI seam: the `RendererBackend` concept plus `Device<B>`.** One virtual seam (`Renderer`) chosen
   once at startup; everything beneath it — `Device<B>`, the caches, `Context`/`Queue`/`CommandList` — a
   template over a backend trait struct, checked at compile time. See "Why concepts" below.
2. **Feature level: modern Vulkan 1.3** — dynamic rendering, timeline semaphores, synchronization2,
   descriptor-indexing/**bindless** — gated by `DeviceCapabilities` with feature-gated fallbacks.
3. **Threading: a dedicated render thread** plus parallel command-buffer recording fanned out over
   `core/jobs/`. `FrameGraph::RecordParallel` is the seam this lands on.
4. **Shaders: offline SPIR-V** (ship) **+ runtime shaderc** (dev hot-reload) **+ SPIRV-Cross
   reflection**. `ShaderCompiler`, `ShaderModuleCache<B>` and `PipelineCache<B>` are the stub seams.
5. **Vulkan first; OpenGL minimal.** Vulkan is the feature-complete backend. `OpenGLBackend` stays a
   conformance stub: it exists to keep the concept honest — a change only Vulkan can satisfy fails a
   `static_assert` immediately rather than surfacing as a design smell six phases later — and to carry
   whatever the editor and examples need, no more. Effort does not go into `OpenGLContext::Create*`
   beyond that. The capability constants already encode the asymmetry: `OpenGLBackend` sets
   `kUsesBindlessHeap`, `kHasTimelineSync`, `kHasExplicitBarriers`, `kHasMemoryAliasing` and
   `kRecordsInParallel` all `false`; `VulkanBackend` sets them all `true`. Where the two must diverge
   inside `RendererImpl<B>`, `if constexpr` on those constants is the sanctioned mechanism.
   - Consequence to schedule deliberately: the editor and examples currently *build* but *draw nothing*.
     Giving them a picture again means either porting them onto the new seam or reviving enough of the
     backed-up GL renderer. That is real work the green build hides, and it is not in Phase 0.

### Why concepts

- **Upside.** Zero vtable overhead below the one seam that genuinely needs runtime dispatch — the
  graphics API is a startup config value, not a per-call decision. Handles are `static_assert`-enforced
  trivially-copyable POD at the concept boundary, not POD by convention. The capability constants encode
  "GL literally cannot express this" at compile time, and `backend_concepts.h` documents *why* inline:
  GL has `GLsync` fences rather than monotonic `u64` timeline values, so `TimelineTarget()` has nothing
  to return; GL has no barrier objects carrying layouts; GL cannot place two textures on one allocation;
  GL contexts are thread-affine, so recording is a serial replay. One `RendererImpl<B>` replaces two
  hand-maintained classes that would inevitably drift.
- **Cost, to design around.** Every RHI-level type is compiled once per backend — code size and build
  time scale with backend count, and a bug in shared template logic surfaces separately in each
  instantiation. `RendererImpl<B>` has to stay genuinely generic; the day Vulkan needs something OpenGL
  structurally cannot do, the template starts accumulating `if constexpr` branches, and the design's
  cleanliness depends on that staying disciplined rather than sprawling.
- **A structural consequence.** The concept requires method-call syntax (`c.CreateImage(id)`), which
  forces `Context` to be a class with member functions rather than a plain aggregate operated on by free
  functions. Pure-function testability is not lost — `VulkanContext`'s methods should delegate to
  free functions like `SelectPhysicalDevice`/`ScorePhysicalDevice`, so selection logic still tests
  without a `VkInstance` — but it is pushed one level down instead of being the seam itself. Worth
  holding onto so the decomposition does not get skipped just because the outer shape is a class.
- **A cost already realized.** Because member functions of a class template instantiate lazily, the
  concept can under-specify what `Device<B>` actually needs and nothing complains — defect 2 above. The
  concept has to be maintained as the real contract, not assumed to be one.

---

## Architecture

### 1. RHI (`renderer/rhi/` + `renderer/backends/`)

Scaffolding exists; what is missing, in dependency order:

- The descriptor structs (`BufferDescriptor`, `ImageDescriptor`, `SamplerDescriptor`,
  `GraphicsPipelineDescriptor`, `ComputePipelineDescriptor`) — forward-declared only, and every
  `Context::Create*` signature depends on them.
- `Context` implementations that actually create and destroy GPU resources.
- `DeletionQueue<B>` deferring destruction across `B::kFramesInFlight` buckets — `Bucket` is an empty
  struct and `Push`/`Collect` are unimplemented.
- `TransientPool<B>` pooling anything.
- The concept corrections in defects 2 and 3.

### 2. Vulkan backend (`renderer/backends/vulkan/`)

Instance and device selection, `DeviceCapabilities`, dedicated graphics/async-compute/async-transfer
queues, a VMA allocator wired to a `GpuVram` memory-tracker bucket, a bindless global descriptor set
with a classic fallback, per-worker-thread command pools, a `VkPipelineCache` persisted to disk, and a
transfer-queue staging ring. All future work. See the phase-0 document for the bring-up sequence
(instance → surface → device → swapchain → frame resources) that lands first.

### 3. Frame graph integration (reuse, do not rebuild)

The graph-side work is done. What remains is backend-specific:

- Vulkan resource types satisfying `FrameGraphResourceType` — a `Descriptor`, `Create`/`Destroy` taking
  `FrameGraphContext`, and the placed `Create` overload for aliasing — wrapping `B::Image`/`B::Buffer`.
- Mapping `ResourceUsage`'s opaque stage/access/layout/queue masks onto `VkPipelineStageFlags2`,
  `VkAccessFlags2` and `VkImageLayout` inside a Vulkan `EmitBarriers` passed through `FrameGraphContext`.
- Finishing `FrameGraphBuffer`, needed before any pass can declare a graph-managed buffer.
- Wiring the graph to the backend at all: nothing under `frame_graph/` currently references
  `RendererImpl`, `RendererBackend` or `Device<B>`, and `RendererImpl<B>::Render()` is a bare TODO. The
  two subsystems were built independently and have never been connected.

### 4. Threading

- **Render thread** decoupled from game/sim via a triple-buffered `RenderView`/render-packet queue. Not
  built; `Application::Run()` is single-threaded end to end.
- **Parallel recording** has its landing spot: `FrameGraph::RecordParallel()` fans pass bodies across
  `core/jobs/`. What is missing is a Vulkan backend to record *into* — per-worker command pools reached
  through `FrameGraphContext::RenderContext`, per the graph's documented constraint that pass bodies
  draw scratch storage from backend-owned per-worker state, never from the graph.
- **Frames in flight:** `B::kFramesInFlight` is already a per-backend compile-time constant (2 for both),
  and `DeletionQueue<B>`'s bucket array is sized off it.

### 5. Statistics (`renderer/rhi/rhi_types.h::RendererStatistics`)

The struct already carries `FrameIndex`, `TransientBytesPeak`, `PassesDeclared`, `PassesCulled`,
`BarrierBatches`, `DrawCalls`, `PipelineBinds`, `CpuRecordMs`, `GraphCompileMs`, `GpuFrameMs` and
`PlanCacheHit`. **None of it is populated** — `RendererImpl<B>::GetStatistics()` returns a
default-constructed member. Still needed: GPU timestamp and pipeline-statistics queries, VRAM budget via
`VK_EXT_memory_budget` + VMA stats feeding a `GpuVram` bucket, frame-pacing history, an ImGui HUD,
`Profiler` chrome-tracing integration, CSV/JSON export, and `VK_EXT_debug_utils` object naming.

### 6. Shader pipeline (`renderer/backends/{shader_compiler,shader_module_cache,pipeline_cache}.h`)

The seams exist and are empty. Offline `glslc`→SPIR-V for shipping, runtime `shaderc` for hot-reload,
SPIRV-Cross reflection driving descriptor and pipeline layouts automatically, permutations via a
hash-keyed cache.

### 7. Dependencies & build

Entirely undone: add `vulkan-headers` + `vulkan-loader` (or `volk`), `vulkan-memory-allocator`,
`shaderc` and `spirv-cross` to `vcpkg.json`; `find_package` wiring in `Dependencies.cmake`; a
shader-compile custom target; the ImGui Vulkan binding. See the phase-0 document for `volk`-first
sequencing.

### 8. Engine integration & lifecycle

`Renderer` is the real seam and `Application` owns it. Three concrete gaps, all phase-0 work:

- `Renderer::Create` is called with a default-constructed `DeviceCreationInfo{}`
  (`application.cpp:46`) — no window handle, no real size, `NativeWindow` is `nullptr`.
- **`Render()` is never called.** `Run()`'s loop processes layer operations, updates layers, and calls
  `mPlatform->OnUpdate()`. There is no render step at all.
- `OnWindowResize()` *is* wired (`application.cpp:124`), but unconditionally — and `mRenderer` is not
  assigned until `Run()` line 46, after `mPlatform->CreateWindow()`. A resize event arriving during
  window creation dereferences a null `mRenderer`.

---

## Feature set

Foundation first (RHI, Vulkan bring-up, frame-graph integration, threading, stats), then: bindless
materials and textures; GPU-driven culling with indirect draw; clustered forward (Forward+) PBR with
IBL; cascaded shadow maps plus a point/spot shadow atlas; an HDR and post-processing stack (tonemap,
bloom, TAA, SSAO, motion blur, SMAA/FXAA) as frame-graph passes; async compute; pipeline and shader
hot-reload with a persistent pipeline cache; ImGui-on-Vulkan with an editor scene viewport;
multi-camera/multi-viewport and dynamic resolution scaling; frame capture and screenshots,
RenderDoc-friendly labels, a runtime validation toggle; resource streaming tied to memory budgets.

Alongside: a capability/extension layer with graceful fallback and seams for mesh shaders and ray
tracing; device-lost recovery and swapchain out-of-date/suboptimal handling; pipeline-cache persistence
across runs; golden-image determinism for render tests; a MoltenVK note (not built now); and tie-ins to
the memory subsystem's `GpuVram` budgets, the `Profiler` chrome-tracing stream, and `core/jobs/`.

---

## Phasing

RHI shape and shader-cache shape already exist ahead of any Vulkan bring-up, so:

- **Phase 0 — Bring-up on the existing scaffolding.** Fix the `Application` and `Platform` defects;
  `volk` + GLFW `NO_API` + surface; instance/device/swapchain; validation and debug-utils; capability
  query; `Context`'s create/destroy methods implemented for real; a clear-screen frame driven through
  `RendererImpl<B>::Render()`. See [the phase-0 document](vulkan-renderer-phase-0-bring-up.md).
- **Phase 1 — RHI made real.** `DeletionQueue<B>` and `TransientPool<B>` implemented; VMA allocator →
  `GpuVram`; the descriptor structs filled in; staging and upload; `DeviceCreationInfo` populated from
  the real window.
- **Phase 2 — Shaders & pipelines.** `ShaderCompiler` does something; offline SPIR-V plus shaderc
  hot-reload plus SPIRV-Cross reflection; `PipelineCache<B>` persisted to disk.
- **Phase 3 — Frame graph on Vulkan.** Vulkan `FrameGraphResourceType` implementations; `EmitBarriers`
  mapping `ResourceUsage` to real barriers; finish `FrameGraphBuffer`; connect the graph to
  `RendererImpl<B>::Render()`.
- **Phase 4 — Threading.** Render thread plus triple-buffered render packets; `RecordParallel` wired to
  real per-worker Vulkan command pools.
- **Phase 5 — Stats & tooling.** Populate `RendererStatistics`; timestamp and pipeline-stats queries;
  VRAM budget; ImGui HUD; profiler integration; CSV/JSON export.
- **Phase 6 — Bindless & GPU-driven.** Descriptor indexing; indirect draw; compute culling.
- **Phase 7 — Rendering features.** Clustered PBR with IBL; CSM shadows; HDR and post stack;
  ImGui-on-Vulkan editor viewport.
- **Phase 8 — Enhancements.** Async compute; dynamic resolution; device-lost robustness; mesh-shader and
  ray-tracing seams.

**Not on this ladder, and needing its own slot:** giving `editor` and `examples/sandbox` a working render
path again. They compile and run today but draw nothing, so the build gate will never flag this. Phase 0
targets a minimal host app instead, which means this can be scheduled independently — but it has to be
scheduled, not assumed.

---

## Representative files

**Exist (stubs):** `renderer/{rhi/rhi_types,renderer}.h`;
`renderer/backends/{backend_concepts,device,deletion_queue,transient_pool,pipeline_cache,shader_module_cache,shader_compiler}.h`;
`src/renderer/renderer_impl.h`; `src/renderer/renderer.cpp`;
`src/renderer/backends/vulkan/{vulkan_backend,vulkan_context,vulkan_types,vulkan_queue,vulkan_pool,vulkan_command_list,vulkan_swapchain}.h`
plus `vulkan_context.cpp` and `vulkan_renderer.cpp`; the `open_gl/` mirror of the same list.

**To create:**
`src/renderer/backends/vulkan/{vulkan_common,vulkan_loader,vulkan_platform,vulkan_instance,vulkan_physical_device,vulkan_device,vulkan_swapchain_support}.{h,cpp}`
(see the phase-0 document for the decomposition); `renderer/stats/{renderer_stats.h,gpu_profiler.{h,cpp}}`;
`renderer/{render_thread.{h,cpp},render_view.h}`; `editor/src/renderer_stats_panel.{h,cpp}`;
`assets/shaders/*.glsl`.

**To modify:** `core/application.{h,cpp}` (destruction order, populate `DeviceCreationInfo`, wire
`Render()`/`OnWindowResize()`), `core/vulkyrie_glfw_platform.cpp` (API-neutral hardening), `vcpkg.json`,
`Dependencies.cmake`, `engine/CMakeLists.txt`, the editor's ImGui backend.

---

## Verification

- **Correctness:** RHI handle-lifetime tests, shader-reflection tests, and frame-graph
  barrier-correctness tests (Catch2, `tests/src/renderer/`); **validation layers clean** in debug. Note
  that no tests currently cover the backend architecture — `tests/src/renderer/` contains only the frame
  graph suite.
- **Golden-image tests:** render known scenes headless and diff against references.
- **Performance:** per-pass GPU and CPU timings from the stats system on a heavy scene; parallel
  recording scales with core count.
- **End-to-end:** the phase-0 host app runs on Vulkan; the stats HUD populates; a RenderDoc capture shows
  labelled passes; resize, minimize and device-lost are handled cleanly. Editor and sandbox rejoin this
  list once they have a render path again.
- Iterate with `/build clang-all-debug` and capture benchmark numbers in a release preset. **Treat a
  green build as a weak signal here:** every target links today while nothing draws, so the gate proves
  the templates instantiate, not that the renderer works.

## See also

- [vulkan-renderer-phase-0-bring-up.md](vulkan-renderer-phase-0-bring-up.md) — the first slice: design
  and ordered execution for Vulkan bring-up on this scaffolding.
- [frame_graph/README.md](engine/include/renderer/frame_graph/README.md) and
  [core/jobs/README.md](engine/include/core/jobs/README.md) — the two mature subsystems this plan
  reuses. Both are the live source of truth for their areas.
- [roadmap.md](roadmap.md) — cross-plan sequencing.
