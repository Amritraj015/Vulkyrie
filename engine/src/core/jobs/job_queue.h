#pragma once

// Private header (lives in src/, included as "core/jobs/job_queue.h" like console_log_sink.h): the
// work-stealing deque is an implementation detail of the job system, not public API.
#include "vlkypch.h"

#include "core/asserts.h"
#include "core/jobs/job.h"

#include <atomic>
#include <optional>

namespace Vulkyrie {

    /** @brief A fixed-capacity Chase-Lev work-stealing deque of packed job handles.
     *
     * One instance per participating thread. The owning thread pushes and pops at the bottom
     * (LIFO, cache-hot); thief threads steal from the top (FIFO) by CAS. The memory ordering follows
     * Lê/Pop/Cocchini/Nardelli, "Correct and Efficient Work-Stealing for Weak Memory Models"
     * (PPoPP'13): buffer slots are relaxed atomics (so racy top-reads are defined behavior), a
     * seq_cst fence orders the owner's bottom-decrement against thieves' top-reads in `TryPop`, and
     * `TrySteal` resolves races over the last element with a seq_cst CAS on `_top`.
     *
     * The capacity is fixed (power of two, mask indexing) — no growth on the hot path. The job
     * system sizes each queue to hold every job in the pool, so a full queue indicates a logic
     * error upstream; callers still handle `TryPush` failure by running the job inline.
     */
    class JobQueue final {
    public:
        /** @brief Constructs a deque with `capacity` slots (must be a power of two). */
        explicit JobQueue(std::size_t capacity)
            : _mask(capacity - 1)
            , _slots(new std::atomic<u64>[capacity]) {
            VASSERT(std::has_single_bit(capacity), "JobQueue capacity must be a power of two.");
        }

        VE_DELETE_MOVE_AND_COPY(JobQueue);

        ~JobQueue() = default;

        /** @brief Owner-only: pushes a packed handle at the bottom.
         * @param packedHandle The job to publish for execution.
         * @returns False if the deque is full (caller should run the job inline).
         */
        [[nodiscard]] bool TryPush(u64 packedHandle) {
            const i64 bottom = _bottom.load(std::memory_order_relaxed);
            const i64 top = _top.load(std::memory_order_acquire);

            if (bottom - top > static_cast<i64>(_mask)) {
                return false; // Full.
            }

            _slots[static_cast<std::size_t>(bottom) & _mask].store(packedHandle, std::memory_order_relaxed);

            // Release-publish the slot write together with the new bottom.
            _bottom.store(bottom + 1, std::memory_order_release);
            return true;
        }

        /** @brief Owner-only: pops the most recently pushed handle (LIFO).
         * @returns The packed handle, or `std::nullopt` if the deque was empty (or the single
         * remaining element was lost to a concurrent thief).
         */
        [[nodiscard]] std::optional<u64> TryPop() {
            const i64 bottom = _bottom.load(std::memory_order_relaxed) - 1;
            _bottom.store(bottom, std::memory_order_relaxed);

            // The owner's bottom-decrement must be globally visible before it reads top, or a thief
            // and the owner could both take the same (last) element.
            std::atomic_thread_fence(std::memory_order_seq_cst);

            i64 top = _top.load(std::memory_order_relaxed);

            if (top > bottom) {
                // Empty: restore bottom.
                _bottom.store(bottom + 1, std::memory_order_relaxed);
                return std::nullopt;
            }

            u64 packedHandle = _slots[static_cast<std::size_t>(bottom) & _mask].load(std::memory_order_relaxed);

            if (top == bottom) {
                // Last element: race against thieves for it via the same CAS they use.
                const bool won = _top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed);
                _bottom.store(bottom + 1, std::memory_order_relaxed);

                if (!won) {
                    return std::nullopt;
                }
            }

            return packedHandle;
        }

        /** @brief Thief-safe: steals the oldest handle (FIFO) from any thread.
         * @returns The packed handle, or `std::nullopt` if the deque looked empty or the steal
         * lost a race.
         */
        [[nodiscard]] std::optional<u64> TrySteal() {
            i64 top = _top.load(std::memory_order_acquire);

            // Order the top-read before the bottom-read (pairs with the fence in TryPop).
            std::atomic_thread_fence(std::memory_order_seq_cst);

            const i64 bottom = _bottom.load(std::memory_order_acquire);

            if (top >= bottom) {
                return std::nullopt; // Empty.
            }

            // Read the slot before claiming it; if the CAS wins, this value was the valid element.
            const u64 packedHandle = _slots[static_cast<std::size_t>(top) & _mask].load(std::memory_order_relaxed);

            if (!_top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
                return std::nullopt; // Lost the race to another thief or the owner.
            }

            return packedHandle;
        }

        /** @brief Approximate emptiness check for idle heuristics (never used for correctness). */
        [[nodiscard]] bool LooksEmpty() const {
            return _top.load(std::memory_order_acquire) >= _bottom.load(std::memory_order_acquire);
        }

    private:
        /** @brief Steal end; thieves and the owner CAS this to claim elements. */
        alignas(VE_CACHE_LINE_SIZE) std::atomic<i64> _top{ 0 };

        /** @brief Owner end; only the owning thread writes it. Separate cache line from `_top`. */
        alignas(VE_CACHE_LINE_SIZE) std::atomic<i64> _bottom{ 0 };

        /** @brief Power-of-two index mask (`capacity - 1`). */
        alignas(VE_CACHE_LINE_SIZE) std::size_t _mask;

        /** @brief The slot buffer; relaxed atomics so racy reads by late thieves are defined. */
        Scope<std::atomic<u64>[]> _slots;
    };

} // namespace Vulkyrie
