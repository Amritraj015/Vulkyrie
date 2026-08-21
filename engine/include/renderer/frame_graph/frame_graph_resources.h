#pragma once

#include "vlkypch.h"
#include "renderer/frame_graph/frame_graph_core.h"
#include "renderer/frame_graph/resource_entry.h"

namespace Vulkyrie {

    /** @brief The read-only view of a compiled graph's resources that a pass's execute function receives.
     *
     * This is everything a pass can reach of the graph, and it is deliberately very little: resolve a handle to the
     * resource object, or to the descriptor it was declared with. There is no `AddPass`, no `Compile`, no mutable
     * resource access, and no way to acquire or release anything - resource lifetime belongs to the graph, and a
     * pass body that tried to take part in it would corrupt the plan `Compile` just built.
     *
     * `Get` returns `const T &`, which is what closes that door: `Acquire` and `Release` are non-const members of
     * every resource type, so they are unreachable through this view. Binding a resource only needs to read its
     * handle, so the constness costs a pass nothing.
     *
     * Non-owning and built per execution; it must not outlive the graph it views.
     *
     * @tparam B The renderer backend the graph was built for. */
    template <RendererBackend B> class FrameGraphResources final {
    public:
        VE_DELETE_MOVE_AND_COPY(FrameGraphResources);

        ~FrameGraphResources() = default;

        /** @brief Resolves a typed handle to the resource object it refers to. This is what lets a pass actually
         * bind the resources it declared.
         * @tparam T The resource type; validated against the entry's recorded type in Debug builds.
         * @param handle The resource to resolve. */
        template <FrameGraphResourceType<B> T> [[nodiscard]] VE_INLINE const T &Get(FrameGraphHandle<T> handle) const {
            return entry(handle.ID).template GetResource<T>();
        }

        /** @brief Returns the descriptor a resource was declared with, so a pass can read an extent or a format
         * without carrying a copy of it in its pass data.
         * @tparam T The resource type.
         * @param handle The resource to look up. */
        template <FrameGraphResourceType<B> T> [[nodiscard]] VE_INLINE const typename T::Descriptor &GetDescriptor(FrameGraphHandle<T> handle) const {
            return entry(handle.ID).template GetDescriptor<T>();
        }

        /** @brief Returns the name a resource was declared with, for debug markers and diagnostics.
         * @tparam T The resource type.
         * @param handle The resource to look up. */
        template <typename T> [[nodiscard]] VE_INLINE StaticString GetName(FrameGraphHandle<T> handle) const {
            return mCore.GetResourceName(handle.ID);
        }

    private:
        template <RendererBackend> friend class FrameGraph;

        FrameGraphResources(const detail::FrameGraphCore &core, const std::vector<ResourceEntry<B>> &entries)
            : mCore(core)
            , mEntries(entries) {
        }

        [[nodiscard]] VE_INLINE const ResourceEntry<B> &entry(FrameGraphResourceID resourceID) const {
            return mEntries[mCore.GetEntryID(resourceID).Get()];
        }

        const detail::FrameGraphCore &mCore;
        const std::vector<ResourceEntry<B>> &mEntries;
    };

} // namespace Vulkyrie
