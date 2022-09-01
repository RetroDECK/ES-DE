//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ThemeData.cpp
//
//  Finds available themes on the file system and loads and parses these.
//  Basic error checking for valid elements and data types is done here,
//  with additional validation handled by the individual components.
//

#include "ThemeData.h"

#include "Log.h"
#include "Settings.h"
#include "components/ImageComponent.h"
#include "components/TextComponent.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

#include <algorithm>
#include <pugixml.hpp>

#define MINIMUM_LEGACY_THEME_FORMAT_VERSION 3

// clang-format off
std::vector<std::string> ThemeData::sSupportedViews {
    {"all"},
    {"system"},
    {"gamelist"}};

std::vector<std::string> ThemeData::sLegacySupportedViews {
    {"all"},
    {"system"},
    {"basic"},
    {"detailed"},
    {"grid"},
    {"video"}};

std::vector<std::string> ThemeData::sLegacySupportedFeatures {
    {"navigationsounds"},
    {"video"},
    {"carousel"},
    {"z-index"},
    {"visible"}};

std::vector<std::string> ThemeData::sLegacyElements {
    {"showSnapshotNoVideo"},
    {"showSnapshotDelay"},
    {"forceUppercase"},
    {"alignment"},
    {"defaultLogo"},
    {"logoSize"},
    {"logoScale"},
    {"logoRotation"},
    {"logoRotationOrigin"},
    {"logoAlignment"},
    {"maxLogoCount"}};

std::vector<std::pair<std::string, std::string>> ThemeData::sSupportedAspectRatios {
    {"16:9", "16:9"},
    {"16:9_vertical", "16:9 vertical"},
    {"16:10", "16:10"},
    {"16:10_vertical", "16:10 vertical"},
    {"3:2", "3:2"},
    {"3:2_vertical", "3:2 vertical"},
    {"4:3", "4:3"},
    {"4:3_vertical", "4:3 vertical"},
    {"5:4", "5:4"},
    {"5:4_vertical", "5:4 vertical"},
    {"21:9", "21:9"},
    {"21:9_vertical", "21:9 vertical"},
    {"32:9", "32:0"},
    {"32:9_vertical", "32:9 vertical"}};

std::map<std::string, std::map<std::string, std::string>> ThemeData::sPropertyAttributeMap
    // The data type is defined by the parent property.
    {
     {"badges",
      {{"customBadgeIcon", "badge"},
       {"customControllerIcon", "controller"}}},
     {"helpsystem",
      {{"customButtonIcon", "button"}}},
    };

