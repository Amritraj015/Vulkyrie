#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "renderer/backend_concepts.h"
#include "renderer/common/deletion_queue.h"

namespace Vulkyrie {

    template <RendererBackend B> class TransientPool final {
    public:
        struct Stats {
            // u64 ImageBytes = 0;  // estimated, see EstimateImageBytes
            // u64 BufferBytes = 0; // exact
            u32 ImageCount = 0;
            u32 BufferCount = 0;
            u32 ImagesCreatedThisFrame = 0;
            u32 BuffersCreatedThisFrame = 0;
        };

        struct ImageAcquisition final {
            typename B::Image Image{};
            bool RequiresDiscard = true;
        };

        struct LifeTime final {
            u32 FirstUse = 0;
            u32 LastUse = 0;

            [[nodiscard]] constexpr bool Valid() const noexcept {
                return LastUse >= FirstUse;
            }
        };

        explicit TransientPool(B::Context &context, DeletionQueue<B> &deletionQueue, size_t imageCount, size_t bufferCount) noexcept
            : mContext(context)
            , mDeletionQueue(deletionQueue) {

            // Reserve vectors so there are no steady state allocations.
            mImages.Entries.reserve(imageCount);
            mBuffers.Entries.reserve(bufferCount);

            // Buckets are keyed by descriptor hash; reserving the map avoids rehashing
            // when the first frame introduces every distinct descriptor at once.
            mImages.EntriesByHash.reserve(imageCount);
            mBuffers.EntriesByHash.reserve(bufferCount);
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

        [[nodiscard]] ImageAcquisition Acquire(const TextureDescriptor &descriptor, LifeTime lifetime = {}) {
            const u64 hash = HashDescriptor(descriptor);
            const auto handle = acquireFrom(mImages, hash, lifetime, [&] { return mContext.CreateImage(descriptor); }, mStats.ImagesCreatedThisFrame);

            return ImageAcquisition{ handle, true };
        }

        [[nodiscard]] typename B::Buffer Acquire(const BufferDescriptor &descriptor, LifeTime lifetime = {}) {
            const u64 hash = HashDescriptor(descriptor);
            return acquireFrom(mBuffers, hash, lifetime, [&] { return mContext.CreateBuffer(descriptor); }, mStats.BuffersCreatedThisFrame);
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
        template <typename TResource> struct Entry {
            TResource Handle{};
            u64 DescriptorHash = 0;
            // u64 SizeInBytes = 0;
            u64 LastUsedFrame = 0;

            /** @brief The execution-order interval this entry was last handed out for. Only meaningful when
             * `LastUsedFrame == mFrameIndex`; an entry from an earlier frame carries no interval that this
             * frame's requests could conflict with. */
            LifeTime Interval{};

            bool InUse = false;
        };

        /** @brief All entries of one resource kind, indexed by descriptor hash. An entry's slot in `Entries` is
         * assigned once and, unlike a classic free-list, is never removed from `EntriesByHash` on acquire - within
         * a frame the same slot can be handed out again for a later, disjoint interval, which is what lets several
         * same-descriptor transients with non-overlapping lifetimes share one underlying resource. */
        template <typename TResource> struct Pool {
            std::vector<Entry<TResource>> Entries;
            std::unordered_map<u64, std::vector<u32>> EntriesByHash;
        };

        B::Context &mContext;
        DeletionQueue<B> &mDeletionQueue;
        Pool<typename B::Image> mImages;
        Pool<typename B::Buffer> mBuffers;
        Stats mStats{};
        u64 mFrameIndex = 0;

        template <typename Resource, typename CreateFn>
        Resource acquireFrom(Pool<Resource> &pool, u64 descriptorHash, LifeTime lifetime, CreateFn &&create, u32 &createdCounter) {
            VASSERT(lifetime.Valid(), "TransientPool::Acquire: lifetime.LastUse must be >= lifetime.FirstUse.");

            auto &indices = pool.EntriesByHash[descriptorHash];

            for (const u32 index : indices) {
                Entry<Resource> &e = pool.Entries[index];

                const bool fromEarlierFrame = e.LastUsedFrame != mFrameIndex;
                const bool disjointThisFrame = lifetime.FirstUse > e.Interval.LastUse || lifetime.LastUse < e.Interval.FirstUse;

                if (fromEarlierFrame || disjointThisFrame) {
                    e.InUse = true;
                    e.LastUsedFrame = mFrameIndex;
                    e.Interval = lifetime;

                    return e.Handle;
                }
            }

            Entry<Resource> e{};
            e.DescriptorHash = descriptorHash;
            e.Handle = create();
            e.InUse = true;
            e.LastUsedFrame = mFrameIndex;
            e.Interval = lifetime;
            pool.Entries.push_back(e);
            indices.push_back(static_cast<u32>(pool.Entries.size() - 1));
            ++createdCounter;

            return e.Handle;
        }

        template <typename Resource> void resetPool(Pool<Resource> &pool) {
            for (auto &e : pool.Entries) {
                e.InUse = false;
            }
        }

        template <typename Resource> void trimPool(Pool<Resource> &pool, u32 threshold) {
            // If there are no resources to clean up, then return;
            if (pool.Entries.empty()) {
                return;
            }

            u32 write = 0;

            // Else loop over each resource and shrink the resource
            // storage vector by submitting stale resources to the deletion queue.
            for (u32 read = 0; read < pool.Entries.size(); ++read) {
                const Entry<Resource> &e = pool.Entries[read];
                const bool stale = !e.InUse && mFrameIndex >= e.LastUsedFrame && ((mFrameIndex - e.LastUsedFrame) > threshold);

                if (stale) {
                    mDeletionQueue.Push(e.Handle);
                    continue;
                }

                // Move resources to previous empty slots.
                if (write != read) {
                    pool.Entries[write] = e;
                }

                ++write;
            }

            // If nothing is stale, then we don't need to do anything, just return;
            if (write == pool.Entries.size()) {
                return;
            }

            // Re-size the resource vector storage.
            pool.Entries.resize(write);

            // Now, we will need to invalidate every index stored in `EntriesByHash`
            // and rebuild the indicies cache.
            for (auto &[hash, indices] : pool.EntriesByHash) {
                indices.clear();
            }

            for (u32 i = 0; i < pool.Entries.size(); ++i) {
                pool.EntriesByHash[pool.Entries[i].DescriptorHash].push_back(i);
            }
        }
    };

} // namespace Vulkyrie
