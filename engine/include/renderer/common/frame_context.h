#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "renderer/backend_concepts.h"

namespace Vulkyrie {

    /** @brief Everything scoped to one frame in flight: the command lists each worker records into, and the slot
     * that identifies which set of per-frame resources this frame owns.
     *
     * Command lists are held one per `(worker, queue type)` pair, because recording fans out across workers while
     * submission stays ordered - two workers must never touch the same list. The pair is flattened into a single
     * vector sized once at construction, so acquiring a list during recording is an index rather than an
     * allocation or a lock. */
    template <RendererBackend B> class FrameContext final {
    public:
        /** @brief Constructs the frame's per-worker recording storage.
         * @param context The backend context, retained for the command lists' eventual pool allocation.
         * @param slot Which frame-in-flight slot this context owns; `[0, B::kFramesInFlight)`.
         * @param workerCount How many threads may record concurrently. A backend that cannot record in parallel
         * (`B::kRecordsInParallel == false`) is clamped to one.
         * @param uploadBytes Size of this frame's upload ring. */
        FrameContext(typename B::Context &context, u32 slot, u32 workerCount, u64 uploadBytes)
            : mContext(context)
            , mSlot(slot)
            , mWorkerCount(B::kRecordsInParallel ? std::max(1U, workerCount) : 1U)
            , mUploadBytes(uploadBytes) {

            VASSERT(slot < B::kFramesInFlight, "FrameContext slot is outside the frames-in-flight range.");

            mLists.resize(static_cast<size_t>(mWorkerCount) * QUEUE_TYPE_COUNT);
        }

        VE_DELETE_MOVE_AND_COPY(FrameContext);

        ~FrameContext() = default;

        /** @brief Opens the frame, readying every command list for recording.
         * @param frameIndex The monotonic frame number this slot is being used for. */
        void BeginFrame(u32 frameIndex) {
            mFrameIndex = frameIndex;

            for (auto &list : mLists) {
                list.Begin();
            }
        }

        /** @brief Closes every command list the frame recorded into. Submission is the queue's business. */
        void EndFrame() {
            for (auto &list : mLists) {
                list.End();
            }
        }

        /** @brief Returns the command list a worker records into for one queue. Each `(worker, queue)` pair has its
         * own list, so concurrent recording needs no synchronization.
         * @param workerIndex The recording thread's index; must be below `GetWorkerCount()`.
         * @param queueType Which queue the work is destined for. */
        [[nodiscard]] VE_INLINE typename B::CommandList &AcquireCommandList(u32 workerIndex, QueueType queueType) {
            VASSERT(workerIndex < mWorkerCount, "FrameContext::AcquireCommandList: worker index is out of range.");
            VASSERT(queueType != QueueType::Count, "FrameContext::AcquireCommandList: QueueType::Count is not a queue.");

            return mLists[(static_cast<size_t>(workerIndex) * QUEUE_TYPE_COUNT) + static_cast<size_t>(queueType)];
        }

        /** @brief Returns how many threads may record into this frame concurrently. */
        [[nodiscard]] VE_INLINE u32 GetWorkerCount() const noexcept {
            return mWorkerCount;
        }

        /** @brief Returns this context's frame-in-flight slot. */
        [[nodiscard]] VE_INLINE u32 GetSlot() const noexcept {
            return mSlot;
        }

        /** @brief Returns the monotonic frame number set by the last `BeginFrame`. */
        [[nodiscard]] VE_INLINE u32 GetFrameIndex() const noexcept {
            return mFrameIndex;
        }

    private:
        static constexpr size_t QUEUE_TYPE_COUNT = static_cast<size_t>(QueueType::Count);

        typename B::Context &mContext;

        /** @brief One list per `(worker, queue type)`, flattened as `worker * QUEUE_TYPE_COUNT + queueType`. */
        std::vector<typename B::CommandList> mLists;

        u32 mSlot = 0;
        u32 mWorkerCount = 1;
        u32 mFrameIndex = 0;
        u64 mUploadBytes = 0;
    };

} // namespace Vulkyrie
