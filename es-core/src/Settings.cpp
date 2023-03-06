//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Settings.cpp
//
//  Functions to read from and write to the configuration file es_settings.xml.
//  The default values for the application settings are defined here as well.
//  This class is not thread safe.
//

#include "Settings.h"

#include "GuiComponent.h"
#include "Log.h"
#include "Scripting.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

#include <algorithm>
#include <pugixml.hpp>
#include <vector>

namespace
{
    // These settings are not saved to es_settings.xml. Most can be set using command-line
    // arguments but a couple are debug flags or used for other application-internal purposes.
    std::vector<std::string> settingsSkipSaving {
        // clang-format off
        // These options can be set using command-line arguments:
        "ScreenWidth",          // Set via --resolution [width] [height]
        "ScreenHeight",         // set via --resolution [width] [height]
        "ScreenOffsetX",        // Set via --screenoffset [horiz.] [vert.]
        "ScreenOffsetY",        // Set via --screenoffset [horiz.] [vert.]
        "FullscreenPadding",    // Set via --fullscreen-padding [1/on or 0/off]
        "VSync",                // --vsync [1/on or 0/off]
        "IgnoreGamelist",       // --ignore-gamelist
        "SplashScreen",         // --no-splash
        "ForceFull",            // --force-full
        "ForceKiosk",           // --force-kiosk
        "ForceKid",             // --force-kid
        "Debug",                // --debug

        // These options are only used internally during the application session:
        "PortableMode",
        "DebugGrid",
        "DebugText",
        "DebugImage",
        "ScraperFilter",
        "TransitionsSystemToSystem",
        "TransitionsSystemToGamelist",
        "TransitionsGamelistToGamelist",
        "TransitionsGamelistToSystem",
        "TransitionsStartupToSystem",
        "TransitionsStartupToGamelist"
        // clang-format on
    };

    template <typename K, typename V>
    void saveMap(pugi::xml_document& doc, std::map<K, V>& map, const std::string& type)
    {
        for (auto it = map.cbegin(); it != map.cend(); ++it) {
            // Key is on the "don't save" list, so don't save it.
            if (std::find(settingsSkipSaving.cbegin(), settingsSkipSaving.cend(), it->first) !=
                settingsSkipSaving.cend()) {
                continue;
            }

            pugi::xml_node node = doc.append_child(type.c_str());
            node.append_attribute("name").set_value(it->first.c_str());
            node.append_attribute("value").set_value(it->second.second);
        }
    }
} // namespace

Settings::Settings()
{
    mWasChanged = false;
    setDefaults();
    loadFile();
}

