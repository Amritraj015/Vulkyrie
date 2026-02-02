#pragma once

#include "core/logger.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <thread>
#include <mutex>
#include <sstream>

namespace Vulkyrie::Debug {

    struct ProfileResult {
        public:
            const std::string_view Name;
            const std::chrono::time_point<std::chrono::steady_clock> Start;
            const double ElapsedTime;
            const std::thread::id ThreadID;
    };

    class Profiler {
        public:
            Profiler(const Profiler &) = delete;
            Profiler(Profiler &&) = delete;

            void BeginSession(std::string_view name, std::string_view filepath = "results.json") {
                std::lock_guard lock(_mutex);

                if (!_sessionName.empty()) {
                    // If there is already a current session, then close it before beginning new one.
                    // Subsequent profiling output meant for the original session will end up in the
                    // newly opened session instead.  That's better than having badly formatted
                    // profiling output.
                    VERROR("Profiler::BeginSession('{}') when session '{}' already open.", name, _sessionName)

                    InternalEndSession();
                }

                _fileStream.open(filepath.data());

                if (_fileStream.is_open()) {
                    _sessionName = name;
                    WriteHeader();
                } else {
                    VERROR("Profiler could not open results file '{}'.", filepath)
                }
            }

            void EndSession() {
                std::lock_guard lock(_mutex);
                InternalEndSession();
            }

            void WriteProfile(const ProfileResult &result) {
                std::stringstream json;

                json << std::setprecision(3) << std::fixed;
                json << ",{";
                json << "\"cat\":\"function\",";
                json << "\"dur\":" << (result.ElapsedTime) << ',';
                json << "\"name\":\"" << result.Name << "\",";
                json << "\"ph\":\"X\",";
                json << "\"pid\":0,";
                json << "\"tid\":" << result.ThreadID << ",";
                json << "\"ts\":" << result.Start.time_since_epoch().count();
                json << "}";

                std::lock_guard lock(_mutex);

                if (!_sessionName.empty()) {
                    _fileStream << json.str();
                    _fileStream.flush();
                }
            }

            static Profiler &GetSingleton() {
                static Profiler instance;
                return instance;
            }

        private:
            Profiler() {
            }

            ~Profiler() {
                EndSession();
            }

            void WriteHeader() {
                _fileStream << "{\"otherData\": { \"version\": \"1.0\", \"app\": \"Vulkyrie Game Engine\" },\"traceEvents\":[{}";
                _fileStream.flush();
            }

            void WriteFooter() {
                _fileStream << "]}";
                _fileStream.flush();
            }

            // Note: you must already own lock on _mutex before
            // calling InternalEndSession()
            void InternalEndSession() {
                if (!_sessionName.empty()) {
                    WriteFooter();
                    _fileStream.close();
                }
            }

        private:
            std::mutex _mutex;
            std::string_view _sessionName;
            std::ofstream _fileStream;
    };

    /** @brief A simple timer class for measuring elapsed time. */
    class Timer {
        public:
            /** @brief Constructs a Timer object and starts timing.
             * @param name An optional name for the timer.
             */
            explicit Timer(std::string_view name = "")
                : _name(name)
                , _stopped(false)
                , _startTime(std::chrono::steady_clock::now()) {
            }

            Timer(const Timer &) = delete;
            Timer &operator=(const Timer &) = delete;

            Timer(Timer &&) = default;
            Timer &operator=(Timer &&) = default;

            /** @brief Destructor that stops the timer and logs the elapsed time if not already stopped. */
            ~Timer() noexcept {
                if (!_stopped) {
                    Stop();
                }
            }

            /** @brief Stops the timer and logs the elapsed time. */
            void Stop() noexcept {
                const auto endTime = std::chrono::steady_clock::now();
                const auto duration = endTime - _startTime;
                const auto elapsedTime = std::chrono::duration<double, std::micro>(duration).count();

                // if (_name.empty()) {
                //     VINFO("Timer took {} ms.", elapsedTime);
                // } else {
                //     VINFO("Timer '{}' took {} ms.", _name, elapsedTime);
                // }

                Profiler::GetSingleton().WriteProfile({ _name, _startTime, elapsedTime, std::this_thread::get_id() });

                _stopped = true;
            }

        private:
            /** @brief The name of the timer. */
            std::string_view _name;

            /** @brief Indicates whether the timer has been stopped. */
            bool _stopped;

            /** @brief The start time point of the timer. */
            std::chrono::time_point<std::chrono::steady_clock> _startTime;
    };

} // namespace Vulkyrie::Debug

#define VLKY_PROFILE 1
#if VLKY_PROFILE
  // Resolve which function signature macro will be used. Note that this only
// is resolved when the (pre)compiler starts, so the syntax highlighting
// could mark the wrong one in your editor!
#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
#define VLKY_FUNC_SIG __PRETTY_FUNCTION__
#elif defined(__DMC__) && (__DMC__ >= 0x810)
#define VLKY_FUNC_SIG __PRETTY_FUNCTION__
#elif (defined(__FUNCSIG__) || (_MSC_VER))
#define VLKY_FUNC_SIG __FUNCSIG__
#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
#define VLKY_FUNC_SIG __FUNCTION__
#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
#define VLKY_FUNC_SIG __FUNC__
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
#define VLKY_FUNC_SIG __func__
#elif defined(__cplusplus) && (__cplusplus >= 201103)
#define VLKY_FUNC_SIG __func__
#else
#define VLKY_FUNC_SIG "VLKY_FUNC_SIG unknown!"
#endif

#define VLKY_PROFILE_BEGIN_SESSION(name, filepath) ::Vulkyrie::Debug::Profiler::GetSingleton().BeginSession(name, filepath)
#define VLKY_PROFILE_END_SESSION() ::Vulkyrie::Debug::Profiler::GetSingleton().EndSession()
#define VLKY_PROFILE_SCOPE_LINE(name, line) ::Vulkyrie::Debug::Timer timer##line(name)
#define VLKY_PROFILE_SCOPE(name) VLKY_PROFILE_SCOPE_LINE(name, __LINE__)
#define VLKY_PROFILE_FUNCTION() VLKY_PROFILE_SCOPE(VLKY_FUNC_SIG)
#else
#define VLKY_PROFILE_BEGIN_SESSION(name, filepath)
#define VLKY_PROFILE_END_SESSION()
#define VLKY_PROFILE_SCOPE(name)
#define VLKY_PROFILE_FUNCTION()
#endif
