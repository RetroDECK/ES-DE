//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Settings.cpp
//
//  Functions to read from and write to the configuration file es_settings.cfg.
//  The default values for the application settings are defined here as well.
//

#include "Settings.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "Log.h"
#include "Platform.h"
#include "Scripting.h"

#include <algorithm>
#include <pugixml.hpp>
#include <vector>

Settings* Settings::sInstance = nullptr;

// These values are NOT saved to es_settings.cfg since they're not set via
// the in-program settings menu. Most can be set using command-line arguments,
// but some are debug flags that are either hardcoded or set by internal debug
// functions.
std::vector<const char*> settings_dont_save {
    // These options can be set using command-line arguments:
    "Debug",                // --debug
    "ForceKid",             // --force-kid
    "ForceKiosk",           // --force-kiosk
    "IgnoreGamelist",       // --ignore-gamelist
    "ShowExit",             // --no-exit
    "SplashScreen",         // --no-splash
    "VSync",                // --vsync [1/on or 0/off]
    #if !defined(_WIN64)
    "Windowed",             // --windowed
    #endif
    "WindowWidth",          // Set via --resolution [width] [height]
    "WindowHeight",         // set via --resolution [width] [height]

    // These options are not shown in the --help text and are intended
    // for debugging and testing purposes:
    "ScreenWidth",          // Set via --screensize [width] [height]
    "ScreenHeight",         // set via --screensize [width] [height]
    "ScreenOffsetX",        // Set via --screenoffset [X] [Y]
    "ScreenOffsetY",        // Set via --screenoffset [X] [Y]
    "ScreenRotate",         // --screenrotate [0-3]

    // These options are not configurable from the command-line:
    "DebugGrid",
    "DebugText",
    "DebugImage",
    "SplashScreenProgress"
};

Settings::Settings()
{
    mWasChanged = false;
    setDefaults();
    loadFile();
}

Settings* Settings::getInstance()
{
    if (sInstance == nullptr)
        sInstance = new Settings();

    return sInstance;
}

