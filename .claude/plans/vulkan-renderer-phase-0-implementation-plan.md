# Vulkan Renderer — Phase 0 Implementation Plan

## Context

Memory Phase 0 and the shared job system are done, which clears the roadmap's critical path
(`roadmap.md:75`). The next tranche is the **Vulkan renderer, Phase 0 — "Deps & bootstrap"**
(`vulkan-renderer-architecture.md:228`): Vulkan dependencies, GLFW `NO_API` + surface,
instance/device/swapchain, validation + debug-utils, capability query, and a clear-screen frame.

The renderer is the least-developed subsystem in the engine, so Phase 0 is as much demolition as
construction. Today:

- `Renderer` (`engine/include/renderer/renderer.h:38`) is a literally empty class with no subclass.
- Rendering is driven by a free function `Initialize(GraphicsAPI)` plus two namespace-scope globals in
  [renderer.cpp](engine/src/renderer/renderer.cpp) — and **nothing in the frame loop calls the renderer at
  all**. `RendererContext::SwapBuffers()` is dead code; presentation happens in
  `VulkyrieGLFWPlatform::OnUpdate()`.
- There is **no renderer shutdown anywhere**. `Scope<RendererContext> _graphicsContext` is destroyed during
  static destruction, so `~OpenGLRendererContext` issues `glDeleteBuffers` *after* `glfwTerminate()` has
  destroyed the GL context.
- `Engine` (`core/engine.h`) is uninstantiated dead code; `Application::Run()` is the real lifecycle.
- Vulkan exists only as `GraphicsAPI::Vulkan = 2` and a **0-byte** `vulkan_renderer.cpp`.

The intended outcome: a Vulkan-capable window presenting an animated clear colour with validation clean,
reached through a **real `Renderer` subsystem** that `Application` owns and drives — with OpenGL still
working for the editor and sandbox.

## Locked decisions

1. **Introduce the real `Renderer` subsystem now.** Abstract base with `Create` / `Initialize` /
   `BeginFrame` / `EndFrame` / `OnResize` / `Shutdown`, owned by `Application`. `VulkanRenderer` and a thin
   `OpenGLRenderer` (delegating to the existing `RendererContext`) keep both backends alive. `Engine` stays
   dead — resurrecting it is a separate, non-Vulkan refactor.
2. **Host app = a new minimal example**, `examples/vulkan_bootstrap/`.
3. **Adopt volk now** (`VK_NO_PROTOTYPES`, single include seam) rather than linking the loader and
   retrofitting at Phase 4.
4. **Target Vulkan 1.3** core (dynamic rendering, synchronization2, timeline semaphores) behind a
   capability query.

## Scope

**In:** deps, platform hardening, the `Renderer` seam + both backends, instance/messenger/surface/device/
swapchain, capability query, a synchronised 2-frames-in-flight clear, resize/minimize robustness, unit
tests for the pure selection logic, a subsystem README.

**Out (later phases, do not build):** VMA and `GpuVram` reporting (P1), the `renderer/rhi/` abstraction —
`IRenderDevice`/`ICommandList`/handle pools (P1), shaders/SPIR-V/pipelines (P2), frame graph on Vulkan (P3),
job-system threading and per-thread pools (P4), stats/HUD (P5). Do **not** make `VulkanRenderer` implement
`RendererContext` — that f32-only buffer API is a dead end Phase 1 deletes.

---

## Design

### 1. Dependencies

Add `"volk"` to [vcpkg.json](vcpkg.json) (its port is `1.4.341.0`, matching your SDK, and depends only on
`vulkan-headers` — no loader). `find_package(volk CONFIG REQUIRED)` in
[Dependencies.cmake](Dependencies.cmake); link `volk::volk` **PUBLIC** in
[engine/CMakeLists.txt](engine/CMakeLists.txt) so `tests` inherits the include path.

- Do **not** also link `Vulkan::Vulkan` — one header source, no version skew.
- Do **not** define any `VK_USE_PLATFORM_*`. vcpkg builds `volk::volk` without platform defines, so those
  entry points do not exist in `volk.o`; GLFW creates the surface for us.
- volk is its own vcpkg target and arrives via `-isystem`, so the engine's `-Werror` set never touches it.

### 2. `-Werror` house rules (verified on this machine, clang 22 **and** g++)

