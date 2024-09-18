//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  PlatformUtil.cpp
//
//  Platform utility functions.
//

#if !defined(_WIN64)
#define MAX_GAME_LOG_OUTPUT 5242880
#endif

#include "utils/PlatformUtil.h"

#include "InputManager.h"
#include "Log.h"
#include "Scripting.h"
#include "Window.h"
#if defined(_WIN64)
#include "utils/StringUtil.h"
#include <SDL2/SDL_opengl.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>

#if !defined(_WIN64)
#include <unistd.h>
#endif

#if defined(__HAIKU__)
#include <sys/time.h>
#endif

#include <array>
#include <fcntl.h>

namespace Utils
{
    namespace Platform
    {
        int runRebootCommand()
        {
#if defined(_WIN64)
            return system("shutdown -r -t 0");
#elif defined(__APPLE__) || defined(__FreeBSD__)
            // This will probably never be used on macOS as it requires root privileges to reboot.
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
#elif defined(__FreeBSD__)
            return system("shutdown -p now");
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

        int launchGameUnix(const std::string& cmd_utf8,
                           const std::string& startDirectory,
                           bool runInBackground)
        {
#if defined(__unix__) || defined(__APPLE__) || defined(__HAIKU__)
            std::string command = std::string(cmd_utf8) + " 2>&1 &";

            // Launching games while keeping ES-DE running in the background is very crude as for
            // instance no output from the command is captured and no real error handling is
            // implemented. It should therefore only be used when absolutely necessary.
            if (runInBackground) {
                if (startDirectory != "")
                    command = "cd " + startDirectory + " && " + command;
                LOG(LogDebug)
                    << "Platform::launchGameUnix(): Launching game while keeping ES-DE running "
                       "in the background, no command output will be written to the log file";
                return system(command.c_str());
            }

            FILE* commandPipe;
            std::array<char, 128> buffer {};
            std::string commandOutput;
            int returnValue;

            if (startDirectory != "")
                command = "cd " + startDirectory + " && " + command;

            if (!(commandPipe = reinterpret_cast<FILE*>(popen(command.c_str(), "r")))) {
                LOG(LogError) << "Couldn't open pipe to command";
                return -1;
            }

            int fd {fileno(commandPipe)};
            fd_set readfds;
            struct timeval timeout;

            FD_ZERO(&readfds);
            FD_SET(fd, &readfds);
            timeout.tv_sec = 0;
            timeout.tv_usec = 10000;

            SDL_Event event {};

            // We're not completely suspended when launching a game, instead we'll continue to
            // poll events. As part of this we'll handle adding and removal of controllers, all
            // other events are discarded.
            while (true) {
                // Check if pipe is available for reading.
                if (select(fd + 1, &readfds, nullptr, nullptr, &timeout) != 0) {
                    if (fgets(buffer.data(), buffer.size(), commandPipe) != nullptr)
                        commandOutput.append(
                            commandOutput.length() < MAX_GAME_LOG_OUTPUT ? buffer.data() : "");
                    else
                        break;
                }
                FD_SET(fd, &readfds);
                timeout.tv_sec = 0;
                timeout.tv_usec = 10000;
                // Drop all events except those for adding and removing controllers.
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_CONTROLLERDEVICEADDED ||
                        event.type == SDL_CONTROLLERDEVICEREMOVED)
                        InputManager::getInstance().parseEvent(event);
                }
            }

            returnValue = pclose(commandPipe);

#if defined(RASPBERRY_PI)
            // Hack to avoid that the application window occasionally loses focus when returning
            // from a game, which only seems to happen on Raspberry Pi OS 10.
            SDL_Delay(50);
            SDL_SetWindowInputFocus(Renderer::getInstance()->getSDLWindow());
#endif

            // We need to shift the return value as it contains some flags (which we don't need).
            returnValue >>= 8;

            const size_t commandOutputSize {commandOutput.size()};

            // Remove any trailing newline from the command output.
            if (commandOutput.size()) {
                if (commandOutput.back() == '\n')
                    commandOutput.pop_back();
            }

            if (returnValue) {
                LOG(LogError) << "launchGameUnix - return value "
                              << std::to_string(returnValue) + ":";
                if (commandOutput.size())
                    LOG(LogError) << commandOutput;
                else
                    LOG(LogError) << "No error output provided by game or emulator";
            }
            else if (commandOutput.size()) {
                LOG(LogDebug) << "Platform::launchGameUnix():";
                LOG(LogDebug) << "Output from launched game:\n" << commandOutput;
                if (commandOutputSize >= MAX_GAME_LOG_OUTPUT) {
                    LOG(LogWarning) << "Output was capped to " << commandOutputSize << " bytes";
                }
            }

            return returnValue;

#else
            return 0;
#endif
        }

