#pragma once

#include "Defines.h"

// Disable assertions by commenting out the below line.
#define V_ASSERTIONS_ENABLED

#ifdef V_ASSERTIONS_ENABLED
#if _MSC_VER
#include <intrin.h>
#define debugBreak() __debugbreak()
#else
#define debugBreak() __builtin_trap()
#endif

namespace Vkr
{
#define VASSERT(expr)                                                                                               \
    {                                                                                                               \
        if (expr)                                                                                                   \
        {                                                                                                           \
        }                                                                                                           \
        else                                                                                                        \
        {                                                                                                           \
            VFATAL("Assertion Failure: %s, message: '%s', in file: %s, line: %d\n", #expr, "", __FILE__, __LINE__); \
            debugBreak();                                                                                           \
        }                                                                                                           \
    }

#define VASSERT_MSG(expr, message)                                                                                       \
    {                                                                                                                    \
        if (expr)                                                                                                        \
        {                                                                                                                \
        }                                                                                                                \
        else                                                                                                             \
        {                                                                                                                \
            VFATAL("Assertion Failure: %s, message: '%s', in file: %s, line: %d\n", #expr, message, __FILE__, __LINE__); \
            debugBreak();                                                                                                \
        }                                                                                                                \
    }

#if defined(_DEBUG)
#define VASSERT_DEBUG(expr)                                                                                         \
    {                                                                                                               \
        if (expr)                                                                                                   \
        {                                                                                                           \
        }                                                                                                           \
        else                                                                                                        \
        {                                                                                                           \
            VFATAL("Assertion Failure: %s, message: '%s', in file: %s, line: %d\n", #expr, "", __FILE__, __LINE__); \
            debugBreak();                                                                                           \
        }                                                                                                           \
    }
#else
#define V_ASSERT_DEBUG(expr) // Does nothing at all
#endif

#else
#define V_ASSERT(expr)              // Does nothing at all
#define V_ASSERT_MSG(expr, message) // Does nothing at all
#define V_ASSERT_DEBUG(expr)        // Does nothing at all
#endif
}