#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "renderer/frame_graph/frame_graph_concepts.h"
#include "renderer/frame_graph/frame_graph_types.h"

namespace Vulkyrie {

    /** @brief The arena block a resource owns: its descriptor followed by the resource object. */
    template <FrameGraphResourceType T> struct FrameGraphResourceStorage final {
    public:
        FrameGraphResourceStorage(const typename T::Descriptor &descriptor, T &&resource)
            : Descriptor(descriptor)
            , Resource(std::move(resource)) {
        }

        typename T::Descriptor Descriptor;
        T Resource;
    };

    /** @brief The backing store for one resource: the type-erased resource object plus the lifetime and access
     * bookkeeping the graph needs. Several `ResourceNode` versions can point at a single entry.
     *
     * Type erasure is by function pointer rather than by virtual dispatch: the storage lives in the frame arena and
     * the entry holds trampolines for it. Optional hooks are null when the resource type does not implement them,
     * so a type without `PreRead` costs an inspected pointer rather than an indirect call into an empty body. */
    class ResourceEntry final {
        friend class FrameGraph;

    public:
        /** @brief Whether the graph owns the resource's lifetime. */
        enum class Lifetime : u32 {
            /** @brief Created and destroyed by the graph, within the frame. */
            Transient,

            /** @brief Owned externally; the graph neither creates nor destroys it. */
            Imported
        };

        /** @brief Materializes the resource.
         *
         * Takes the entry rather than the storage so the placement is read inside the trampoline, which knows at
         * compile time whether the resource type wants it. Passing the placement in would make every type pay to
         * load and forward two values most of them discard; this way the plain-`Create` path is byte-identical to
         * one that has never heard of aliasing. */
        using CreateFn = void (*)(ResourceEntry &entry, const FrameGraphContext &context);

        /** @brief Releases the resource. */
        using DestroyResourceFn = void (*)(void *storage, const FrameGraphContext &context);

        /** @brief Notifies the resource type of an upcoming access. */
        using AccessFn = void (*)(void *storage, const ResourceUsage &usage, const FrameGraphContext &context);

        /** @brief Runs the storage block's destructor; null when it is trivially destructible. */
        using DestructStorageFn = void (*)(void *storage);

        /** @brief Reports the memory the resource needs, for transient aliasing. */
        using MemoryRequirementsFn = ResourceMemoryRequirements (*)(const void *storage);

        ResourceEntry() = default;

        [[nodiscard]] VE_INLINE FrameGraphResourceEntryID GetResourceEntryID() const {
            return _resourceEntryID;
        }

        /** @brief Whether the graph creates and destroys this resource within the frame. */
        [[nodiscard]] VE_INLINE bool IsTransient() const {
            return _lifetime == Lifetime::Transient;
        }

        /** @brief Whether the resource is owned outside the graph. */
        [[nodiscard]] VE_INLINE bool IsImported() const {
            return _lifetime == Lifetime::Imported;
        }

        /** @brief Returns the current version; each write produces the next one. */
        [[nodiscard]] VE_INLINE u32 GetVersion() const {
            return _version;
        }

        /** @brief Returns the stored resource object.
         * @tparam T The type the resource was created with. Naming a different type is a programming error; it is
         * caught by an assertion in Debug and is undefined behavior in Release, which is why callers should come
         * through a typed `FrameGraphHandle<T>` rather than a raw id. */
        template <FrameGraphResourceType T> [[nodiscard]] T &GetResource() {
            VASSERT(_resourceTypeID == FrameGraphTypeID<T>(), "Frame graph resource accessed as the wrong resource type.");
            return static_cast<FrameGraphResourceStorage<T> *>(_storage)->Resource;
        }

        /** @brief Returns the stored resource object.
         * @tparam T The type the resource was created with. */
        template <FrameGraphResourceType T> [[nodiscard]] const T &GetResource() const {
            VASSERT(_resourceTypeID == FrameGraphTypeID<T>(), "Frame graph resource accessed as the wrong resource type.");
            return static_cast<const FrameGraphResourceStorage<T> *>(_storage)->Resource;
        }

        /** @brief Returns the descriptor the resource was created with.
         * @tparam T The type the resource was created with. */
        template <FrameGraphResourceType T> [[nodiscard]] const typename T::Descriptor &GetDescriptor() const {
            VASSERT(_resourceTypeID == FrameGraphTypeID<T>(), "Frame graph resource accessed as the wrong resource type.");
            return static_cast<const FrameGraphResourceStorage<T> *>(_storage)->Descriptor;
        }

