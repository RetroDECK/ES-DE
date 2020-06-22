//
//  Scripting.cpp
//
//  Executes custom scripts for various events in EmulationStation.
//  By calling fireEvent() the scripts inside the directory corresponding to the
//  argument 'eventName' will be executed with arg1 and arg2 as the script arguments.
//
//  The scripts are searched for in $HOME/.emulationstation/scripts/<eventName>.
//  For example, if the event is called 'game-start', all scripts inside the directory
//  $HOME/.emulationstation/scripts/game-start/ will be executed.
//

#include "Scripting.h"
#include "Log.h"
#include "Platform.h"
#include "utils/FileSystemUtil.h"

namespace Scripting
{
	void fireEvent(const std::string& eventName, const std::string& arg1, const std::string& arg2)
	{
		LOG(LogDebug) << "fireEvent: " << eventName << " " << arg1 << " " << arg2;

        std::list<std::string> scriptDirList;
        std::string test;

        // Check in homepath.
        test = Utils::FileSystem::getHomePath() + "/.emulationstation/scripts/" + eventName;
        if(Utils::FileSystem::exists(test))
            scriptDirList.push_back(test);

        for(std::list<std::string>::const_iterator dirIt = scriptDirList.cbegin();
                dirIt != scriptDirList.cend(); ++dirIt) {
            std::list<std::string> scripts = Utils::FileSystem::getDirContent(*dirIt);
            for (std::list<std::string>::const_iterator it = scripts.cbegin();
                    it != scripts.cend(); ++it) {
                // Append folder to path.
                std::string script = *it + " \"" + arg1 + "\" \"" + arg2 + "\"";
                LOG(LogDebug) << "  executing: " << script;
                runSystemCommand(script);
            }
        }
	}
}