Settings* Settings::getInstance()
{
    static Settings instance;
    return &instance;
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
    mStringMap["ScraperUsernameScreenScraper"] = {"", ""};
    mStringMap["ScraperPasswordScreenScraper"] = {"", ""};
    mBoolMap["ScraperUseAccountScreenScraper"] = {true, true};

    mBoolMap["ScrapeGameNames"] = {true, true};
    mBoolMap["ScrapeRatings"] = {true, true};
    // ScreenScraper controller scraping is currently broken, it's unclear if they will fix it.
    // mBoolMap["ScrapeControllers"] = {true, true};
    mBoolMap["ScrapeMetadata"] = {true, true};
    mBoolMap["ScrapeVideos"] = {true, true};
    mBoolMap["ScrapeScreenshots"] = {true, true};
    mBoolMap["ScrapeTitleScreens"] = {true, true};
    mBoolMap["ScrapeCovers"] = {true, true};
    mBoolMap["ScrapeBackCovers"] = {true, true};
    mBoolMap["ScrapeMarquees"] = {true, true};
    mBoolMap["Scrape3DBoxes"] = {true, true};
    mBoolMap["ScrapePhysicalMedia"] = {true, true};
    mBoolMap["ScrapeFanArt"] = {true, true};

    mStringMap["MiximageResolution"] = {"1280x960", "1280x960"};
    mStringMap["MiximageScreenshotScaling"] = {"sharp", "sharp"};
    mStringMap["MiximageBoxSize"] = {"medium", "medium"};
    mStringMap["MiximagePhysicalMediaSize"] = {"medium", "medium"};
    mBoolMap["MiximageGenerate"] = {true, true};
    mBoolMap["MiximageOverwrite"] = {true, true};
    mBoolMap["MiximageRemoveLetterboxes"] = {true, true};
    mBoolMap["MiximageRemovePillarboxes"] = {true, true};
    mBoolMap["MiximageRotateHorizontalBoxes"] = {true, true};
    mBoolMap["MiximageIncludeMarquee"] = {true, true};
    mBoolMap["MiximageIncludeBox"] = {true, true};
    mBoolMap["MiximageCoverFallback"] = {true, true};
    mBoolMap["MiximageIncludePhysicalMedia"] = {true, true};

    mStringMap["ScraperRegion"] = {"eu", "eu"};
    mStringMap["ScraperLanguage"] = {"en", "en"};
    mIntMap["ScraperRetryOnErrorCount"] = {3, 3};
    mIntMap["ScraperRetryOnErrorTimer"] = {3, 3};
    mBoolMap["ScraperOverwriteData"] = {true, true};
    mBoolMap["ScraperHaltOnInvalidMedia"] = {true, true};
    mBoolMap["ScraperSearchMetadataName"] = {true, true};
    mBoolMap["ScraperIncludeFolders"] = {true, true};
    mBoolMap["ScraperInteractive"] = {false, false};
    mBoolMap["ScraperSemiautomatic"] = {true, true};
    mBoolMap["ScraperRespectExclusions"] = {true, true};
    mBoolMap["ScraperExcludeRecursively"] = {true, true};
    mBoolMap["ScraperConvertUnderscores"] = {true, true};
    mBoolMap["ScraperAutomaticRemoveDots"] = {true, true};
    mBoolMap["ScraperRegionFallback"] = {true, true};

    // UI settings.
    mStringMap["ThemeSet"] = {"slate-es-de", "slate-es-de"};
    mStringMap["ThemeVariant"] = {"", ""};
    mStringMap["ThemeColorScheme"] = {"", ""};
    mStringMap["ThemeAspectRatio"] = {"", ""};
    mStringMap["ThemeTransitions"] = {"automatic", "automatic"};
    mStringMap["GamelistViewStyle"] = {"automatic", "automatic"};
    mStringMap["LegacyThemeTransitions"] = {"builtin-instant", "builtin-instant"};
    mStringMap["QuickSystemSelect"] = {"leftrightshoulders", "leftrightshoulders"};
    mStringMap["StartupSystem"] = {"", ""};
    mStringMap["DefaultSortOrder"] = {"filename, ascending", "filename, ascending"};
    mStringMap["MenuOpeningEffect"] = {"scale-up", "scale-up"};
    mStringMap["LaunchScreenDuration"] = {"normal", "normal"};
    mStringMap["UIMode"] = {"full", "full"};
    mStringMap["RandomEntryButton"] = {"games", "games"};

    // UI settings -> media viewer settings.
    mBoolMap["MediaViewerKeepVideoRunning"] = {true, true};
    mBoolMap["MediaViewerStretchVideos"] = {false, false};
#if defined(RASPBERRY_PI)
    mBoolMap["MediaViewerVideoScanlines"] = {false, false};
#else
    mBoolMap["MediaViewerVideoScanlines"] = {true, true};
#endif
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
#if defined(RASPBERRY_PI)
    mBoolMap["ScreensaverVideoScanlines"] = {false, false};
#else
    mBoolMap["ScreensaverVideoScanlines"] = {true, true};
#endif
    mBoolMap["ScreensaverVideoBlur"] = {false, false};

    mBoolMap["ThemeVariantTriggers"] = {true, true};
    mBoolMap["MenuBlurBackground"] = {true, true};
    mBoolMap["GamelistVideoPillarbox"] = {true, true};
    mBoolMap["GamelistVideoScanlines"] = {false, false};
    mBoolMap["FoldersOnTop"] = {true, true};
    mBoolMap["FavoritesFirst"] = {true, true};
    mBoolMap["FavoritesStar"] = {true, true};
    mBoolMap["ListScrollOverlay"] = {false, false};
    mBoolMap["VirtualKeyboard"] = {true, true};
    mBoolMap["FavoritesAddButton"] = {true, true};
    mBoolMap["GamelistFilters"] = {true, true};
    mBoolMap["ShowHelpPrompts"] = {true, true};

    // Sound settings.
    mIntMap["SoundVolumeNavigation"] = {70, 70};
    mIntMap["SoundVolumeVideos"] = {80, 80};
    mBoolMap["ViewsVideoAudio"] = {true, true};
    mBoolMap["MediaViewerVideoAudio"] = {true, true};
    mBoolMap["ScreensaverVideoAudio"] = {true, true};
    mBoolMap["NavigationSounds"] = {true, true};

    // Input device settings.
    mStringMap["InputControllerType"] = {"xbox", "xbox"};
    mBoolMap["InputOnlyFirstController"] = {false, false};
    mBoolMap["InputIgnoreKeyboard"] = {false, false};

    // Game collection settings.
    mStringMap["CollectionSystemsAuto"] = {"", ""};
    mStringMap["CollectionSystemsCustom"] = {"", ""};
    mStringMap["CollectionCustomGrouping"] = {"unthemed", "unthemed"};
    mBoolMap["FavFirstCustom"] = {false, false};
    mBoolMap["FavStarCustom"] = {false, false};

    // Other settings.
    mStringMap["MediaDirectory"] = {"", ""};
#if defined(STEAM_DECK) || defined(RETRODECK)
    mIntMap["MaxVRAM"] = {512, 512};
#elif defined(RASPBERRY_PI)
    mIntMap["MaxVRAM"] = {192, 192};
#else
    mIntMap["MaxVRAM"] = {512, 512};
#endif
#if !defined(USE_OPENGLES)
    mIntMap["AntiAliasing"] = {0, 0};
#endif
    mIntMap["DisplayIndex"] = {1, 1};
    mIntMap["ScreenRotate"] = {0, 0};
#if defined(__APPLE__)
    mStringMap["KeyboardQuitShortcut"] = {"CmdQ", "CmdQ"};
#else
    mStringMap["KeyboardQuitShortcut"] = {"AltF4", "AltF4"};
#endif
    mStringMap["SaveGamelistsMode"] = {"always", "always"};
    mStringMap["ApplicationUpdaterFrequency"] = {"always", "always"};
    mBoolMap["ApplicationUpdaterPrereleases"] = {false, false};
#if defined(_WIN64)
    mBoolMap["HideTaskbar"] = {false, false};
#endif
    mBoolMap["RunInBackground"] = {false, false};
#if defined(VIDEO_HW_DECODING)
    mBoolMap["VideoHardwareDecoding"] = {false, false};
#endif
#if defined(STEAM_DECK) || defined(RETRODECK)
    mBoolMap["VideoUpscaleFrameRate"] = {true, true};
#else
    mBoolMap["VideoUpscaleFrameRate"] = {false, false};
#endif
    mBoolMap["AlternativeEmulatorPerGame"] = {true, true};
    mBoolMap["ShowHiddenFiles"] = {true, true};
    mBoolMap["ShowHiddenGames"] = {true, true};
    mBoolMap["CustomEventScripts"] = {false, false};
    mBoolMap["ParseGamelistOnly"] = {false, false};
    mBoolMap["MAMENameStripExtraInfo"] = {true, true};
#if defined(__unix__)
    mBoolMap["DisableComposition"] = {false, false};
#endif
    mBoolMap["DisplayGPUStatistics"] = {false, false};
    mBoolMap["EnableMenuKidMode"] = {false, false};
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
    mBoolMap["FullscreenPadding"] = {false, false};
    mIntMap["ScreenWidth"] = {0, 0};
    mIntMap["ScreenHeight"] = {0, 0};
    mIntMap["ScreenOffsetX"] = {0, 0};
    mIntMap["ScreenOffsetY"] = {0, 0};

    //
    // Settings that can be changed in es_settings.xml
    // but that are not configurable via the GUI.
    //

    mBoolMap["DebugSkipInputLogging"] = {false, false};
    mBoolMap["DebugSkipMissingThemeFiles"] = {false, false};
    mBoolMap["DebugSkipMissingThemeFilesCustomCollections"] = {true, true};
    mBoolMap["LegacyGamelistFileLocation"] = {false, false};
    mStringMap["OpenGLVersion"] = {"", ""};
    mStringMap["ROMDirectory"] = {"", ""};
    mStringMap["UIMode_passkey"] = {"uuddlrlrba", "uuddlrlrba"};
    mIntMap["LottieMaxFileCache"] = {150, 150};
    mIntMap["LottieMaxTotalCache"] = {1024, 1024};
    mIntMap["ScraperConnectionTimeout"] = {30, 30};
    mIntMap["ScraperTransferTimeout"] = {120, 120};

    //
    // Hardcoded or program-internal settings.
    //

    mStringMap["ApplicationVersion"] = {"", ""};
    mStringMap["ApplicationUpdaterLastCheck"] = {"", ""};
    mBoolMap["PortableMode"] = {false, false};
    mBoolMap["DebugGrid"] = {false, false};
    mBoolMap["DebugText"] = {false, false};
    mBoolMap["DebugImage"] = {false, false};
    mIntMap["ScraperFilter"] = {0, 0};
    mIntMap["TransitionsSystemToSystem"] = {ViewTransitionAnimation::INSTANT,
                                            ViewTransitionAnimation::INSTANT};
    mIntMap["TransitionsSystemToGamelist"] = {ViewTransitionAnimation::INSTANT,
                                              ViewTransitionAnimation::INSTANT};
    mIntMap["TransitionsGamelistToGamelist"] = {ViewTransitionAnimation::INSTANT,
                                                ViewTransitionAnimation::INSTANT};
    mIntMap["TransitionsGamelistToSystem"] = {ViewTransitionAnimation::INSTANT,
                                              ViewTransitionAnimation::INSTANT};
    mIntMap["TransitionsStartupToSystem"] = {ViewTransitionAnimation::INSTANT,
                                             ViewTransitionAnimation::INSTANT};
    mIntMap["TransitionsStartupToGamelist"] = {ViewTransitionAnimation::INSTANT,
                                               ViewTransitionAnimation::INSTANT};
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

    for (auto it = mStringMap.cbegin(); it != mStringMap.cend(); ++it) {
        if (std::find(settingsSkipSaving.cbegin(), settingsSkipSaving.cend(), it->first) !=
            settingsSkipSaving.cend()) {
            continue;
        }
        pugi::xml_node node = doc.append_child("string");
        node.append_attribute("name").set_value(it->first.c_str());
        node.append_attribute("value").set_value(it->second.second.c_str());
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
    const std::string configFile {Utils::FileSystem::getHomePath() +
                                  "/.emulationstation/es_settings.xml"};

    if (!Utils::FileSystem::exists(configFile))
        return;

    pugi::xml_document doc;
#if defined(_WIN64)
    pugi::xml_parse_result result {
        doc.load_file(Utils::String::stringToWideString(configFile).c_str())};
#else
    pugi::xml_parse_result result {doc.load_file(configFile.c_str())};
#endif
    if (!result) {
        LOG(LogError) << "Couldn't parse the es_settings.xml file: " << result.description();
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
SETTINGS_GETSET(bool, mBoolMap, getBool, getDefaultBool, setBool)
SETTINGS_GETSET(int, mIntMap, getInt, getDefaultInt, setInt)
SETTINGS_GETSET(float, mFloatMap, getFloat, getDefaultFloat, setFloat)
SETTINGS_GETSET(const std::string&, mStringMap, getString, getDefaultString, setString)
