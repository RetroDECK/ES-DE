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
std::vector<std::string> settings_dont_save {
    // These options can be set using command-line arguments:
    "Debug",                // --debug
    "ForceFull",            // --force-full
    "ForceKid",             // --force-kid
    "ForceKiosk",           // --force-kiosk
    "IgnoreGamelist",       // --ignore-gamelist
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
    "SplashScreenProgress",
    "ScraperFilter"
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

    // All settings are in pairs of default values and current values.
    // As such, in this function we set these pairs identically.

    //
    // Settings configured via the in-program settings menu.
    //

    // Scraper.
    mStringMap["Scraper"] = { "screenscraper", "screenscraper" };
    mBoolMap["ScraperUseAccountScreenScraper"] = { false, false };
    mStringMap["ScraperUsernameScreenScraper"] = { "", "" };
    mStringMap["ScraperPasswordScreenScraper"] = { "", "" };
    mBoolMap["ScrapeGameNames"] = { true, true };
    mBoolMap["ScrapeRatings"] = { true, true };
    mBoolMap["ScrapeMetadata"] = { true, true };
    mBoolMap["ScrapeVideos"] = { true, true };
    mBoolMap["ScrapeScreenshots"] = { true, true };
    mBoolMap["ScrapeCovers"] = { true, true };
    mBoolMap["ScrapeMarquees"] = { true, true };
    mBoolMap["Scrape3DBoxes"] = { true, true };
    mStringMap["ScraperRegion"] = { "eu", "eu" };
    mStringMap["ScraperLanguage"] = { "en", "en" };
    mBoolMap["ScraperOverwriteData"] = { true, true };
    mBoolMap["ScraperHaltOnInvalidMedia"] = { true, true };
    mBoolMap["ScraperSearchMetadataName"] = { true, true };
    mBoolMap["ScraperInteractive"] = { true, true };
    mBoolMap["ScraperSemiautomatic"] = { true, true };
    mBoolMap["ScraperRespectExclusions"] = { true, true };
    mBoolMap["ScraperExcludeRecursively"] = { true, true };
    mBoolMap["ScraperIncludeFolders"] = { false, false };

    // UI settings.
    mStringMap["StartupSystem"] = { "", "" };
    mStringMap["GamelistViewStyle"] = { "automatic", "automatic" };
    mStringMap["TransitionStyle"] = { "slide", "slide" };
    mStringMap["ThemeSet"] = { "rbsimple-DE", "rbsimple-DE" };
    mStringMap["UIMode"] = { "full", "full" };
    mStringMap["DefaultSortOrder"] = { "filename, ascending", "filename, ascending" };
    mStringMap["MenuOpeningEffect"] = { "scale-up", "scale-up" };
    mBoolMap["GamelistVideoPillarbox"] = { true, true };
    mBoolMap["GamelistVideoScanlines"] = { true, true };
    mBoolMap["FoldersOnTop"] = { true, true };
    mBoolMap["FavoritesFirst"] = { true, true };
    mBoolMap["FavoritesStar"] = { true, true };
    mBoolMap["FavoritesAddButton"] = { true, true };
    mBoolMap["GamelistFilters"] = { true, true };
    mBoolMap["QuickSystemSelect"] = { true, true };
    mBoolMap["ShowHelpPrompts"] = { true, true };
    mBoolMap["PlayVideosImmediately"] = { false, false };
    mBoolMap["EnableMenuKidMode"] = { false, false };

    // UI settings -> screensaver settings.
    mIntMap["ScreensaverTimer"] = { 5*60*1000, 5*60*1000 }; // 5 minutes
    mStringMap["ScreensaverType"] = { "dim", "dim" };
    mBoolMap["ScreensaverControls"] = { true, true };

    // UI settings -> screensaver settings -> slideshow screensaver settings.
    mIntMap["ScreensaverSwapImageTimeout"] = { 10000, 10000 };
    mBoolMap["ScreensaverStretchImages"] = { false, false };
    mBoolMap["ScreensaverSlideshowGameInfo"] = { true, true };
    mBoolMap["ScreensaverSlideshowScanlines"] = { true, true };
    mBoolMap["ScreensaverSlideshowCustomImages"] = { false, false };
    mBoolMap["ScreensaverSlideshowRecurse"] = { false, false };
    mStringMap["ScreensaverSlideshowImageDir"] = {
            "~/.emulationstation/slideshow/custom_images",
            "~/.emulationstation/slideshow/custom_images" };

    // UI settings -> screensaver settings -> video screensaver settings.
    mIntMap["ScreensaverSwapVideoTimeout"] = { 0, 0 };
    mBoolMap["ScreensaverVideoAudio"] = { false, false };
    mBoolMap["ScreensaverStretchVideos"] = { false, false };
    mBoolMap["ScreensaverVideoGameInfo"] = { true, true };
    mBoolMap["ScreensaverVideoScanlines"] = { true, true };
    mBoolMap["ScreensaverVideoBlur"] = { false, false };

    // Sound settings.
    // The ALSA Audio Card and Audio Device selection code is disabled at the moment.
    // As PulseAudio controls the sound devices for the desktop environment, it doesn't
    // make much sense to be able to select ALSA devices directly. Normally (always?)
    // the selection doesn't make any difference at all. But maybe some PulseAudio
    // settings could be added later on, if needed.
    // The code is still active for Raspberry Pi though as I'm not sure if this is
    // useful for that device.
    #if defined(_RPI_)
    mStringMap["AudioCard"] = { "default", "default" };
    // Audio out device for volume control.
    //#endif
    //#if defined(_RPI_)
    mStringMap["AudioDevice"] = { "PCM", "PCM" };
    // Audio out device for Video playback using OMX player.
    mStringMap["OMXAudioDev"] = { "both", "both" };
    //#else
    //    mStringMap["AudioDevice"] = { "Master", "Master" };
    #endif
    mBoolMap["GamelistVideoAudio"] = { true, true };
    mBoolMap["NavigationSounds"] = { true, true };

    // Game collection settings.
    mStringMap["CollectionSystemsAuto"] = { "", "" };
    mStringMap["CollectionSystemsCustom"] = { "", "" };
    mBoolMap["FavFirstCustom"] = { false, false };
    mBoolMap["FavStarCustom"] = { false, false };
    mBoolMap["UseCustomCollectionsSystem"] = { true, true };
    mBoolMap["CollectionShowSystemInfo"] = { true, true };

    // Other settings.
    #if defined(_RPI_)
    mIntMap["MaxVRAM"] = { 80, 80 };
    #else
    mIntMap["MaxVRAM"] = { 128, 128 };
    #endif
    #if defined (__unix__)
    mStringMap["FullscreenMode"] = { "normal", "normal" };
    #endif
    #if defined(_RPI_)
    mBoolMap["VideoOmxPlayer"] = { false, false };
    // We're defaulting to OMX Player for full screen video on the Pi.
    mBoolMap["ScreensaverOmxPlayer"] = { true, true };
    #endif
    mStringMap["SaveGamelistsMode"] = { "always", "always" };
    #if defined(_WIN64)
    mBoolMap["HideTaskbar"] = { false, false };
    // This setting may cause problems on some Windows versions, but it seems as if Windows 10
    // handles the suspension of ES correctly. As there are some adverse affects from running ES
    // in the background while a game is running, by default this is set to false.
    mBoolMap["RunInBackground"] = { false, false };
    #endif
    mStringMap["MediaDirectory"] = { "", "" };
    #if defined(__APPLE__) || defined(_WIN64)
    mStringMap["EmulatorCorePath"] = { "", "" };
    #else
    const std::string emulatorCorePaths =
            "~/.config/retroarch/cores:"                        // Compiled from source
            "~/snap/retroarch/current/.config/retroarch/cores:" // Snap package
            // As installed via the OS repositories:
            "/usr/lib/x86_64-linux-gnu/libretro:"               // Ubuntu and Linux Mint
            "/usr/lib64/libretro:"                              // Fedora
            "/usr/lib/libretro:"                                // Manjaro
            "/usr/local/lib/libretro:"                          // FreeBSD and OpenBSD
            "/usr/pkg/lib/libretro";                            // NetBSD
    mStringMap["EmulatorCorePath"] = { emulatorCorePaths, emulatorCorePaths };
    #endif
    mBoolMap["LaunchCommandOverride"] = { true, true };
    mBoolMap["ShowHiddenFiles"] = { true, true };
    mBoolMap["ShowHiddenGames"] = { true, true };
    mBoolMap["CustomEventScripts"] = { false, false };
    mBoolMap["ParseGamelistOnly"] = { false, false };
    mBoolMap["ROMDirGameMedia"] = { false, false };
    mBoolMap["DisplayGPUStatistics"] = { false, false };
    // macOS requires root privileges to reboot and power off so it doesn't make much
    // sense to enable these settings and menu entries for this operating system.
    #if defined(__APPLE__)
    mBoolMap["ShowQuitMenu"] = { false, false };
    #else
    mBoolMap["ShowQuitMenu"] = { true, true };
    #endif

    //
    // Settings configured via command-line arguments.
    //

    // Options listed using --help
    mBoolMap["Debug"] = { false, false };
    mBoolMap["ForceFull"] = { false, false };
    mBoolMap["ForceKid"] = { false, false };
    mBoolMap["ForceKiosk"] = { false, false };
    mBoolMap["IgnoreGamelist"] = { false, false };
    mBoolMap["SplashScreen"] = { true, true };
    mBoolMap["VSync"] = { true, true };
    #if !defined(_WIN64)
    mBoolMap["Windowed"] = { false, false };
    #endif
    mIntMap["WindowWidth"]   = { 0, 0 };
    mIntMap["WindowHeight"]  = { 0, 0 };
    mIntMap["ScreenWidth"]   = { 0, 0 };

    // Undocumented options.
    mIntMap["ScreenHeight"]  = { 0, 0 };
    mIntMap["ScreenOffsetX"] = { 0, 0 };
    mIntMap["ScreenOffsetY"] = { 0, 0 };
    mIntMap["ScreenRotate"]  = { 0, 0 };

    //
    // Settings that can be changed in es_settings.cfg
    // but that are not configurable via the GUI.
    //

    mBoolMap["ShowDefaultKeyboardWarning"] = { true, true };
    mStringMap["ROMDirectory"] = { "", "" };
    mIntMap["ScraperResizeMaxWidth"] = { 600, 600 };
    mIntMap["ScraperResizeMaxHeight"] = { 0, 0 };

    //
    // Hardcoded or program-internal settings.
    //

    mBoolMap["BackgroundJoystickInput"] = { false, false };
    mBoolMap["DebugGrid"] = { false, false };
    mBoolMap["DebugText"] = { false, false };
    mBoolMap["DebugImage"] = { false, false };
    mBoolMap["SplashScreenProgress"] = { true, true };
    mIntMap["ScraperFilter"] = { 0, 0 };
    mStringMap["UIMode_passkey"] = { "uuddlrlrba", "uuddlrlrba" };
}

