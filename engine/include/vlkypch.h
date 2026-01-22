#pragma once

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

#include <string>
#include <string_view>
#include <format>
#include <sstream>
#include <vector>
#include <ranges>
#include <array>
#include <unordered_map>
#include <unordered_set>

// GLM - OpenGL Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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

// Platform detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)

// Windows OS.
#define PLATFORM_WINDOWS 1

#if defined(VULKYRIE_EXPORTS)
#define VULKYRIE_API __declspec(dllexport)
#else
#define VULKYRIE_API __declspec(dllimport)
#endif

#elif defined(__linux__) || defined(__gnu_linux__)

// Linux OS.
#define PLATFORM_LINUX 1
#define VULKYRIE_API

#else

// Unsupported platform.
#error "Unknown platform!"
#define VULKYRIE_API

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