std::map<std::string, std::map<std::string, ThemeData::ElementPropertyType>>
    ThemeData::sElementMap {
     {"image",
      {{"pos", NORMALIZED_PAIR},
       {"size", NORMALIZED_PAIR},
       {"maxSize", NORMALIZED_PAIR},
       {"origin", NORMALIZED_PAIR},
       {"rotation", FLOAT},
       {"rotationOrigin", NORMALIZED_PAIR},
       {"path", PATH},
       {"default", PATH},
       {"imageType", STRING},
       {"metadataElement", BOOLEAN},
       {"gameselector", STRING},
       {"tile", BOOLEAN},
       {"interpolation", STRING},
       {"color", COLOR},
       {"colorEnd", COLOR},
       {"gradientType", STRING},
       {"scrollFadeIn", BOOLEAN},
       {"opacity", FLOAT},
       {"saturation", FLOAT},
       {"visible", BOOLEAN},
       {"zIndex", FLOAT}}},
     {"video",
      {{"pos", NORMALIZED_PAIR},
       {"size", NORMALIZED_PAIR},
       {"maxSize", NORMALIZED_PAIR},
       {"origin", NORMALIZED_PAIR},
       {"metadataElement", BOOLEAN},
       {"path", PATH},
       {"default", PATH},
       {"defaultImage", PATH},
       {"imageType", STRING},
       {"gameselector", STRING},
       {"audio", BOOLEAN},
       {"interpolation", STRING},
       {"pillarboxes", BOOLEAN},
       {"scanlines", BOOLEAN},
       {"delay", FLOAT},
       {"fadeInTime", FLOAT},
       {"scrollFadeIn", BOOLEAN},
       {"opacity", FLOAT},
       {"saturation", FLOAT},
       {"visible", BOOLEAN},
       {"zIndex", FLOAT},
       {"showSnapshotNoVideo", BOOLEAN},           // For backward compatibility with legacy themes.
       {"showSnapshotDelay", BOOLEAN}}},           // For backward compatibility with legacy themes.
     {"animation",
      {{"pos", NORMALIZED_PAIR},
       {"size", NORMALIZED_PAIR},
       {"origin", NORMALIZED_PAIR},
       {"rotation", FLOAT},
       {"rotationOrigin", NORMALIZED_PAIR},
       {"metadataElement", BOOLEAN},
       {"path", PATH},
       {"speed", FLOAT},
       {"direction", STRING},
       {"keepAspectRatio", BOOLEAN},
       {"interpolation", STRING},
       {"opacity", FLOAT},
       {"saturation", FLOAT},
       {"visible", BOOLEAN},
       {"zIndex", FLOAT}}},
     {"badges",
      {{"pos", NORMALIZED_PAIR},
       {"size", NORMALIZED_PAIR},
       {"origin", NORMALIZED_PAIR},
       {"rotation", FLOAT},
       {"rotationOrigin", NORMALIZED_PAIR},
       {"horizontalAlignment", STRING},
       {"alignment", STRING},                      // For backward compatibility with legacy themes.
       {"direction", STRING},
       {"lines", UNSIGNED_INTEGER},
       {"itemsPerLine", UNSIGNED_INTEGER},
       {"itemMargin", NORMALIZED_PAIR},
       {"slots", STRING},
       {"controllerPos", NORMALIZED_PAIR},
       {"controllerSize", FLOAT},
       {"customBadgeIcon", PATH},
       {"customControllerIcon", PATH},
       {"folderLinkPos", NORMALIZED_PAIR},
       {"folderLinkSize", FLOAT},
       {"customFolderLinkIcon", PATH},
       {"opacity", FLOAT},
       {"visible", BOOLEAN},
       {"zIndex", FLOAT}}},
     {"text",
      {{"pos", NORMALIZED_PAIR},
       {"size", NORMALIZED_PAIR},
       {"origin", NORMALIZED_PAIR},
       {"rotation", FLOAT},
       {"rotationOrigin", NORMALIZED_PAIR},
       {"text", STRING},
       {"systemdata", STRING},
       {"metadata", STRING},
       {"metadataElement", BOOLEAN},
       {"gameselector", STRING},
       {"container", BOOLEAN},
       {"containerScrollSpeed", FLOAT},
       {"containerStartDelay", FLOAT},
       {"containerResetDelay", FLOAT},
       {"fontPath", PATH},
       {"fontSize", FLOAT},
       {"horizontalAlignment", STRING},
       {"verticalAlignment", STRING},
       {"alignment", STRING},                      // For backward compatibility with legacy themes.
       {"color", COLOR},
       {"backgroundColor", COLOR},
       {"letterCase", STRING},
       {"forceUppercase", BOOLEAN},                // For backward compatibility with legacy themes.
       {"lineSpacing", FLOAT},
       {"opacity", FLOAT},
       {"visible", BOOLEAN},
       {"zIndex", FLOAT}}},
     {"datetime",
      {{"pos", NORMALIZED_PAIR},
       {"size", NORMALIZED_PAIR},
       {"origin", NORMALIZED_PAIR},
       {"rotation", FLOAT},
       {"rotationOrigin", NORMALIZED_PAIR},
       {"metadata", STRING},
       {"gameselector", STRING},
       {"fontPath", PATH},
       {"fontSize", FLOAT},
       {"horizontalAlignment", STRING},
       {"verticalAlignment", STRING},
       {"alignment", STRING},                      // For backward compatibility with legacy themes.
       {"color", COLOR},
       {"backgroundColor", COLOR},
       {"letterCase", STRING},
       {"forceUppercase", BOOLEAN},                // For backward compatibility with legacy themes.
       {"lineSpacing", FLOAT},
       {"format", STRING},
       {"displayRelative", BOOLEAN},
       {"opacity", FLOAT},
       {"visible", BOOLEAN},
       {"zIndex", FLOAT}}},
     {"gamelistinfo",
      {{"pos", NORMALIZED_PAIR},
       {"size", NORMALIZED_PAIR},
       {"origin", NORMALIZED_PAIR},
       {"rotation", FLOAT},
       {"rotationOrigin", NORMALIZED_PAIR},
       {"fontPath", PATH},
       {"fontSize", FLOAT},
       {"color", COLOR},
       {"backgroundColor", COLOR},
       {"horizontalAlignment", STRING},
       {"verticalAlignment", STRING},
       {"alignment", STRING},                      // For backward compatibility with legacy themes.
       {"opacity", FLOAT},
       {"visible", BOOLEAN},
       {"zIndex", FLOAT}}},
     {"rating",
      {{"pos", NORMALIZED_PAIR},
       {"size", NORMALIZED_PAIR},
       {"origin", NORMALIZED_PAIR},
       {"rotation", FLOAT},
       {"rotationOrigin", NORMALIZED_PAIR},
       {"gameselector", STRING},
       {"interpolation", STRING},
       {"color", COLOR},
       {"filledPath", PATH},
       {"unfilledPath", PATH},
       {"overlay", BOOLEAN},
       {"opacity", FLOAT},
       {"visible", BOOLEAN},
       {"zIndex", FLOAT}}},
     {"carousel",
      {{"type", STRING},
       {"size", NORMALIZED_PAIR},
       {"pos", NORMALIZED_PAIR},
       {"origin", NORMALIZED_PAIR},
       {"color", COLOR},
       {"colorEnd", COLOR},
       {"gradientType", STRING},
       {"staticItem", PATH},
       {"itemType", STRING},
       {"defaultItem", PATH},
       {"itemSize", NORMALIZED_PAIR},
       {"itemInterpolation", STRING},
       {"itemScale", FLOAT},
       {"itemRotation", FLOAT},
       {"itemRotationOrigin", NORMALIZED_PAIR},
       {"itemHorizontalAlignment", STRING},
       {"itemVerticalAlignment", STRING},
       {"wheelHorizontalAlignment", STRING},
       {"horizontalOffset", FLOAT},
       {"verticalOffset", FLOAT},
       {"reflections", BOOLEAN},
       {"reflectionsOpacity", FLOAT},
       {"reflectionsFalloff", FLOAT},
       {"unfocusedItemOpacity", FLOAT},
       {"maxItemCount", FLOAT},
       {"defaultLogo", PATH},                      // For backward compatibility with legacy themes.
       {"logoSize", NORMALIZED_PAIR},              // For backward compatibility with legacy themes.
       {"logoScale", FLOAT},                       // For backward compatibility with legacy themes.
       {"logoRotation", FLOAT},                    // For backward compatibility with legacy themes.
       {"logoRotationOrigin", NORMALIZED_PAIR},    // For backward compatibility with legacy themes.
       {"logoAlignment", STRING},                  // For backward compatibility with legacy themes.
       {"maxLogoCount", FLOAT},                    // For backward compatibility with legacy themes.
       {"text", STRING},
       {"textColor", COLOR},
       {"textBackgroundColor", COLOR},
       {"letterCase", STRING},
       {"fontPath", PATH},
       {"fontSize", FLOAT},
       {"lineSpacing", FLOAT},
       {"zIndex", FLOAT},
       {"legacyZIndexMode", STRING}}},             // For backward compatibility with legacy themes.
     {"textlist",
      {{"pos", NORMALIZED_PAIR},
       {"size", NORMALIZED_PAIR},
       {"origin", NORMALIZED_PAIR},
       {"selectorHeight", FLOAT},
       {"selectorOffsetY", FLOAT},
       {"selectorColor", COLOR},
       {"selectorColorEnd", COLOR},
       {"selectorGradientType", STRING},
       {"selectorImagePath", PATH},
       {"selectorImageTile", BOOLEAN},
       {"selectedColor", COLOR},
       {"primaryColor", COLOR},
       {"secondaryColor", COLOR},
       {"fontPath", PATH},
       {"fontSize", FLOAT},
       {"scrollSound", PATH},                      // For backward compatibility with legacy themes.
       {"horizontalAlignment", STRING},
       {"alignment", STRING},                      // For backward compatibility with legacy themes.
       {"horizontalMargin", FLOAT},
       {"letterCase", STRING},
       {"forceUppercase", BOOLEAN},                // For backward compatibility with legacy themes.
       {"lineSpacing", FLOAT},
       {"indicators", STRING},
       {"collectionIndicators", STRING},
       {"zIndex", FLOAT}}},
     {"gameselector",
      {{"selection", STRING},
       {"gameCount", UNSIGNED_INTEGER}}},
     {"helpsystem",
      {{"pos", NORMALIZED_PAIR},
       {"origin", NORMALIZED_PAIR},
       {"textColor", COLOR},
       {"textColorDimmed", COLOR},
       {"iconColor", COLOR},
       {"iconColorDimmed", COLOR},
       {"fontPath", PATH},
       {"fontSize", FLOAT},
       {"entrySpacing", FLOAT},
       {"iconTextSpacing", FLOAT},
       {"letterCase", STRING},
       {"textStyle", STRING},                      // For backward compatibility with legacy themes.
       {"opacity", FLOAT},
       {"customButtonIcon", PATH}}},
     {"sound",
      {{"path", PATH}}},
     {"navigationsounds",
      {{"systembrowseSound", PATH},
       {"quicksysselectSound", PATH},
       {"selectSound", PATH},
       {"backSound", PATH},
       {"scrollSound", PATH},
       {"favoriteSound", PATH},
       {"launchSound", PATH}}},
     // Legacy components below, not in use any longer but needed for backward compatibility.
     {"imagegrid",
      {{"pos", NORMALIZED_PAIR},
       {"size", NORMALIZED_PAIR},
       {"margin", NORMALIZED_PAIR},
       {"padding", NORMALIZED_RECT},
       {"autoLayout", NORMALIZED_PAIR},
       {"autoLayoutSelectedZoom", FLOAT},
       {"gameImage", PATH},
       {"folderImage", PATH},
       {"imageSource", STRING},
       {"scrollDirection", STRING},
       {"centerSelection", BOOLEAN},
       {"scrollLoop", BOOLEAN},
       {"animate", BOOLEAN},
       {"zIndex", FLOAT}}},
     {"gridtile",
      {{"size", NORMALIZED_PAIR},
       {"padding", NORMALIZED_PAIR},
       {"imageColor", COLOR},
       {"backgroundImage", PATH},
       {"backgroundCornerSize", NORMALIZED_PAIR},
       {"backgroundColor", COLOR},
       {"backgroundCenterColor", COLOR},
       {"backgroundEdgeColor", COLOR}}},
     {"ninepatch",
      {{"pos", NORMALIZED_PAIR},
       {"size", NORMALIZED_PAIR},
       {"path", PATH},
       {"visible", BOOLEAN},
       {"zIndex", FLOAT}}}};