template <typename K, typename V>
void saveMap(pugi::xml_document& doc, std::map<K, V>& map, const std::string& type)
{
    for (auto iter = map.cbegin(); iter != map.cend(); iter++) {
        // Key is on the "don't save" list, so don't save it.
        if (std::find(settings_dont_save.cbegin(), settings_dont_save.cend(),
                iter->first) != settings_dont_save.cend())
            continue;

        pugi::xml_node node = doc.append_child(type.c_str());
        node.append_attribute("name").set_value(iter->first.c_str());
        node.append_attribute("value").set_value(iter->second.second);
    }
}

void Settings::saveFile()
{
    LOG(LogDebug) << "Settings::saveFile(): Saving Settings to file.";
    const std::string path = Utils::FileSystem::getHomePath() +
            "/.emulationstation/es_settings.cfg";

    pugi::xml_document doc;

    saveMap<std::string, std::pair<bool, bool>>(doc, mBoolMap, "bool");
    saveMap<std::string, std::pair<int, int>>(doc, mIntMap, "int");
    saveMap<std::string, std::pair<float, float>>(doc, mFloatMap, "float");

    for (auto iter = mStringMap.cbegin(); iter != mStringMap.cend(); iter++) {
        pugi::xml_node node = doc.append_child("string");
        node.append_attribute("name").set_value(iter->first.c_str());
        node.append_attribute("value").set_value(iter->second.second.c_str());
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
#define SETTINGS_GETSET(type, mapName, getFunction, getDefaultFunction, setFunction) \
type Settings::getFunction(const std::string& name) \
{ \
    if (mapName.find(name) == mapName.cend()) { \
        LOG(LogError) << "Tried to use unset setting " << name << "!"; \
    } \
    return mapName[name].second; \
} \
type Settings::getDefaultFunction(const std::string& name) \
{ \
    if (mapName.find(name) == mapName.cend()) { \
        LOG(LogError) << "Tried to use unset setting " << name << "!"; \
    } \
    return mapName[name].first; \
} \
bool Settings::setFunction(const std::string& name, type value) \
{ \
    if (mapName.count(name) == 0 || mapName[name].second != value) { \
        mapName[name].second = value; \
\
        if (std::find(settings_dont_save.cbegin(), settings_dont_save.cend(), name) \
                 == settings_dont_save.cend()) \
            mWasChanged = true; \
\
        return true; \
    } \
    return false; \
}

SETTINGS_GETSET(bool, mBoolMap, getBool, getDefaultBool, setBool);
SETTINGS_GETSET(int, mIntMap, getInt, getDefaultInt, setInt);
SETTINGS_GETSET(float, mFloatMap, getFloat, getDefaultFloat, setFloat);
SETTINGS_GETSET(const std::string&, mStringMap, getString, getDefaultString, setString);
