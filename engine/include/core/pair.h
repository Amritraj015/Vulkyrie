#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    /** @brief A simple generic Pair struct that holds two values of potentially different types. */
    template <typename T1, typename T2> struct Pair {
    public:
        /** @brief The first value of the pair. */
        T1 First;

        /** @brief The second value of the pair. */
        T2 Second;

        /** @brief Constructs a Pair with the given first and second values.
         * @param first The first value of the pair.
         * @param second The second value of the pair.
         */
        Pair(const T1 &first, const T2 &second)
            : First(first)
            , Second(second) {
        }

        bool operator==(const Pair<T1, T2> &other) const {
            return First == other.First && Second == other.Second;
        }

        bool operator!=(const Pair<T1, T2> &other) const {
            return !(*this == other);
        }
    };

} // namespace Vulkyrie

namespace std {

    template <typename T1, typename T2> struct hash<Vulkyrie::Pair<T1, T2>> {
    public:
        size_t operator()(const Vulkyrie::Pair<T1, T2> &pair) const noexcept {
            size_t seed = 0;
            CombineHash(seed, pair.First);
            CombineHash(seed, pair.Second);
            return seed;
        }
    };

} // namespace std
