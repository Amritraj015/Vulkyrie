#pragma once

#include "vlkypch.h"
#include "renderer/backend_concepts.h"
#include "renderer/common/device.h"
#include "renderer/common/frame_context.h"

namespace Vulkyrie {

    /** @brief What acquiring and releasing a frame graph resource needs: the device that owns the transient pool,
     * and the frame the resources belong to.
     *
     * Deliberately carries no command list. Materializing a resource must not be able to record commands - that is
     * a pass's job, and a pass gets `FrameGraphPassContext` instead. Both structs hold references, so neither is
     * default-constructible: there is no way to hand a resource type a zeroed context.
     *
     * @tparam B The renderer backend trait struct. */
    template <RendererBackend B> struct FrameGraphContext final {
    public:
        /** @brief The device owning the transient pool resources are acquired from. */
        Vulkyrie::Device<B> &Device;

        /** @brief The frame in flight this graph is being executed for. */
        Vulkyrie::FrameContext<B> &Frame;
    };

    /** @brief What one pass's execute function receives: the device, the command list this pass records into, and
     * which worker is doing the recording.
     *
     * Built per pass by the graph rather than passed in, because under `RecordParallel` each worker records into
     * its own command list - a single list shared by the frame would be a data race by construction.
     *
     * @tparam B The renderer backend trait struct. */
    template <RendererBackend B> struct FrameGraphPassContext final {
    public:
        /** @brief The device, for reaching caches and capabilities while recording. */
        Vulkyrie::Device<B> &Device;

        /** @brief The command list this pass records into. Owned by the frame, not by the pass. */
        typename B::CommandList &Commands;

        /** @brief Which worker is recording this pass; always zero outside `RecordParallel`. Anything a pass needs
         * to allocate while recording must be partitioned by this index. */
        u32 WorkerIndex = 0;
    };

} // namespace Vulkyrie
