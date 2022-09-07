//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Scripting.cpp
//
//  Executes custom scripts for various events.
//  By calling fireEvent() the scripts inside the directory corresponding to the
//  argument "eventName" will be executed with arg1, arg2, arg3 and arg4 as arguments.
//
//  The scripts are searched for in ~/.emulationstation/scripts/<eventName>
//  For example, if the event is called "game-start", all scripts inside the directory
//  ~/.emulationstation/scripts/game-start/ will be executed.
//

#include "Scripting.h"

#include "Log.h"
#include "Settings.h"
#include "utils/FileSystemUtil.h"
#include "utils/PlatformUtil.h"
#include "utils/StringUtil.h"

#include <algorithm>

namespace Scripting
{
    void fireEvent(const std::string& eventName,
                   const std::string& arg1,
                   const std::string& arg2,
                   const std::string& arg3,
                   const std::string& arg4)
    {
        if (!Settings::getInstance()->getBool("CustomEventScripts"))
            return;

        LOG(LogDebug) << "Scripting::fireEvent(): " << eventName << " \"" << arg1 << "\" \"" << arg2
                      << "\" \"" << arg3 << "\" \"" << arg4 << "\"";

        std::list<std::string> scriptDirList;
        std::string scriptDir;

        // Check in homepath.
        scriptDir = Utils::FileSystem::getHomePath() + "/.emulationstation/scripts/" + eventName;
        if (Utils::FileSystem::exists(scriptDir))
            scriptDirList.push_back(scriptDir);

        for (auto dirIt = scriptDirList.cbegin(); dirIt != scriptDirList.cend(); ++dirIt) {
            std::list<std::string> scripts {Utils::FileSystem::getDirContent(*dirIt)};
            // Sort the scripts in case-sensitive order on Unix/Linux and in case-insensitive order
            // on macOS and Windows.
#if defined(__unix__)
            scripts.sort([](std::string a, std::string b) { return a.compare(b) < 0; });
#else
            scripts.sort([](std::string a, std::string b) {
                return Utils::String::toUpper(a).compare(Utils::String::toUpper(b)) < 0;
            });
#endif
            for (auto it = scripts.cbegin(); it != scripts.cend(); ++it) {
                std::string arg1Quotation;
                std::string arg2Quotation;
                std::string arg3Quotation;
                std::string arg4Quotation;
                // Add quotation marks around the arguments as long as these are not already
                // present (i.e. for arguments with spaces in them).
                if (!arg1.empty() && arg1.front() != '\"')
                    arg1Quotation = "\"";
                if (!arg2.empty() && arg2.front() != '\"')
                    arg2Quotation = "\"";
                if (!arg3.empty() && arg3.front() != '\"')
                    arg3Quotation = "\"";
                if (!arg4.empty() && arg4.front() != '\"')
                    arg4Quotation = "\"";
                std::string script;
                script.append(*it)
                    .append(" ")
                    .append(arg1Quotation)
                    .append(arg1)
                    .append(arg1Quotation)
                    .append(" ")
                    .append(arg2Quotation)
                    .append(arg2)
                    .append(arg2Quotation)
                    .append(" ")
                    .append(arg3Quotation)
                    .append(arg3)
                    .append(arg3Quotation)
                    .append(" ")
                    .append(arg4Quotation)
                    .append(arg4)
                    .append(arg4Quotation);
                LOG(LogDebug) << "Executing: " << script;
                Utils::Platform::runSystemCommand(script);
            }
        }
    }
} // namespace Scripting
