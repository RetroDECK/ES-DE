//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Platform.h
//
//  Platform-specific functions.
//

#ifndef ES_CORE_PLATFORM_H
#define ES_CORE_PLATFORM_H

#include <string>

#if defined(_WIN64)
#include <winsock2.h>
// This order is required as MinGW complains if windows.h is included before winsock2.h.
#include <windows.h>
#endif

enum QuitMode {
    QUIT = 0, // Replace with AllowShortEnumsOnASingleLine: false (clang-format >=11.0).
    REBOOT = 1,
    POWEROFF = 2
};

// Uses UTF-8 for Unix and does a UTF-16/wstring conversion for Windows.
int runSystemCommand(const std::string& cmd_utf8);
// Windows specific UTF-16/wstring function. (FOR FUTURE USE)
int runSystemCommand(const std::wstring& cmd_utf16);

int launchGameUnix(const std::string& cmd_utf8, bool runInBackground);
int launchGameWindows(const std::wstring& cmd_utf16, bool runInBackground);

unsigned int getTaskbarState();
void hideTaskbar();
void revertTaskbarState(unsigned int& state);

// Clean, normal shutdown.
int quitES(QuitMode mode = QuitMode::QUIT);

// Immediately shut down the application as cleanly as possible.
void emergencyShutdown();
void processQuitMode();

#endif // ES_CORE_PLATFORM_H
