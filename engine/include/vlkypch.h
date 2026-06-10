#pragma once

#include <numbers>
#include <cstdint>
#include <cstring>
#include <cstdarg>

#include <filesystem>
#include <iostream>
#include <memory>
#include <memory.h>
#include <utility>
#include <algorithm>
#include <functional>
#include <random>
#include <chrono>

#include <string>
#include <string_view>
#include <format>
#include <sstream>
#include <vector>
#include <ranges>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <typeindex>
#include <any>
#include <span>
#include <optional>

// GLM - OpenGL Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

#include "core/logger.h"
#include "debug/profiler.h"

// Unsigned int types.
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// Signed int types.
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

// Floating point types
typedef float f32;
typedef double f64;

// #if defined(VE_USE_DOUBLE_INSTEAD_OF_FLOAT)
// typedef f64 decimal;
// #else
// typedef f32 decimal;
// #endif
//
// typedef glm::qua<decimal, glm::defaultp> quat;
// typedef glm::vec<2, decimal, glm::defaultp> vec2;
// typedef glm::vec<3, decimal, glm::defaultp> vec3;
// typedef glm::vec<4, decimal, glm::defaultp> vec4;
// typedef glm::mat<2, 2, decimal, glm::defaultp> mat2;
// typedef glm::mat<3, 3, decimal, glm::defaultp> mat3;
// typedef glm::mat<4, 4, decimal, glm::defaultp> mat4;

// Platform detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64)

// Windows OS.
#define VE_PLATFORM_WINDOWS 1

#if defined(VULKYRIE_EXPORTS)
#define VE_API __declspec(dllexport)
#else
#define VE_API __declspec(dllimport)
#endif

#elif defined(__linux__) || defined(__gnu_linux__)

// Linux OS.
#define VE_PLATFORM_LINUX 1
#define VE_API __attribute__((visibility("default")))

#else

// Unsupported platform.
#error "Unknown platform!"
#define VE_API

#endif

#if defined(__GNUC__) || defined(__clang__)
#define VE_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define VE_INLINE __forceinline
#else
#define VE_INLINE inline
#endif

/** @brief A scoped pointer type alias using std::unique_ptr.
 * @tparam T The type of the object being pointed to.
 */
template <typename T> using Scope = std::unique_ptr<T>;
template <typename T, typename... Args> constexpr Scope<T> CreateScope(Args &&...args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

/** @brief A reference counted pointer type alias using std::shared_ptr.
 * @tparam T The type of the object being pointed to.
 */
template <typename T> using Ref = std::shared_ptr<T>;
template <typename T, typename... Args> constexpr Ref<T> CreateRef(Args &&...args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

/** @brief Combines the hash of a value into an existing hash seed.
 * @tparam T The type of the value being hashed.
 * @param seed The existing hash seed to combine with.
 * @param v The value to hash and combine with the seed.
 */
template <typename T> VE_INLINE void CombineHash(std::size_t &seed, const T &v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
