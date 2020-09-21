//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Scripting.h
//
//  Executes custom scripts for various events in EmulationStation.
//  By calling fireEvent() the scripts inside the directory corresponding to the
//  argument 'eventName' will be executed with arg1 and arg2 as the script arguments.
//
//  The scripts are searched for in $HOME/.emulationstation/scripts/<eventName>.
//  For example, if the event is called 'game-start', all scripts inside the directory
//  $HOME/.emulationstation/scripts/game-start/ will be executed.
//

#ifndef ES_CORE_SCRIPTING_H
#define ES_CORE_SCRIPTING_H

#include <string>

namespace Scripting
{
    void fireEvent(const std::string& eventName,
            const std::string& arg1="", const std::string& arg2="");
}

#endif //ES_CORE_SCRIPTING_H
