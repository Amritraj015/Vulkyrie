#pragma once

namespace Vulkyrie {

#define VE_VK_CHECK(expr, errorMessage, statusCode)                                                                                                            \
    do {                                                                                                                                                       \
        if (VK_SUCCESS != (expr)) {                                                                                                                            \
            VERROR(errorMessage);                                                                                                                              \
            return statusCode;                                                                                                                                 \
        }                                                                                                                                                      \
    } while (false)

} // namespace Vulkyrie