// clang-format on

ThemeData::ThemeData()
    : mLegacyTheme {false}
{
    mCurrentThemeSet = mThemeSets.find(Settings::getInstance()->getString("ThemeSet"));
}

void ThemeData::loadFile(const std::map<std::string, std::string>& sysDataMap,
                         const std::string& path)
{
    mPaths.push_back(path);

    ThemeException error;
    error << "ThemeData::loadFile(): ";
    error.setFiles(mPaths);

    if (!Utils::FileSystem::exists(path))
        throw error << "File does not exist";

    mViews.clear();
    mVariables.clear();

    mVariables.insert(sysDataMap.cbegin(), sysDataMap.cend());

    pugi::xml_document doc;
#if defined(_WIN64)
    pugi::xml_parse_result res {doc.load_file(Utils::String::stringToWideString(path).c_str())};
#else
    pugi::xml_parse_result res {doc.load_file(path.c_str())};
#endif
    if (!res)
        throw error << ": XML parsing error: " << res.description();

    pugi::xml_node root {doc.child("theme")};
    if (!root)
        throw error << ": Missing <theme> tag";

    if (mCurrentThemeSet != mThemeSets.cend())
        mLegacyTheme = mCurrentThemeSet->second.capabilities.legacyTheme;

    // The resolution tag introduced in RetroPie EmulationStation in 2020 is a very bad idea
    // as it changes sizing of components from relative values to absolute pixel values.
    // So themes using it will simply not get loaded at all.
    if (root.child("resolution") != nullptr)
        throw error << ": <resolution> tag not supported";

    // Check for legacy theme version.
    int legacyVersion {root.child("formatVersion").text().as_int(-1)};

    if (mLegacyTheme) {
        if (legacyVersion == -1)
            throw error << ": <formatVersion> tag missing for legacy theme set";

        if (legacyVersion < MINIMUM_LEGACY_THEME_FORMAT_VERSION)
            throw error << ": Defined legacy format version " << legacyVersion
                        << " is less than the minimum supported version "
                        << MINIMUM_LEGACY_THEME_FORMAT_VERSION;
    }
    else if (legacyVersion != -1) {
        throw error << ": Legacy <formatVersion> tag found for non-legacy theme set";
    }

    if (!mLegacyTheme) {
        if (mCurrentThemeSet->second.capabilities.variants.size() > 0) {
            for (auto& variant : mCurrentThemeSet->second.capabilities.variants)
                mVariants.emplace_back(variant.name);

            if (std::find(mVariants.cbegin(), mVariants.cend(),
                          Settings::getInstance()->getString("ThemeVariant")) != mVariants.cend())
                mSelectedVariant = Settings::getInstance()->getString("ThemeVariant");
            else
                mSelectedVariant = mVariants.front();
        }

        if (mCurrentThemeSet->second.capabilities.aspectRatios.size() > 0) {
            if (std::find(mCurrentThemeSet->second.capabilities.aspectRatios.cbegin(),
                          mCurrentThemeSet->second.capabilities.aspectRatios.cend(),
                          Settings::getInstance()->getString("ThemeAspectRatio")) !=
                mCurrentThemeSet->second.capabilities.aspectRatios.cend())
                mSelectedAspectRatio = Settings::getInstance()->getString("ThemeAspectRatio");
            else
                mSelectedAspectRatio = mCurrentThemeSet->second.capabilities.aspectRatios.front();
        }
    }

    parseVariables(root);
    parseIncludes(root);
    parseViews(root);
    // For non-legacy themes this will simply check for the presence of a feature tag and throw
    // an error if it's found.
    parseFeatures(root);

    if (!mLegacyTheme) {
        parseVariants(root);
        parseAspectRatios(root);
    }
}

