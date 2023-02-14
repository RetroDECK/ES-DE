//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  MameNames.h
//
//  Provides expanded game names based on short MAME name arguments. Also contains
//  functions to check whether a passed argument is a MAME BIOS or a MAME device.
//  The data sources are stored in the .emulationstation/resources directory
//  as the files mamebioses.xml, mamedevices.xml and mamenames.xml.
//

#ifndef ES_CORE_MAMENAMES_H
#define ES_CORE_MAMENAMES_H

#include "Settings.h"
#include "utils/StringUtil.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

// Expand MAME names to full game names and lookup device and BIOS entries.
class MameNames
{
public:
    static MameNames& getInstance();

    std::string getRealName(const std::string& mameName)
    {
        std::string name = mNamePairs[mameName];
        if (name == "")
            return mameName;
        else
            return name;
    }

    std::string getCleanName(const std::string& mameName)
    {
        static const bool stripInfo {Settings::getInstance()->getBool("MAMENameStripExtraInfo")};
        if (stripInfo)
            return Utils::String::removeParenthesis(getRealName(mameName));
        else
            return getRealName(mameName);
    }

    const bool isBios(const std::string& biosName)
    {
        return std::find(mMameBioses.cbegin(), mMameBioses.cend(), biosName) != mMameBioses.cend();
    }

    const bool isDevice(const std::string& deviceName)
    {
        return std::find(mMameDevices.cbegin(), mMameDevices.cend(), deviceName) !=
               mMameDevices.cend();
    }

private:
    MameNames();

    std::unordered_map<std::string, std::string> mNamePairs;
    std::vector<std::string> mMameBioses;
    std::vector<std::string> mMameDevices;
};

#endif // ES_CORE_MAMENAMES_H
