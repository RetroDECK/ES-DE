//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Settings.h
//
//  Functions to read from and write to the configuration file es_settings.xml.
//  The default values for the application settings are defined here as well.
//  This class is not thread safe.
//

#ifndef ES_CORE_SETTINGS_H
#define ES_CORE_SETTINGS_H

#include <map>
#include <memory>
#include <string>

//	This is a singleton for storing settings.
class Settings
{
public:
    static Settings* getInstance();

    void loadFile();
    void saveFile();

    //	You will get a warning if you try a get on a key that is not already present.
    bool getBool(const std::string& name);
    bool getDefaultBool(const std::string& name);
    int getInt(const std::string& name);
    int getDefaultInt(const std::string& name);
    float getFloat(const std::string& name);
    float getDefaultFloat(const std::string& name);
    const std::string& getString(const std::string& name);
    const std::string& getDefaultString(const std::string& name);

    bool setBool(const std::string& name, bool value);
    bool setInt(const std::string& name, int value);
    bool setFloat(const std::string& name, float value);
    bool setString(const std::string& name, const std::string& value);

private:
    Settings();

    //	Clear everything and load default values.
    void setDefaults();

    bool mWasChanged;

    // Pair of settings: default value, current value.
    std::map<std::string, std::pair<bool, bool>> mBoolMap;
    std::map<std::string, std::pair<int, int>> mIntMap;
    std::map<std::string, std::pair<float, float>> mFloatMap;
    std::map<std::string, std::pair<std::string, std::string>> mStringMap;
};

#endif // ES_CORE_SETTINGS_H
