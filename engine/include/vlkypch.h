#pragma once

#include <numbers>
#include <cstdint>
#include <cstring>
#include <cstdarg>

#include <filesystem>
#include <memory>
#include <utility>
#include <compare>
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
#include <span>
#include <optional>
#include <concepts>

// GLM - OpenGL Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

#include "core/logger.h"
#include "debug/profiler.h"

// Unsigned int types.
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

// Signed int types.
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

// Floating point types
using f32 = float;
using f64 = double;

using usize = std::size_t;

// Memory subsystem vocabulary: the thread-local scope stack, the VE_MEMORY_SCOPE macro, and the
// linker anchor that keeps the global operator new/delete override from being dropped. Included
// here (after the integer typedefs) so it is force-included into every engine translation unit.
// Kept self-contained so it introduces no circular include with this precompiled header.
#include "memory/memory_scope.h"

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

#if defined(VE_EXPORTS)
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

#define VE_DELETE_COPY(type)                                                                                                                                   \
    type(const type &) = delete;                                                                                                                               \
    type &operator=(const type &) = delete

#define VE_DELETE_MOVE(type)                                                                                                                                   \
    type(type &&) = delete;                                                                                                                                    \
    type &operator=(type &&) = delete

#define VE_DELETE_MOVE_AND_COPY(type)                                                                                                                          \
    VE_DELETE_COPY(type);                                                                                                                                      \
    VE_DELETE_MOVE(type)

#define BIT(x) 1u << x

#define VE_DEFAULT_MOVE(type)                                                                                                                                  \
    type(type &&) = default;                                                                                                                                   \
    type &operator=(type &&) = default

#define VE_DEFAULT_COPY(type)                                                                                                                                  \
    type(const type &) = default;                                                                                                                              \
    type &operator=(const type &) = default

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
