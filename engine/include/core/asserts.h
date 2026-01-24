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

#define VASSERT(condition, message, ...)                                                                                                                       \
    do {                                                                                                                                                       \
        if (!(condition)) {                                                                                                                                    \
            VERROR("[ASSERT] " message, ##__VA_ARGS__);                                                                                                        \
            VERROR("  File: {}", __FILE__);                                                                                                                    \
            VERROR("  Line: {}", __LINE__);                                                                                                                    \
            VERROR("  Function: {}", __func__);                                                                                                                \
            VDEBUGBREAK();                                                                                                                                     \
        }                                                                                                                                                      \
    } while (0)

#define VASSERT_EXPR(condition, message, ...)                                                                                                                  \
    do {                                                                                                                                                       \
        if (!(condition)) {                                                                                                                                    \
            VERROR("[ASSERT] (" #condition ") " message, ##__VA_ARGS__);                                                                                       \
            VERROR("  File: {}", __FILE__);                                                                                                                    \
            VERROR("  Line: {}", __LINE__);                                                                                                                    \
            VERROR("  Function: {}", __func__);                                                                                                                \
            VDEBUGBREAK();                                                                                                                                     \
        }                                                                                                                                                      \
    } while (0)

#else

// Release: optional logging or no-op
#define VASSERT(condition, message, ...)                                                                                                                       \
    do {                                                                                                                                                       \
    } while (0)
#define VASSERT_EXPR(condition, message, ...)                                                                                                                  \
    do {                                                                                                                                                       \
    } while (0)

#endif
