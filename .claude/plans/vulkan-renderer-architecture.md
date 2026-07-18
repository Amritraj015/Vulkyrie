# Vulkyrie Vulkan Renderer — Architecture & Feature Plan

## Context

Vulkyrie's renderer today is an early-stage **OpenGL-only** backend behind a thin, partly-formed
abstraction. The goal is a **high-performance, multi-threaded, enterprise-grade Vulkan renderer**
with first-class statistics/metrics tracking and the surrounding feature set expected of a modern
engine. This document is the architecture blueprint + phased roadmap for later execution — not an
implementation task.

The good news: several foundations already point the right way and should be reused, not rebuilt —
a backend-agnostic **frame graph**, a **generational-handle resource model**, a `RendererStatistics`
seed, and the `GraphicsAPI` enum. The work is to turn these into a real Render Hardware Interface
(RHI) with a Vulkan backend, a threaded submission architecture, a shader pipeline, and the
metrics/tooling layer.

---

## Current-state findings (what to build on)

- **`Renderer` is an empty shell** (`renderer/renderer.h`) — rendering is currently driven by free
  functions + global state in `renderer/renderer.cpp` (`RendererAPI`, `_graphicsContext`). This
  global, OpenGL-coupled entry point (`Initialize(GraphicsAPI)`) is what a real `Renderer`/RHI
  replaces.
- **A nascent RHI already exists**: `renderer/renderer_context.h` defines `RendererContext` (abstract,
  `Create()` factory keyed on `GraphicsAPI`) and **generational handles** `Handle<T>{Index,
  Generation}` / `BufferHandle`. Only buffer create/destroy is implemented so far
  (`open_gl_renderer_context.cpp`). Extend this handle model to all resource types.
- **A real frame graph is already implemented** (`renderer/frame_graph/frame_graph.h`): DAG of
  passes, reference-count culling, resource lifetime analysis, transient vs. imported resources,
  versioned resource handles, `AddPass<PassData>(setup, execute)`, and a backend-agnostic
  `Execute(void* context, void* allocator)`. **The Vulkan renderer is a *consumer* of this graph** —
  passes record into Vulkan command buffers via the `context`. Reuse it as the render-pass
  organizer; do not build a second one.
- **`RendererStatistics`** (`renderer/renderer.h`): `FramesRendered`, `DrawCalls`,
  `TrianglesRendered` — the seed to grow into the full metrics system.
- **Vulkan backend is greenfield**: `renderer/vulkan/vulkan_renderer.cpp` is empty; no Vulkan headers
  anywhere.
- **No Vulkan dependencies**: `vcpkg.json` lists assimp/imgui/glfw3/glad/glm/stb/openal-soft/catch2
  only. ImGui is built with `glfw-binding` + `opengl3-binding` (needs `vulkan-binding` added).
  Must add `vulkan`, `vulkan-memory-allocator` (VMA), `shaderc`, `spirv-cross`, and optionally
  `volk` via `vcpkg.json` + `Dependencies.cmake`.
- **Windowing is GLFW, hard-wired to OpenGL**: `vulkyrie_glfw_platform.cpp` sets
  `GLFW_CONTEXT_VERSION 4.6` + core profile and `glfwCreateWindow`. Vulkan needs
  `glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API)`, `glfwGetRequiredInstanceExtensions`, and
  `glfwCreateWindowSurface`. The platform must branch on `GraphicsAPI`.
- **Engine lifecycle is thin**: `Engine` (`core/engine.h`) owns `Ref<Renderer>` (+ Platform, Audio)
  and the loop just calls `Application::Run()` (`core/engine.cpp`). Per the earlier correction, all
  subsystems — including the renderer — are engine-owned; the renderer initializes/shuts down and is
  ticked from the engine frame loop.
- **Editor is ImGui + docking** (`editor/src/vulkyrie_layer_ui.cpp`) — host for the stats/metrics
  HUD, but the ImGui backend must move from OpenGL3 to Vulkan.
- **Cross-plan dependencies**: the physics performance plan introduces a **shared engine-wide job
  system** (`core/jobs/`) and the memory plan reserves a **`GpuVram`** tracking bucket. This renderer
  must consume both: parallel command recording on the shared job system, and VMA allocations
  reported to the memory tracker's `GpuVram` bucket.

---

## Locked decisions

1. **Backend:** a clean **RHI seam, Vulkan-first**; Vulkan is the one feature-complete backend,
   OpenGL is demoted to a minimal/legacy fallback (seam kept, no GL feature-parity burden).
