//
//  Platform.cpp
//
//  Platform-specific functions.
//

#include "Platform.h"
#include "utils/StringUtil.h"

#if defined(__linux__) || defined(_WIN64)
#include <SDL2/SDL_events.h>
#else
#include "SDL_events.h"
#endif

#ifdef _WIN64
#include <windows.h>
#include <codecvt>
#include <locale>
#else
#include <unistd.h>
#endif
#include <fcntl.h>

#include "Log.h"

int runRebootCommand()
{
#ifdef _WIN64 // Windows.
    return system("shutdown -r -t 0");
#else // macOS and Linux.
    return system("shutdown --reboot now");
#endif
}

int runPoweroffCommand()
{
#ifdef _WIN64 // Windows.
    return system("shutdown -s -t 0");
#else // macOS and Linux.
    return system("shutdown --poweroff now");
#endif
}

int runSystemCommand(const std::string& cmd_utf8)
{
    #ifdef _WIN64
    // On Windows we use _wsystem to support non-ASCII paths
    // which requires converting from UTF-8 to a wstring.
    std::wstring wchar_str = Utils::String::charToWideChar(cmd_utf8);
    return _wsystem(wchar_str.c_str());
    #else
    return system(cmd_utf8.c_str());
    #endif
}

int runSystemCommand(const std::wstring& cmd_utf16)
{
    #ifdef _WIN64
    return _wsystem(cmd_utf16.c_str());
    #else
    return 0;
    #endif
}

int launchEmulatorUnix(const std::string& cmd_utf8)
{
    #ifdef __unix__
    return system(cmd_utf8.c_str());
    #else
    return 0;
    #endif
}

int launchEmulatorWindows(const std::wstring& cmd_utf16)
{
    #ifdef _WIN64
    STARTUPINFOW si {};
    PROCESS_INFORMATION pi;

    si.cb = sizeof(si);
    bool processReturnValue = true;
    DWORD errorCode = 0;

    processReturnValue = CreateProcessW(
            nullptr,                        // No application name (use command line).
            (wchar_t*) cmd_utf16.c_str(),   // Command line.
            nullptr,                        // Process attributes.
            nullptr,                        // Thread attributes.
            FALSE,                          // Handles inheritance.
            0,                              // Creation flags.
            nullptr,                        // Use parent's environment block.
            nullptr,                        // Use parent's starting directory.
            &si,                            // Pointer to the STARTUPINFOW structure.
            &pi );                          // Pointer to the PROCESS_INFORMATION structure.

    // Wait for the child process to exit.
    WaitForSingleObject(pi.hThread, INFINITE);
    WaitForSingleObject(pi.hProcess, INFINITE);

    // If the return value is false, then something failed.
    if (!processReturnValue) {
        LPWSTR pBuffer = nullptr;

        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPWSTR)&pBuffer, 0, nullptr);

        errorCode = GetLastError();

        std::string errorMessage = Utils::String::wideCharToChar(pBuffer);
        // Remove trailing newline from the error message.
        if (errorMessage.back() == '\n');
            errorMessage.pop_back();
        if (errorMessage.back() == '\r');
            errorMessage.pop_back();

        LOG(LogError) << "Error - launchEmulatorWindows - system error code " <<
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

QuitMode quitMode = QuitMode::QUIT;

int quitES(QuitMode mode)
{
    quitMode = mode;

    SDL_Event *quit = new SDL_Event();
    quit->type = SDL_QUIT;
    SDL_PushEvent(quit);
    return 0;
}

void touch(const std::string& filename)
{
#ifdef _WIN64
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
