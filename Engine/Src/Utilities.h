#pragma once

#include "StatusCode.h"
#include "Core/Logger/Logger.h"
#include "Core/Logger/Asserts.h"

#define BIT(x) (1 << x)

#define VCLAMP(value, min, max) (value <= min) ? min : (value >= max) ? max \
                                                                      : value;

// NOTE: DO NOT PASS FUNCTIONS into this macro or else it "could" get executed multiple times.
#define ENSURE_SUCCESS(statusCode, message, ...) \
    if (statusCode != StatusCode::Successful)    \
    {                                            \
        VERROR(message, ##__VA_ARGS__)           \
        return statusCode;                       \
    }

// NOTE: DO NOT PASS FUNCTIONS into this macro or else it "could" get executed multiple times.
#define RETURN_ON_FAIL(statusCode) if (statusCode != StatusCode::Successful) return statusCode;                       \

// VULKAN ONLY - Checks the given expression's return value against VK_SUCCESS.
#define VK_CHECK(expr)               \
    {                                \
        VASSERT(expr == VK_SUCCESS)  \
    }

template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) noexcept
{
    return static_cast<typename std::underlying_type<E>::type>(e);
}