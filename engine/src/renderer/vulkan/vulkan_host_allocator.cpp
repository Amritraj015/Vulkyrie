#include "renderer/vulkan/vulkan_host_allocator.h"

#include "memory/memory_tracker.h"

namespace Vulkyrie {

    VulkanHostAllocator::VulkanHostAllocator(MemoryTag tag)
        : mHeap{ tag }
        , mCallbacks{ .pUserData = this,
                      .pfnAllocation = &VulkanHostAllocator::allocate,
                      .pfnReallocation = &VulkanHostAllocator::reallocate,
                      .pfnFree = &VulkanHostAllocator::release,
                      .pfnInternalAllocation = &VulkanHostAllocator::notifyInternalAllocation,
                      .pfnInternalFree = &VulkanHostAllocator::notifyInternalFree }
        , mInternalBytes{ 0 } {
    }

    VulkanHostAllocator::~VulkanHostAllocator() {
        const size_t outstanding = mHeap.AllocationCount();

        if (outstanding > 0) {
            VWARN("Vulkan host allocator destroyed with {} allocation(s), {} bytes, still outstanding.", outstanding, mHeap.Used());
        }
    }

    void *VulkanHostAllocator::allocate(void *userData, size_t size, size_t alignment, VkSystemAllocationScope scope) {
        // Scope is the driver saying how long it expects to keep the block. Nothing records it: the tracker's tags
        // are per-subsystem, not per-lifetime.
        (void)scope;

        return static_cast<VulkanHostAllocator *>(userData)->mHeap.Allocate(size, alignment);
    }

    void *VulkanHostAllocator::reallocate(void *userData, void *original, size_t size, size_t alignment, VkSystemAllocationScope scope) {
        (void)scope;

        return static_cast<VulkanHostAllocator *>(userData)->mHeap.Reallocate(original, size, alignment);
    }

    void VulkanHostAllocator::release(void *userData, void *memory) {
        static_cast<VulkanHostAllocator *>(userData)->mHeap.Free(memory);
    }

    void VulkanHostAllocator::notifyInternalAllocation(void *userData, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope) {
        // Notification only: the driver took this itself and will return it itself.
        (void)type;
        (void)scope;

        auto *self = static_cast<VulkanHostAllocator *>(userData);
        self->mInternalBytes.fetch_add(size, std::memory_order_relaxed);

        MemoryTracker::OnAllocation(self->mHeap.Tag(), static_cast<i64>(size));
    }

    void VulkanHostAllocator::notifyInternalFree(void *userData, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope) {
        (void)type;
        (void)scope;

        auto *self = static_cast<VulkanHostAllocator *>(userData);
        self->mInternalBytes.fetch_sub(size, std::memory_order_relaxed);

        MemoryTracker::OnFree(self->mHeap.Tag(), static_cast<i64>(size));
    }

} // namespace Vulkyrie
