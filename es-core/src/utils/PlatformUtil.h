//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  Platform.h
//
//  Platform utility functions.
//

#ifndef ES_CORE_UTILS_PLATFORM_UTIL_H
#define ES_CORE_UTILS_PLATFORM_UTIL_H

#include <string>

#if defined(_WIN64)
#include <windows.h>
#endif

#if defined(__ANDROID__)
#include <jni.h>
#include <map>

extern "C" void Java_org_es_1de_frontend_MainActivity_nativeLogOutput(JNIEnv* jniEnv,
                                                                      jclass jniClass,
                                                                      jstring output,
                                                                      jint logLevel);
#endif

namespace Utils
{
    namespace Platform
    {
        enum QuitMode {
            QUIT = 0,
            REBOOT = 1,
            POWEROFF = 2
        };

        int runRebootCommand();
        int runPoweroffCommand();

        // Uses UTF-8 for Unix and does a UTF-16/wstring conversion for Windows.
        int runSystemCommand(const std::string& cmd_utf8);
        // Windows specific UTF-16/wstring function. (FOR FUTURE USE)
        int runSystemCommand(const std::wstring& cmd_utf16);

        int launchGameUnix(const std::string& cmd_utf8,
                           const std::string& startDirectory,
                           bool runInBackground);
        int launchGameWindows(const std::wstring& cmd_utf16,
                              const std::wstring& startDirectory,
                              bool runInBackground,
                              bool hideWindow);

        unsigned int getTaskbarState();
        void hideTaskbar();
        void revertTaskbarState(unsigned int& state);

        // Clean, normal shutdown.
        int quitES(QuitMode mode = QuitMode::QUIT);
        void processQuitMode();

        // Immediately shut down the application as cleanly as possible.
        void emergencyShutdown();

#if defined(__ANDROID__)
        namespace Android
        {
            bool requestStoragePermission();
            void setDataDirectory();
            bool checkNeedResourceCopy(const std::string& buildIdentifier);
            bool setupResources(const std::string& buildIdentifier);
            bool checkEmulatorInstalled(const std::string& packageName,
                                        const std::string& activity);
            int launchGame(const std::string& packageName,
                           const std::string& activity,
                           const std::string& action,
                           const std::string& category,
                           const std::string& mimeType,
                           const std::string& data,
                           const std::string& romRaw,
                           const std::map<std::string, std::string>& extrasString,
                           const std::map<std::string, std::string>& extrasBool,
                           const std::vector<std::string>& activityFlags);

        } // namespace Android
#endif

        static QuitMode sQuitMode = QuitMode::QUIT;
        // This is simply to get rid of a GCC false positive -Wunused-variable compiler warning.
        static QuitMode sQuitModeDummy = sQuitMode;

    } // namespace Platform

} // namespace Utils

#endif // ES_CORE_UTILS_PLATFORM_UTIL_H