**Sparse designated initializers are a build error** under `-Wextra`
(`-Wmissing-designated-field-initializers` / `-Wmissing-field-initializers`), and every `VkXxxCreateInfo`
would trip it. **House style: `VkXxx info{};` followed by member assignment.** Do not weaken the engine's
warning set to work around this.

Two helpers in `vulkan_common.h` absorb most of the remaining friction:

- `template <typename T> [[nodiscard]] VE_INLINE u32 ToU32(T value)` — narrows with a Debug-only `VASSERT`.
  Kills the `-Wshorten-64-to-32` from `vector::size()` and the `-Wsign-conversion` from GLFW's `i32` sizes.
- `template <typename T, typename TFn> [[nodiscard]] std::vector<T> EnumerateVulkan(TFn &&query)` — wraps the
  Vulkan two-call count/resize/fill idiom, removing ~8 hand-rolled conversion sites.

Also: clear-colour literals must be `0.0F` (`-Wimplicit-float-conversion`). Follow `.clang-tidy` naming —
`ConstexprVariableCase: UPPER_CASE` (so `FRAMES_IN_FLIGHT`), `PrivateMethodCase: camelBack`,
`PrivateMemberCase: camelBack` with `_` prefix.

### 3. Renderer seam — the only public surface

[engine/include/renderer/renderer.h](engine/include/renderer/renderer.h):

```cpp
enum class FrameResult : u8 { Rendered, Skipped, Failed };

class Renderer {
public:
    VE_DELETE_MOVE_AND_COPY(Renderer);
    virtual ~Renderer() = default;

    /** @brief API setup that must happen BEFORE the window exists (Vulkan: volkInitialize +
     *  glfwInitVulkanLoader). Must be called before Platform::CreateWindow(). */
    [[nodiscard]] static StatusCode PrepareForWindowCreation(GraphicsAPI api);

    /** @brief Creates the backend renderer and latches the active graphics API. */
    [[nodiscard]] static Scope<Renderer> Create(GraphicsAPI api);

    [[nodiscard]] virtual StatusCode Initialize()   = 0;
    virtual void                     Shutdown()     = 0;   // idempotent
    [[nodiscard]] virtual FrameResult BeginFrame()  = 0;
    virtual void                     EndFrame()     = 0;
    virtual void OnResize(u32 width, u32 height)    = 0;

    VE_INLINE void SetClearColor(const glm::vec4 &color) { _clearColor = color; }
    [[nodiscard]] VE_INLINE const RendererStatistics &GetStatistics() const { return _statistics; }

protected:
    Renderer() = default;
    glm::vec4          _clearColor{ 0.0F, 0.0F, 0.0F, 1.0F };
    RendererStatistics _statistics{};   // retires today's unreferenced struct
};
```

Rules the header must document:
- `Shutdown()` is virtual, so **each concrete class calls its own `Shutdown()` from its own destructor** —
  never from `~Renderer()`.
- `FrameResult::Skipped` (minimized, swapchain rebuilding) is normal; only `Failed` should stop the app.

**Keep `GetCurrentGraphicsAPI()` / `GetCurrentGraphicsAPIName()`.** Ten factory TUs switch on it
(`texture_2D.cpp`, `shader.cpp`, `vertex_buffer.cpp`, `graphics_context.cpp`, …) and would silently start
returning `nullptr`. `Renderer::Create` sets the backing global **before** constructing `OpenGLRenderer`,
whose `Initialize()` calls `RendererContext::Create()`, which reads it.

Delete: the free `Initialize(GraphicsAPI)`, the namespace-scope `_graphicsContext`, and the never-defined
`RendererContext(GraphicsAPI)` declaration.

### 4. Platform hardening ([platform.h](engine/include/core/platform.h), [vulkyrie_glfw_platform.cpp](engine/src/core/vulkyrie_glfw_platform.cpp))