bool ThemeData::hasView(const std::string& view)
{
    auto viewIt = mViews.find(view);
    return (viewIt != mViews.cend());
}

std::vector<GuiComponent*> ThemeData::makeExtras(const std::shared_ptr<ThemeData>& theme,
                                                 const std::string& view)
{
    std::vector<GuiComponent*> comps;

    auto viewIt = theme->mViews.find(view);
    if (viewIt == theme->mViews.cend())
        return comps;

    for (auto it = viewIt->second.legacyOrderedKeys.cbegin(); // Line break.
         it != viewIt->second.legacyOrderedKeys.cend(); ++it) {
        ThemeElement& elem {viewIt->second.elements.at(*it)};
        if (elem.extra) {
            GuiComponent* comp {nullptr};
            const std::string& t {elem.type};
            if (t == "image")
                comp = new ImageComponent;
            else if (t == "text")
                comp = new TextComponent;

            if (comp) {
                comp->setDefaultZIndex(10.0f);
                comp->applyTheme(theme, view, *it, ThemeFlags::ALL);
                comps.push_back(comp);
            }
        }
    }

    return comps;
}

const ThemeData::ThemeElement* ThemeData::getElement(const std::string& view,
                                                     const std::string& element,
                                                     const std::string& expectedType) const
{
    auto viewIt = mViews.find(view);
    if (viewIt == mViews.cend())
        return nullptr; // Not found.

    auto elemIt = viewIt->second.elements.find(element);
    if (elemIt == viewIt->second.elements.cend())
        return nullptr;

    // If expectedType is an empty string, then skip type checking.
    if (elemIt->second.type != expectedType && !expectedType.empty()) {
        LOG(LogWarning) << " requested mismatched theme type for [" << view << "." << element
                        << "] - expected \"" << expectedType << "\", got \"" << elemIt->second.type
                        << "\"";
        return nullptr;
    }

    return &elemIt->second;
}

void ThemeData::populateThemeSets()
{
    assert(mThemeSets.empty());

    LOG(LogInfo) << "Checking for available theme sets...";

    // Check for themes first under the home directory, then under the data installation
    // directory (Unix only) and last under the ES-DE binary directory.

#if defined(__unix__) || defined(__APPLE__)
#if defined(APPIMAGE_BUILD)
    static const size_t pathCount {2};
#else
    static const size_t pathCount {3};
#endif
#else
    static const size_t pathCount {2};
#endif
    std::string paths[pathCount] = {
        Utils::FileSystem::getExePath() + "/themes",
#if defined(__APPLE__)
        Utils::FileSystem::getExePath() + "/../Resources/themes",
#elif defined(__unix__) && !defined(APPIMAGE_BUILD)
        Utils::FileSystem::getProgramDataPath() + "/themes",
#endif
        Utils::FileSystem::getHomePath() + "/.emulationstation/themes"
    };

    for (size_t i = 0; i < pathCount; ++i) {
        if (!Utils::FileSystem::isDirectory(paths[i]))
            continue;

        Utils::FileSystem::StringList dirContent {Utils::FileSystem::getDirContent(paths[i])};

        for (Utils::FileSystem::StringList::const_iterator it = dirContent.cbegin();
             it != dirContent.cend(); ++it) {
            if (Utils::FileSystem::isDirectory(*it)) {
#if defined(_WIN64)
                LOG(LogDebug) << "Loading theme set capabilities for \""
                              << Utils::String::replace(*it, "/", "\\") << "\"...";
#else
                LOG(LogDebug) << "Loading theme set capabilities for \"" << *it << "\"...";
#endif
                ThemeCapability capabilities {parseThemeCapabilities(*it)};

#if defined(_WIN64)
                LOG(LogInfo) << "Added" << (capabilities.legacyTheme ? " legacy" : "")
                             << " theme set \"" << Utils::String::replace(*it, "/", "\\") << "\"";
#else
                LOG(LogInfo) << "Added" << (capabilities.legacyTheme ? " legacy" : "")
                             << " theme set \"" << *it << "\"";
#endif
                if (!capabilities.legacyTheme) {
                    LOG(LogDebug) << "Theme set includes support for "
                                  << capabilities.variants.size() << " variant"
                                  << (capabilities.variants.size() != 1 ? "s" : "") << " and "
                                  << capabilities.aspectRatios.size() << " aspect ratio"
                                  << (capabilities.aspectRatios.size() != 1 ? "s" : "");
                }
                ThemeSet set {*it, capabilities};
                mThemeSets[set.getName()] = set;
            }
        }
    }

    if (mThemeSets.empty()) {
        LOG(LogWarning) << "Couldn't find any theme sets, creating dummy entry";
        ThemeSet set {"no-theme-sets", ThemeCapability()};
        mThemeSets[set.getName()] = set;
        mCurrentThemeSet = mThemeSets.begin();
    }
}

const std::string ThemeData::getThemeFromCurrentSet(const std::string& system)
{
    if (mThemeSets.empty())
        getThemeSets();

    if (mThemeSets.empty())
        // No theme sets available.
        return "";

    std::map<std::string, ThemeSet, StringComparator>::const_iterator set {
        mThemeSets.find(Settings::getInstance()->getString("ThemeSet"))};
    if (set == mThemeSets.cend()) {
        // Currently configured theme set is missing, attempt to load the default theme set
        // slate-DE instead, and if that's also missing then pick the first available set.
        bool defaultSetFound {true};

        set = mThemeSets.find("slate-DE");

        if (set == mThemeSets.cend()) {
            set = mThemeSets.cbegin();
            defaultSetFound = false;
        }

        LOG(LogWarning) << "Configured theme set \""
                        << Settings::getInstance()->getString("ThemeSet")
                        << "\" does not exist, loading" << (defaultSetFound ? " default " : " ")
                        << "theme set \"" << set->first << "\" instead";

        Settings::getInstance()->setString("ThemeSet", set->first);
        mCurrentThemeSet = mThemeSets.find(Settings::getInstance()->getString("ThemeSet"));
    }

    return set->second.getThemePath(system);
}