void Settings::setDefaults()
{
    mBoolMap.clear();
    mIntMap.clear();

    //
    // Settings configured via the in-program settings menu.
    //

    // Scraper.
    mStringMap["Scraper"] = "screenscraper";
    mBoolMap["ScraperUseAccountScreenScraper"] = false;
    mStringMap["ScraperUsernameScreenScraper"] = "";
    mStringMap["ScraperPasswordScreenScraper"] = "";
    mBoolMap["ScrapeGameNames"] = true;
    mBoolMap["ScrapeRatings"] = true;
    mBoolMap["ScrapeMetadata"] = true;
    mBoolMap["ScrapeVideos"] = true;
    mBoolMap["ScrapeScreenshots"] = true;
    mBoolMap["ScrapeCovers"] = true;
    mBoolMap["ScrapeMarquees"] = true;
    mBoolMap["Scrape3DBoxes"] = true;
    mStringMap["ScraperRegion"] = "eu";
    mStringMap["ScraperLanguage"] = "en";
    mBoolMap["ScraperOverwriteData"] = true;
    mBoolMap["ScraperSearchMetadataName"] = true;
    mBoolMap["ScraperInteractive"] = true;
    mBoolMap["ScraperSemiautomatic"] = true;
    mBoolMap["ScraperRespectExclusions"] = true;
    mBoolMap["ScraperExcludeRecursively"] = true;
    mBoolMap["ScraperIncludeFolders"] = false;

    // UI settings.
    mStringMap["StartupSystem"] = "";
    mStringMap["GamelistViewStyle"] = "automatic";
    mStringMap["TransitionStyle"] = "instant";
    mStringMap["ThemeSet"] = "rbsimple-DE";
    mStringMap["UIMode"] = "full";
    mStringMap["DefaultSortOrder"] = "filename, ascending";
    mStringMap["MenuOpeningEffect"] = "scale-up";
    mBoolMap["CarouselTransitions"] = true;
    mBoolMap["GamelistVideoScanlines"] = true;
    mBoolMap["FoldersOnTop"] = true;
    mBoolMap["FavoritesFirst"] = true;
    mBoolMap["FavoritesStar"] = true;
    mBoolMap["FavoritesAddButton"] = true;
    mBoolMap["GamelistFilters"] = true;
    mBoolMap["QuickSystemSelect"] = true;
    mBoolMap["ShowHelpPrompts"] = true;
    mBoolMap["PlayVideosImmediately"] = false;
    mBoolMap["ShowKidStartMenu"] = false;

    // UI settings -> screensaver settings.
    mIntMap["ScreensaverTimer"] = 5*60*1000; // 5 minutes
    mBoolMap["ScreensaverControls"] = true;
    mStringMap["ScreensaverBehavior"] = "dim";

    // UI settings -> screensaver settings -> slideshow screensaver settings.
    mIntMap["ScreensaverSwapImageTimeout"] = 8000;
    mBoolMap["ScreensaverStretchImages"] = false;
    mBoolMap["ScreensaverImageScanlines"] = true;
    mStringMap["ScreensaverSlideshowAudioFile"] =
            "~/.emulationstation/slideshow/audio/slideshow.wav";
    mBoolMap["ScreensaverSlideshowCustomImages"] = false;
    mStringMap["ScreensaverSlideshowImageDir"] = "~/.emulationstation/slideshow/custom_images";
    mBoolMap["ScreensaverSlideshowRecurse"] = false;

    // UI settings -> screensaver settings -> video screensaver settings.
    mIntMap["ScreensaverSwapVideoTimeout"] = 25000;
    mBoolMap["ScreensaverStretchVideos"] = false;
    #if defined(_RPI_)
    mStringMap["ScreensaverGameInfo"] = "never";
    #endif
    mBoolMap["ScreensaverVideoAudio"] = false;
    mBoolMap["ScreensaverVideoScanlines"] = true;
    mBoolMap["ScreensaverVideoBlur"] = false;

    // Sound settings.
    // The ALSA Audio Card and Audio Device selection code is disabled at the moment.
    // As PulseAudio controls the sound devices for the desktop environment, it doesn't
    // make much sense to be able to select ALSA devices directly. Normally (always?)
    // the selection doesn't make any difference at all. But maybe some PulseAudio
    // settings could be added later on, if needed.
    // The code is still active for Raspberry Pi though as I'm not sure if this is
    // useful for that device.
    #if defined(_RPI_)
    mStringMap["AudioCard"] = "default";
    // Audio out device for volume control.
    //#endif
    //#if defined(_RPI_)
    mStringMap["AudioDevice"] = "PCM";
    // Audio out device for Video playback using OMX player.
    mStringMap["OMXAudioDev"] = "both";
    //#else
    //    mStringMap["AudioDevice"] = "Master";
    #endif
    mBoolMap["GamelistVideoAudio"] = true;
    mBoolMap["NavigationSounds"] = true;

    // Game collection settings.
    mStringMap["CollectionSystemsAuto"] = "";
    mStringMap["CollectionSystemsCustom"] = "";
    mBoolMap["FavFirstCustom"] = false;
    mBoolMap["FavStarCustom"] = false;
    mBoolMap["UseCustomCollectionsSystem"] = true;
    mBoolMap["CollectionShowSystemInfo"] = true;

    // Other settings.
    #if defined(_RPI_)
    mIntMap["MaxVRAM"] = 80;
    #else
    mIntMap["MaxVRAM"] = 128;
    #endif
    #if defined (__unix__)
    mStringMap["FullscreenMode"] = "normal";
    #endif
    mStringMap["PowerSaverMode"] = "disabled";
    // This setting only applies to Raspberry Pi but we set it for all platforms so
    // we don't get a warning if we encounter it on a different platform.
    mBoolMap["VideoOmxPlayer"] = false;
    #if defined(_RPI_)
    // We're defaulting to OMX Player for full screen video on the Pi.
    mBoolMap["ScreensaverOmxPlayer"] = true;
    #endif
    mStringMap["SaveGamelistsMode"] = "always";
    #if defined(_WIN64)
    mBoolMap["HideTaskbar"] = false;
    // This setting may cause problems on some Windows versions, but it seems as if Windows 10
    // handles the suspension of ES correctly. As there are some adverse affects from running ES
    // in the background while a game is running, by default this is set to false.
    mBoolMap["RunInBackground"] = false;
    #endif
    mStringMap["MediaDirectory"] = "";
    mBoolMap["LaunchCommandOverride"] = true;
    mBoolMap["ShowHiddenFiles"] = true;
    mBoolMap["ShowHiddenGames"] = true;
    mBoolMap["CustomEventScripts"] = false;
    mBoolMap["ParseGamelistOnly"] = false;
    mBoolMap["ROMDirGameMedia"] = false;
    mBoolMap["DisplayGPUStatistics"] = false;
    // macOS requires root privileges to reboot and power off so it doesn't make much
    // sense to enable these settings and menu entries for this operating system.
    #if !defined(__APPLE__)
    mBoolMap["ShowRebootSystem"] = true;
    mBoolMap["ShowPoweroffSystem"] = true;
    #endif

    //
    // Settings configured via command-line arguments.
    //

    // Options listed using --help
    mBoolMap["Debug"] = false;
    mBoolMap["ForceKid"] = false;
    mBoolMap["ForceKiosk"] = false;
    mBoolMap["IgnoreGamelist"] = false;
    mBoolMap["ShowExit"] = true;
    mBoolMap["SplashScreen"] = true;
    mBoolMap["VSync"] = true;
    #if !defined(_WIN64)
    mBoolMap["Windowed"] = false;
    #endif
    mIntMap["WindowWidth"]   = 0;
    mIntMap["WindowHeight"]  = 0;
    mIntMap["ScreenWidth"]   = 0;

    // Undocumented options.
    mIntMap["ScreenHeight"]  = 0;
    mIntMap["ScreenOffsetX"] = 0;
    mIntMap["ScreenOffsetY"] = 0;
    mIntMap["ScreenRotate"]  = 0;

    //
    // Settings that can be changed in es_settings.cfg
    // but that are not configurable via the GUI.
    //

    mBoolMap["ShowDefaultKeyboardWarning"] = true;
    mStringMap["ROMDirectory"] = "";
    mIntMap["ScraperResizeMaxWidth"] = 600;
    mIntMap["ScraperResizeMaxHeight"] = 0;

    //
    // Hardcoded or program-internal settings.
    //

    mBoolMap["BackgroundJoystickInput"] = false;
    mBoolMap["DebugGrid"] = false;
    mBoolMap["DebugText"] = false;
    mBoolMap["DebugImage"] = false;
    mBoolMap["SplashScreenProgress"] = true;
    mStringMap["UIMode_passkey"] = "uuddlrlrba";
}

