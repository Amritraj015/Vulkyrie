#pragma once

// Unsigned int types.
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

// Signed int types.
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

// Floating point types
typedef float f32;
typedef double f64;

// Properly define static assertions.
#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

// Ensure all types are of the correct size.
STATIC_ASSERT(sizeof(u8) == 1, "Expected u8 to be 1 byte.");
STATIC_ASSERT(sizeof(u16) == 2, "Expected u16 to be 2 bytes.");
STATIC_ASSERT(sizeof(u32) == 4, "Expected u32 to be 4 bytes.");
STATIC_ASSERT(sizeof(u64) == 8, "Expected u64 to be 8 bytes.");

STATIC_ASSERT(sizeof(i8) == 1, "Expected i8 to be 1 byte.");
STATIC_ASSERT(sizeof(i16) == 2, "Expected i16 to be 2 bytes.");
STATIC_ASSERT(sizeof(i32) == 4, "Expected i32 to be 4 bytes.");
STATIC_ASSERT(sizeof(i64) == 8, "Expected i64 to be 8 bytes.");

STATIC_ASSERT(sizeof(f32) == 4, "Expected f32 to be 4 bytes.");
STATIC_ASSERT(sizeof(f64) == 8, "Expected f64 to be 8 bytes.");

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

#include <cstring>
#include <cstdarg>
#include <cstdint>

#include <filesystem>
#include <iostream>
#include <memory>
#include <memory.h>
#include <utility>
#include <algorithm>
#include <functional>

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