const std::string ThemeData::getAspectRatioLabel(const std::string& aspectRatio)
{
    auto it = std::find_if(sSupportedAspectRatios.cbegin(), sSupportedAspectRatios.cend(),
                           [&aspectRatio](const std::pair<std::string, std::string>& entry) {
                               return entry.first == aspectRatio;
                           });
    if (it != sSupportedAspectRatios.cend())
        return it->second;
    else
        return "invalid ratio";
}

unsigned int ThemeData::getHexColor(const std::string& str)
{
    ThemeException error;

    if (str == "")
        throw error << "Empty color property";

    size_t len {str.size()};
    if (len != 6 && len != 8)
        throw error << "Invalid color property \"" << str
                    << "\" (must be 6 or 8 characters in length)";

    unsigned int val;
    std::stringstream ss;
    ss << str;
    ss >> std::hex >> val;

    if (len == 6)
        val = (val << 8) | 0xFF;

    return val;
}

std::string ThemeData::resolvePlaceholders(const std::string& in)
{
    if (in.empty())
        return in;

    const size_t variableBegin {in.find("${")};
    const size_t variableEnd {in.find("}", variableBegin)};

    if ((variableBegin == std::string::npos) || (variableEnd == std::string::npos))
        return in;

    std::string prefix {in.substr(0, variableBegin)};
    std::string replace {in.substr(variableBegin + 2, variableEnd - (variableBegin + 2))};
    std::string suffix {resolvePlaceholders(in.substr(variableEnd + 1).c_str())};

    return prefix + mVariables[replace] + suffix;
}

ThemeData::ThemeCapability ThemeData::parseThemeCapabilities(const std::string& path)
{
    ThemeCapability capabilities;
    std::vector<std::string> aspectRatiosTemp;

    std::string capFile {path + "/capabilities.xml"};

    if (Utils::FileSystem::isRegularFile(capFile) || Utils::FileSystem::isSymlink(capFile)) {
        capabilities.legacyTheme = false;

        pugi::xml_document doc;
#if defined(_WIN64)
        pugi::xml_parse_result res =
            doc.load_file(Utils::String::stringToWideString(capFile).c_str());
#else
        pugi::xml_parse_result res = doc.load_file(capFile.c_str());
#endif
        if (res.status == pugi::status_no_document_element) {
            LOG(LogDebug) << "Found a capabilities.xml file with no configuration";
        }
        else if (!res) {
            LOG(LogError) << "Couldn't parse capabilities.xml: " << res.description();
            return capabilities;
        }
        pugi::xml_node themeCapabilities {doc.child("themeCapabilities")};
        if (!themeCapabilities) {
            LOG(LogError) << "Missing <themeCapabilities> tag in capabilities.xml";
            return capabilities;
        }

        for (pugi::xml_node aspectRatio = themeCapabilities.child("aspectRatio"); aspectRatio;
             aspectRatio = aspectRatio.next_sibling("aspectRatio")) {
            std::string value = aspectRatio.text().get();
            if (std::find_if(sSupportedAspectRatios.cbegin(), sSupportedAspectRatios.cend(),
                             [&value](const std::pair<std::string, std::string>& entry) {
                                 return entry.first == value;
                             }) == sSupportedAspectRatios.cend()) {
                LOG(LogWarning) << "Declared aspect ratio \"" << value
                                << "\" is not supported, ignoring entry in \"" << capFile << "\"";
            }
            else {
                if (std::find(aspectRatiosTemp.cbegin(), aspectRatiosTemp.cend(), value) !=
                    aspectRatiosTemp.cend()) {
                    LOG(LogWarning)
                        << "Aspect ratio \"" << value
                        << "\" is declared multiple times, ignoring entry in \"" << capFile << "\"";
                }
                else {
                    aspectRatiosTemp.emplace_back(value);
                }
            }
        }

        for (pugi::xml_node variant = themeCapabilities.child("variant"); variant;
             variant = variant.next_sibling("variant")) {
            ThemeVariant readVariant;
            std::string name {variant.attribute("name").as_string()};
            if (name.empty()) {
                LOG(LogWarning)
                    << "Found <variant> tag without name attribute, ignoring entry in \"" << capFile
                    << "\"";
            }
            else {
                readVariant.name = name;
            }

            pugi::xml_node labelTag {variant.child("label")};
            if (labelTag == nullptr) {
                LOG(LogDebug)
                    << "No variant <label> tag found, setting label value to the variant name \""
                    << name << "\" for \"" << capFile << "\"";
                readVariant.label = name;
            }
            else {
                std::string labelValue {labelTag.text().as_string()};
                if (labelValue == "") {
                    LOG(LogWarning) << "No variant <label> value defined, setting value to "
                                       "the variant name \""
                                    << name << "\" for \"" << capFile << "\"";
                    readVariant.label = name;
                }
                else {
                    readVariant.label = labelValue;
                }
            }

            pugi::xml_node selectableTag {variant.child("selectable")};
            if (selectableTag != nullptr) {
                std::string value {selectableTag.text().as_string()};
                if (value.front() == '0' || value.front() == 'f' || value.front() == 'F' ||
                    value.front() == 'n' || value.front() == 'N')
                    readVariant.selectable = false;
                else
                    readVariant.selectable = true;
            }

            pugi::xml_node overrideTag {variant.child("override")};
            if (overrideTag != nullptr) {
                pugi::xml_node triggerTag {overrideTag.child("trigger")};
                if (triggerTag != nullptr) {
                    std::string triggerValue {triggerTag.text().as_string()};
                    if (triggerValue == "") {
                        LOG(LogWarning)
                            << "No <trigger> tag value defined for variant \"" << readVariant.name
                            << "\", ignoring entry in \"" << capFile << "\"";
                    }
                    else {
                        pugi::xml_node useVariantTag {overrideTag.child("useVariant")};
                        if (useVariantTag != nullptr) {
                            std::string useVariantValue {useVariantTag.text().as_string()};
                            if (useVariantValue == "") {
                                LOG(LogWarning)
                                    << "No <useVariant> tag value defined for variant \""
                                    << readVariant.name << "\", ignoring entry in \"" << capFile
                                    << "\"";
                            }
                            else {
                                readVariant.override = true;
                                readVariant.overrideTrigger = triggerValue;
                                readVariant.overrideVariant = useVariantValue;
                            }
                        }
                        else {
                            LOG(LogWarning) << "Found an <override> tag without a corresponding "
                                               "<useVariant> tag, "
                                            << "ignoring entry for variant \"" << readVariant.name
                                            << "\" in \"" << capFile << "\"";
                        }
                    }
                }
                else {
                    LOG(LogWarning)
                        << "Found an <override> tag without a corresponding <trigger> tag, "
                        << "ignoring entry for variant \"" << readVariant.name << "\" in \""
                        << capFile << "\"";
                }
            }

            if (readVariant.name != "") {
                bool duplicate {false};
                for (auto& variant : capabilities.variants) {
                    if (variant.name == readVariant.name) {
                        LOG(LogWarning) << "Variant \"" << readVariant.name
                                        << "\" is declared multiple times, ignoring entry in \""
                                        << capFile << "\"";
                        duplicate = true;
                        break;
                    }
                }
                if (!duplicate)
                    capabilities.variants.emplace_back(readVariant);
            }
        }
    }
    else {
        LOG(LogDebug) << "No capabilities.xml file found, flagging as legacy theme set";
        capabilities.legacyTheme = true;
    }

    // Add the aspect ratios in the order they are defined in sSupportedAspectRatios so they
    // always show up in the same order in the UI Settings menu.
    if (!aspectRatiosTemp.empty()) {
        for (auto& aspectRatio : sSupportedAspectRatios) {
            if (std::find(aspectRatiosTemp.cbegin(), aspectRatiosTemp.cend(), aspectRatio.first) !=
                aspectRatiosTemp.cend()) {
                capabilities.aspectRatios.emplace_back(aspectRatio.first);
            }
        }
    }

    return capabilities;
}

