#pragma once

#include "defines.h"

// Disable assertions by commenting out the below line.
#define V_ASSERTIONS_ENABLED

#ifdef V_ASSERTIONS_ENABLED
#if _MSC_VER
#include <intrin.h>
#define debugBreak() __debugbreak()
#else
#define debugBreak() __builtin_trap()
#endif

V_API void report_assertion_failure(const char *expression, const char *message, const char *file, i32 line);

#define V_ASSERT(expr)                                               \
    {                                                                \
        if (expr)                                                    \
        {                                                            \
        }                                                            \
        else                                                         \
        {                                                            \
            report_assertion_failure(#expr, "", __FILE__, __LINE__); \
            debugBreak();                                            \
        }                                                            \
    }

#define V_ASSERT_MSG(expr, message)                                       \
    {                                                                     \
        if (expr)                                                         \
        {                                                                 \
        }                                                                 \
        else                                                              \
        {                                                                 \
            report_assertion_failure(#expr, message, __FILE__, __LINE__); \
            debugBreak();                                                 \
        }                                                                 \
    }

#ifdef _DEBUG
#define V_ASSERT_DEBUG(expr)                                         \
    {                                                                \
        if (expr)                                                    \
        {                                                            \
        }                                                            \
        else                                                         \
        {                                                            \
            report_assertion_failure(#expr, "", __FILE__, __LINE__); \
            debugBreak();                                            \
        }                                                            \
    }
#else
#define V_ASSERT_DEBUG(expr) // Does nothing at all
#endif

#else
#define V_ASSERT(expr)              // Does nothing at all
#define V_ASSERT_MSG(expr, message) // Does nothing at all
#define V_ASSERT_DEBUG(expr)        // Does nothing at all
#endif