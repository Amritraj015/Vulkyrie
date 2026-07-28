#pragma once

#include "vlkypch.h"

#include "core/jobs/job_system.h"

namespace Vulkyrie {

    /** @brief Fixed upper bound on `ParallelFor` chunks. A machine-independent constant (never
     * derived from core count), so the partition — and therefore any chunk-keyed result — is
     * identical on every machine; worker count affects only scheduling, never results. */
    inline constexpr u32 MAX_PARALLEL_FOR_CHUNKS = 1024;

    /** @brief A half-open index range `[Begin, End)` covered by one chunk. */
    struct IndexRange {
    public:
        u32 Begin = 0;
        u32 End = 0;

        [[nodiscard]] constexpr bool operator==(const IndexRange &) const = default;
    };

    /** @brief The partition contract: how many chunks a parallel-for over `count` items splits
     * into. Pure — depends only on `(count, grainSize)` and `MAX_PARALLEL_FOR_CHUNKS`, never on
     * hardware, so a determinism hash matches across a 4-core and a 32-core machine.
     * @param count Number of items to iterate.
     * @param grainSize Target items per chunk (0 is treated as 1).
     * @returns `clamp(ceil(count / grainSize), 1, MAX_PARALLEL_FOR_CHUNKS)`, or 0 when `count` is 0.
     */
    [[nodiscard]] constexpr u32 ChunkCountFor(u32 count, u32 grainSize) {
        if (count == 0U) {
            return 0U;
        }

        const u64 grain = std::max<u64>(grainSize, 1U);
        const u64 chunks = (static_cast<u64>(count) + grain - 1U) / grain;
        return static_cast<u32>(std::min<u64>(chunks, MAX_PARALLEL_FOR_CHUNKS));
    }

    /** @brief The half-open range chunk `chunkIndex` covers. Pure; chunks tile `[0, count)` exactly
     * once with no gap or overlap: chunk c covers
     * `[c*base + min(c, remainder), (c+1)*base + min(c+1, remainder))` where `base = count / chunkCount`
     * and `remainder = count % chunkCount` (the first `remainder` chunks get one extra item).
     * @param count Number of items being iterated.
     * @param chunkCount The value returned by `ChunkCountFor` for this loop.
     * @param chunkIndex The chunk to query, in `[0, chunkCount)`.
     * @returns The chunk's `[Begin, End)` range.
     */
    [[nodiscard]] constexpr IndexRange ChunkRangeFor(u32 count, u32 chunkCount, u32 chunkIndex) {
        const u32 base = count / chunkCount;
        const u32 remainder = count % chunkCount;
        const u32 begin = chunkIndex * base + std::min(chunkIndex, remainder);
        const u32 end = begin + base + (chunkIndex < remainder ? 1U : 0U);
        return IndexRange{ begin, end };
    }

    /** @brief A `ParallelForRange` body: invocable as `fn(chunkIndex, begin, end)`, returning `void`. */
    template <typename TFunc>
    concept ParallelForRangeFunc = std::invocable<TFunc &, u32, u32, u32> && std::same_as<std::invoke_result_t<TFunc &, u32, u32, u32>, void>;

    /** @brief A `ParallelFor` body: invocable as `fn(index)`, returning `void`. */
    template <typename TFunc>
    concept ParallelForFunc = std::invocable<TFunc &, u32> && std::same_as<std::invoke_result_t<TFunc &, u32>, void>;