- Register `glfwSetErrorCallback` **before** `glfwInit()` (it's legal, and today an init failure is silent),
  then check `glfwInit()`'s return.
- Move the `GLFW_CONTEXT_VERSION_MAJOR/MINOR 4/6` hints **inside** the `GraphicsAPI::OpenGL` branch
  (`vulkyrie_glfw_platform.cpp:346`); add `else { glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); }`.
- **Remove the `glViewport` call from the framebuffer-size callback**
  ([vulkyrie_glfw_platform.cpp:383](engine/src/core/vulkyrie_glfw_platform.cpp#L383)). Under Vulkan glad is
  never loaded, so that pointer is NULL and the first resize crashes. It relocates to
  `OpenGLRenderer::OnResize`, driven from `Application::OnWindowResized` — same thread, same GL context,
  same safety as today.
- Replace `OnUpdate()` with `PollEvents()`, and add `WaitEvents()` (`glfwWaitEvents`, for the minimized
  path) and `GetFramebufferSize(u32 &w, u32 &h)`. No `Platform::SwapBuffers` —
  `OpenGLRendererContext::SwapBuffers()` already owns the `GLFWwindow*`.
- Seed `_windowProps` from the real framebuffer size after creation (HiDPI: requested size is in screen
  coords).
- `SetVSync` stays GL-only and its currently-commented-out call site moves into
  `OpenGLRenderer::Initialize()`. Calling `glfwSwapInterval` with no GL context raises
  `GLFW_NO_CURRENT_CONTEXT`, which the error callback would turn into a `VERROR` every Vulkan launch.
  For Vulkan, `WindowProps::EnableVSync` maps to present-mode selection instead.

### 5. Application rewiring ([application.h](engine/include/core/application.h) / [.cpp](engine/src/core/application.cpp))

Member declaration order becomes load-bearing — `VkSurfaceKHR` must be destroyed before the GLFW window,
and today `_platform` is declared before `_windowProps` so `Platform::_windowProps` binds to a
not-yet-constructed member. Reorder to:

```cpp
WindowProps     _windowProps;   // constructed 1st, destroyed last
Ref<Platform>   _platform;      // constructed 2nd
Scope<Renderer> _renderer;      // constructed 3rd, destroyed FIRST
```

`Run()` becomes:

```cpp
RETURN_ON_FAILURE(Renderer::PrepareForWindowCreation(_windowProps.GraphicsAPI));
RETURN_ON_FAILURE(_platform->CreateWindow());
// ... existing VINFO banner ...
_renderer = Renderer::Create(_windowProps.GraphicsAPI);
if (nullptr == _renderer) return StatusCode::UnsupportedGraphicsAPI;
RETURN_ON_FAILURE(_renderer->Initialize());
// ... OnInit ...
while (_running) {
    _platform->PollEvents();
    _layers.ProcessQueuedOperations();
    for (const auto &layer : _layers) layer->OnUpdate(deltaTime);   // ALWAYS — see below
    const FrameResult result = _renderer->BeginFrame();
    if (FrameResult::Rendered == result) { /* P2+: layer->OnRender() */ _renderer->EndFrame(); }
    else if (FrameResult::Failed == result) { Stop(); }
    else { _platform->WaitEvents(); }   // minimized: don't spin a core at 10k it/s
}
_renderer->Shutdown();
_renderer.reset();
```

Layer updates must stay **outside** the `BeginFrame` guard — otherwise minimizing freezes input, physics and
timers, and `lastFrameTime` goes stale so restoring produces a delta spike.

`OnWindowResized` forwards to `_renderer->OnResize(...)`. Add `virtual void OnRender() {}` to
[layer.h](engine/include/core/layer.h) now — one non-breaking line, and exactly the hook Phase 3 needs.

### 6. Vulkan backend — **under `engine/src/renderer/vulkan/`, not `include/`**

Private impl headers live under `src/` in this repo (precedent: `open_gl_renderer_context.h`). Putting volk
in `engine/include/` would leak Vulkan onto every downstream target's include path and into the engine's
public ABI. `tests/CMakeLists.txt` already adds `engine/src` as a PRIVATE include, so white-box tests work
unchanged.

| File | Responsibility |
|---|---|
| `vulkan_common.{h,cpp}` | **The only** place including `<volk.h>`. `ToU32`, `EnumerateVulkan`, `VE_VK_CHECK`/`VE_VK_VERIFY`, `VkResult`/device-type/present-mode/format → string, `SetDebugName(...)`. |
| `vulkan_loader.h` | Deliberately **Vulkan-type-free** so `renderer.cpp` can include it: `InitializeVulkanLoader()`, `IsVulkanSupported()`. |
| `vulkan_platform.{h,cpp}` | The **single TU** including both volk and `<GLFW/glfw3.h>` (volk first). Required instance extensions, `CreateWindowSurface`, framebuffer size. Never includes `vulkyrie_glfw_platform.h` (that would drag in glad). |
| `vulkan_context.h` | `struct VulkanContext` — plain aggregate of `VkInstance/VkPhysicalDevice/VkDevice/VkSurfaceKHR`, `QueueFamilyIndices`, queues, `DeviceCapabilities`, `const VkAllocationCallbacks *Allocator`. Passed by `const&`. **This struct is the Phase-1 RHI seam.** |
| `vulkan_instance.{h,cpp}` | Instance (API 1.3), layer/extension enumeration, opt-in validation, debug-utils messenger, `volkLoadInstanceOnly`. |
| `vulkan_physical_device.{h,cpp}` | Free functions + PODs, not a class. Pure `SelectQueueFamilies` / `ScorePhysicalDevice`; impure `SelectPhysicalDevice` / `QueryDeviceCapabilities`. |
| `vulkan_device.{h,cpp}` | Logical device, dedup'd queue families, feature pNext chain, `volkLoadDevice`. |
| `vulkan_swapchain_support.{h,cpp}` | **Pure selection functions** — the unit-test surface (format, present mode, extent, image count, pre-transform, composite alpha). |
| `vulkan_swapchain.{h,cpp}` | Swapchain, images, views, and the **per-image** `renderFinished` semaphores. |
| `vulkan_renderer.{h,cpp}` | `VulkanRenderer : Renderer`. Owns the above; frames-in-flight; the frame loop. |
| `README.md` | Subsystem doc mirroring [core/jobs/README.md](engine/include/core/jobs/README.md). |

**Cheap now, expensive later — do these in Phase 0:**
- Thread `const VkAllocationCallbacks *Allocator` (currently `nullptr`) through every create/destroy call.
  Phase 1's memory-tracker hook becomes a one-line change instead of ~60 call sites.
- `SetDebugName(...)` (~15 lines, no-op in Release). Every later resource gets named for free.
- Make the command pool a **per-frame struct member**, so Phase 4's per-frame-per-thread pools are a nested
  array rather than a redesign.

**Deliberately not abstracted:** any `IRenderDevice`/`ICommandList` interface, any handle/generation resource
pool, format translation tables, `VkRenderPass`/`VkFramebuffer` wrappers (dynamic rendering makes them
unnecessary), VMA, descriptors, pipeline cache.

### 7. Correctness rules that must be right the first time

**Initialization order — surface comes before device selection.** Present-queue support
(`vkGetPhysicalDeviceSurfaceSupportKHR`) and swapchain adequacy are device-selection criteria:

> loader → instance → messenger → **surface** → physical device → logical device + queues → swapchain →
> frame resources → banner

Shutdown is the exact reverse, preceded by `vkDeviceWaitIdle`. Wrap `Initialize`/`Shutdown` in
`VE_MEMORY_SCOPE(MemoryTag::Rendering)`.

**Semaphore ownership.** `imageAvailable` + the in-flight fence + command pool/buffer are **per
frame-in-flight**. `renderFinished` is **per swapchain image**, owned by `VulkanSwapchain` and recreated with
it. A fence covers the submit, not the present — with 2 frames in flight and 3 images, a per-frame
`renderFinished` gets re-signalled while a pending present still references it
(`VUID-vkQueueSubmit-pSignalSemaphores-00067`).

**Fences.** Create with `VK_FENCE_CREATE_SIGNALED_BIT` or the first wait hangs forever. `vkResetFences` must
come **after** the acquire early-outs — resetting and then returning `Skipped` leaves an unsignalled fence
nothing will ever signal.

**Acquire / present result matrix:**

| Result | Semaphore state | Action |
|---|---|---|
| `VK_ERROR_OUT_OF_DATE_KHR` (acquire) | not signalled, safe to reuse | mark dirty, **don't** reset the fence, return `Skipped` |
| `VK_SUBOPTIMAL_KHR` (acquire) | **signalled** | mark dirty, **render and present this frame normally** — bailing abandons a signalled semaphore |
| `VK_SUCCESS` | signalled | proceed |
| `OUT_OF_DATE` / `SUBOPTIMAL` (present) | already consumed | mark dirty |

`VK_SUBOPTIMAL_KHR` is a **positive** (success-class) result, so `VE_VK_CHECK` must not be used on acquire or
present. `VkResult` is a signed enum — cast to `i32` for numeric fallback.

**Recreation happens only at the top of `BeginFrame`, before any acquire** — never inside a GLFW callback,
never mid-frame. `OnResize` sets a dirty flag and nothing else. The sequence:
`glfwGetFramebufferSize` (if `0×0`, stay dirty and return `Skipped`) → `vkDeviceWaitIdle` → re-query surface
caps → create with `oldSwapchain` → destroy old views, then the old swapchain, **then** re-create the
per-image `renderFinished` semaphores (destroying the swapchain is what releases the presentation engine's
references; `vkDeviceWaitIdle` does not cover the present engine).

**Extent rule:** if `caps.currentExtent.width != 0xFFFFFFFF`, use `currentExtent` verbatim — it is
authoritative and overrides GLFW. Only clamp the framebuffer size when it is `0xFFFFFFFF`.

Binary semaphores + one fence per frame-in-flight is the complete Phase-0 answer; timeline semaphores cannot
be used with acquire/present anyway.

**Feature-query hazard:** the `VkPhysicalDeviceFeatures2` pNext chain used to *query* must be a separate,
persistent chain from the one handed to `VkDeviceCreateInfo::pNext`. Querying into a local and passing the
dangling chain to device creation is the classic bug here.

### 8. Validation layers — warn, never fail

`/usr/share/vulkan/explicit_layer.d/` is **empty** on this machine; the layers live only under the SDK.
`VK_ADD_LAYER_PATH` is set in your shell, so validation works from a terminal but will be silently absent
when launched from an IDE or file manager. So: enumerate layers, and if `VK_LAYER_KHRONOS_validation` is
missing emit a `VWARN` containing the exact remedy, then continue.

`VK_EXT_debug_utils` is an **instance extension independent of the validation layer** — enable it in Debug
builds regardless, so object naming and labels work either way. Pass a
`VkDebugUtilsMessengerCreateInfoEXT` via `VkInstanceCreateInfo::pNext` so instance create/destroy is covered
too. Map severity → `VERROR/VWARN/VINFO/VTRACE` with a `[VulkanRenderer]` prefix, mirroring the GL debug
callback at [open_gl_renderer_context.cpp:20-119](engine/src/renderer/open_gl/open_gl_renderer_context.cpp#L20-L119).

### 9. Status codes

Append to [status_codes.h](engine/include/core/status_codes.h) (values become `main` exit codes, so append
only): `VulkanLoaderNotFound`, `VulkanVersionNotSupported`, `FailedToCreateVulkanInstance`,
`NoSuitableVulkanDevice`, `FailedToCreateVulkanDevice`, `FailedToCreateVulkanSurface`,
`FailedToCreateSwapchain`, `FailedToCreateVulkanSyncObjects`, `VulkanDeviceLost`.

### 10. Example app

`examples/vulkan_bootstrap/` — CMakeLists copied from sandbox **minus** the `copy_directory` POST_BUILD step
(there is no assets dir; the command would fail) and minus `imgui::imgui`. Register it inside the
`VULKYRIE_BUILD_EXAMPLES` block of the root [CMakeLists.txt](CMakeLists.txt); no preset changes needed.

`Application` has no per-frame virtual hook, so push exactly **one** trivial layer whose
`OnUpdate(Timestep)` accumulates time and calls `GetRenderer()->SetClearColor(...)`. That also proves the
layer stack works under Vulkan, and gives `SetClearColor` on the base a real reason to exist (GL implements
it as `glClearColor`/`glClear`, keeping both backends honest).

### 11. Tests — `tests/src/renderer/vulkan/`

The selection logic is pure free functions over plain structs precisely so it tests without a `VkInstance`.
Repo style: full-sentence `TEST_CASE` names, lowercase tags.

- `vulkan_swapchain_selection_tests.cpp` `[renderer][vulkan][swapchain]` — preferred
  `B8G8R8A8_SRGB`/`SRGB_NONLINEAR` chosen when present; first-format fallback; FIFO when VSync is requested
  even if MAILBOX exists; MAILBOX preferred when VSync is off; IMMEDIATE→FIFO fallback; `currentExtent`
  returned verbatim when concrete; framebuffer size clamped when it is `0xFFFFFFFF`; `minImageCount + 1`
  respecting a non-zero `maxImageCount`.
- `vulkan_device_selection_tests.cpp` `[renderer][vulkan][device]` — combined graphics+present family
  preferred; separate-family fallback; dedicated transfer family picked; discrete scored above integrated;
  a device missing `dynamicRendering` scores 0.
- `vulkan_instance_smoke_tests.cpp` — tagged `[.][vulkan-device]` (Catch2 hidden) so `ctest` stays hermetic;
  creates an instance + picks a device when an ICD is present.

---

## Implementation sequence

Steps 1–4 contain **zero Vulkan code** — deliberately, so the abstraction refactor and the new API never
fail at the same time. Each step ends green and is independently bisectable.

| # | Work | Checkpoint |
|---|---|---|
| 0 | Baseline | `/build clang-all-debug`; run sandbox + editor, resize both. Know what "working" looks like. |
| 1 | `vcpkg.json` + `Dependencies.cmake` + engine link | vcpkg re-resolves (slow, one-time); `volk.h` present in `vcpkg_installed`; sandbox still runs. |
| 2 | `vulkan_common.{h,cpp}` **only** — include seam, `ToU32`, `EnumerateVulkan`, string helpers, `VE_VK_CHECK`, `SetDebugName` | Builds under `-Werror` on **both** `clang-all-debug` and `gcc-all-debug`. Don't skip — this is where the initializer-style decision gets proven at near-zero risk. |
| 3 | Platform hardening, **additive only** (error callback first, `glfwInit` checked, hints gated, `NO_API`, `PollEvents`/`WaitEvents`/`GetFramebufferSize`). Keep `OnUpdate` and the callback's `glViewport` for now. | sandbox + editor + asteroids behave exactly as step 0. |
| 4 | `Renderer` seam + `OpenGLRenderer` + `Application` rewiring; delete the free `Initialize` and `_graphicsContext`; relocate `glViewport`; fix member order; `Layer::OnRender()` | **The big one.** All three GL apps render, resize correctly, and exit with no GL errors after `main` returns (static-destruction bug proven fixed). Test suite green. |
| 5 | Vulkan loader bootstrap + instance + messenger + the example (`Initialize` stops after the instance; `BeginFrame` returns `Skipped`) | `vulkan_bootstrap` opens a blank window, logs instance version / extensions / validation status, exits clean. Re-run with `VK_ADD_LAYER_PATH` unset to prove warn-and-continue. |
| 6 | Surface + physical device + logical device + capabilities | Banner logs device name, API version, driver, queue families, and `dynamicRendering`/`synchronization2`/`timelineSemaphore`. |
| 7 | Swapchain + pure selection functions + unit tests | `tests "[vulkan]"` passes; flipping `EnableVSync` in the example changes the logged present mode. |
| 8 | Frame loop — pools, sync objects, sync2 barriers, `vkCmdBeginRendering` clear, submit, present, recreation | Animated clear colour; drag-resize; minimize/restore; alt-tab; close. Validation silent. |
| 9 | README, statistics wired, `/format-check`, cross-compiler + release builds | Everything below. |

## Verification

- **Build:** `/build clang-all-debug`, then `gcc-all-debug` and `clang-all-release`. Warnings are errors, so
  a clean build is a real gate.
- **Tests:** `/test clang-all-debug "[vulkan]"` for the selection logic; full `/test clang-all-debug` for no
  regressions (notably the existing `[framegraph]` suite).
- **Vulkan end-to-end:** `build/clang-all-debug/examples/vulkan_bootstrap/vulkan_bootstrap` — animated clear
  colour, continuous drag-resize, minimize→restore, alt-tab, clean close. **Zero validation errors.** Then
  re-run with synchronization validation enabled (vkconfig, or
  `VK_LAYER_ENABLES=VALIDATION_CHECK_ENABLE_SYNCHRONIZATION_VALIDATION`) — this is what proves the per-image
  `renderFinished` decision.
- **OpenGL regression (equally important):** `examples/sandbox`, `examples/asteroids`, and `editor` all still
  render, resize correctly, and now shut down without post-`main` GL calls.
- **Tooling:** a RenderDoc capture shows the named device/queue/swapchain objects from `SetDebugName`.
- **Style:** `/format-check` clean.