    private:
        /** @brief Wires an entry to an arena-constructed storage block of a concrete resource type.
         * @param lifetime Whether the graph owns the resource's lifetime.
         * @param resourceEntryID This entry's index in the graph's entry array.
         * @param storage The arena block holding the descriptor and resource object. */
        template <FrameGraphResourceType T>
        static ResourceEntry create(Lifetime lifetime, FrameGraphResourceEntryID resourceEntryID, FrameGraphResourceStorage<T> *storage) {
            using Storage = FrameGraphResourceStorage<T>;

            ResourceEntry entry;
            entry._storage = storage;
            entry._lifetime = lifetime;
            entry._resourceEntryID = resourceEntryID;
            entry._version = INITIAL_RESOURCE_VERSION;
            entry._resourceTypeID = FrameGraphTypeID<T>();

            entry._create = [](ResourceEntry &e, const FrameGraphContext &context) {
                auto *typed = static_cast<Storage *>(e._storage);

                if constexpr (HasPlacedCreate<T>) {
                    typed->Resource.Create(typed->Descriptor, ResourcePlacement{ .Offset = e._aliasOffset, .IsAliased = e._isAliased }, context);
                } else {
                    typed->Resource.Create(typed->Descriptor, context);
                }
            };

            entry._destroyResource = [](void *s, const FrameGraphContext &context) {
                auto *typed = static_cast<Storage *>(s);
                typed->Resource.Destroy(typed->Descriptor, context);
            };

            // Null rather than an empty body: the execute loop skips the call entirely instead of paying an
            // indirect jump into a function that does nothing.
            if constexpr (HasPreRead<T>) {
                entry._preRead = [](void *s, const ResourceUsage &usage, const FrameGraphContext &context) {
                    static_cast<Storage *>(s)->Resource.PreRead(usage, context);
                };
            }

            if constexpr (HasPreWrite<T>) {
                entry._preWrite = [](void *s, const ResourceUsage &usage, const FrameGraphContext &context) {
                    static_cast<Storage *>(s)->Resource.PreWrite(usage, context);
                };
            }

            if constexpr (HasMemoryRequirements<T>) {
                entry._memoryRequirements = [](const void *s) {
                    const auto *typed = static_cast<const Storage *>(s);
                    return typed->Resource.GetMemoryRequirements(typed->Descriptor);
                };
            }

            if constexpr (!std::is_trivially_destructible_v<Storage>) {
                entry._destructStorage = [](void *s) { std::destroy_at(static_cast<Storage *>(s)); };
            }

            return entry;
        }

        /** @brief Materializes the resource. Only transients are created; imported ones already exist. */
        VE_INLINE void createResource(const FrameGraphContext &context) {
            if (!_isLive && _lifetime == Lifetime::Transient) {
                _create(*this, context);
                _isLive = true;
            }
        }

        /** @brief Releases the resource. Only transients are destroyed; imported ones are managed externally. */
        VE_INLINE void destroyResource(const FrameGraphContext &context) {
            if (_isLive && _lifetime == Lifetime::Transient) {
                _destroyResource(_storage, context);
                _isLive = false;
            }
        }

        /** @brief Notifies the resource type of an upcoming read, if it implements the hook. */
        VE_INLINE void preRead(const ResourceUsage &usage, const FrameGraphContext &context) {
            if (nullptr != _preRead) {
                _preRead(_storage, usage, context);
            }
        }

        /** @brief Notifies the resource type of an upcoming write, if it implements the hook. */
        VE_INLINE void preWrite(const ResourceUsage &usage, const FrameGraphContext &context) {
            if (nullptr != _preWrite) {
                _preWrite(_storage, usage, context);
            }
        }

        /** @brief Returns the resource's memory requirements, or nullopt when the resource type does not report
         * them - in which case it is excluded from the aliasing plan. */
        [[nodiscard]] VE_INLINE std::optional<ResourceMemoryRequirements> getMemoryRequirements() const {
            if (nullptr == _memoryRequirements) {
                return std::nullopt;
            }

            return _memoryRequirements(_storage);
        }

        /** @brief Runs the storage block's destructor if it has one. The arena releases the storage itself. */
        VE_INLINE void destructStorage() {
            if (nullptr != _destructStorage) {
                _destructStorage(_storage);
            }
        }

        static constexpr u32 INITIAL_RESOURCE_VERSION = 1U;

        /** @brief Arena block holding `{ Descriptor, T }`. Not owned - the arena owns it. */
        void *_storage = nullptr;

        CreateFn _create = nullptr;

        DestroyResourceFn _destroyResource = nullptr;

        /** @brief Read hook, or null when the resource type does not implement one. */
        AccessFn _preRead = nullptr;

        /** @brief Write hook, or null when the resource type does not implement one. */
        AccessFn _preWrite = nullptr;

        /** @brief Storage destructor, or null when the storage block is trivially destructible. */
        DestructStorageFn _destructStorage = nullptr;

        /** @brief Memory-requirements query, or null when the resource type does not report them. */
        MemoryRequirementsFn _memoryRequirements = nullptr;

        /** @brief The ID (index) assigned to this resource entry by the frame graph. */
        FrameGraphResourceEntryID _resourceEntryID{};

        /** @brief Bumped by each write, so stale handles can be detected. */
        u32 _version = INITIAL_RESOURCE_VERSION;

        /** @brief Execution-order position of the first pass that touches this resource, or `~0U` if unused.
         * Together with `_lastUseIndex` this is the interval the aliasing allocator packs. */
        u32 _firstUseIndex = std::numeric_limits<u32>::max();

        /** @brief Execution-order position of the last pass that touches this resource, or `~0U` if unused. The
         * resource is released after that pass. */
        u32 _lastUseIndex = std::numeric_limits<u32>::max();

        /** @brief Offset the transient aliasing plan assigned, meaningful only when `_isAliased` is set. Held
         * unpacked rather than as a `ResourcePlacement` so the flag can share the tail padding below instead of
         * costing the entry another eight bytes of alignment; `createResource` reassembles the pair in registers. */
        u64 _aliasOffset = 0;

        /** @brief Lifetime of this resource. */
        Lifetime _lifetime = Lifetime::Transient;

        /** @brief Resource type id, used to validate `GetResource<T>()` in Debug builds. */
        u16 _resourceTypeID = 0;

        /** @brief Whether the resource is currently materialized. Guards against a double create or a double
         * destroy when a resource's last user appears more than once. */
        bool _isLive = false;

        /** @brief Whether the aliasing plan placed this resource. Sits here, in the padding after `_isLive`, so it
         * is free. */
        bool _isAliased = false;
    };

    static_assert(std::is_move_assignable_v<ResourceEntry>, "ResourceEntry must be move-assignable so the graph can reuse its entries across frames.");

} // namespace Vulkyrie
