#pragma once

#include "memory/allocators/heap_allocator.h"
#include <volk.h>

namespace Vulkyrie {

    /** @brief Presents a `HeapAllocator` to the Vulkan driver as a `VkAllocationCallbacks`.
     *
     * This covers the CPU-side memory a driver takes for its own bookkeeping while servicing calls - not device
     * memory, which is the memory allocator's job. Handing these callbacks to every entry point puts that traffic
     * under one memory tag instead of leaving it invisible inside the driver.
     *
     * The class itself only translates between Vulkan's callback signatures and the allocator; every rule about
     * sizes, alignment and reallocation lives in `HeapAllocator`.
     *
     * Thread-safe, which the callbacks are required to be. */
    class VulkanHostAllocator final {
    public:
        /** @brief Builds the callback table.
         * @param tag Subsystem the driver's host allocations are attributed to. */
        explicit VulkanHostAllocator(MemoryTag tag = MemoryTag::Rendering);

        ~VulkanHostAllocator();

        // pUserData points at this object, so neither copying nor moving is meaningful.
        VE_DELETE_MOVE_AND_COPY(VulkanHostAllocator);

        /** @brief Returns the table to hand to `vkCreateInstance` and every other entry point taking one.
         * @returns A pointer valid for this object's lifetime, which must outlive every Vulkan object created with
         * it - the driver calls back on destruction too. */
        [[nodiscard]] VE_INLINE const VkAllocationCallbacks *Callbacks() const noexcept {
            return &mCallbacks;
        }

        /** @brief Returns the bytes the driver currently holds through these callbacks. */
        [[nodiscard]] VE_INLINE size_t LiveBytes() const noexcept {
            return mHeap.Used();
        }

        /** @brief Returns the number of outstanding allocations. Non-zero after teardown means the driver leaked,
         * or that an object was created with these callbacks and destroyed without them. */
        [[nodiscard]] VE_INLINE size_t LiveAllocations() const noexcept {
            return mHeap.AllocationCount();
        }

        /** @brief Returns the high-water mark of `LiveBytes()`. */
        [[nodiscard]] VE_INLINE size_t HighWaterMark() const noexcept {
            return mHeap.HighWaterMark();
        }

        /** @brief Returns the bytes the driver reports allocating through its own mechanism, outside these
         * callbacks. Reported for visibility only - nothing here owns or frees it. */
        [[nodiscard]] VE_INLINE size_t InternalBytes() const noexcept {
            return mInternalBytes.load(std::memory_order_relaxed);
        }

    private:
        static VKAPI_ATTR void *VKAPI_CALL allocate(void *userData, size_t size, size_t alignment, VkSystemAllocationScope scope);

        static VKAPI_ATTR void *VKAPI_CALL reallocate(void *userData, void *original, size_t size, size_t alignment, VkSystemAllocationScope scope);

        static VKAPI_ATTR void VKAPI_CALL release(void *userData, void *memory);

        static VKAPI_ATTR void VKAPI_CALL notifyInternalAllocation(void *userData, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope);

        static VKAPI_ATTR void VKAPI_CALL notifyInternalFree(void *userData, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope);

        /** @brief Where the driver's host memory actually comes from. */
        HeapAllocator mHeap;

        /** @brief The table handed out by `Callbacks()`. */
        VkAllocationCallbacks mCallbacks;

        /** @brief Bytes the driver allocated outside these callbacks. */
        std::atomic<size_t> mInternalBytes;
    };

} // namespace Vulkyrie
