//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Settings.cpp
//
//  Functions to read from and write to the configuration file es_settings.xml.
//  The default values for the application settings are defined here as well.
//

#include "Settings.h"

#include "Log.h"
#include "Platform.h"
#include "Scripting.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

#include <algorithm>
#include <pugixml.hpp>
#include <vector>

Settings* Settings::sInstance = nullptr;

// These values are NOT saved to es_settings.xml since they're not set via
// the in-program settings menu. Most can be set using command-line arguments,
// but some are debug flags that are either hardcoded or set by internal debug
// functions.
std::vector<std::string> settingsSkipSaving
{
    // clang-format off
    // These options can be set using command-line arguments:
    "WindowWidth",          // Set via --resolution [width] [height]
    "WindowHeight",         // set via --resolution [width] [height]
    "ParseGamelistOnly"     // --gamelist-only
    "IgnoreGamelist",       // --ignore-gamelist
    "SplashScreen",         // --no-splash
    "Debug",                // --debug
    #if !defined(_WIN64)
    "Windowed",             // --windowed
    #endif
    "VSync",                // --vsync [1/on or 0/off]
    "ForceFull",            // --force-full
    "ForceKiosk",           // --force-kiosk
    "ForceKid",             // --force-kid

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
    // clang-format on
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

void Settings::deinit()
{
    if (sInstance) {
        delete sInstance;
        sInstance = nullptr;
    }
}

void Settings::setDefaults()
{
    mBoolMap.clear();
    mIntMap.clear();
    mStringMap.clear();

    // All settings are in pairs of default values and current values.
    // As such, in this function we set these pairs identically.

    //
    // Settings configured via the in-program settings menu.
    //

    // Scraper.
    mStringMap["Scraper"] = {"screenscraper", "screenscraper"};
    mBoolMap["ScraperUseAccountScreenScraper"] = {false, false};
    mStringMap["ScraperUsernameScreenScraper"] = {"", ""};
    mStringMap["ScraperPasswordScreenScraper"] = {"", ""};

    mBoolMap["ScrapeGameNames"] = {true, true};
    mBoolMap["ScrapeRatings"] = {true, true};
    mBoolMap["ScrapeMetadata"] = {true, true};
    mBoolMap["ScrapeVideos"] = {true, true};
    mBoolMap["ScrapeScreenshots"] = {true, true};
    mBoolMap["ScrapeCovers"] = {true, true};
    mBoolMap["ScrapeMarquees"] = {true, true};
    mBoolMap["Scrape3DBoxes"] = {true, true};

    mStringMap["MiximageResolution"] = {"1280x960", "1280x960"};
    mStringMap["MiximageScreenshotScaling"] = {"sharp", "sharp"};
    mBoolMap["MiximageGenerate"] = {true, true};
    mBoolMap["MiximageOverwrite"] = {true, true};
    mBoolMap["MiximageRemoveLetterboxes"] = {true, true};
    mBoolMap["MiximageRemovePillarboxes"] = {true, true};
    mBoolMap["MiximageIncludeMarquee"] = {true, true};
    mBoolMap["MiximageIncludeBox"] = {true, true};
    mBoolMap["MiximageCoverFallback"] = {true, true};

    mStringMap["ScraperRegion"] = {"eu", "eu"};
    mStringMap["ScraperLanguage"] = {"en", "en"};
    mBoolMap["ScraperOverwriteData"] = {true, true};
    mBoolMap["ScraperHaltOnInvalidMedia"] = {true, true};
    mBoolMap["ScraperSearchMetadataName"] = {true, true};
    mBoolMap["ScraperInteractive"] = {true, true};
    mBoolMap["ScraperSemiautomatic"] = {true, true};
    mBoolMap["ScraperRespectExclusions"] = {true, true};
    mBoolMap["ScraperExcludeRecursively"] = {true, true};
    mBoolMap["ScraperIncludeFolders"] = {false, false};
    mBoolMap["ScraperRetryPeerVerification"] = {false, false};

    // UI settings.
    mStringMap["StartupSystem"] = {"", ""};
    mStringMap["GamelistViewStyle"] = {"automatic", "automatic"};
    mStringMap["TransitionStyle"] = {"slide", "slide"};
    mStringMap["ThemeSet"] = {"rbsimple-DE", "rbsimple-DE"};
    mStringMap["UIMode"] = {"full", "full"};
    mStringMap["DefaultSortOrder"] = {"filename, ascending", "filename, ascending"};
    mStringMap["MenuOpeningEffect"] = {"scale-up", "scale-up"};
    mStringMap["LaunchScreenDuration"] = {"normal", "normal"};

    // UI settings -> media viewer settings.
    mBoolMap["MediaViewerKeepVideoRunning"] = {true, true};
    mBoolMap["MediaViewerStretchVideos"] = {false, false};
    mBoolMap["MediaViewerVideoScanlines"] = {true, true};
    mBoolMap["MediaViewerVideoBlur"] = {false, false};
    mBoolMap["MediaViewerScreenshotScanlines"] = {true, true};

    // UI settings -> screensaver settings.
    mIntMap["ScreensaverTimer"] = {5 * 60 * 1000, 5 * 60 * 1000}; // 5 minutes.
    mStringMap["ScreensaverType"] = {"video", "video"};
    mBoolMap["ScreensaverControls"] = {true, true};

    // UI settings -> screensaver settings -> slideshow screensaver settings.
    mIntMap["ScreensaverSwapImageTimeout"] = {10000, 10000};
    mBoolMap["ScreensaverStretchImages"] = {false, false};
    mBoolMap["ScreensaverSlideshowGameInfo"] = {true, true};
    mBoolMap["ScreensaverSlideshowScanlines"] = {true, true};
    mBoolMap["ScreensaverSlideshowCustomImages"] = {false, false};
    mBoolMap["ScreensaverSlideshowRecurse"] = {false, false};
    mStringMap["ScreensaverSlideshowImageDir"] = {"~/.emulationstation/slideshow/custom_images",
                                                  "~/.emulationstation/slideshow/custom_images"};

    // UI settings -> screensaver settings -> video screensaver settings.
    mIntMap["ScreensaverSwapVideoTimeout"] = {0, 0};
    mBoolMap["ScreensaverStretchVideos"] = {false, false};
    mBoolMap["ScreensaverVideoGameInfo"] = {true, true};
    mBoolMap["ScreensaverVideoScanlines"] = {true, true};
    mBoolMap["ScreensaverVideoBlur"] = {false, false};

    mBoolMap["MenuBlurBackground"] = {true, true};
    mBoolMap["GamelistVideoPillarbox"] = {true, true};
    mBoolMap["GamelistVideoScanlines"] = {false, false};
    mBoolMap["FoldersOnTop"] = {true, true};
    mBoolMap["FavoritesFirst"] = {true, true};
    mBoolMap["FavoritesStar"] = {true, true};
    mBoolMap["SpecialCharsASCII"] = {false, false};
    mBoolMap["ListScrollOverlay"] = {false, false};
    mBoolMap["VirtualKeyboard"] = {true, true};
    mBoolMap["FavoritesAddButton"] = {true, true};
    mBoolMap["RandomAddButton"] = {false, false};
    mBoolMap["GamelistFilters"] = {true, true};
    mBoolMap["QuickSystemSelect"] = {true, true};
    mBoolMap["ShowHelpPrompts"] = {true, true};
    mBoolMap["PlayVideosImmediately"] = {false, false};
    mBoolMap["EnableMenuKidMode"] = {false, false};

    // Sound settings.
    mIntMap["SoundVolumeNavigation"] = {80, 80};
    mIntMap["SoundVolumeVideos"] = {100, 100};
    mBoolMap["GamelistVideoAudio"] = {true, true};
    mBoolMap["MediaViewerVideoAudio"] = {true, true};
    mBoolMap["ScreensaverVideoAudio"] = {false, false};
    mBoolMap["NavigationSounds"] = {true, true};

    // Input device settings.
    mStringMap["InputControllerType"] = {"xbox", "xbox"};
    mBoolMap["InputOnlyFirstController"] = {false, false};

    // Game collection settings.
    mStringMap["CollectionSystemsAuto"] = {"", ""};
    mStringMap["CollectionSystemsCustom"] = {"", ""};
    mBoolMap["FavFirstCustom"] = {false, false};
    mBoolMap["FavStarCustom"] = {false, false};
    mBoolMap["UseCustomCollectionsSystem"] = {true, true};
    mBoolMap["CollectionShowSystemInfo"] = {true, true};

    // Other settings.
    mStringMap["MediaDirectory"] = {"", ""};
#if defined(_RPI_)
    mIntMap["MaxVRAM"] = {80, 80};
#else
    mIntMap["MaxVRAM"] = {256, 256};
#endif
    mIntMap["DisplayIndex"] = {1, 1};
#if defined(__unix__)
    mStringMap["FullscreenMode"] = {"normal", "normal"};
#endif
#if defined(BUILD_VLC_PLAYER)
#if defined(_RPI_)
    // As the FFmpeg video player is not HW accelerated, use VLC as default on this weak device.
    mStringMap["VideoPlayer"] = {"vlc", "vlc"};
#else
    mStringMap["VideoPlayer"] = {"ffmpeg", "ffmpeg"};
#endif
#endif
    mStringMap["ExitButtonCombo"] = {"F4", "F4"};
    mStringMap["SaveGamelistsMode"] = {"always", "always"};
#if defined(_WIN64)
    mBoolMap["HideTaskbar"] = {false, false};
#endif
    mBoolMap["RunInBackground"] = {false, false};
#if defined(_WIN64)
    mBoolMap["LaunchWorkaround"] = {true, true};
#endif
#if !defined(_RPI_)
    mBoolMap["VideoHardwareDecoding"] = {false, false};
#endif
    mBoolMap["VideoUpscaleFrameRate"] = {false, false};
    mBoolMap["AlternativeEmulatorPerGame"] = {true, true};
    mBoolMap["ShowHiddenFiles"] = {true, true};
    mBoolMap["ShowHiddenGames"] = {true, true};
    mBoolMap["CustomEventScripts"] = {false, false};
    mBoolMap["ParseGamelistOnly"] = {false, false};
#if defined(__unix__)
    mBoolMap["DisableComposition"] = {true, true};
#endif
    mBoolMap["DisplayGPUStatistics"] = {false, false};
// macOS requires root privileges to reboot and power off so it doesn't make much
// sense to enable this setting and menu entry for that operating system.
#if !defined(__APPLE__)
    mBoolMap["ShowQuitMenu"] = {false, false};
#endif

    //
    // Settings configured via command-line arguments.
    //

    // Options listed using --help
    mBoolMap["Debug"] = {false, false};
    mBoolMap["ForceFull"] = {false, false};
    mBoolMap["ForceKid"] = {false, false};
    mBoolMap["ForceKiosk"] = {false, false};
    mBoolMap["IgnoreGamelist"] = {false, false};
    mBoolMap["SplashScreen"] = {true, true};
    mBoolMap["VSync"] = {true, true};
#if !defined(_WIN64)
    mBoolMap["Windowed"] = {false, false};
#endif
    mIntMap["WindowWidth"] = {0, 0};
    mIntMap["WindowHeight"] = {0, 0};
    mIntMap["ScreenWidth"] = {0, 0};

    // Undocumented options.
    mIntMap["ScreenHeight"] = {0, 0};
    mIntMap["ScreenOffsetX"] = {0, 0};
    mIntMap["ScreenOffsetY"] = {0, 0};
    mIntMap["ScreenRotate"] = {0, 0};

    //
    // Settings that can be changed in es_settings.xml
    // but that are not configurable via the GUI.
    //

    mBoolMap["DebugSkipInputLogging"] = {false, false};
    mStringMap["ROMDirectory"] = {"", ""};
    mStringMap["UIMode_passkey"] = {"uuddlrlrba", "uuddlrlrba"};

    //
    // Hardcoded or program-internal settings.
    //

    mStringMap["ApplicationVersion"] = {"", ""};
    mBoolMap["DebugGrid"] = {false, false};
    mBoolMap["DebugText"] = {false, false};
    mBoolMap["DebugImage"] = {false, false};
    mBoolMap["SplashScreenProgress"] = {true, true};
    mIntMap["ScraperFilter"] = {0, 0};
}

template <typename K, typename V>
void saveMap(pugi::xml_document& doc, std::map<K, V>& map, const std::string& type)
{
    for (auto iter = map.cbegin(); iter != map.cend(); iter++) {
        // Key is on the "don't save" list, so don't save it.
        if (std::find(settingsSkipSaving.cbegin(), settingsSkipSaving.cend(), iter->first) !=
            settingsSkipSaving.cend()) {
            continue;
        }

        pugi::xml_node node = doc.append_child(type.c_str());
        node.append_attribute("name").set_value(iter->first.c_str());
        node.append_attribute("value").set_value(iter->second.second);
    }
}

void Settings::saveFile()
{
    LOG(LogDebug) << "Settings::saveFile(): Saving settings to es_settings.xml";
    const std::string path =
        Utils::FileSystem::getHomePath() + "/.emulationstation/es_settings.xml";

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
    // Prior to ES-DE v1.1, the configuration file had the .cfg suffix instead of .xml
    const std::string legacyConfigFile =
        Utils::FileSystem::getHomePath() + "/.emulationstation/es_settings.cfg";

    const std::string configFile =
        Utils::FileSystem::getHomePath() + "/.emulationstation/es_settings.xml";

    if (Utils::FileSystem::exists(legacyConfigFile) && !Utils::FileSystem::exists(configFile))
        Utils::FileSystem::copyFile(legacyConfigFile, configFile, false);

    if (!Utils::FileSystem::exists(configFile))
        return;

    pugi::xml_document doc;
#if defined(_WIN64)
    pugi::xml_parse_result result =
        doc.load_file(Utils::String::stringToWideString(configFile).c_str());
#else
    pugi::xml_parse_result result = doc.load_file(configFile.c_str());
#endif
    if (!result) {
        LOG(LogError) << "Could not parse the es_settings.xml file\n   " << result.description();
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

// Macro to create the get and set functions for the various data types.
#define SETTINGS_GETSET(type, mapName, getFunction, getDefaultFunction, setFunction)               \
    type Settings::getFunction(const std::string& name)                                            \
    {                                                                                              \
        if (mapName.find(name) == mapName.cend()) {                                                \
            LOG(LogError) << "Tried to use unset setting " << name;                                \
        }                                                                                          \
        return mapName[name].second;                                                               \
    }                                                                                              \
    type Settings::getDefaultFunction(const std::string& name)                                     \
    {                                                                                              \
        if (mapName.find(name) == mapName.cend()) {                                                \
            LOG(LogError) << "Tried to use unset setting " << name;                                \
        }                                                                                          \
        return mapName[name].first;                                                                \
    }                                                                                              \
    bool Settings::setFunction(const std::string& name, type value)                                \
    {                                                                                              \
        if (mapName.count(name) == 0 || mapName[name].second != value) {                           \
            mapName[name].second = value;                                                          \
                                                                                                   \
            if (std::find(settingsSkipSaving.cbegin(), settingsSkipSaving.cend(), name) ==         \
                settingsSkipSaving.cend())                                                         \
                mWasChanged = true;                                                                \
                                                                                                   \
            return true;                                                                           \
        }                                                                                          \
        return false;                                                                              \
    }

// Parameters for the macro defined above.
SETTINGS_GETSET(bool, mBoolMap, getBool, getDefaultBool, setBool);
SETTINGS_GETSET(int, mIntMap, getInt, getDefaultInt, setInt);
SETTINGS_GETSET(float, mFloatMap, getFloat, getDefaultFloat, setFloat);
SETTINGS_GETSET(const std::string&, mStringMap, getString, getDefaultString, setString);
