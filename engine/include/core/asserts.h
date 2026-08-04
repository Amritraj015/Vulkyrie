#pragma once

#include "core/logger.h"

#if defined(_MSC_VER)
#define VDEBUGBREAK() __debugbreak()
#elif defined(__clang__) || defined(__GNUC__)
#define VDEBUGBREAK() __builtin_trap()
#else
#include <cstdlib>
#define VDEBUGBREAK() std::abort()
#endif

#ifdef VULKYRIE_DEBUG

// clang-format off
#define VASSERT(condition, message, ...)                                                                                                                       \
    do {                                                                                                                                                       \
        if (!(condition)) {                                                                                                                                    \
            VERROR("[ASSERT] " message __VA_OPT__(,) __VA_ARGS__);                                                                                                        \
            VERROR("  File: {}", __FILE__);                                                                                                                    \
            VERROR("  Line: {}", __LINE__);                                                                                                                    \
            VERROR("  Function: {}", __func__);                                                                                                                \
            VDEBUGBREAK();                                                                                                                                     \
        }                                                                                                                                                      \
    } while (false)

#define VASSERT_EXPR(condition, message, ...)                                                                                                                  \
    do {                                                                                                                                                       \
        if (!(condition)) {                                                                                                                                    \
            VERROR("[ASSERT] (" #condition ") " message __VA_OPT__(,) __VA_ARGS__);                                                                           \
            VERROR("  File: {}", __FILE__);                                                                                                                    \
            VERROR("  Line: {}", __LINE__);                                                                                                                    \
            VERROR("  Function: {}", __func__);                                                                                                                \
            VDEBUGBREAK();                                                                                                                                     \
        }                                                                                                                                                      \
    } while (false)

#else
// clang-format on

// Release: optional logging or no-op
#define VASSERT(condition, message, ...)                                                                                                                       \
    do {                                                                                                                                                       \
    } while (false)
#define VASSERT_EXPR(condition, message, ...)                                                                                                                  \
    do {                                                                                                                                                       \
    } while (false)

#endif
