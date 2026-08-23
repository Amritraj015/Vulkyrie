#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "renderer/backend_concepts.h"
#include "renderer/common/deletion_queue.h"
#include "renderer/common/transient_registry.h"
#include "renderer/rhi/resource_types.h"

namespace Vulkyrie {

    /** @brief Hands out backend resources for the duration of a frame and reclaims them in bulk, so nothing is
     * created or destroyed per frame.
     *
     * Keyed by `TransientTextureID` / `TransientBufferID` rather than by descriptor. That is what keeps acquisition
     * free of hashing: the registry established the descriptor's identity once, so a bucket is an array index and
     * the candidates in it are an intrusive chain through the entry array. A frame declaring the same resources it
     * declared last frame performs no hash and touches no hash map.
     *
     * @tparam B The renderer backend the resources come from. */
    template <RendererBackend B> class TransientPool final {
    public:
        struct Stats {
            u32 ImageCount = 0;
            u32 BufferCount = 0;
            u32 ImagesCreatedThisFrame = 0;
            u32 BuffersCreatedThisFrame = 0;
        };

        /** @brief One image handed out, and whether its contents belong to something else.
         *
         * `RequiresDiscard` is false only for an image the pool created on the spot. Anything reused holds either
         * another resource's pixels from earlier this frame or its own from a previous frame, and a pass that
         * blends into it rather than fully overwriting it has to know the difference. */
        struct ImageAcquisition final {
            typename B::Image Image{};
            bool RequiresDiscard = true;
        };

        /** @brief The buffer counterpart of `ImageAcquisition`; same contract, same reason. */
        struct BufferAcquisition final {
            typename B::Buffer Buffer{};
            bool RequiresDiscard = true;
        };

        /** @brief Constructs an empty pool.
         * @param context The backend that creates resources on a miss.
         * @param registry Resolves an id back to the descriptor a new resource must be created from.
         * @param deletionQueue Where trimmed and destroyed resources go.
         * @param maxTransientImages Reserve hint for live images.
         * @param maxTransientBuffers Reserve hint for live buffers. */
        explicit TransientPool(typename B::Context &context,
                               TransientRegistry<B> &registry,
                               DeletionQueue<B> &deletionQueue,
                               size_t maxTransientImages,
                               size_t maxTransientBuffers) noexcept
            : mContext(context)
            , mRegistry(registry)
            , mDeletionQueue(deletionQueue) {

            // Reserve so there are no steady state allocations.
            mImages.Entries.reserve(maxTransientImages);
            mBuffers.Entries.reserve(maxTransientBuffers);
        }

        VE_DELETE_MOVE_AND_COPY(TransientPool);

        ~TransientPool() {
            for (auto &e : mImages.Entries) {
                mDeletionQueue.Push(e.Handle);
            }

            for (auto &e : mBuffers.Entries) {
                mDeletionQueue.Push(e.Handle);
            }
        }

        /** @brief Takes an image for a registered descriptor.
         * @param id The registered descriptor to take an image for.
         * @param lifetime The execution-order interval the image is needed over. Two requests for the same id with
         * disjoint intervals are served by one image. */
        [[nodiscard]] ImageAcquisition Acquire(TransientTextureID id, ResourceLifetime lifetime = {}) {
            const auto acquisition =
                acquireFrom(mImages, id.Get(), lifetime, [&] { return mContext.CreateImage(mRegistry.Descriptor(id)); }, mStats.ImagesCreatedThisFrame);

            return ImageAcquisition{ .Image = acquisition.Handle, .RequiresDiscard = acquisition.RequiresDiscard };
        }

        /** @brief Takes a buffer for a registered descriptor.
         * @param id The registered descriptor to take a buffer for.
         * @param lifetime The execution-order interval the buffer is needed over. */
        [[nodiscard]] BufferAcquisition Acquire(TransientBufferID id, ResourceLifetime lifetime = {}) {
            const auto acquisition =
                acquireFrom(mBuffers, id.Get(), lifetime, [&] { return mContext.CreateBuffer(mRegistry.Descriptor(id)); }, mStats.BuffersCreatedThisFrame);

            return BufferAcquisition{ .Buffer = acquisition.Handle, .RequiresDiscard = acquisition.RequiresDiscard };
        }

        void ResetFrame() {
            resetPool(mImages);
            resetPool(mBuffers);

            ++mFrameIndex;

            mStats.ImagesCreatedThisFrame = 0;
            mStats.BuffersCreatedThisFrame = 0;

            mStats.ImageCount = static_cast<u32>(mImages.Entries.size());
            mStats.BufferCount = static_cast<u32>(mBuffers.Entries.size());
        }

        void TrimUnused(u32 unusedFrameThreshold) {
            trimPool(mImages, unusedFrameThreshold);
            trimPool(mBuffers, unusedFrameThreshold);

            mStats.ImageCount = static_cast<u32>(mImages.Entries.size());
            mStats.BufferCount = static_cast<u32>(mBuffers.Entries.size());
        }

        [[nodiscard]] VE_INLINE const Stats &GetStats() const noexcept {
            return mStats;
        }

    private:
        static constexpr u32 END_OF_BUCKET = std::numeric_limits<u32>::max();

        template <typename TResource> struct Entry {
            TResource Handle{};
            u64 LastUsedFrame = 0;

            /** @brief The execution-order interval this entry was last handed out for. Only meaningful when
             * `LastUsedFrame == mFrameIndex`; an entry from an earlier frame carries no interval that this
             * frame's requests could conflict with. */
            ResourceLifetime Interval{};

            /** @brief Which registered descriptor this entry belongs to. Kept so trimming can rebuild the chains. */
            u32 RegistrationIndex = 0;

            /** @brief Next entry for the same registration, or `END_OF_BUCKET`. An intrusive chain rather than a
             * vector per bucket: one array, no per-bucket allocation, and the walk stays in the entry array that
             * the interval test is reading anyway. */
            u32 NextInBucket = END_OF_BUCKET;

            bool InUse = false;
        };

        /** @brief All entries of one resource kind. `BucketHeads` is indexed directly by registration index, so
         * finding the candidates for a descriptor is an array read - no hashing, no probing. */
        template <typename TResource> struct Pool {
            std::vector<Entry<TResource>> Entries;
            std::vector<u32> BucketHeads;
        };

        /** @brief One resource handed out, and whether whatever it holds belongs to someone else. */
        template <typename TResource> struct Acquisition final {
            TResource Handle{};
            bool RequiresDiscard = true;
        };

        template <typename TResource, typename CreateFn>
        Acquisition<TResource> acquireFrom(Pool<TResource> &pool, u32 registrationIndex, ResourceLifetime lifetime, CreateFn &&create, u32 &createdCounter) {
            VASSERT(lifetime.Valid(), "TransientPool::Acquire: lifetime.LastUse must be >= lifetime.FirstUse.");

            if (pool.BucketHeads.size() <= registrationIndex) {
                pool.BucketHeads.resize(static_cast<size_t>(registrationIndex) + 1, END_OF_BUCKET);
            }

            for (u32 index = pool.BucketHeads[registrationIndex]; index != END_OF_BUCKET; index = pool.Entries[index].NextInBucket) {
                Entry<TResource> &e = pool.Entries[index];

                const bool fromEarlierFrame = e.LastUsedFrame != mFrameIndex;
                const bool disjointThisFrame = lifetime.DisjointFrom(e.Interval);

                if (fromEarlierFrame || disjointThisFrame) {
                    e.InUse = true;
                    e.LastUsedFrame = mFrameIndex;
                    e.Interval = lifetime;

                    return Acquisition<TResource>{ .Handle = e.Handle, .RequiresDiscard = true };
                }
            }

            Entry<TResource> e{};
            e.Handle = create();
            e.InUse = true;
            e.LastUsedFrame = mFrameIndex;
            e.Interval = lifetime;
            e.RegistrationIndex = registrationIndex;
            e.NextInBucket = pool.BucketHeads[registrationIndex];

            pool.Entries.push_back(e);
            pool.BucketHeads[registrationIndex] = static_cast<u32>(pool.Entries.size() - 1);
            ++createdCounter;

            // Brand new: nothing has ever been in it, so there is nothing to discard.
            return Acquisition<TResource>{ .Handle = e.Handle, .RequiresDiscard = false };
        }

        template <typename TResource> void resetPool(Pool<TResource> &pool) {
            for (auto &e : pool.Entries) {
                e.InUse = false;
            }
        }

        template <typename TResource> void trimPool(Pool<TResource> &pool, u32 threshold) {
            if (pool.Entries.empty()) {
                return;
            }

            u32 write = 0;

            for (u32 read = 0; read < pool.Entries.size(); ++read) {
                const Entry<TResource> &e = pool.Entries[read];
                const bool stale = !e.InUse && mFrameIndex >= e.LastUsedFrame && ((mFrameIndex - e.LastUsedFrame) > threshold);

                if (stale) {
                    mDeletionQueue.Push(e.Handle);
                    continue;
                }

                if (write != read) {
                    pool.Entries[write] = e;
                }

                ++write;
            }

            if (write == pool.Entries.size()) {
                return;
            }

            pool.Entries.resize(write);

            // Compaction moved entries, so every chain is stale. Rebuilt back to front so each bucket ends up in
            // ascending index order, which keeps acquisition deterministic.
            std::ranges::fill(pool.BucketHeads, END_OF_BUCKET);

            for (u32 i = static_cast<u32>(pool.Entries.size()); i > 0; --i) {
                Entry<TResource> &e = pool.Entries[i - 1];

                e.NextInBucket = pool.BucketHeads[e.RegistrationIndex];
                pool.BucketHeads[e.RegistrationIndex] = i - 1;
            }
        }

        typename B::Context &mContext;
        TransientRegistry<B> &mRegistry;
        DeletionQueue<B> &mDeletionQueue;
        Pool<typename B::Image> mImages;
        Pool<typename B::Buffer> mBuffers;
        Stats mStats{};
        u64 mFrameIndex = 0;
    };

} // namespace Vulkyrie
