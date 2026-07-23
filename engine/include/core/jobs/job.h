#pragma once

#include "vlkypch.h"

#include <atomic>
#include <new>

namespace Vulkyrie {

    /** @brief Assumed destructive-interference (cache line) size, used to keep concurrently-touched
     * job slots from false-sharing. A fixed constant rather than
     * `std::hardware_destructive_interference_size` so the ABI-affecting value cannot vary between
     * compiler versions. */
    inline constexpr std::size_t VE_CACHE_LINE_SIZE = 64;

    /** @brief Bytes of inline storage for a job's captured callable. A capture larger than this is a
     * compile error (never a silent heap allocation). */
    inline constexpr std::size_t JOB_PAYLOAD_CAPACITY = 64;

    /** @brief Sentinel index marking a `JobHandle` that refers to no job. */
    inline constexpr std::size_t INVALID_JOB_INDEX = ~std::size_t{ 0 };

    /** @brief Sentinel edge index terminating a job's successor list. */
    inline constexpr u32 INVALID_JOB_EDGE = ~u32{ 0 };

    /** @brief Sentinel head stored in `Job::FirstEdge` once the job has finished and its successor
     * list has been claimed by the finishing thread; a dependency added past this point is already
     * satisfied. */
    inline constexpr u32 JOB_EDGE_LIST_CLOSED = INVALID_JOB_EDGE - 1;

    /** @brief Packs a job's generation and its successor-list head into the single `u64` stored in
     * `Job::FirstEdge`. Tagging the head with the generation makes the edge-push CAS atomically
     * verify it is linking into the intended incarnation: without it, a predecessor that finishes
     * and is recycled between handle validation and the push would silently collect the edge on an
     * unrelated new job.
     * @param generation The slot incarnation the head belongs to.
     * @param head The first edge index, `INVALID_JOB_EDGE`, or `JOB_EDGE_LIST_CLOSED`.
     * @returns The packed value for `Job::FirstEdge`.
     */
    [[nodiscard]] VE_INLINE constexpr u64 PackEdgeHead(u32 generation, u32 head) {
        return (static_cast<u64>(generation) << 32U) | static_cast<u64>(head);
    }

    /** @brief Generational handle to a job slot. Mirrors `renderer_context.h`'s `Handle<T>`: the
     * index locates the slot, the generation distinguishes incarnations after the slot is recycled,
     * so a stale handle is always detected instead of aliasing a newer job. */
    struct JobHandle {
    public:
        std::size_t Index = INVALID_JOB_INDEX;
        u32 Generation = 0;

        /** @brief Returns true if this handle refers to a job slot (it may still be stale).
         * @returns True if the handle was produced by `JobSystem::Create`/`Run`.
         */
        [[nodiscard]] VE_INLINE constexpr bool IsValid() const {
            return Index != INVALID_JOB_INDEX;
        }

        [[nodiscard]] constexpr bool operator==(const JobHandle &) const = default;
    };

    /** @brief A single schedulable unit of work: a type-erased, allocation-free callable plus the
     * scheduling state (dependency count, successor list head, completion flag, generation) the job
     * system needs. Slots live in a fixed pool inside the job system and are recycled by generation.
     *
     * Layout note: the first cache line holds the scheduling state and dispatch pointers (the fields
     * other threads contend on); the second line is exactly the inline payload. `alignas` keeps
     * neighboring slots from false-sharing.
     */
    struct alignas(VE_CACHE_LINE_SIZE) Job {
    public:
        using InvokeFn = void (*)(void *);

        /** @brief Number of unmet prerequisites + 1 while unscheduled (`Create` seeds the count with
         * one so the job can never start before `Schedule` releases it). The job becomes ready when
         * this reaches zero. */
        std::atomic<u32> PendingDependencies{ 0 };

        /** @brief Generation-tagged head of the lock-free successor edge list (see `PackEdgeHead`);
         * the head is `JOB_EDGE_LIST_CLOSED` once the finishing thread has claimed the list (and
         * for free slots awaiting a claim). */
        std::atomic<u64> FirstEdge{ PackEdgeHead(0, JOB_EDGE_LIST_CLOSED) };

        /** @brief Incarnation counter, bumped on every slot claim; validates handles. */
        std::atomic<u32> Generation{ 0 };

        /** @brief True once the job has executed (and for free slots awaiting a claim). */
        std::atomic<bool> Finished{ true };

        /** @brief Memory tag captured from the submitting thread; execution runs under a
         * `MemoryScope` of this tag so worker-side allocations attribute correctly. */
        MemoryTag Tag = MemoryTag::Untagged;

        /** @brief Invokes the callable stored in `Payload`. */
        InvokeFn Invoke = nullptr;

        /** @brief Destroys the callable stored in `Payload`; null for trivially destructible captures. */
        InvokeFn Destroy = nullptr;

        /** @brief Inline storage for the captured callable. */
        alignas(VE_CACHE_LINE_SIZE) std::byte Payload[JOB_PAYLOAD_CAPACITY];

        /** @brief Move-constructs a callable into the inline payload and wires the dispatch pointers.
         * The caller must own the slot (freshly claimed, not yet scheduled).
         * @tparam TFunc The callable type; its decayed capture must fit `JOB_PAYLOAD_CAPACITY`.
         * @param fn The callable to store; invoked as `fn()` when the job executes.
         */
        template <typename TFunc> void Emplace(TFunc &&fn) {
            using TCallable = std::decay_t<TFunc>;

            static_assert(std::is_invocable_v<TCallable &>, "Job callables are invoked with no arguments: fn().");
            static_assert(sizeof(TCallable) <= JOB_PAYLOAD_CAPACITY,
                          "Job capture exceeds JOB_PAYLOAD_CAPACITY (64 bytes). Shrink the capture (capture pointers/indices, not values).");
            static_assert(alignof(TCallable) <= VE_CACHE_LINE_SIZE, "Job capture is over-aligned beyond the payload's cache-line alignment.");
            static_assert(std::is_nothrow_move_constructible_v<TCallable>, "Job captures must be nothrow-move-constructible.");

            std::construct_at(reinterpret_cast<TCallable *>(static_cast<void *>(Payload)), std::forward<TFunc>(fn));
            Invoke = &InvokeTrampoline<TCallable>;
            Destroy = std::is_trivially_destructible_v<TCallable> ? nullptr : &DestroyTrampoline<TCallable>;
        }

    private:
        template <typename TCallable> static void InvokeTrampoline(void *payload) {
            (*std::launder(reinterpret_cast<TCallable *>(payload)))();
        }

        template <typename TCallable> static void DestroyTrampoline(void *payload) {
            std::destroy_at(std::launder(reinterpret_cast<TCallable *>(payload)));
        }
    };

    static_assert(sizeof(Job) == 2 * VE_CACHE_LINE_SIZE, "Job is expected to occupy exactly two cache lines: state + payload.");

} // namespace Vulkyrie