void ThemeData::parseIncludes(const pugi::xml_node& root)
{
    ThemeException error;
    error << "ThemeData::parseIncludes(): ";
    error.setFiles(mPaths);

    if (!mLegacyTheme) {
        if (root.child("formatVersion").text().as_int(-1) != -1)
            throw error << ": Legacy <formatVersion> tag found for non-legacy theme set";
    }

    for (pugi::xml_node node = root.child("include"); node; node = node.next_sibling("include")) {
        std::string relPath {resolvePlaceholders(node.text().as_string())};
        std::string path {Utils::FileSystem::resolveRelativePath(relPath, mPaths.back(), true)};
        if (!ResourceManager::getInstance().fileExists(path))
            throw error << " -> \"" << relPath << "\" not found (resolved to \"" << path << "\")";
        error << " -> \"" << relPath << "\"";

        mPaths.push_back(path);

        pugi::xml_document includeDoc;
#if defined(_WIN64)
        pugi::xml_parse_result result {
            includeDoc.load_file(Utils::String::stringToWideString(path).c_str())};
#else
        pugi::xml_parse_result result {includeDoc.load_file(path.c_str())};
#endif
        if (!result)
            throw error << ": Error parsing file: " << result.description();

        pugi::xml_node theme {includeDoc.child("theme")};
        if (!theme)
            throw error << ": Missing <theme> tag";

        parseVariables(theme);
        parseIncludes(theme);
        parseViews(theme);
        // For non-legacy themes this will simply check for the presence of a feature tag and throw
        // an error if it's found.
        parseFeatures(theme);

        if (!mLegacyTheme) {
            parseVariants(theme);
            parseAspectRatios(theme);
        }

        mPaths.pop_back();
    }
}

void ThemeData::parseFeatures(const pugi::xml_node& root)
{
    ThemeException error;
    error << "ThemeData::parseFeatures(): ";
    error.setFiles(mPaths);

    if (!mLegacyTheme && root.child("feature") != nullptr)
        throw error << ": Legacy <feature> tag found for non-legacy theme set";

    for (pugi::xml_node node = root.child("feature"); node; node = node.next_sibling("feature")) {
        if (!node.attribute("supported"))
            throw error << ": Feature missing \"supported\" attribute";

        const std::string supportedAttr {node.attribute("supported").as_string()};

        if (std::find(sLegacySupportedFeatures.cbegin(), sLegacySupportedFeatures.cend(),
                      supportedAttr) != sLegacySupportedFeatures.cend()) {
            parseViews(node);
        }
    }
}

void ThemeData::parseVariants(const pugi::xml_node& root)
{
    if (mCurrentThemeSet == mThemeSets.end())
        return;

    if (mSelectedVariant == "")
        return;

    ThemeException error;
    error << "ThemeData::parseVariants(): ";
    error.setFiles(mPaths);

    for (pugi::xml_node node = root.child("variant"); node; node = node.next_sibling("variant")) {
        if (!node.attribute("name"))
            throw error << ": <variant> tag missing \"name\" attribute";

        const std::string delim {" \t\r\n,"};
        const std::string nameAttr {node.attribute("name").as_string()};
        size_t prevOff {nameAttr.find_first_not_of(delim, 0)};
        size_t off {nameAttr.find_first_of(delim, prevOff)};
        std::string viewKey;
        while (off != std::string::npos || prevOff != std::string::npos) {
            viewKey = nameAttr.substr(prevOff, off - prevOff);
            prevOff = nameAttr.find_first_not_of(delim, off);
            off = nameAttr.find_first_of(delim, prevOff);

            if (std::find(mVariants.cbegin(), mVariants.cend(), viewKey) == mVariants.cend()) {
                throw error << ": <variant> value \"" << viewKey
                            << "\" is not defined in capabilities.xml";
            }

            if (mSelectedVariant == viewKey) {
                parseIncludes(node);
                parseViews(node);
            }
        }
    }
}

