#pragma once

// NOTE: Logging must never heap-allocate: the memory tracker's global operator new/delete override
// logs through VASSERT/VERROR, and the shutdown memory report must not mutate the counters it is
// printing. The standard provides no bounded `vformat_to_n`, so this header supplies a truncating
// output iterator plus a helper that formats directly into a caller-provided stack buffer.
#include <array>
#include <charconv>
#include <cstddef>
#include <format>
#include <string_view>

namespace Vulkyrie {

    /** @brief Output iterator that writes characters into a fixed-size buffer, silently discarding
     * anything past its capacity while still counting every character the formatter emitted. */
    class TruncatingBufferIterator {
    public:
        using difference_type = std::ptrdiff_t;

        /** @brief Write proxy returned by `operator*`; drops writes once the buffer is full. */
        struct Reference {
            char *Buffer;
            std::size_t Capacity;
            std::size_t Index;

            // Const-qualified and void-returning by design: `std::output_iterator` requires
            // assignment through a const rvalue of the proxy (same idiom as flat_map/zip proxies).
            void operator=(char character) const { // NOLINT(misc-unconventional-assign-operator)
                if (Index < Capacity) {
                    Buffer[Index] = character;
                }
            }
        };

        TruncatingBufferIterator() = default;

        /** @brief Creates an iterator writing into `buffer`, dropping writes beyond `capacity`.
         * @param buffer The destination character buffer.
         * @param capacity The maximum number of characters to store.
         */
        TruncatingBufferIterator(char *buffer, std::size_t capacity)
            : _buffer(buffer)
            , _capacity(capacity) {
        }

        [[nodiscard]] Reference operator*() const {
            return Reference{ _buffer, _capacity, _index };
        }

        TruncatingBufferIterator &operator++() {
            ++_index;
            return *this;
        }

        TruncatingBufferIterator operator++(int) {
            TruncatingBufferIterator copy = *this;
            ++_index;
            return copy;
        }

        /** @brief Returns the total characters the formatter emitted, including discarded ones. */
        [[nodiscard]] std::size_t Emitted() const {
            return _index;
        }

    private:
        char *_buffer = nullptr;
        std::size_t _capacity = 0;
        std::size_t _index = 0;
    };

    static_assert(std::output_iterator<TruncatingBufferIterator, const char &>);

    /** @brief Formats `fmt`/`args` directly into `buffer` without heap allocation, truncating on
     * overflow.
     * @param buffer The destination character buffer.
     * @param capacity The maximum number of characters to write.
     * @param fmt The std::format-style format string.
     * @param args The type-erased format arguments.
     * @returns The number of characters actually written (at most `capacity`).
     */
    [[nodiscard]] inline std::size_t FormatToBuffer(char *buffer, std::size_t capacity, std::string_view fmt, std::format_args args) {
        const TruncatingBufferIterator end = std::vformat_to(TruncatingBufferIterator(buffer, capacity), fmt, args);
        return end.Emitted() < capacity ? end.Emitted() : capacity;
    }

    /** @brief Writes `"<file> (<line>): "` into `buffer` without heap-allocating, truncating on overflow.
     *
     * Goes through `TruncatingBufferIterator` and `std::to_chars` rather than `std::format` so the call-site
     * prefix costs no more than the copy itself - the formatter is already being invoked once for the message,
     * and every log line would otherwise pay for a second pass over a two-argument format string.
     *
     * @param buffer The destination character buffer.
     * @param capacity The maximum number of characters to write.
     * @param fileName The file name to write, directory already stripped.
     * @param line The line number to write.
     * @returns The number of characters actually written (at most `capacity`).
     */
    [[nodiscard]] inline std::size_t FormatSiteToBuffer(char *buffer, std::size_t capacity, std::string_view fileName, int line) {
        TruncatingBufferIterator out(buffer, capacity);

        const auto write = [&out](std::string_view text) {
            for (const char character : text) {
                *out = character;
                ++out;
            }
        };

        write(fileName);
        write(" (");

        // Wide enough for any int; a line number cannot overflow it.
        std::array<char, 16> digits{};
        const std::to_chars_result result = std::to_chars(digits.data(), digits.data() + digits.size(), line);
        write(std::string_view{ digits.data(), static_cast<std::size_t>(result.ptr - digits.data()) });

        write("): ");

        return out.Emitted() < capacity ? out.Emitted() : capacity;
    }

} // namespace Vulkyrie
