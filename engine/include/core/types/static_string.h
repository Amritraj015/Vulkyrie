#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    /** @brief A string reference the compiler has proven points at static storage, so it is safe to store without
     * copying. Accepts string literals; `std::string`, `const char *` and anything built at runtime are rejected.
     * There is no escape hatch by design - a name that varies comes from a table of literals. */
    class StaticString final {
    public:
        constexpr StaticString() noexcept = default;

        /** @brief Adopts a string literal.
         *
         * `consteval` rather than `constexpr` is what enforces the guarantee: it makes the call itself have to be a
         * constant expression, so an array whose address is not known until runtime cannot bind. Relaxing it to
         * `constexpr` silently admits automatic buffers and destroys the type's entire purpose.
         *
         * @tparam N Array extent, deduced; the trailing NUL is not counted in the size.
         * @param literal The character array to reference. */
        template <size_t N>
        consteval StaticString(const char (&literal)[N]) noexcept // NOLINT(google-explicit-constructor)
            : _data{ literal }
            , _size{ N - 1 } {
        }

        /** @brief Returns a pointer to the characters. Not guaranteed to be NUL-terminated. */
        [[nodiscard]] VE_INLINE constexpr const char *Data() const noexcept {
            return _data;
        }

        /** @brief Returns the number of characters, excluding any terminator. */
        [[nodiscard]] VE_INLINE constexpr size_t Size() const noexcept {
            return _size;
        }

        /** @brief Returns whether the string has no characters. */
        [[nodiscard]] VE_INLINE constexpr bool IsEmpty() const noexcept {
            return _size == 0;
        }

        /** @brief Returns a view of the characters, for formatting and comparison. */
        [[nodiscard]] VE_INLINE constexpr std::string_view View() const noexcept {
            return std::string_view{ _data, _size };
        }

        /** @brief Converts to `std::string_view` implicitly. */
        [[nodiscard]] VE_INLINE constexpr operator std::string_view() const noexcept { // NOLINT(google-explicit-constructor)
            return View();
        }

        /** @brief Compares by content. The only comparison overload: a second one taking `StaticString` would make
         * `name == "literal"` ambiguous. */
        friend constexpr bool operator==(const StaticString &lhs, std::string_view rhs) noexcept {
            return lhs.View() == rhs;
        }

    private:
        const char *_data = "";
        size_t _size = 0;
    };

    static_assert(std::is_trivially_copyable_v<StaticString>, "StaticString must be trivially copyable so nodes stay cheap to move.");

} // namespace Vulkyrie
