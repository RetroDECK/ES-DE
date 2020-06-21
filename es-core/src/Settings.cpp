//
//	Settings.cpp
//
//	Functions to read from and write to the configuration file es_settings.cfg.
//	The default values for the application settings are defined here as well.
//

#include "Settings.h"

#include "utils/FileSystemUtil.h"
#include "Log.h"
#include "Scripting.h"
#include "Platform.h"
#include <pugixml/src/pugixml.hpp>
#include <algorithm>
#include <vector>

Settings* Settings::sInstance = NULL;

// These values are NOT saved to es_settings.cfg since they're not set via
// the in-program settings menu. Most can be set using command-line arguments,
// but some are debug flags that are either hardcoded or set by internal debug
// functions.
std::vector<const char*> settings_dont_save {
	// These options can be set using command-line arguments:
	{ "Debug" },				// --debug
	{ "HideConsole" },			// Implicitly set via the --debug flag.
	{ "ForceKid" },				// --force-kid
	{ "ForceKiosk" },			// --force-kiosk
	{ "IgnoreGamelist" },		// --ignore-gamelist
	{ "ShowExit" },				// --no-exit
	{ "SplashScreen" },			// --no-splash
	{ "VSync" },				// --vsync [1/on or 0/off]
	{ "Windowed" },				// --windowed
	{ "WindowWidth" },			// Set via --resolution [width] [height]
	{ "WindowHeight" },			// set via --resolution [width] [height]

	// These options are not shown in the --help text and are intended
	// for debugging and testing purposes:
	{ "ScreenWidth" },			// Set via --screensize [width] [height]
	{ "ScreenHeight" },			// set via --screensize [width] [height]
	{ "ScreenOffsetX" },		// Set via --screenoffset [X] [Y]
	{ "ScreenOffsetY" },		// Set via --screenoffset [X] [Y]
	{ "ScreenRotate" },			// --screenrotate [0-3]

	// These options are not configurable from the command-line:
	{ "DebugGrid" },
	{ "DebugText" },
	{ "DebugImage" },
	{ "SplashScreenProgress" }
};

Settings::Settings()
{
	mWasChanged = false;
	setDefaults();
	loadFile();
}

