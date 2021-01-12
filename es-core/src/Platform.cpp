//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Platform.cpp
//
//  Platform-specific functions.
//

#include "Platform.h"

#include "renderers/Renderer.h"
#include "utils/StringUtil.h"
#include "AudioManager.h"
#include "Log.h"
#include "MameNames.h"
#include "Settings.h"

#include <SDL2/SDL_events.h>

#if defined(__APPLE__)
#include <array>
#endif

#if !defined(_WIN64)
#include <unistd.h>
#endif
#include <fcntl.h>

int runRebootCommand()
{
#if defined(_WIN64)
    return system("shutdown -r -t 0");
#elif defined(__APPLE__)
    // This will probably never be used as macOS requires root privileges to reboot.
    return system("shutdown -r now");
#else
    return system("shutdown --reboot now");
#endif
}

int runPoweroffCommand()
{
#if defined(_WIN64)
    return system("shutdown -s -t 0");
#elif defined(__APPLE__)
    // This will probably never be used as macOS requires root privileges to power off.
    return system("shutdown now");
#else
    return system("shutdown --poweroff now");
#endif
}

int runSystemCommand(const std::string& cmd_utf8)
{
    #if defined(_WIN64)
    // On Windows we use _wsystem to support non-ASCII paths
    // which requires converting from UTF-8 to a wstring.
    std::wstring wchar_str = Utils::String::stringToWideString(cmd_utf8);
    return _wsystem(wchar_str.c_str());
    #else
    return system(cmd_utf8.c_str());
    #endif
}

int runSystemCommand(const std::wstring& cmd_utf16)
{
    #if defined(_WIN64)
    return _wsystem(cmd_utf16.c_str());
    #else
    return 0;
    #endif
}

int launchEmulatorUnix(const std::string& cmd_utf8)
{
    #if defined(__unix__) || defined (__APPLE__)
    std::string command = std::string(cmd_utf8) + " 2>&1";

    FILE* commandPipe;
    std::array<char, 128> buffer;
    std::string commandOutput;
    int returnValue;

    if (!(commandPipe = (FILE*)popen(command.c_str(), "r"))) {
        LOG(LogError) << "Couldn't open pipe to command.";
        return -1;
    }

    while (fgets(buffer.data(), buffer.size(), commandPipe) != nullptr) {
        commandOutput += buffer.data();
    }

    returnValue = pclose(commandPipe);
    // We need to shift the return value as it contains some flags (which we don't need).
    returnValue >>= 8;

    // Remove any trailing newline from the command output.
    if (commandOutput.size()) {
        if (commandOutput.back() == '\n')
           commandOutput.pop_back();
    }

    if (returnValue) {
        LOG(LogError) << "launchEmulatorUnix - return value " <<
                std::to_string(returnValue) + ":";
        if (commandOutput.size())
            LOG(LogError) << commandOutput;
        else
            LOG(LogError) << "No error output provided by emulator.";
    }
    else if (commandOutput.size()) {
        LOG(LogDebug) << "Platform::launchEmulatorUnix():";
        LOG(LogDebug) << "Output from launched game:\n" << commandOutput;
    }

    return returnValue;

    #else // __unix__
    return 0;
    #endif
}

int launchEmulatorWindows(const std::wstring& cmd_utf16)
{
    #if defined(_WIN64)
    STARTUPINFOW si {};
    PROCESS_INFORMATION pi;

    si.cb = sizeof(si);
    bool processReturnValue = true;
    DWORD errorCode = 0;

    processReturnValue = CreateProcessW(
            nullptr,                            // No application name (use command line).
            const_cast<wchar_t*>(cmd_utf16.c_str()), // Command line.
            nullptr,                            // Process attributes.
            nullptr,                            // Thread attributes.
            FALSE,                              // Handles inheritance.
            0,                                  // Creation flags.
            nullptr,                            // Use parent's environment block.
            nullptr,                            // Use parent's starting directory.
            &si,                                // Pointer to the STARTUPINFOW structure.
            &pi);                               // Pointer to the PROCESS_INFORMATION structure.

    // Unfortunately suspending ES and resuming when the emulator process has exited
    // doesn't work reliably on Windows, so we may need to keep ES running in the
    // background while the game is launched. I'm not sure if there is a workaround
    // for this, but on some Windows installations it seems to work fine so we'll let
    // the user choose via a menu option.
    if (!Settings::getInstance()->getBool("RunInBackground")) {
        // Wait for the child process to exit.
        WaitForSingleObject(pi.hThread, INFINITE);
        WaitForSingleObject(pi.hProcess, INFINITE);
    }

    // If the return value is false, then something failed.
    if (!processReturnValue) {
        LPWSTR pBuffer = nullptr;

        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPWSTR>(&pBuffer), 0, nullptr);

        errorCode = GetLastError();

        std::string errorMessage = Utils::String::wideStringToString(pBuffer);
        // Remove trailing newline from the error message.
        if (errorMessage.size()) {
            if (errorMessage.back() == '\n')
                errorMessage.pop_back();
            if (errorMessage.size()) {
                if (errorMessage.back() == '\r')
                    errorMessage.pop_back();
            }
        }

        LOG(LogError) << "launchEmulatorWindows - system error code " <<
                errorCode << ": " << errorMessage;
    }

    // Close process and thread handles.
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return errorCode;

    #else // _WIN64
    return 0;
    #endif
}

unsigned int getTaskbarState()
{
    #if defined(_WIN64)
    APPBARDATA barData;
    barData.cbSize = sizeof(APPBARDATA);
    return static_cast<UINT>(SHAppBarMessage(ABM_GETSTATE, &barData));
    #else
    return 0;
    #endif
}

void hideTaskbar()
{
    #if defined(_WIN64)
    APPBARDATA barData;
    barData.cbSize = sizeof(APPBARDATA);
    barData.lParam = ABS_AUTOHIDE;
    SHAppBarMessage(ABM_SETSTATE, &barData);
    #endif
}

void revertTaskbarState(unsigned int& state)
{
    #if defined(_WIN64)
    APPBARDATA barData;
    barData.cbSize = sizeof(APPBARDATA);
    barData.lParam = state;
    SHAppBarMessage(ABM_SETSTATE, &barData);
    #endif
}

QuitMode quitMode = QuitMode::QUIT;

int quitES(QuitMode mode)
{
    quitMode = mode;

    SDL_Event quit;
    quit.type = SDL_QUIT;
    SDL_PushEvent(&quit);
    return 0;
}

void emergencyShutdown()
{
    LOG(LogError) << "Critical - Performing emergency shutdown...";

    MameNames::deinit();
    // Most of the SDL deinitialization is done in Renderer.
    Renderer::deinit();
    Log::flush();

    exit(EXIT_FAILURE);
}

void touch(const std::string& filename)
{
#if defined(_WIN64)
    FILE* fp = fopen(filename.c_str(), "ab+");
    if (fp != nullptr)
        fclose(fp);
#else
    int fd = open(filename.c_str(), O_CREAT|O_WRONLY, 0644);
    if (fd >= 0)
        close(fd);
#endif
}

void processQuitMode()
{
    switch (quitMode) {
    case QuitMode::REBOOT:
        LOG(LogInfo) << "Rebooting system";
        runRebootCommand();
        break;
    case QuitMode::POWEROFF:
        LOG(LogInfo) << "Powering off system";
        runPoweroffCommand();
        break;
    default:
        break;
    }
}