        int launchGameWindows(const std::wstring& cmd_utf16,
                              const std::wstring& startDirectory,
                              bool runInBackground,
                              bool hideWindow)
        {
#if defined(_WIN64)
            STARTUPINFOW si {};
            PROCESS_INFORMATION pi;

            si.cb = sizeof(si);
            if (hideWindow) {
                // Optionally hide the window. This is intended primarily for hiding console windows
                // when launching scripts (used for example by Steam games and source ports).
                si.dwFlags = STARTF_USESHOWWINDOW;
                si.wShowWindow = SW_HIDE;
            }
            bool processReturnValue {true};
            DWORD errorCode {0};

            std::wstring startDirectoryTemp {startDirectory};
            wchar_t* startDir {startDirectory == L"" ? nullptr : &startDirectoryTemp[0]};

            // clang-format off
            processReturnValue = CreateProcessW(
                nullptr,                         // No application name (use command line).
                const_cast<wchar_t*>(cmd_utf16.c_str()), // Command line.
                nullptr,                         // Process attributes.
                nullptr,                         // Thread attributes.
                FALSE,                           // Handles inheritance.
                0,                               // Creation flags.
                nullptr,                         // Use parent's environment block.
                startDir,                        // Starting directory, possibly the same as parent.
                &si,                             // Pointer to the STARTUPINFOW structure.
                &pi);                            // Pointer to the PROCESS_INFORMATION structure.
            // clang-format on

            if (!runInBackground) {
                int width {};
                int height {};

                // Hack to make the emulator window render correctly when launching games while
                // running in full screen mode. If not done, the emulator window will simply be
                // black although the game actually works and outputs sounds, accepts input etc.
                // There is sometimes a white flash the first time an emulator is started during
                // the program session and possibly some other brief screen flashing on game
                // launch but it's at least a tolerable workaround.
                SDL_GetWindowSize(Renderer::getInstance()->getSDLWindow(), &width, &height);
                SDL_SetWindowSize(Renderer::getInstance()->getSDLWindow(), width + 1, height);
                SDL_Delay(100);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                Renderer::getInstance()->swapBuffers();
                SDL_Event event {};

                // We're not completely suspended when launching a game, instead we'll continue to
                // poll events. As part of this we'll handle adding and removal of controllers, all
                // other events are discarded.
                while (true) {
                    if (WaitForSingleObject(pi.hProcess, 10) == 0)
                        break;
                    // Drop all events except those for adding and removing controllers.
                    while (SDL_PollEvent(&event)) {
                        if (event.type == SDL_CONTROLLERDEVICEADDED ||
                            event.type == SDL_CONTROLLERDEVICEREMOVED)
                            InputManager::getInstance().parseEvent(event);
                    }
                }

                SDL_SetWindowSize(Renderer::getInstance()->getSDLWindow(), width, height);
            }

            // If the return value is false, then something failed.
            if (!processReturnValue) {
                LPWSTR pBuffer {nullptr};

                FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, nullptr,
                               GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                               reinterpret_cast<LPWSTR>(&pBuffer), 0, nullptr);

                errorCode = GetLastError();

                std::string errorMessage {Utils::String::wideStringToString(pBuffer)};
                // Remove trailing newline from the error message.
                if (errorMessage.size()) {
                    if (errorMessage.back() == '\n')
                        errorMessage.pop_back();
                    if (errorMessage.size()) {
                        if (errorMessage.back() == '\r')
                            errorMessage.pop_back();
                    }
                }

                LOG(LogError) << "launchGameWindows - system error code " << errorCode << ": "
                              << errorMessage;
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

        int quitES(QuitMode mode)
        {
            sQuitMode = mode;

            SDL_Event quit {};
            quit.type = SDL_QUIT;
            SDL_PushEvent(&quit);
            return 0;
        }

        void processQuitMode()
        {
            switch (sQuitMode) {
                case QuitMode::REBOOT: {
                    LOG(LogInfo) << "Rebooting system";
                    Scripting::fireEvent("reboot");
                    Scripting::fireEvent("quit");
                    runRebootCommand();
                    break;
                }
                case QuitMode::POWEROFF: {
                    LOG(LogInfo) << "Powering off system";
                    Scripting::fireEvent("poweroff");
                    Scripting::fireEvent("quit");
                    runPoweroffCommand();
                    break;
                }
                default: {
                    Scripting::fireEvent("quit");
                    break;
                }
            }
        }

        void emergencyShutdown()
        {
            LOG(LogError) << "Critical - Performing emergency shutdown...";
            Scripting::fireEvent("quit");

            Window::getInstance()->deinit();
            Log::flush();

            exit(EXIT_FAILURE);
        }

    } // namespace Platform

} // namespace Utils
