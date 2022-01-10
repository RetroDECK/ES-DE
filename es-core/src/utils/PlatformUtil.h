//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Platform.h
//
//  Platform utility functions.
//

#ifndef ES_CORE_UTILS_PLATFORM_UTIL_H
#define ES_CORE_UTILS_PLATFORM_UTIL_H

#include <string>

#if defined(_WIN64)
#include <winsock2.h>
// This order is required as MinGW complains if windows.h is included before winsock2.h.
#include <windows.h>
#endif

namespace Utils
{
    namespace Platform
    {
        enum QuitMode {
            QUIT = 0, // Replace with AllowShortEnumsOnASingleLine: false (clang-format >=11.0).
            REBOOT = 1,
            POWEROFF = 2
        };

        int runRebootCommand();
        int runPoweroffCommand();

        // Uses UTF-8 for Unix and does a UTF-16/wstring conversion for Windows.
        int runSystemCommand(const std::string& cmd_utf8);
        // Windows specific UTF-16/wstring function. (FOR FUTURE USE)
        int runSystemCommand(const std::wstring& cmd_utf16);

        int launchGameUnix(const std::string& cmd_utf8, bool runInBackground);
        int launchGameWindows(const std::wstring& cmd_utf16, bool runInBackground, bool hideWindow);

        unsigned int getTaskbarState();
        void hideTaskbar();
        void revertTaskbarState(unsigned int& state);

        // Clean, normal shutdown.
        int quitES(QuitMode mode = QuitMode::QUIT);

        // Immediately shut down the application as cleanly as possible.
        void emergencyShutdown();
        void processQuitMode();

        inline static QuitMode quitMode = QuitMode::QUIT;

    } // namespace Platform

} // namespace Utils

#endif // ES_CORE_UTILS_PLATFORM_UTIL_H