    /** @brief Runs `fn(chunkIndex, begin, end)` once per chunk across the job system and returns
     * when every chunk has completed (the calling thread executes chunks too). The partition is
     * machine-independent (see `ChunkCountFor`); with zero workers the chunks run inline in
     * ascending order. `fn` is invoked concurrently and must be safe to call from multiple threads.
     * @tparam TFunc Callable as `fn(u32 chunkIndex, u32 begin, u32 end)`.
     * @param count Number of items to iterate.
     * @param grainSize Target items per chunk.
     * @param fn The chunk body; keyed by `chunkIndex` for deterministic output (see `ChunkedOutput`).
     */
    template <ParallelForRangeFunc TFunc> void ParallelForRange(u32 count, u32 grainSize, TFunc &&fn) {
        const u32 chunkCount = ChunkCountFor(count, grainSize);
        if (chunkCount == 0U) {
            return;
        }

        if (chunkCount == 1U) {
            fn(0U, 0U, count);
            return;
        }

        // The chunk jobs capture the callable by reference: safe because this function does not
        // return until the join job (which depends on every chunk) completes.
        const JobHandle join = JobSystem::Create([] {});

        for (u32 chunk = 0; chunk < chunkCount; ++chunk) {
            const IndexRange range = ChunkRangeFor(count, chunkCount, chunk);
            const JobHandle handle = JobSystem::Create([&fn, chunk, range] { fn(chunk, range.Begin, range.End); });
            JobSystem::AddDependency(join, handle);
            JobSystem::Schedule(handle);
        }

        JobSystem::Schedule(join);
        JobSystem::Wait(join);
    }

    /** @brief Runs `fn(index)` for every index in `[0, count)` across the job system; blocks until
     * complete. Chunking and determinism guarantees are those of `ParallelForRange`.
     * @tparam TFunc Callable as `fn(u32 index)`.
     * @param count Number of items to iterate.
     * @param grainSize Target items per chunk.
     * @param fn The per-index body; invoked concurrently across chunks.
     */
    template <ParallelForFunc TFunc> void ParallelFor(u32 count, u32 grainSize, TFunc &&fn) {
        ParallelForRange(count, grainSize, [&fn](u32 /*chunkIndex*/, u32 begin, u32 end) {
            for (u32 index = begin; index < end; ++index) {
                fn(index);
            }
        });
    }

    /** @brief Per-chunk output buffers merged in ascending chunk order — the primitive for
     * deterministic parallel collection (e.g. broadphase pair lists). Results are keyed by chunk
     * index, never by worker index, so the merged output is byte-identical for any worker count.
     * Chunk slots are cache-line padded so concurrent `push_back`s on neighbouring chunks do not
     * false-share. Use `JobSystem::CurrentWorkerIndex()` only for scratch that never reaches a
     * result.
     * @tparam T The collected element type.
     */
    template <typename T> class ChunkedOutput {
    public:
        /** @brief Creates one empty buffer per chunk.
         * @param chunkCount The value returned by `ChunkCountFor` for the loop producing into this.
         */
        explicit ChunkedOutput(u32 chunkCount)
            : _chunks(chunkCount) {
        }

        /** @brief Returns the number of chunk buffers. */
        [[nodiscard]] VE_INLINE u32 ChunkCount() const {
            return static_cast<u32>(_chunks.size());
        }

        /** @brief Returns chunk `chunkIndex`'s buffer. Each chunk job may only touch its own chunk.
         * @param chunkIndex The chunk to write into, in `[0, ChunkCount())`.
         */
        [[nodiscard]] VE_INLINE std::vector<T> &Chunk(u32 chunkIndex) {
            return _chunks[chunkIndex].Items;
        }

        /** @brief Const access to chunk `chunkIndex`'s buffer.
         * @param chunkIndex The chunk to read, in `[0, ChunkCount())`.
         */
        [[nodiscard]] VE_INLINE const std::vector<T> &Chunk(u32 chunkIndex) const {
            return _chunks[chunkIndex].Items;
        }

        /** @brief Appends every chunk's elements to `out` in ascending chunk order (deterministic
         * for any worker count). Reserves once up front.
         * @param out The destination vector; existing contents are preserved.
         */
        void MergeInto(std::vector<T> &out) const {
            std::size_t total = out.size();
            for (const PaddedChunk &chunk : _chunks) {
                total += chunk.Items.size();
            }
            out.reserve(total);

            for (const PaddedChunk &chunk : _chunks) {
                out.insert(out.end(), chunk.Items.begin(), chunk.Items.end());
            }
        }

    private:
        /** @brief Cache-line padded so adjacent chunks' vector headers never false-share. */
        struct alignas(VE_CACHE_LINE_SIZE) PaddedChunk {
        public:
            std::vector<T> Items;
        };

        std::vector<PaddedChunk> _chunks;
    };

} // namespace Vulkyrie
