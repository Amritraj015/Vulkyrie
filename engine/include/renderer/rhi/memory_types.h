#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    enum class HeapUsage : u32 {
        None = 0,
        Buffers = BIT(0),
        Textures = BIT(1),      // non-render-target textures
        RenderTargets = BIT(2), // color/depth attachments, and MSAA targets
    };

    [[nodiscard]] constexpr HeapUsage operator|(HeapUsage a, HeapUsage b) noexcept {
        return static_cast<HeapUsage>(static_cast<u32>(a) | static_cast<u32>(b));
    }

    // struct HeapDescriptor {
    //     std::string_view DebugName;
    //     u64 SizeBytes = 0;
    //     MemoryDomain Domain = MemoryDomain::DeviceLocal;
    //     HeapUsage Usage = HeapUsage::RenderTargets;
    // };

    struct MemoryStats {
        u64 DeviceLocalUsed = 0;
        u64 DeviceLocalBudget = 0;
        u64 HostVisibleUsed = 0;
        u64 AllocationCount = 0;
        u64 BlockCount = 0;
    };

} // namespace Vulkyrie
