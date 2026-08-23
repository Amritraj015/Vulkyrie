#pragma once

#include "vlkypch.h"
#include "renderer/backend_concepts.h"
#include "renderer/common/device.h"
#include "renderer/common/frame_context.h"

namespace Vulkyrie {

    // Held by reference below, so the declaration is enough. Including the header would be circular:
    // frame_graph_resources.h -> resource_entry.h -> frame_graph_concepts.h -> this file.
    template <RendererBackend B> class FrameGraphResources;

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

    /** @brief What one pass's execute function receives: the device, the resources it declared, the command list
     * it records into, and which worker is doing the recording.
     *
     * Built per pass by the graph rather than passed in, because under `RecordParallel` each worker records into
     * its own command list - a single list shared by the frame would be a data race by construction.
     *
     * @tparam B The renderer backend trait struct. */
    template <RendererBackend B> struct FrameGraphPassContext final {
    public:
        /** @brief The device, for reaching caches and capabilities while recording. */
        Vulkyrie::Device<B> &Device;

        /** @brief Resolves the handles in the pass data to the resource objects behind them. Const, so a pass can
         * read a resource but cannot take part in the lifetime the graph just planned. */
        const FrameGraphResources<B> &Resources;

        /** @brief The command list this pass records into. Owned by the frame, not by the pass. */
        typename B::CommandList &Commands;

        /** @brief Which worker is recording this pass; always zero outside `RecordParallel`. Anything a pass needs
         * to allocate while recording must be partitioned by this index. */
        u32 WorkerIndex = 0;
    };

} // namespace Vulkyrie
