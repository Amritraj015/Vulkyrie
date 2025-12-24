// // WARNING: DO NOT PASS FUNCTIONS into this macro or else it "could" get executed multiple times.
// #define RETURN_ON_FAIL(statusCode)                                                                                                         \
//     if (StatusCode::Successful != statusCode) return statusCode;

// // WARNING: DO NOT PASS FUNCTIONS into this macro or else it "could" get executed multiple times.
// #define ENSURE_SUCCESS(statusCode, message, ...)                                                                                           \
//     if (StatusCode::Successful != statusCode) {                                                                                            \
//         LFATAL(message, ##__VA_ARGS__)                                                                                                     \
//         return statusCode;                                                                                                                 \
//     }

#define BIT(x) (1 << x)