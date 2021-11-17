//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Scripting.cpp
//
//  Executes custom scripts for various events in EmulationStation.
//  By calling fireEvent() the scripts inside the directory corresponding to the
//  argument 'eventName' will be executed with arg1 and arg2 as the script arguments.
//
//  The scripts are searched for in ~/.emulationstation/scripts/<eventName>
//  For example, if the event is called 'game-start', all scripts inside the directory
//  ~/.emulationstation/scripts/game-start/ will be executed.
//

#include "Scripting.h"

#include "Log.h"
#include "Platform.h"
#include "Settings.h"
#include "utils/FileSystemUtil.h"

namespace Scripting
{
    void fireEvent(const std::string& eventName, const std::string& arg1, const std::string& arg2)
    {
        if (!Settings::getInstance()->getBool("CustomEventScripts"))
            return;

        LOG(LogDebug) << "Scripting::fireEvent(): " << eventName << " \"" << arg1 << "\" \"" << arg2
                      << "\"";

        std::list<std::string> scriptDirList;
        std::string scriptDir;

        // Check in homepath.
        scriptDir = Utils::FileSystem::getHomePath() + "/.emulationstation/scripts/" + eventName;
        if (Utils::FileSystem::exists(scriptDir))
            scriptDirList.push_back(scriptDir);

        for (auto dirIt = scriptDirList.cbegin(); dirIt != scriptDirList.cend(); ++dirIt) {
            std::list<std::string> scripts = Utils::FileSystem::getDirContent(*dirIt);
            for (auto it = scripts.cbegin(); it != scripts.cend(); ++it) {
                std::string arg1Quotation;
                std::string arg2Quotation;
                // Add quotation marks around the arguments as long as these are not already
                // present (i.e. for arguments with spaces in them).
                if (arg1.front() != '\"')
                    arg1Quotation = "\"";
                if (arg2.front() != '\"')
                    arg2Quotation = "\"";
                std::string script;
                script.append(*it)
                    .append(" ")
                    .append(arg1Quotation)
                    .append(arg1)
                    .append(arg1Quotation)
                    .append(" ")
                    .append(arg2Quotation)
                    .append(arg2)
                    .append(arg2Quotation);
                LOG(LogDebug) << "Executing: " << script;
                runSystemCommand(script);
            }
        }
    }
} // namespace Scripting