Settings* Settings::getInstance()
{
	if (sInstance == NULL)
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

	// UI settings.
	mStringMap["StartupSystem"] = "";
	mStringMap["GamelistViewStyle"] = "automatic";
	mStringMap["TransitionStyle"] = "instant";
	mStringMap["ThemeSet"] = "";
	mStringMap["UIMode"] = "Full";
	mBoolMap["FavoritesFirst"] = true;
	mBoolMap["ForceDisableFilters"] = false;
	mBoolMap["QuickSystemSelect"] = true;
	mBoolMap["MoveCarousel"] = true;
	mBoolMap["DisableKidStartMenu"] = true;
	mBoolMap["ShowHelpPrompts"] = true;

	// UI settings -> scrensaver settings.
	mIntMap["ScreenSaverTime"] = 5*60*1000; // 5 minutes
	mBoolMap["ScreenSaverControls"] = true;
	mStringMap["ScreenSaverBehavior"] = "dim";

	// UI settings -> screensaver settings -> video screensaver settings.
	mIntMap["ScreenSaverSwapVideoTimeout"] = 30000;
	mBoolMap["StretchVideoOnScreenSaver"] = false;
	mStringMap["ScreenSaverGameInfo"] = "never";
	mBoolMap["ScreenSaverVideoMute"] = false;			// Raspberry Pi only
	mBoolMap["CaptionsCompatibility"] = true;

	// UI settings -> screensaver settings -> slideshow screensaver settings.
	mIntMap["ScreenSaverSwapImageTimeout"] = 10000;
	mBoolMap["SlideshowScreenSaverStretch"] = false;
	mStringMap["SlideshowScreenSaverBackgroundAudioFile"] = Utils::FileSystem::getHomePath() +
			"/.emulationstation/slideshow/audio/slideshow_bg.wav";
	mBoolMap["SlideshowScreenSaverCustomImageSource"] = false;
	mStringMap["SlideshowScreenSaverImageDir"] = Utils::FileSystem::getHomePath() +
			"/.emulationstation/slideshow/image";
	mBoolMap["SlideshowScreenSaverRecurse"] = false;
	mStringMap["SlideshowScreenSaverImageFilter"] = ".png,.jpg";

	// Sound settings.
	mStringMap["AudioCard"] = "default";
	// Audio out device for volume control.
	#ifdef _RPI_
		mStringMap["AudioDevice"] = "PCM";
	#else
		mStringMap["AudioDevice"] = "Master";
	#endif
	mBoolMap["VideoAudio"] = true;
	mBoolMap["EnableSounds"] = true;
	// Audio out device for Video playback using OMX player.
	mStringMap["OMXAudioDev"] = "both";

	// Game collection settings.
	mStringMap["CollectionSystemsAuto"] = "";
	mStringMap["CollectionSystemsCustom"] = "";
	mBoolMap["UseCustomCollectionsSystem"] = true;
	mBoolMap["FavFirstCustom"] = true;
	mBoolMap["CollectionShowSystemInfo"] = true;

	// Scraper.
	mStringMap["Scraper"] = "ScreenScraper";
	mStringMap["ScraperRegion"] = "eu";
	mStringMap["ScraperLanguage"] = "en";
//	mBoolMap["ScraperGenerateMiximages"] = false;
//	mBoolMap["ScraperGenerateThumbnails"] = false;
	mBoolMap["ScraperInteractive"] = true;
	mBoolMap["ScraperSemiautomatic"] = true;
	mBoolMap["ScraperOverwriteData"] = false;
	mBoolMap["ScrapeMetadata"] = true;
	mBoolMap["ScrapeGameNames"] = true;
	mBoolMap["ScrapeRatings"] = true;
	mBoolMap["Scrape3DBoxes"] = true;
	mBoolMap["ScrapeCovers"] = true;
	mBoolMap["ScrapeMarquees"] = true;
	mBoolMap["ScrapeScreenshots"] = true;

	// Other settings.
	#ifdef _RPI_
		mIntMap["MaxVRAM"] = 80;
	#else
		mIntMap["MaxVRAM"] = 100;
	#endif
	mStringMap["FullscreenMode"] = "normal";
	mStringMap["PowerSaverMode"] = "disabled";
	// This setting only applies to raspberry pi but set it for all platforms so
	// we don't get a warning if we encounter it on a different platform.
	mBoolMap["VideoOmxPlayer"] = false;
	#ifdef _RPI_
	// We're defaulting to OMX Player for full screen video on the Pi.
	mBoolMap["ScreenSaverOmxPlayer"] = true;
	// Use OMX Player defaults.
	mStringMap["SubtitleFont"] = "/usr/share/fonts/truetype/freefont/FreeSans.ttf";
	mStringMap["SubtitleItalicFont"] = "/usr/share/fonts/truetype/freefont/FreeSansOblique.ttf";
	mIntMap["SubtitleSize"] = 55;
	mStringMap["SubtitleAlignment"] = "left";
	#else
	mBoolMap["ScreenSaverOmxPlayer"] = false;
	#endif
	mStringMap["SaveGamelistsMode"] = "always";
	mBoolMap["LaunchstringOverride"] = true;
	mBoolMap["ParseGamelistOnly"] = false;
	mBoolMap["LocalArt"] = false;
	mBoolMap["ShowHiddenFiles"] = false;
	mBoolMap["DrawFramerate"] = false;
	mBoolMap["ShowRebootSystem"] = true;
	mBoolMap["ShowPoweroffSystem"] = true;

	//
	// Settings configured via command-line arguments.
	//

	// Options listed using --help
	mBoolMap["Debug"] = false;
	mBoolMap["HideConsole"] = true; // Implicitly set via the --debug flag.
	mBoolMap["ForceKid"] = false;
	mBoolMap["ForceKiosk"] = false;
	mBoolMap["IgnoreGamelist"] = false;
	mBoolMap["ShowExit"] = true;
	mBoolMap["SplashScreen"] = true;
	mBoolMap["VSync"] = true;
	mBoolMap["Windowed"] = false;
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
	// but that are not configurable via the GUI (yet).
	//

	mStringMap["DefaultSortOrder"] = "filename, ascending";
	mStringMap["MediaDirectory"] = "";
	mStringMap["ROMDirectory"] = "";
	mIntMap["ScraperResizeWidth"] = 600;
	mIntMap["ScraperResizeHeight"] = 0;

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
	LOG(LogDebug) << "Settings::saveFile() : Saving Settings to file.";
	const std::string path = Utils::FileSystem::getHomePath() +
			"/.emulationstation/es_settings.cfg";

	pugi::xml_document doc;

	saveMap<std::string, bool>(doc, mBoolMap, "bool");
	saveMap<std::string, int>(doc, mIntMap, "int");
	saveMap<std::string, float>(doc, mFloatMap, "float");

	//saveMap<std::string, std::string>(doc, mStringMap, "string");
	for (auto iter = mStringMap.cbegin(); iter != mStringMap.cend(); iter++) {
		pugi::xml_node node = doc.append_child("string");
		node.append_attribute("name").set_value(iter->first.c_str());
		node.append_attribute("value").set_value(iter->second.c_str());
	}

	doc.save_file(path.c_str());

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
	pugi::xml_parse_result result = doc.load_file(path.c_str());
	if (!result) {
		LOG(LogError) << "Could not parse Settings file!\n   " << result.description();
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
