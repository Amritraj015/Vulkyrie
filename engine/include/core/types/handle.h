#pragma once

#include "vlkypch.h"
#include "core/asserts.h"

namespace Vulkyrie {

    template <typename T> struct Handle {
    private:
        static constexpr u32 INVALID_INDEX = std::numeric_limits<u32>::max();

        u32 mValue = INVALID_INDEX;

    public:
        constexpr Handle() noexcept = default;

        constexpr explicit Handle(u32 value) noexcept
            : mValue(value) {
        }

        [[nodiscard]] VE_INLINE explicit constexpr operator u32() const noexcept {
            return mValue;
        }

        /** @brief Returns the raw value, for use as an array subscript. */
        [[nodiscard]] VE_INLINE constexpr u32 Get() const noexcept {
            return mValue;
        }

        /** @brief Checks whether the index refers to an element rather than being the invalid sentinel. */
        [[nodiscard]] VE_INLINE constexpr bool IsValid() const noexcept {
            return mValue != INVALID_INDEX;
        }

        friend constexpr auto operator<=>(Handle, Handle) = default;
    };

    template <typename T> struct GenerationalHandle {
    private:
        static constexpr u32 INDEX_BITS = 20;
        static constexpr u32 GENERATION_BITS = 12;
        static constexpr u32 INDEX_MASK = (1u << INDEX_BITS) - 1u;
        static constexpr u32 MAX_INDEX = INDEX_MASK;
        static constexpr u32 MAX_GENERATION = (1u << GENERATION_BITS) - 1u;
        static constexpr u32 INVALID_BITS = 0xFFFFFFFFu;

        u32 mBits = INVALID_BITS;

    public:
        constexpr GenerationalHandle() noexcept = default;

        constexpr explicit GenerationalHandle(u32 index, u32 generation) noexcept
            : mBits((generation << INDEX_BITS) | (index & INDEX_MASK)) {
            VASSERT(index <= MAX_INDEX, "index exceeds handle index capacity");
            VASSERT(generation <= MAX_GENERATION, "generation exceeds handle generation capacity");
        }

        [[nodiscard]] VE_INLINE constexpr u32 GetBits() const noexcept {
            return mBits;
        }

        [[nodiscard]] VE_INLINE constexpr u32 Index() const noexcept {
            return mBits & INDEX_MASK;
        }

        [[nodiscard]] VE_INLINE constexpr u32 Generation() const noexcept {
            return mBits >> INDEX_BITS;
        }

        [[nodiscard]] VE_INLINE constexpr bool IsValid() const noexcept {
            return mBits != INVALID_BITS;
        }

        friend constexpr auto operator<=>(GenerationalHandle, GenerationalHandle) = default;
    };

    template <typename T>
    concept Hashable = requires(const T &value) {
        { std::hash<T>{}(value) } -> std::convertible_to<std::size_t>;
    };

} // namespace Vulkyrie

namespace std {

    template <typename Tag> struct hash<Vulkyrie::Handle<Tag>> {
        [[nodiscard]] size_t operator()(const Vulkyrie::Handle<Tag> &h) const noexcept {
            return static_cast<size_t>(h.Get());
        }
    };

    template <typename Tag> struct hash<Vulkyrie::GenerationalHandle<Tag>> {
        [[nodiscard]] size_t operator()(const Vulkyrie::GenerationalHandle<Tag> &h) const noexcept {
            return static_cast<size_t>(h.GetBits());
        }
    };

} // namespace std

static_assert(Vulkyrie::Hashable<Vulkyrie::Handle<struct Tag>>);
static_assert(Vulkyrie::Hashable<Vulkyrie::GenerationalHandle<struct Tag>>);
static_assert(sizeof(Vulkyrie::Handle<struct Tag>) == sizeof(u32));
static_assert(sizeof(Vulkyrie::GenerationalHandle<struct Tag>) == sizeof(u32));
