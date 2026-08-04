#include "memory/callstack.h"

#include <cstdio>
#include <cstdlib>

#if VE_MEMORY_CALLSTACKS
#if defined(VE_PLATFORM_WINDOWS) || defined(_WIN32)
#define VE_CALLSTACK_WINDOWS 1
// clang-format off
#include <windows.h>
#include <dbghelp.h>
// clang-format on
#pragma comment(lib, "dbghelp.lib")
#else
#define VE_CALLSTACK_EXECINFO 1
#include <execinfo.h>
#endif
#endif

namespace Vulkyrie {

    namespace {

#if VE_MEMORY_CALLSTACKS
        /** @brief Guards against a capture routine that allocates re-entering the tracker that called it.
         * `backtrace` touches the dynamic loader on first use and can allocate; without this the first captured
         * allocation would recurse indefinitely. */
        thread_local bool tCapturing = false;

        /** @brief FNV-1a over the frame addresses. Order-sensitive, so two different paths into the same leaf
         * function are distinct sites. */
        [[nodiscard]] std::uint64_t HashFrames(const void *const *frames, std::uint32_t count) {
            std::uint64_t hash = 1469598103934665603ULL;

            for (std::uint32_t i = 0; i < count; ++i) {
                auto address = reinterpret_cast<std::uintptr_t>(frames[i]);

                for (std::uint32_t byte = 0; byte < sizeof(address); ++byte) {
                    hash ^= static_cast<std::uint64_t>(address & 0xFFU);
                    hash *= 1099511628211ULL;
                    address >>= 8U;
                }
            }

            // Zero is reserved for "no callstack"; nudge the (astronomically unlikely) collision.
            return hash == 0 ? 1 : hash;
        }
#endif

    } // namespace

    Callstack CaptureCallstack([[maybe_unused]] std::uint32_t skipFrames) {
        Callstack callstack{};

#if VE_MEMORY_CALLSTACKS
        if (tCapturing) {
            return callstack;
        }

        tCapturing = true;

        // One extra frame for CaptureCallstack itself, which is never interesting.
        const std::uint32_t skip = skipFrames + 1;
        void *raw[VE_MEMORY_CALLSTACK_DEPTH + 32]{};
        const std::uint32_t maximum = skip + VE_MEMORY_CALLSTACK_DEPTH;
        const std::uint32_t capacity = maximum < std::size(raw) ? maximum : static_cast<std::uint32_t>(std::size(raw));

#if VE_CALLSTACK_WINDOWS
        const std::uint32_t captured = ::RtlCaptureStackBackTrace(0, capacity, raw, nullptr);
#else
        const auto captured = static_cast<std::uint32_t>(::backtrace(raw, static_cast<int>(capacity)));
#endif

        if (captured > skip) {
            const std::uint32_t usable = captured - skip;
            callstack.FrameCount = usable < VE_MEMORY_CALLSTACK_DEPTH ? usable : VE_MEMORY_CALLSTACK_DEPTH;

            for (std::uint32_t i = 0; i < callstack.FrameCount; ++i) {
                callstack.Frames[i] = raw[skip + i];
            }

            callstack.Hash = HashFrames(callstack.Frames.data(), callstack.FrameCount);
        }

        tCapturing = false;
#endif

        return callstack;
    }

    std::string FormatCallstack(const Callstack &callstack) {
        std::string result;

        if (0 == callstack.FrameCount) {
            return result;
        }

#if VE_MEMORY_CALLSTACKS && VE_CALLSTACK_EXECINFO
        // backtrace_symbols returns a single malloc'd block; it does not route through operator new, so it is safe
        // to call from a tracker context.
        char **symbols = ::backtrace_symbols(callstack.Frames.data(), static_cast<int>(callstack.FrameCount));

        if (nullptr != symbols) {
            for (std::uint32_t i = 0; i < callstack.FrameCount; ++i) {
                result += '\t';
                result += symbols[i];
                result += '\n';
            }

            std::free(symbols);

            return result;
        }
#endif

#if VE_MEMORY_CALLSTACKS && VE_CALLSTACK_WINDOWS
        const HANDLE process = ::GetCurrentProcess();
        alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME]{};
        auto *symbol = reinterpret_cast<SYMBOL_INFO *>(buffer);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        for (std::uint32_t i = 0; i < callstack.FrameCount; ++i) {
            result += '\t';

            if (::SymFromAddr(process, reinterpret_cast<DWORD64>(callstack.Frames[i]), nullptr, symbol) != FALSE) {
                result += symbol->Name;
            } else {
                char address[32]{};
                std::snprintf(address, sizeof(address), "0x%p", callstack.Frames[i]);
                result += address;
            }

            result += '\n';
        }

        return result;
#endif

        // No symbolizer available: raw addresses are still actionable through addr2line.
        for (std::uint32_t i = 0; i < callstack.FrameCount; ++i) {
            char address[32]{};
            std::snprintf(address, sizeof(address), "    %p\n", callstack.Frames[i]);
            result += address;
        }

        return result;
    }

} // namespace Vulkyrie
