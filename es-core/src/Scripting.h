//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Scripting.h
//
//  Executes custom scripts for various events.
//  By calling fireEvent() the scripts inside the directory corresponding to the
//  argument "eventName" will be executed with arg1, arg2, arg3 and arg4 as arguments.
//
//  The scripts are searched for in ~/.emulationstation/scripts/<eventName>
//  For example, if the event is called "game-start", all scripts inside the directory
//  ~/.emulationstation/scripts/game-start/ will be executed.
//

#ifndef ES_CORE_SCRIPTING_H
#define ES_CORE_SCRIPTING_H

#include <string>

namespace Scripting
{
    void fireEvent(const std::string& eventName,
                   const std::string& arg1 = "",
                   const std::string& arg2 = "",
                   const std::string& arg3 = "",
                   const std::string& arg4 = "");
}

#endif // ES_CORE_SCRIPTING_H
