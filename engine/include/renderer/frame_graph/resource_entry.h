#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "renderer/frame_graph/frame_graph_concepts.h"
#include "renderer/frame_graph/frame_graph_types.h"

namespace Vulkyrie {

    /** @brief The arena block a resource owns: its descriptor followed by the resource object. */
    template <typename T> struct FrameGraphResourceStorage final {
    public:
        FrameGraphResourceStorage(const typename T::Descriptor &descriptor, T &&resource)
            : Descriptor(descriptor)
            , Resource(std::move(resource)) {
        }

        typename T::Descriptor Descriptor;
        T Resource;
    };

    /** @brief The backing store for one resource: the type-erased resource object and the trampolines that reach
     * it. Several `ResourceNode` versions can point at a single entry, and the entry's compile-time state -
     * lifetime, alias placement, version - lives beside it in `detail::EntryState`, which `Compile` owns.
     *
     * Type erasure is by function pointer rather than by virtual dispatch: the storage lives in the frame arena and
     * the entry holds trampolines for it. Optional hooks are null when the resource type does not implement them,
     * so a type without `PreRead` costs an inspected pointer rather than an indirect call into an empty body.
     *
     * @tparam B The renderer backend resources are acquired against. */
    template <RendererBackend B> class ResourceEntry final {
        template <RendererBackend> friend class FrameGraph;
        template <RendererBackend> friend class FrameGraphResources;

    public:
        /** @brief Takes the resource from the transient pool, or binds it to the storage the aliasing plan gave it. */
        using AcquireFn = void (*)(void *storage, ResourceLifetime lifetime, ResourcePlacement placement, const FrameGraphContext<B> &context);

        /** @brief Returns the resource to the pool. */
        using ReleaseFn = void (*)(void *storage, const FrameGraphContext<B> &context);

        /** @brief Notifies the resource type of an upcoming access. */
        using AccessFn = void (*)(void *storage, const ResourceUsage &usage, const FrameGraphContext<B> &context);

        /** @brief Runs the storage block's destructor; null when it is trivially destructible. */
        using DestructStorageFn = void (*)(void *storage);

        /** @brief Reports the memory the resource needs, for the byte-packing aliasing plan. */
        using MemoryRequirementsFn = ResourceMemoryRequirements (*)(const void *storage, const Device<B> &device);

        ResourceEntry() = default;

        /** @brief Returns the stored resource object.
         *
         * Const only, and deliberately so. `Acquire` and `Release` are non-const members of the resource type, so a
         * pass body holding one of these can neither materialize nor release a resource behind the graph's back -
         * the lifetime is the graph's to manage, and the compiler enforces that rather than a comment asking nicely.
         *
         * @tparam T The type the resource was created with. Naming a different type is a programming error; it is
         * caught by an assertion in Debug and is undefined behavior in Release, which is why callers should come
         * through a typed `FrameGraphHandle<T>` rather than a raw id. */
        template <FrameGraphResourceType<B> T> [[nodiscard]] const T &GetResource() const {
            VASSERT(mResourceTypeID == FrameGraphTypeID<T>(), "Frame graph resource accessed as the wrong resource type.");

            return static_cast<const FrameGraphResourceStorage<T> *>(pStorage)->Resource;
        }

        /** @brief Returns the descriptor the resource was created with.
         * @tparam T The type the resource was created with. */
        template <FrameGraphResourceType<B> T> [[nodiscard]] const typename T::Descriptor &GetDescriptor() const {
            VASSERT(mResourceTypeID == FrameGraphTypeID<T>(), "Frame graph resource accessed as the wrong resource type.");

            return static_cast<const FrameGraphResourceStorage<T> *>(pStorage)->Descriptor;
        }

    private:
        /** @brief Wires an entry to an arena-constructed storage block of a concrete resource type.
         * @param storage The arena block holding the descriptor and resource object. */
        template <FrameGraphResourceType<B> T> [[nodiscard]] static ResourceEntry create(FrameGraphResourceStorage<T> *storage) {
            using Storage = FrameGraphResourceStorage<T>;

            ResourceEntry entry;
            entry.pStorage = storage;
            entry.mResourceTypeID = FrameGraphTypeID<T>();

            entry.mAcquireFn = [](void *s, ResourceLifetime lifetime, ResourcePlacement placement, const FrameGraphContext<B> &context) {
                auto *typed = static_cast<Storage *>(s);

                if constexpr (HasPlacedAcquire<T, B>) {
                    typed->Resource.Acquire(typed->Descriptor, lifetime, placement, context);
                } else {
                    (void)placement;
                    typed->Resource.Acquire(typed->Descriptor, lifetime, context);
                }
            };

            entry.mReleaseFn = [](void *s, const FrameGraphContext<B> &context) { static_cast<Storage *>(s)->Resource.Release(context); };

            // Null rather than an empty body: the execute loop skips the call entirely instead of paying an
            // indirect jump into a function that does nothing.
            if constexpr (HasPreRead<T, B>) {
                entry.mPreReadFn = [](void *s, const ResourceUsage &usage, const FrameGraphContext<B> &context) {
                    static_cast<Storage *>(s)->Resource.PreRead(usage, context);
                };
            }

            if constexpr (HasPreWrite<T, B>) {
                entry.mPreWriteFn = [](void *s, const ResourceUsage &usage, const FrameGraphContext<B> &context) {
                    static_cast<Storage *>(s)->Resource.PreWrite(usage, context);
                };
            }

            if constexpr (HasMemoryRequirements<T, B>) {
                entry.mMemoryRequirementsFn = [](const void *s, const Device<B> &device) {
                    const auto *typed = static_cast<const Storage *>(s);

                    return typed->Resource.GetMemoryRequirements(typed->Descriptor, device);
                };
            }

            if constexpr (!std::is_trivially_destructible_v<Storage>) {
                entry.mDestructStorageFn = [](void *s) { std::destroy_at(static_cast<Storage *>(s)); };
            }

            return entry;
        }

        /** @brief Takes the resource from the pool. Only transients are acquired; imported ones already exist.
         * @param isTransient Whether the graph owns this resource's lifetime, from `detail::EntryState`.
         * @param lifetime The execution-order interval the resource is live over.
         * @param placement Where the byte-packing plan put it, if anywhere. */
        VE_INLINE void acquireResource(bool isTransient, ResourceLifetime lifetime, ResourcePlacement placement, const FrameGraphContext<B> &context) {
            if (!mIsLive && isTransient) {
                mAcquireFn(pStorage, lifetime, placement, context);
                mIsLive = true;
            }
        }

        /** @brief Returns the resource to the pool. Only transients are released; imported ones are managed
         * externally. */
        VE_INLINE void releaseResource(bool isTransient, const FrameGraphContext<B> &context) {
            if (mIsLive && isTransient) {
                mReleaseFn(pStorage, context);
                mIsLive = false;
            }
        }

        /** @brief Notifies the resource type of an upcoming read, if it implements the hook. */
        VE_INLINE void preRead(const ResourceUsage &usage, const FrameGraphContext<B> &context) {
            if (nullptr != mPreReadFn) {
                mPreReadFn(pStorage, usage, context);
            }
        }

        /** @brief Notifies the resource type of an upcoming write, if it implements the hook. */
        VE_INLINE void preWrite(const ResourceUsage &usage, const FrameGraphContext<B> &context) {
            if (nullptr != mPreWriteFn) {
                mPreWriteFn(pStorage, usage, context);
            }
        }

        /** @brief Returns the resource's memory requirements, or a zero size when the resource type does not report
         * them - in which case it is excluded from the byte-packing plan. */
        [[nodiscard]] VE_INLINE ResourceMemoryRequirements getMemoryRequirements(const Device<B> &device) const {
            if (nullptr == mMemoryRequirementsFn) {
                return ResourceMemoryRequirements{};
            }

            return mMemoryRequirementsFn(pStorage, device);
        }

        /** @brief Runs the storage block's destructor if it has one. The arena releases the storage itself. */
        VE_INLINE void destructStorage() {
            if (nullptr != mDestructStorageFn) {
                mDestructStorageFn(pStorage);
            }
        }

        /** @brief Whether the resource is currently materialized. */
        [[nodiscard]] VE_INLINE bool isLive() const noexcept {
            return mIsLive;
        }

        /** @brief Arena block holding `{ Descriptor, T }`. Not owned - the arena owns it. */
        void *pStorage = nullptr;

        AcquireFn mAcquireFn = nullptr;

        ReleaseFn mReleaseFn = nullptr;

        /** @brief Read hook, or null when the resource type does not implement one. */
        AccessFn mPreReadFn = nullptr;

        /** @brief Write hook, or null when the resource type does not implement one. */
        AccessFn mPreWriteFn = nullptr;

        /** @brief Storage destructor, or null when the storage block is trivially destructible. */
        DestructStorageFn mDestructStorageFn = nullptr;

        /** @brief Memory-requirements query, or null when the resource type does not report them. */
        MemoryRequirementsFn mMemoryRequirementsFn = nullptr;

        /** @brief Resource type id, used to validate `GetResource<T>()` in Debug builds. */
        u16 mResourceTypeID = 0;

        /** @brief Guards against a double acquire or a double release when a resource's last user appears more
         * than once. */
        bool mIsLive = false;
    };

} // namespace Vulkyrie