void ThemeData::parseAspectRatios(const pugi::xml_node& root)
{
    if (mCurrentThemeSet == mThemeSets.end())
        return;

    if (mSelectedAspectRatio == "")
        return;

    ThemeException error;
    error << "ThemeData::parseAspectRatios(): ";
    error.setFiles(mPaths);

    for (pugi::xml_node node = root.child("aspectRatio"); node;
         node = node.next_sibling("aspectRatio")) {
        if (!node.attribute("name"))
            throw error << ": <aspectRatio> tag missing \"name\" attribute";

        const std::string delim {" \t\r\n,"};
        const std::string nameAttr {node.attribute("name").as_string()};
        size_t prevOff {nameAttr.find_first_not_of(delim, 0)};
        size_t off {nameAttr.find_first_of(delim, prevOff)};
        std::string viewKey;
        while (off != std::string::npos || prevOff != std::string::npos) {
            viewKey = nameAttr.substr(prevOff, off - prevOff);
            prevOff = nameAttr.find_first_not_of(delim, off);
            off = nameAttr.find_first_of(delim, prevOff);

            if (std::find(mCurrentThemeSet->second.capabilities.aspectRatios.cbegin(),
                          mCurrentThemeSet->second.capabilities.aspectRatios.cend(),
                          viewKey) == mCurrentThemeSet->second.capabilities.aspectRatios.cend()) {
                throw error << ": aspectRatio value \"" << viewKey
                            << "\" is not defined in capabilities.xml";
            }

            if (mSelectedAspectRatio == viewKey) {
                parseIncludes(node);
                parseViews(node);
            }
        }
    }
}

void ThemeData::parseVariables(const pugi::xml_node& root)
{
    ThemeException error;
    error.setFiles(mPaths);

    pugi::xml_node variables {root.child("variables")};

    if (!variables)
        return;

    for (pugi::xml_node_iterator it = variables.begin(); it != variables.end(); ++it) {
        std::string key {it->name()};
        std::string val {resolvePlaceholders(it->text().as_string())};

        if (!val.empty())
            mVariables.insert(std::pair<std::string, std::string>(key, val));
    }
}

void ThemeData::parseViews(const pugi::xml_node& root)
{
    ThemeException error;
    error << "ThemeData::parseViews(): ";
    error.setFiles(mPaths);

    // Parse views.
    for (pugi::xml_node node = root.child("view"); node; node = node.next_sibling("view")) {
        if (!node.attribute("name"))
            throw error << ": View missing \"name\" attribute";

        const std::string delim {" \t\r\n,"};
        const std::string nameAttr {node.attribute("name").as_string()};
        size_t prevOff {nameAttr.find_first_not_of(delim, 0)};
        size_t off {nameAttr.find_first_of(delim, prevOff)};
        std::string viewKey;
        while (off != std::string::npos || prevOff != std::string::npos) {
            viewKey = nameAttr.substr(prevOff, off - prevOff);
            prevOff = nameAttr.find_first_not_of(delim, off);
            off = nameAttr.find_first_of(delim, prevOff);

            if (mLegacyTheme) {
                if (std::find(sLegacySupportedViews.cbegin(), sLegacySupportedViews.cend(),
                              viewKey) != sLegacySupportedViews.cend()) {
                    ThemeView& view {
                        mViews.insert(std::pair<std::string, ThemeView>(viewKey, ThemeView()))
                            .first->second};
                    parseView(node, view);
                }
                else {
                    throw error << ": Unsupported \"" << viewKey << "\" view style defined";
                }
            }
            else {
                if (std::find(sSupportedViews.cbegin(), sSupportedViews.cend(), viewKey) !=
                    sSupportedViews.cend()) {
                    ThemeView& view {
                        mViews.insert(std::pair<std::string, ThemeView>(viewKey, ThemeView()))
                            .first->second};
                    parseView(node, view);
                }
                else {
                    throw error << ": Unsupported \"" << viewKey << "\" view style defined";
                }
            }
        }
    }
}

void ThemeData::parseView(const pugi::xml_node& root, ThemeView& view)
{
    ThemeException error;
    error << "ThemeData::parseView(): ";
    error.setFiles(mPaths);

    for (pugi::xml_node node = root.first_child(); node; node = node.next_sibling()) {
        if (!node.attribute("name"))
            throw error << ": Element of type \"" << node.name() << "\" missing \"name\" attribute";

        auto elemTypeIt = sElementMap.find(node.name());
        if (elemTypeIt == sElementMap.cend())
            throw error << ": Unknown element type \"" << node.name() << "\"";

        const std::string delim {" \t\r\n,"};
        const std::string nameAttr {node.attribute("name").as_string()};
        size_t prevOff {nameAttr.find_first_not_of(delim, 0)};
        size_t off {nameAttr.find_first_of(delim, prevOff)};
        while (off != std::string::npos || prevOff != std::string::npos) {
            std::string elemKey {nameAttr.substr(prevOff, off - prevOff)};
            prevOff = nameAttr.find_first_not_of(delim, off);
            off = nameAttr.find_first_of(delim, prevOff);

            // Add the element type as a prefix to avoid name collisions between different
            // component types. Also include a workaround for legacy theme sets if the
            // md_releasedate and md_lastplayed element types are incorrectly defined as
            // text instead of datetime.
            const std::string elementType {node.name()};
            bool dateTimeWorkaround {false};

            if (mLegacyTheme && elementType == "text" &&
                (elemKey == "md_releasedate" || elemKey == "md_lastplayed")) {
                LOG(LogDebug) << "ThemeData::parseView(): Element type for \"" << elemKey
                              << "\" incorrectly set to \"text\" "
                                 "instead of \"datetime\", applying workaround";
                dateTimeWorkaround = true;
                elemKey = "datetime_" + elemKey;
            }
            else {
                elemKey = elementType + "_" + elemKey;
            }

            parseElement(
                node, elemTypeIt->second,
                view.elements.insert(std::pair<std::string, ThemeElement>(elemKey, ThemeElement()))
                    .first->second,
                dateTimeWorkaround);

            if (mLegacyTheme &&
                std::find(view.legacyOrderedKeys.cbegin(), view.legacyOrderedKeys.cend(),
                          elemKey) == view.legacyOrderedKeys.cend())
                view.legacyOrderedKeys.push_back(elemKey);
        }
    }
}

