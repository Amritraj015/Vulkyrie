#pragma once

#include "core/status_codes.h"
#include "core/window_props.h"

namespace Vulkyrie::Platform {
    class PlatformBase {
        public:
            PlatformBase(const PlatformBase &) = delete;
            void operator=(PlatformBase const &) = delete;
            virtual ~PlatformBase() = default;

            /** Creates a new window for the application.
             * @param props The window properties.
             */
            virtual Vulkyrie::Core::StatusCode CreateNewWindow(Vulkyrie::Core::WindowProps props) = 0;

            /** Closes the application window. */
            virtual Vulkyrie::Core::StatusCode CloseWindow() = 0;

            /** Polls for events on the platform specific window. */
            // virtual bool PollForEvents() = 0;

            /** SleepForDuration on the thread for the provided ms. This blocks the main thread.
             * Should only be used for giving time back to the OS for unused update power.
             * Therefore it is not exported.
             * @param duration SleepForDuration duration.
             */
            // virtual void SleepForDuration(u64 duration) = 0;

        protected:
            PlatformBase() = default;
    };
} // namespace Vulkyrie::Platform
