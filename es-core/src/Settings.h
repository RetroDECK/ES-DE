//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Settings.h
//
//  Functions to read from and write to the configuration file es_settings.cfg.
//  The default values for the application settings are defined here as well.
//

#ifndef ES_CORE_SETTINGS_H
#define ES_CORE_SETTINGS_H

#include <map>

//	This is a singleton for storing settings.
class Settings
{
public:
    static Settings* getInstance();

    void loadFile();
    void saveFile();

    //	You will get a warning if you try a get on a key that is not already present.
    bool getBool(const std::string& name);
    int getInt(const std::string& name);
    float getFloat(const std::string& name);
    const std::string& getString(const std::string& name);

    bool setBool(const std::string& name, bool value);
    bool setInt(const std::string& name, int value);
    bool setFloat(const std::string& name, float value);
    bool setString(const std::string& name, const std::string& value);

private:
    static Settings* sInstance;

    Settings();

    //	Clear everything and load default values.
    void setDefaults();

    bool mWasChanged;

    std::map<std::string, bool> mBoolMap;
    std::map<std::string, int> mIntMap;
    std::map<std::string, float> mFloatMap;
    std::map<std::string, std::string> mStringMap;
};

#endif // ES_CORE_SETTINGS_H