2. **Feature level:** **modern Vulkan 1.3** — dynamic rendering, timeline semaphores,
   synchronization2, descriptor-indexing/**bindless** — all behind a capability layer with
   feature-gated fallbacks.
3. **Threading:** a **dedicated render thread** (decoupled from the game/sim thread via
   triple-buffered render packets) **+ parallel command-buffer recording** fanned out over the
   shared engine job system.
4. **Shaders:** **offline SPIR-V** (ship) **+ runtime shaderc** (dev hot-reload) **+ SPIRV-Cross
   reflection** (auto descriptor/pipeline layouts).

---

## Architecture

### 1. RHI — Render Hardware Interface (`renderer/rhi/`)

Promote the nascent `RendererContext` + `Handle<T>` model into a real, backend-agnostic RHI:

- **Generational handles for every resource type** (extend `renderer_context.h`'s `Handle<T>`):
  `BufferHandle, TextureHandle, SamplerHandle, ShaderHandle, PipelineHandle, DescriptorHandle/
  BindlessHandle, RenderTargetHandle, QueryPoolHandle`. Handle index doubles as the bindless
  descriptor slot. Pools recycle indices with generation validation (as `open_gl_renderer_context`
  already does for buffers).
- **Interfaces:** `IRenderDevice` (resource create/destroy, capabilities, submit), `ICommandList`
  (draw/dispatch/copy/barrier recording), `ISwapchain`, `IQueue`. Backend-neutral **descriptors**:
  `BufferDesc/TextureDesc/GraphicsPipelineDesc/ComputePipelineDesc`.
- `RendererContext::Create(GraphicsAPI)` becomes the RHI device factory; the OpenGL context is
  reframed as a thin legacy `IRenderDevice`.

### 2. Vulkan backend (`renderer/vulkan/`)

- **`VulkanDevice`** — `VkInstance`, physical-device scoring/selection, `VkDevice`, and dedicated
  **graphics / async-compute / async-transfer** queues where exposed. A **`DeviceCapabilities`**
  struct queries and gates optional features (bindless, mesh shaders, ray tracing) with fallbacks.
- **`VulkanSwapchain`** — surface via `glfwCreateWindowSurface`, format/present-mode selection,
  robust **recreation on resize / minimize / device-lost**.
- **`VulkanAllocator`** — VMA wrapper; **every allocation is reported to the memory subsystem's
  `GpuVram` bucket** (per the memory plan) and to the renderer stats.
- **Modern 1.3 core:** dynamic rendering (no `VkRenderPass`/framebuffer objects), timeline
  semaphores, synchronization2 barriers, **bindless** global descriptor set (large partially-bound,
  update-after-bind arrays) with a classic bound-descriptor fallback path.
- **Command management:** per-worker-thread command pools, one set per frame-in-flight; primary +
  secondary command buffers, reset per frame — no locking on the record path.
- **Pipelines:** `VkPipelineCache` persisted to disk; PSOs built from `GraphicsPipelineDesc` +
  reflection.
- **Uploads:** transfer-queue staging ring buffer for buffer/texture streaming (async where possible).

### 3. Frame graph integration (reuse `renderer/frame_graph/`)

The existing frame graph is the render-pass organizer — **do not build a second one**. Add Vulkan
resource backends under `renderer/vulkan/frame_graph/` mirroring the existing
`open_gl/frame_graph/` (`FrameGraphTexture`/`FrameGraphBuffer`). Then:

- Derive **automatic barriers + image-layout transitions** and **dynamic-rendering begin/end** from
  the graph's per-pass read/write usage (the graph already does reference-count culling + lifetime
  analysis in `frame_graph.h`).
- Use lifetime analysis to drive **transient-resource memory aliasing** via VMA (reuse memory across
  non-overlapping resources) — surfaced as a "memory saved" stat.
- `Execute(context, allocator)` receives a Vulkan render context + transient allocator.

### 4. Threading architecture

- **Render thread** decoupled from the game/sim thread. The game thread produces an immutable
  per-frame **`RenderView`/render packet** (visible draw items, cameras, lights, material handles)
  into a **triple-buffered** queue; the render thread consumes it — decoupling simulation from
  present and pipelining CPU/GPU.
- **Parallel recording:** the render thread builds the frame graph, then fans out pass/draw-list
  recording across the **shared job system** (`core/jobs/`, co-developed with the physics plan) into
  per-thread secondary command buffers, then joins and submits.
- **Frames-in-flight (2–3):** per-frame command/descriptor pools, upload rings, and timeline
  values; the CPU only blocks at the in-flight limit.
- **Async compute/transfer** coordinated by timeline semaphores across queues.

### 5. Statistics & metrics (`renderer/stats/`) — the explicit ask

Grow `RendererStatistics` (`renderer/renderer.h`) into a real subsystem:

- **CPU counters:** draw calls, dispatches, instances, triangles/vertices, pipeline & descriptor
  binds, render passes, barriers, command buffers recorded, objects culled, FG passes executed vs.
  culled.
- **GPU timing:** `VkQueryPool` **timestamp queries per frame-graph pass** → per-pass GPU ms; **
  pipeline-statistics queries** (vertices, primitives, fragment/compute invocations).
- **VRAM:** `VK_EXT_memory_budget` + VMA stats, per-resource-type breakdown, transient-aliasing
  savings — fed into the memory subsystem `GpuVram` bucket.
- **Frame pacing:** CPU frame time, render-thread time, GPU frame time, acquire/present latency,
  in-flight occupancy, present mode/VSync.
- **History & aggregation:** N-frame ring buffer with min/avg/max/percentiles.
- **Surfaces:** (a) an **ImGui HUD** panel in the editor (per-pass GPU/CPU bars, VRAM, draw stats,
  frame-graph view); (b) reuse the existing **`Profiler`** (`debug/profiler.h`) chrome-tracing stream
  to emit GPU pass spans + counters alongside CPU timings; (c) a programmatic API + **CSV/JSON export**
  for headless/CI capture.
- **Debug labels:** `VK_EXT_debug_utils` — name every resource/queue/command buffer and wrap each
  pass in a labeled region so RenderDoc/Nsight/validation output is legible (and drives HUD names).

### 6. Shader pipeline (`renderer/shaders/`, `assets/shaders/`)

- Author GLSL → **offline `glslc`→SPIR-V** at build (CMake custom target) for shipping; **runtime
  `shaderc`** compilation for **hot-reload** in dev.
- **SPIRV-Cross reflection** auto-derives descriptor-set layouts, push-constant ranges, and vertex
  input → auto-built pipeline layouts (keeps the `shader.h` abstraction, cuts boilerplate).
- Shader **permutations/variants** via preprocessor defines + a hash-keyed shader cache; shared
  include headers (PBR/lighting).

### 7. Dependencies & build

- **`vcpkg.json`:** add `vulkan` (or `vulkan-headers` + `vulkan-loader`),
  `vulkan-memory-allocator`, `shaderc`, `spirv-cross`, optionally `volk`; add ImGui `vulkan-binding`
  feature (alongside/instead of `opengl3-binding`).
- **`Dependencies.cmake`:** `find_package(Vulkan)`, VMA, shaderc, spirv-cross; link into `engine`.
- **`engine/CMakeLists.txt`:** Vulkan libs + a **shader-compile custom target** producing `.spv`.
- **Platform** (`vulkyrie_glfw_platform.cpp`): branch window creation on `GraphicsAPI` —
  `glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API)` + `glfwGetRequiredInstanceExtensions` for Vulkan.
- **Debug builds:** validation layers + GPU-assisted validation + debug-utils, mirroring the
  existing GL debug-callback pattern in `open_gl_renderer_context.cpp`.

### 8. Engine integration & lifecycle

- `Renderer` (`renderer/renderer.h`) becomes a real **engine-owned subsystem**:
  `Initialize(window, config)`, `BeginFrame/EndFrame`, `Submit(RenderView)`, `Resize`, `Shutdown`.
  `Engine` (`core/engine.h/.cpp`) owns and ticks it in the frame loop, wrapping its work in
  `VE_MEMORY_SCOPE(Rendering)` (memory plan) and profiler scopes. This retires the global
  free-function entry point in `renderer.cpp`.
- **Editor** renders the scene to an **offscreen target shown in a dockable ImGui viewport**, via the
  Vulkan ImGui backend.

---

## Must-have feature set (suggested)

Foundation (RHI + Vulkan + frame graph + threading + stats) first, then:

- **Bindless materials & textures** (descriptor indexing) — the substrate for GPU-driven rendering.
- **GPU-driven culling & indirect draw:** compute frustum + occlusion culling,
  `vkCmdDrawIndexedIndirectCount`, draw compaction — scales to massive scenes.
- **Clustered forward (Forward+) PBR:** clustered light culling, metallic-roughness PBR, IBL —
  reusing `light.h`, `camera.h`, `mesh_textures.h`.
- **Shadows:** cascaded shadow maps (directional) + point/spot shadow atlas.
- **HDR + post-processing stack** as frame-graph passes: tonemapping, bloom, TAA, SSAO, motion blur,
  SMAA/FXAA; optional deferred G-buffer and OIT transparency.
- **Async compute** for culling/post/particles.
- **Pipeline & shader hot-reload** (dev) + persistent pipeline cache (ship).
- **ImGui-on-Vulkan** (docking retained) + **editor scene viewport** render target.
- **Multi-camera / multi-viewport**, **dynamic resolution scaling**, optional **HDR/wide-gamut
  swapchain**.
- **Frame capture / screenshots**, RenderDoc-friendly labels, runtime validation toggle.
- **Resource streaming** hooks (textures/meshes) tied into the memory budgets.

---

## Additional enhancements & considerations

- **Capability/extension layer** (`DeviceCapabilities`) gating bindless / mesh shaders / ray tracing
  with graceful fallback — and **seams left for mesh shaders and `VK_KHR_ray_tracing`** as later
  phases.
- **Robustness:** device-lost recovery, swapchain out-of-date/suboptimal handling, minimize/resize.
- **Pipeline-cache persistence** across runs for fast startup.
- **Golden-image determinism** for render tests (reproducible headless capture).
- **Cross-platform:** MoltenVK via the Vulkan loader for macOS (note, not built now).
- **Tie-ins:** GPU memory → memory-subsystem `GpuVram` budgets + HUD; CPU/GPU timings → the existing
  `Profiler` chrome-tracing stream; parallel recording → the shared `core/jobs/` system.

---

## Suggested phasing

- **Phase 0 — Deps & bootstrap:** Vulkan deps; GLFW `NO_API` + surface; instance/device/swapchain;
  validation + debug-utils; capability query; a clear-screen frame.
- **Phase 1 — RHI + resources:** generational-handle RHI; VMA allocator (→ `GpuVram` tracking);
  buffers/textures/samplers; staging/upload; frames-in-flight + per-frame rings.
- **Phase 2 — Shaders & pipelines:** offline SPIR-V + shaderc hot-reload + SPIRV-Cross reflection;
  pipeline cache; first mesh via dynamic rendering.
- **Phase 3 — Frame graph on Vulkan:** Vulkan FG resource backends; automatic barriers/layout
  transitions; transient aliasing; port a forward pass.
- **Phase 4 — Threading:** render thread + triple-buffered render packets + parallel recording on the
  shared job system + async transfer.
- **Phase 5 — Stats & tooling:** timestamp + pipeline-stats queries; VRAM budget; frame pacing; ImGui
  HUD; profiler/chrome-tracing integration; CSV/JSON export.
- **Phase 6 — Bindless + GPU-driven:** descriptor indexing; indirect draw; compute culling.
- **Phase 7 — Rendering features:** clustered PBR + IBL; CSM shadows; HDR + post stack; ImGui-on-Vulkan
  editor viewport.
- **Phase 8 — Enhancements:** async compute; dynamic resolution; device-lost robustness; mesh-shader
  / ray-tracing seams.

---

## Representative files

**New:** `renderer/rhi/{render_device,command_list,resource_handles,pipeline,descriptor,swapchain,
rhi_types,device_capabilities}.h`; `renderer/vulkan/{vulkan_device,vulkan_swapchain,vulkan_allocator,
vulkan_command_list,vulkan_pipeline,vulkan_descriptor,vulkan_bindless,vulkan_upload,vulkan_sync}.{h,cpp}`;
`renderer/vulkan/frame_graph/{frame_graph_texture,frame_graph_buffer}.cpp`;
`renderer/stats/{renderer_stats.h,gpu_profiler.{h,cpp}}`; `renderer/shaders/{shader_compiler,
shader_reflection}.cpp`; `renderer/{render_thread.{h,cpp},render_view.h}`;
`editor/src/renderer_stats_panel.{h,cpp}`; `assets/shaders/*.glsl`.
**Modified:** `renderer/renderer.{h,cpp}`, `renderer/renderer_context.h`,
`core/vulkyrie_glfw_platform.cpp`, `core/engine.{h,cpp}`, `vcpkg.json`, `Dependencies.cmake`,
`engine/CMakeLists.txt`, editor ImGui backend.

---

## Verification

- **Correctness:** RHI handle-lifetime tests, shader-reflection tests, frame-graph barrier-correctness
  tests (Catch2, new `tests/src/renderer/`); **validation layers clean (zero errors)** in debug.
- **Golden-image tests:** render known scenes headless and diff against references (deterministic
  capture) — the regression gate for rendering features.
- **Performance:** per-pass GPU/CPU timings from the stats system on a heavy scene; verify parallel
  recording scales with core count and the render thread improves frame pacing vs. single-threaded.
- **End-to-end:** sandbox + editor run on Vulkan (`build/<preset>/examples/sandbox/sandbox`,
  `build/<preset>/editor/editor`); the stats HUD populates; a RenderDoc capture shows labeled passes;
  resize/minimize/device-lost are handled cleanly.
- Iterate with `/build clang-all-debug`; capture benchmark numbers in a release preset.