void ThemeData::parseElement(const pugi::xml_node& root,
                             const std::map<std::string, ElementPropertyType>& typeMap,
                             ThemeElement& element,
                             bool dateTimeWorkaround)
{
    ThemeException error;
    error << "ThemeData::parseElement(): ";
    error.setFiles(mPaths);

    if (dateTimeWorkaround)
        element.type = "datetime";
    else
        element.type = root.name();

    element.extra = root.attribute("extra").as_bool(false);
    if (mLegacyTheme)
        element.extra = root.attribute("extra").as_bool(false);
    else if (!mLegacyTheme && std::string(root.attribute("extra").as_string("")) != "")
        throw error << ": Legacy \"extra\" attribute found for non-legacy theme set";

    for (pugi::xml_node node = root.first_child(); node; node = node.next_sibling()) {
        auto typeIt = typeMap.find(node.name());
        if (typeIt == typeMap.cend())
            throw error << ": Unknown property type \"" << node.name()
                        << "\" for element of type \"" << root.name() << "\"";

        std::string str {resolvePlaceholders(node.text().as_string())};

        // Skip this check for legacy themes to not break backward compatibility with some
        // themes sets that include empty property values.
        if (!mLegacyTheme && str == "")
            throw error << ": Property \"" << typeIt->first << "\" for element \"" << element.type
                        << "\" has no value defined";

        std::string nodeName = node.name();

        // Strictly enforce removal of legacy elements for non-legacy theme sets by creating
        // an unthemed system if they're present in the configuration.
        if (!mLegacyTheme) {
            for (auto& legacyElement : sLegacyElements) {
                if (nodeName == legacyElement) {
                    throw error << ": Legacy <" << nodeName
                                << "> property found for non-legacy theme set";
                }
            }
        }

        // If an attribute exists, then replace nodeName with its name.
        auto attributeEntry = sPropertyAttributeMap.find(element.type);
        if (attributeEntry != sPropertyAttributeMap.end()) {
            auto attribute = attributeEntry->second.find(typeIt->first);
            if (attribute != attributeEntry->second.end()) {
                if (node.attribute(attribute->second.c_str()) == nullptr) {
                    throw error << ": Unknown attribute \"" << node.first_attribute().name()
                                << "\" for property \"" << typeIt->first << "\" (element \""
                                << attributeEntry->first << "\")";
                }
                else {
                    // Add the attribute name as a prefix to avoid potential name collisions.
                    nodeName = attribute->second + "_" +
                               node.attribute(attribute->second.c_str()).as_string("");
                }
            }
        }

        switch (typeIt->second) {
            case NORMALIZED_RECT: {
                glm::vec4 val;

                auto splits = Utils::String::delimitedStringToVector(str, " ");
                if (splits.size() == 2) {
                    val = glm::vec4 {static_cast<float>(atof(splits.at(0).c_str())),
                                     static_cast<float>(atof(splits.at(1).c_str())),
                                     static_cast<float>(atof(splits.at(0).c_str())),
                                     static_cast<float>(atof(splits.at(1).c_str()))};
                }
                else if (splits.size() == 4) {
                    val = glm::vec4 {static_cast<float>(atof(splits.at(0).c_str())),
                                     static_cast<float>(atof(splits.at(1).c_str())),
                                     static_cast<float>(atof(splits.at(2).c_str())),
                                     static_cast<float>(atof(splits.at(3).c_str()))};
                }

                element.properties[node.name()] = val;
                break;
            }
            case NORMALIZED_PAIR: {
                size_t divider = str.find(' ');
                if (divider == std::string::npos)
                    throw error << ": Invalid normalized pair value \"" << str.c_str()
                                << "\" for property \"" << node.name() << "\"";

                std::string first {str.substr(0, divider)};
                std::string second {str.substr(divider, std::string::npos)};

                glm::vec2 val {static_cast<float>(atof(first.c_str())),
                               static_cast<float>(atof(second.c_str()))};

                element.properties[node.name()] = val;
                break;
            }
            case STRING: {
                element.properties[node.name()] = str;
                break;
            }
            case PATH: {
                std::string path;

                if (!str.empty() && str.front() == ':')
                    path = ResourceManager::getInstance().getResourcePath(str);
                else
                    path = Utils::FileSystem::resolveRelativePath(str, mPaths.back(), true);

                if (!ResourceManager::getInstance().fileExists(path)) {
                    std::stringstream ss;
                    // For explicits paths, print a warning if the file couldn't be found, but
                    // only print a debug message if it was set using a variable.
                    if (str == node.text().as_string()) {
                        LOG(LogWarning)
                            << error.message << ": Couldn't find file \"" << node.text().get()
                            << "\" "
                            << ((node.text().get() != path) ? "which resolves to \"" + path + "\"" :
                                                              "");
                    }
                    else {
                        LOG(LogDebug)
                            << error.message << ": Couldn't find file \"" << node.text().get()
                            << "\" "
                            << ((node.text().get() != path) ? "which resolves to \"" + path + "\"" :
                                                              "");
                    }
                }
                element.properties[nodeName] = path;
                break;
            }
            case COLOR: {
                try {
                    element.properties[node.name()] = getHexColor(str);
                }
                catch (ThemeException& e) {
                    throw error << ": " << e.what();
                }
                break;
            }
            case UNSIGNED_INTEGER: {
                unsigned int integerVal {static_cast<unsigned int>(strtoul(str.c_str(), 0, 0))};
                element.properties[node.name()] = integerVal;
                break;
            }
            case FLOAT: {
                float floatVal {static_cast<float>(strtod(str.c_str(), 0))};
                element.properties[node.name()] = floatVal;
                break;
            }
            case BOOLEAN: {
                bool boolVal = false;
                // Only look at the first character.
                if (str.size() > 0) {
                    if (str.front() == '1' || str.front() == 't' || str.front() == 'T' ||
                        str.front() == 'y' || str.front() == 'Y')
                        boolVal = true;
                }

                element.properties[node.name()] = boolVal;
                break;
            }
            default: {
                throw error << ": Unknown ElementPropertyType for \""
                            << root.attribute("name").as_string() << "\", property " << node.name();
            }
        }
    }
}
