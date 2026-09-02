#pragma once

namespace Vulkyrie {

#define VE_VK_CHECK(expr, statusCode)                                                                                                                          \
    do {                                                                                                                                                       \
        const auto result = (expr);                                                                                                                            \
        if (VK_SUCCESS != result) {                                                                                                                            \
            VERROR("Vulkan Error Code: {}", std::to_underlying(result));                                                                                       \
            return statusCode;                                                                                                                                 \
        }                                                                                                                                                      \
    } while (false)

#define VE_VK_TRY_CREATE(expr)                                                                                                                                 \
    do {                                                                                                                                                       \
        const VkResult result = (expr);                                                                                                                        \
        if (VK_SUCCESS != result) {                                                                                                                            \
            return std::nullopt;                                                                                                                               \
        }                                                                                                                                                      \
    } while (false)

} // namespace Vulkyrie