template <typename K, typename V>
void saveMap(pugi::xml_document& doc, std::map<K, V>& map, const char* type)
{
    for (auto iter = map.cbegin(); iter != map.cend(); iter++) {
        // Key is on the "don't save" list, so don't save it.
        if (std::find(settings_dont_save.cbegin(), settings_dont_save.cend(),
                iter->first) != settings_dont_save.cend())
            continue;

        pugi::xml_node node = doc.append_child(type);
        node.append_attribute("name").set_value(iter->first.c_str());
        node.append_attribute("value").set_value(iter->second);
    }
}

void Settings::saveFile()
{
    LOG(LogDebug) << "Settings::saveFile(): Saving Settings to file.";
    const std::string path = Utils::FileSystem::getHomePath() +
            "/.emulationstation/es_settings.cfg";

    pugi::xml_document doc;

    saveMap<std::string, bool>(doc, mBoolMap, "bool");
    saveMap<std::string, int>(doc, mIntMap, "int");
    saveMap<std::string, float>(doc, mFloatMap, "float");

    for (auto iter = mStringMap.cbegin(); iter != mStringMap.cend(); iter++) {
        pugi::xml_node node = doc.append_child("string");
        node.append_attribute("name").set_value(iter->first.c_str());
        node.append_attribute("value").set_value(iter->second.c_str());
    }

    #if defined(_WIN64)
    doc.save_file(Utils::String::stringToWideString(path).c_str());
    #else
    doc.save_file(path.c_str());
    #endif

    Scripting::fireEvent("config-changed");
    Scripting::fireEvent("settings-changed");
}

void Settings::loadFile()
{
    const std::string path = Utils::FileSystem::getHomePath() +
            "/.emulationstation/es_settings.cfg";

    if (!Utils::FileSystem::exists(path))
        return;

    pugi::xml_document doc;
    #if defined(_WIN64)
    pugi::xml_parse_result result = doc.load_file(Utils::String::stringToWideString(path).c_str());
    #else
    pugi::xml_parse_result result = doc.load_file(path.c_str());
    #endif
    if (!result) {
        LOG(LogError) << "Could not parse Settings file.\n   " << result.description();
        return;
    }

    for (pugi::xml_node node = doc.child("bool"); node; node = node.next_sibling("bool"))
        setBool(node.attribute("name").as_string(), node.attribute("value").as_bool());
    for (pugi::xml_node node = doc.child("int"); node; node = node.next_sibling("int"))
        setInt(node.attribute("name").as_string(), node.attribute("value").as_int());
    for (pugi::xml_node node = doc.child("float"); node; node = node.next_sibling("float"))
        setFloat(node.attribute("name").as_string(), node.attribute("value").as_float());
    for (pugi::xml_node node = doc.child("string"); node; node = node.next_sibling("string"))
        setString(node.attribute("name").as_string(), node.attribute("value").as_string());
}

// Print a warning message if the setting we're trying to get doesn't already exist in
// the map. Then return the value in the map.
#define SETTINGS_GETSET(type, mapName, getMethodName, setMethodName) \
        type Settings::getMethodName(const std::string& name) \
{ \
    if (mapName.find(name) == mapName.cend()) { \
        LOG(LogError) << "Tried to use unset setting " << name << "!"; \
    } \
    return mapName[name]; \
} \
bool Settings::setMethodName(const std::string& name, type value) \
{ \
    if (mapName.count(name) == 0 || mapName[name] != value) { \
        mapName[name] = value; \
\
        if (std::find(settings_dont_save.cbegin(), settings_dont_save.cend(), name) \
                 == settings_dont_save.cend()) \
            mWasChanged = true; \
\
        return true; \
    } \
    return false; \
}

SETTINGS_GETSET(bool, mBoolMap, getBool, setBool);
SETTINGS_GETSET(int, mIntMap, getInt, setInt);
SETTINGS_GETSET(float, mFloatMap, getFloat, setFloat);
SETTINGS_GETSET(const std::string&, mStringMap, getString, setString);
