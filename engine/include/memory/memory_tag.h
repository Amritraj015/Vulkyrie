#pragma once

// NOTE: This header is force-included via `vlkypch.h` (through `memory_scope.h`), so it must stay
// self-contained and must NOT include `vlkypch.h` — doing so would create a circular include.
#include <array>
#include <cstdint>
#include <string_view>

namespace Vulkyrie {

#define VE_MEMORY_TAGS(X)                                                                                                                                      \
    X(Physics)                                                                                                                                                 \
    X(Rendering)                                                                                                                                               \
    X(Audio)                                                                                                                                                   \
    X(Core)                                                                                                                                                    \
    X(Input)                                                                                                                                                   \
    X(Events)                                                                                                                                                  \
    X(Networking)                                                                                                                                              \
    X(Materials)                                                                                                                                               \
    X(Platform)                                                                                                                                                \
    X(Assets)                                                                                                                                                  \
    X(Editor)                                                                                                                                                  \
    X(GpuVram)                                                                                                                                                 \
    X(ThirdParty)                                                                                                                                              \
    X(Untagged)

    /** @brief Identifies the engine subsystem an allocation is attributed to. Values are contiguous
     * from 0 so the tag can index directly into the tracker's per-subsystem counter array. */
    enum class MemoryTag : std::uint32_t {
#define X(name) name,
        VE_MEMORY_TAGS(X)
#undef X
    };

    /** @brief The number of distinct `MemoryTag` values. */
    inline constexpr std::uint32_t MemoryTagCount = []() constexpr {
        std::uint32_t count = 0;
#define X(name) ++count;
        VE_MEMORY_TAGS(X)
#undef X
        return count;
    }();

    /** @brief Human-readable names for each `MemoryTag`, indexed by the enum's underlying value. */
    inline constexpr std::array<std::string_view, MemoryTagCount> MemoryTagNames{
#define X(name) #name,
        VE_MEMORY_TAGS(X)
#undef X
    };

    /** @brief Returns the human-readable name of a memory tag.
     * @param tag The memory tag to name.
     * @returns The tag's name (e.g. "Physics"), or "Invalid" if the tag is out of range.
     */
    [[nodiscard]] constexpr std::string_view MemoryTagName(MemoryTag tag) {
        const auto index = static_cast<std::size_t>(tag);
        return index < MemoryTagNames.size() ? MemoryTagNames[index] : std::string_view{ "Invalid" };
    }

} // namespace Vulkyrie
