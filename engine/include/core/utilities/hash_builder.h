#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    class HashBuilder {
    public:
        using Hash = u64;

        static constexpr Hash kSeed = 0x9e3779b97f4a7c15ull;

        constexpr HashBuilder() noexcept = default;

        constexpr explicit HashBuilder(Hash seed) noexcept
            : mHash(seed) {
        }

        constexpr HashBuilder &Word(u64 x) noexcept {
            x *= kM;
            x ^= x >> 47;
            x *= kM;
            mHash ^= x;
            mHash *= kM;
            mHash += kAdd;
            return *this;
        }

        template <typename T>
        constexpr HashBuilder &Value(T value) noexcept
            requires std::is_integral_v<T>
        {
            if constexpr (std::same_as<T, bool>) {
                return Word(value ? 1u : 0u);
            } else {
                using U = std::make_unsigned_t<T>;
                return Word(static_cast<u64>(static_cast<U>(value)));
            }
        }

        template <typename T>
        constexpr HashBuilder &Value(T value) noexcept
            requires std::is_enum_v<T>
        {
            using U = std::make_unsigned_t<std::underlying_type_t<T>>;
            return Word(static_cast<u64>(static_cast<U>(value)));
        }

        constexpr HashBuilder &Value(float value) noexcept {
            return Word(std::bit_cast<u32>(value));
        }

        constexpr HashBuilder &Value(double value) noexcept {
            return Word(std::bit_cast<u64>(value));
        }

        constexpr HashBuilder &Bytes(std::span<const std::byte> data) noexcept {
            const usize size = data.size();
            usize i = 0;
            for (; i + sizeof(u64) <= size; i += sizeof(u64)) {
                u64 word = 0;
                for (usize j = 0; j < sizeof(u64); ++j) word |= static_cast<u64>(std::to_integer<u8>(data[i + j])) << (j * 8);
                Word(word);
            }
            u64 tail = 0;
            for (usize j = 0; i + j < size; ++j) tail |= static_cast<u64>(std::to_integer<u8>(data[i + j])) << (j * 8);
            Word(tail);
            Word(size);
            return *this;
        }

        // NOT constexpr: casting from const void* is ill-formed during constant
        // evaluation, so marking it so would be a promise the compiler rejects.
        HashBuilder &Bytes(const void *data, usize size) noexcept {
            return Bytes(std::span<const std::byte>{ static_cast<const std::byte *>(data), size });
        }

        // Own loop rather than delegating to the void* overload -- that delegation
        // is what makes String() unusable in a constant expression. Semantics are
        // identical to Bytes(span): 8 bytes/round, tail word, then length.
        constexpr HashBuilder &String(std::string_view value) noexcept {
            const usize size = value.size();
            usize i = 0;
            for (; i + sizeof(u64) <= size; i += sizeof(u64)) {
                u64 word = 0;
                for (usize j = 0; j < sizeof(u64); ++j) word |= static_cast<u64>(static_cast<u8>(value[i + j])) << (j * 8);
                Word(word);
            }
            u64 tail = 0;
            for (usize j = 0; i + j < size; ++j) tail |= static_cast<u64>(static_cast<u8>(value[i + j])) << (j * 8);
            Word(tail);
            Word(size);
            return *this;
        }

        template <std::ranges::input_range R> constexpr HashBuilder &Values(R &&range) noexcept {
            u64 count = 0;
            for (const auto &value : range) {
                Value(value);
                ++count;
            }
            return Word(count);
        }

        [[nodiscard]] constexpr Hash Finish() const noexcept {
            u64 x = mHash;
            x ^= x >> 33;
            x *= 0xff51afd7ed558ccdull;
            x ^= x >> 33;
            x *= 0xc4ceb9fe1a85ec53ull;
            x ^= x >> 33;
            return x;
        }

        [[nodiscard]] constexpr Hash State() const noexcept {
            return mHash;
        }

    private:
        static constexpr u64 kM = 0xc6a4a7935bd1e995ull;
        static constexpr u64 kAdd = 0x9e3779b97f4a7c15ull;

        Hash mHash = kSeed;
    };

} // namespace Vulkyrie
