#pragma once

#include "vlkypch.h"
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
            mImages.FreeByHash.reserve(imageCount);
            mBuffers.FreeByHash.reserve(bufferCount);
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
            // TODO: Finish this.
            (void)lifetime;

            const u64 hash = HashDescriptor(descriptor);
            const auto handle = acquireFrom(mImages, hash, [&] { return mContext.CreateImage(descriptor); }, mStats.ImagesCreatedThisFrame);

            return ImageAcquisition{ handle, true };
        }

        [[nodiscard]] typename B::Buffer Acquire(const BufferDescriptor &descriptor, LifeTime lifetime = {}) {
            // TODO: Finish this.
            (void)lifetime;

            const u64 hash = HashDescriptor(descriptor);
            return acquireFrom(mBuffers, hash, [&] { return mContext.CreateBuffer(descriptor); }, mStats.BuffersCreatedThisFrame);
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
            bool InUse = false;
        };

        template <typename TResource> struct Pool {
            std::vector<Entry<TResource>> Entries;
            std::unordered_map<u64, std::vector<u32>> FreeByHash;
        };

        B::Context &mContext;
        DeletionQueue<B> &mDeletionQueue;
        Pool<typename B::Image> mImages;
        Pool<typename B::Buffer> mBuffers;
        Stats mStats{};
        u64 mFrameIndex = 0;

        template <typename Resource, typename CreateFn> Resource acquireFrom(Pool<Resource> &pool, u64 descriptorHash, CreateFn &&create, u32 &createdCounter) {
            if (auto it = pool.FreeByHash.find(descriptorHash); it != pool.FreeByHash.end() && !it->second.empty()) {
                const auto index = it->second.back();
                it->second.pop_back();

                auto &e = pool.Entries[index];
                e.InUse = true;
                e.LastUsedFrame = mFrameIndex;

                return e.Handle;
            }

            Entry<Resource> e{};
            e.DescriptorHash = descriptorHash;
            e.Handle = create();
            e.InUse = true;
            e.LastUsedFrame = mFrameIndex;
            pool.Entries.push_back(e);
            ++createdCounter;

            return e.Handle;
        }

        template <typename Resource> void resetPool(Pool<Resource> &pool) {
            for (auto &[hash, indices] : pool.FreeByHash) {
                indices.clear();
            }

            for (u32 i = 0; i < static_cast<u32>(pool.Entries.size()); ++i) {
                pool.Entries[i].InUse = false;
                pool.FreeByHash[pool.Entries[i].DescriptorHash].push_back(i);
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

            // Now, we will need to invalidate every index stored in `FreeByHash`
            // and rebuild the indicies cache.
            for (auto &[hash, indices] : pool.FreeByHash) {
                indices.clear();
            }

            for (u32 i = 0; i < pool.Entries.size(); ++i) {
                if (!pool.Entries[i].InUse) {
                    pool.FreeByHash[pool.Entries[i].DescriptorHash].push_back(i);
                }
            }
        }
    };

} // namespace Vulkyrie
