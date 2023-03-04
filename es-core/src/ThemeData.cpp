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

std::vector<std::string> ThemeData::sSupportedMediaTypes {
    {"miximage"},
    {"marquee"},
    {"screenshot"},
    {"titlescreen"},
    {"cover"},
    {"backcover"},
    {"3dbox"},
    {"physicalmedia"},
    {"fanart"},
    {"video"}};

std::vector<std::string> ThemeData::sSupportedTransitions {
    {"systemToSystem"},
    {"systemToGamelist"},
    {"gamelistToGamelist"},
    {"gamelistToSystem"},
    {"startupToSystem"},
    {"startupToGamelist"}};

std::vector<std::string> ThemeData::sSupportedTransitionAnimations {
    {"builtin-instant"},
    {"builtin-slide"},
    {"builtin-fade"}};

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

std::vector<std::string> ThemeData::sLegacyProperties {
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
    {"maxLogoCount"},
    {"selectorOffsetY"}};

std::vector<std::pair<std::string, std::string>> ThemeData::sSupportedAspectRatios {
    {"automatic", "automatic"},
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

std::map<std::string, float> ThemeData::sAspectRatioMap {
    {"16:9", 1.7777f},
    {"16:9_vertical", 0.5625f},
    {"16:10", 1.6f},
    {"16:10_vertical", 0.625f},
    {"3:2", 1.5f},
    {"3:2_vertical", 0.6667f},
    {"4:3", 1.3333f},
    {"4:3_vertical", 0.75f},
    {"5:4", 1.25f},
    {"5:4_vertical", 0.8f},
    {"21:9", 2.3703f},
    {"21:9_vertical", 0.4219f},
    {"32:9", 3.5555f},
    {"32:9_vertical", 0.2813f}};

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
     {"carousel",
      {{"pos", NORMALIZED_PAIR},
       {"size", NORMALIZED_PAIR},
       {"origin", NORMALIZED_PAIR},
       {"type", STRING},
       {"staticImage", PATH},
       {"imageType", STRING},
       {"defaultImage", PATH},
       {"defaultFolderImage", PATH},
       {"maxItemCount", FLOAT},
       {"maxLogoCount", FLOAT},                    // For backward compatibility with legacy themes.
       {"itemsBeforeCenter", UNSIGNED_INTEGER},
       {"itemsAfterCenter", UNSIGNED_INTEGER},
       {"itemStacking", STRING},
       {"selectedItemMargins", NORMALIZED_PAIR},
       {"itemSize", NORMALIZED_PAIR},
       {"itemScale", FLOAT},
       {"itemRotation", FLOAT},
       {"itemRotationOrigin", NORMALIZED_PAIR},
       {"itemAxisHorizontal", BOOLEAN},
       {"itemAxisRotation", FLOAT},
       {"imageFit", STRING},
       {"imageInterpolation", STRING},
       {"imageColor", COLOR},
       {"imageColorEnd", COLOR},
       {"imageGradientType", STRING},
       {"imageSelectedColor", COLOR},
       {"imageSelectedColorEnd", COLOR},
       {"imageSelectedGradientType", STRING},
       {"imageBrightness", FLOAT},
       {"imageSaturation", FLOAT},
       {"itemTransitions", STRING},
       {"itemDiagonalOffset", FLOAT},
       {"itemHorizontalAlignment", STRING},
       {"itemVerticalAlignment", STRING},
       {"wheelHorizontalAlignment", STRING},
       {"wheelVerticalAlignment", STRING},
       {"horizontalOffset", FLOAT},
       {"verticalOffset", FLOAT},
       {"reflections", BOOLEAN},
       {"reflectionsOpacity", FLOAT},
       {"reflectionsFalloff", FLOAT},
       {"unfocusedItemOpacity", FLOAT},
       {"unfocusedItemSaturation", FLOAT},
       {"unfocusedItemDimming", FLOAT},
       {"fastScrolling", BOOLEAN},
       {"defaultLogo", PATH},                      // For backward compatibility with legacy themes.
       {"logoSize", NORMALIZED_PAIR},              // For backward compatibility with legacy themes.
       {"logoScale", FLOAT},                       // For backward compatibility with legacy themes.
       {"logoRotation", FLOAT},                    // For backward compatibility with legacy themes.
       {"logoRotationOrigin", NORMALIZED_PAIR},    // For backward compatibility with legacy themes.
       {"logoAlignment", STRING},                  // For backward compatibility with legacy themes.
       {"color", COLOR},
       {"colorEnd", COLOR},
       {"gradientType", STRING},
       {"text", STRING},
       {"textColor", COLOR},
       {"textBackgroundColor", COLOR},
       {"textSelectedColor", COLOR},
       {"textSelectedBackgroundColor", COLOR},
       {"fontPath", PATH},
       {"fontSize", FLOAT},
       {"letterCase", STRING},
       {"letterCaseAutoCollections", STRING},
       {"letterCaseCustomCollections", STRING},
       {"lineSpacing", FLOAT},
       {"systemNameSuffix", BOOLEAN},
       {"letterCaseSystemNameSuffix", STRING},
       {"fadeAbovePrimary", BOOLEAN},
       {"zIndex", FLOAT},
       {"legacyZIndexMode", STRING}}},             // For backward compatibility with legacy themes.
     {"grid",
      {{"pos", NORMALIZED_PAIR},
       {"size", NORMALIZED_PAIR},
       {"origin", NORMALIZED_PAIR},
       {"staticImage", PATH},
       {"imageType", STRING},
       {"defaultImage", PATH},
       {"defaultFolderImage", PATH},
       {"itemSize", NORMALIZED_PAIR},
       {"itemScale", FLOAT},
       {"itemSpacing", NORMALIZED_PAIR},
       {"fractionalRows", BOOLEAN},
       {"itemTransitions", STRING},
       {"rowTransitions", STRING},
       {"unfocusedItemOpacity", FLOAT},
       {"unfocusedItemSaturation", FLOAT},
       {"unfocusedItemDimming", FLOAT},
       {"imageFit", STRING},
       {"imageRelativeScale", FLOAT},
       {"imageColor", COLOR},
       {"imageColorEnd", COLOR},
       {"imageGradientType", STRING},
       {"imageSelectedColor", COLOR},
       {"imageSelectedColorEnd", COLOR},
       {"imageSelectedGradientType", STRING},
       {"imageBrightness", FLOAT},
       {"imageSaturation", FLOAT},
       {"backgroundImage", PATH},
       {"backgroundRelativeScale", FLOAT},
       {"backgroundColor", COLOR},
       {"backgroundColorEnd", COLOR},
       {"backgroundGradientType", STRING},
       {"selectorImage", PATH},
       {"selectorRelativeScale", FLOAT},
       {"selectorLayer", STRING},
       {"selectorColor", COLOR},
       {"selectorColorEnd", COLOR},
       {"selectorGradientType", STRING},
       {"text", STRING},
       {"textRelativeScale", FLOAT},
       {"textColor", COLOR},
       {"textBackgroundColor", COLOR},
       {"textSelectedColor", COLOR},
       {"textSelectedBackgroundColor", COLOR},
       {"fontPath", PATH},
       {"fontSize", FLOAT},
       {"letterCase", STRING},
       {"letterCaseAutoCollections", STRING},
       {"letterCaseCustomCollections", STRING},
       {"lineSpacing", FLOAT},
       {"systemNameSuffix", BOOLEAN},
       {"letterCaseSystemNameSuffix", STRING},
       {"fadeAbovePrimary", BOOLEAN},
       {"zIndex", FLOAT}}},
     {"textlist",
      {{"pos", NORMALIZED_PAIR},
       {"size", NORMALIZED_PAIR},
       {"origin", NORMALIZED_PAIR},
       {"selectorHeight", FLOAT},
       {"selectorHorizontalOffset", FLOAT},
       {"selectorVerticalOffset", FLOAT},
       {"selectorOffsetY", FLOAT},                 // For backward compatibility with legacy themes.
       {"selectorColor", COLOR},
       {"selectorColorEnd", COLOR},
       {"selectorGradientType", STRING},
       {"selectorImagePath", PATH},
       {"selectorImageTile", BOOLEAN},
       {"primaryColor", COLOR},
       {"secondaryColor", COLOR},
       {"selectedColor", COLOR},
       {"selectedSecondaryColor", COLOR},
       {"selectedBackgroundColor", COLOR},
       {"selectedSecondaryBackgroundColor", COLOR},
       {"fontPath", PATH},
       {"fontSize", FLOAT},
       {"scrollSound", PATH},                      // For backward compatibility with legacy themes.
       {"horizontalAlignment", STRING},
       {"alignment", STRING},                      // For backward compatibility with legacy themes.
       {"horizontalMargin", FLOAT},
       {"letterCase", STRING},
       {"letterCaseAutoCollections", STRING},
       {"letterCaseCustomCollections", STRING},
       {"forceUppercase", BOOLEAN},                // For backward compatibility with legacy themes.
       {"lineSpacing", FLOAT},
       {"indicators", STRING},
       {"collectionIndicators", STRING},
       {"systemNameSuffix", BOOLEAN},
       {"letterCaseSystemNameSuffix", STRING},
       {"fadeAbovePrimary", BOOLEAN},
       {"zIndex", FLOAT}}},
     {"image",
      {{"pos", NORMALIZED_PAIR},
       {"size", NORMALIZED_PAIR},
       {"cropSize", NORMALIZED_PAIR},
       {"maxSize", NORMALIZED_PAIR},
       {"origin", NORMALIZED_PAIR},
       {"rotation", FLOAT},
       {"rotationOrigin", NORMALIZED_PAIR},
       {"flipHorizontal", BOOLEAN},
       {"flipVertical", BOOLEAN},
       {"path", PATH},
       {"default", PATH},
       {"imageType", STRING},
       {"metadataElement", BOOLEAN},
       {"gameselector", STRING},
       {"gameselectorEntry", UNSIGNED_INTEGER},
       {"tile", BOOLEAN},
       {"tileSize", NORMALIZED_PAIR},
       {"tileHorizontalAlignment", STRING},
       {"tileVerticalAlignment", STRING},
       {"interpolation", STRING},
       {"color", COLOR},
       {"colorEnd", COLOR},
       {"gradientType", STRING},
       {"scrollFadeIn", BOOLEAN},
       {"brightness", FLOAT},
       {"opacity", FLOAT},
       {"saturation", FLOAT},
       {"visible", BOOLEAN},
       {"zIndex", FLOAT}}},
     {"video",
      {{"pos", NORMALIZED_PAIR},
       {"size", NORMALIZED_PAIR},
       {"cropSize", NORMALIZED_PAIR},
       {"maxSize", NORMALIZED_PAIR},
       {"origin", NORMALIZED_PAIR},
       {"path", PATH},
       {"default", PATH},
       {"defaultImage", PATH},
       {"imageType", STRING},
       {"metadataElement", BOOLEAN},
       {"gameselector", STRING},
       {"gameselectorEntry", UNSIGNED_INTEGER},
       {"audio", BOOLEAN},
       {"interpolation", STRING},
       {"color", COLOR},
       {"colorEnd", COLOR},
       {"gradientType", STRING},
       {"pillarboxes", BOOLEAN},
       {"pillarboxThreshold", NORMALIZED_PAIR},
       {"scanlines", BOOLEAN},
       {"delay", FLOAT},
       {"fadeInTime", FLOAT},
       {"scrollFadeIn", BOOLEAN},
       {"brightness", FLOAT},
       {"opacity", FLOAT},
       {"saturation", FLOAT},
       {"visible", BOOLEAN},
       {"zIndex", FLOAT},
       {"showSnapshotNoVideo", BOOLEAN},           // For backward compatibility with legacy themes.
       {"showSnapshotDelay", BOOLEAN}}},           // For backward compatibility with legacy themes.
     {"animation",
      {{"pos", NORMALIZED_PAIR},
       {"size", NORMALIZED_PAIR},
       {"maxSize", NORMALIZED_PAIR},
       {"origin", NORMALIZED_PAIR},
       {"rotation", FLOAT},
       {"rotationOrigin", NORMALIZED_PAIR},
       {"metadataElement", BOOLEAN},
       {"path", PATH},
       {"speed", FLOAT},
       {"direction", STRING},
       {"iterationCount", UNSIGNED_INTEGER},
       {"interpolation", STRING},
       {"color", COLOR},
       {"colorEnd", COLOR},
       {"gradientType", STRING},
       {"brightness", FLOAT},
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
       {"badgeIconColor", COLOR},
       {"badgeIconColorEnd", COLOR},
       {"badgeIconGradientType", STRING},
       {"controllerIconColor", COLOR},
       {"controllerIconColorEnd", COLOR},
       {"controllerIconGradientType", STRING},
       {"folderLinkIconColor", COLOR},
       {"folderLinkIconColorEnd", COLOR},
       {"folderLinkIconGradientType", STRING},
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
       {"defaultValue", STRING},
       {"systemNameSuffix", BOOLEAN},
       {"letterCaseSystemNameSuffix", STRING},
       {"metadataElement", BOOLEAN},
       {"gameselector", STRING},
       {"gameselectorEntry", UNSIGNED_INTEGER},
       {"container", BOOLEAN},
       {"containerVerticalSnap", BOOLEAN},
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
       {"defaultValue", STRING},
       {"gameselector", STRING},
       {"gameselectorEntry", UNSIGNED_INTEGER},
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
       {"horizontalAlignment", STRING},
       {"verticalAlignment", STRING},
       {"alignment", STRING},                      // For backward compatibility with legacy themes.
       {"color", COLOR},
       {"backgroundColor", COLOR},
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
       {"gameselectorEntry", UNSIGNED_INTEGER},
       {"interpolation", STRING},
       {"color", COLOR},
       {"filledPath", PATH},
       {"unfilledPath", PATH},
       {"overlay", BOOLEAN},
       {"opacity", FLOAT},
       {"visible", BOOLEAN},
       {"zIndex", FLOAT}}},
     {"gameselector",
      {{"selection", STRING},
       {"gameCount", UNSIGNED_INTEGER},
       {"allowDuplicates", BOOLEAN}}},
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
     {"navigationsounds",
      {{"systembrowseSound", PATH},
       {"quicksysselectSound", PATH},
       {"selectSound", PATH},
       {"backSound", PATH},
       {"scrollSound", PATH},
       {"favoriteSound", PATH},
       {"launchSound", PATH}}},
     // Legacy components below, not in use any longer but needed for backward compatibility.
     {"sound",
      {{"path", PATH}}},
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
    , mCustomCollection {false}
{
    sCurrentThemeSet = sThemeSets.find(Settings::getInstance()->getString("ThemeSet"));
    sVariantDefinedTransitions = "";
}

void ThemeData::loadFile(const std::map<std::string, std::string>& sysDataMap,
                         const std::string& path,
                         const ThemeTriggers::TriggerType trigger,
                         const bool customCollection)
{
    mCustomCollection = customCollection;
    mOverrideVariant = "";

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

    if (sCurrentThemeSet != sThemeSets.cend())
        mLegacyTheme = sCurrentThemeSet->second.capabilities.legacyTheme;

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
        if (sCurrentThemeSet->second.capabilities.variants.size() > 0) {
            for (auto& variant : sCurrentThemeSet->second.capabilities.variants)
                mVariants.emplace_back(variant.name);

            if (std::find(mVariants.cbegin(), mVariants.cend(),
                          Settings::getInstance()->getString("ThemeVariant")) != mVariants.cend())
                mSelectedVariant = Settings::getInstance()->getString("ThemeVariant");
            else
                mSelectedVariant = mVariants.front();
            // Special shortcut variant to apply configuration to all defined variants.
            mVariants.emplace_back("all");

            if (trigger != ThemeTriggers::TriggerType::NONE) {
                auto overrides = getCurrentThemeSetSelectedVariantOverrides();
                if (overrides.find(trigger) != overrides.end())
                    mOverrideVariant = overrides.at(trigger).first;
            }
        }

        if (sCurrentThemeSet->second.capabilities.colorSchemes.size() > 0) {
            for (auto& colorScheme : sCurrentThemeSet->second.capabilities.colorSchemes)
                mColorSchemes.emplace_back(colorScheme.name);

            if (std::find(mColorSchemes.cbegin(), mColorSchemes.cend(),
                          Settings::getInstance()->getString("ThemeColorScheme")) !=
                mColorSchemes.cend())
                mSelectedColorScheme = Settings::getInstance()->getString("ThemeColorScheme");
            else
                mSelectedColorScheme = mColorSchemes.front();
        }

        sAspectRatioMatch = false;

        if (sCurrentThemeSet->second.capabilities.aspectRatios.size() > 0) {
            if (std::find(sCurrentThemeSet->second.capabilities.aspectRatios.cbegin(),
                          sCurrentThemeSet->second.capabilities.aspectRatios.cend(),
                          Settings::getInstance()->getString("ThemeAspectRatio")) !=
                sCurrentThemeSet->second.capabilities.aspectRatios.cend())
                sSelectedAspectRatio = Settings::getInstance()->getString("ThemeAspectRatio");
            else
                sSelectedAspectRatio = sCurrentThemeSet->second.capabilities.aspectRatios.front();

            if (sSelectedAspectRatio == "automatic") {
                // Auto-detect the closest aspect ratio based on what's available in the theme set.
                sSelectedAspectRatio = "16:9";
                const float screenAspectRatio {Renderer::getScreenAspectRatio()};
                float diff {std::fabs(sAspectRatioMap["16:9"] - screenAspectRatio)};

                for (auto& aspectRatio : sCurrentThemeSet->second.capabilities.aspectRatios) {
                    if (aspectRatio == "automatic")
                        continue;

                    if (sAspectRatioMap.find(aspectRatio) != sAspectRatioMap.end()) {
                        const float newDiff {
                            std::fabs(sAspectRatioMap[aspectRatio] - screenAspectRatio)};
                        if (newDiff < 0.01f)
                            sAspectRatioMatch = true;
                        if (newDiff < diff) {
                            diff = newDiff;
                            sSelectedAspectRatio = aspectRatio;
                        }
                    }
                }
            }
        }
    }

    parseVariables(root);
    if (!mLegacyTheme)
        parseColorSchemes(root);

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
        LOG(LogWarning) << "ThemeData::getElement(): Requested element \"" << view << "." << element
                        << "\" has the wrong type, expected \"" << expectedType << "\", got \""
                        << elemIt->second.type << "\"";
        return nullptr;
    }

    return &elemIt->second;
}

void ThemeData::populateThemeSets()
{
    assert(sThemeSets.empty());

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

    for (size_t i {0}; i < pathCount; ++i) {
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

                std::string themeName;
                if (capabilities.themeName != "") {
                    themeName.append(" (theme name \"")
                        .append(capabilities.themeName)
                        .append("\")");
                }

#if defined(_WIN64)
                LOG(LogInfo) << "Added" << (capabilities.legacyTheme ? " legacy" : "")
                             << " theme set \"" << Utils::String::replace(*it, "/", "\\") << "\""
                             << themeName;
#else
                LOG(LogInfo) << "Added" << (capabilities.legacyTheme ? " legacy" : "")
                             << " theme set \"" << *it << "\"" << themeName;
#endif
                if (!capabilities.legacyTheme) {
                    int aspectRatios {0};
                    if (capabilities.aspectRatios.size() > 0)
                        aspectRatios = static_cast<int>(capabilities.aspectRatios.size()) - 1;
                    LOG(LogDebug) << "Theme set includes support for "
                                  << capabilities.variants.size() << " variant"
                                  << (capabilities.variants.size() != 1 ? "s" : "") << ", "
                                  << capabilities.colorSchemes.size() << " color scheme"
                                  << (capabilities.colorSchemes.size() != 1 ? "s" : "") << ", "
                                  << aspectRatios << " aspect ratio"
                                  << (aspectRatios != 1 ? "s" : "") << " and "
                                  << capabilities.transitions.size() << " transition"
                                  << (capabilities.transitions.size() != 1 ? "s" : "");
                }
                ThemeSet set {*it, capabilities};
                sThemeSets[set.getName()] = set;
            }
        }
    }

    if (sThemeSets.empty()) {
        LOG(LogWarning) << "Couldn't find any theme sets, creating dummy entry";
        ThemeSet set {"no-theme-sets", ThemeCapability()};
        sThemeSets[set.getName()] = set;
        sCurrentThemeSet = sThemeSets.begin();
    }
}

const std::string ThemeData::getThemeFromCurrentSet(const std::string& system)
{
    if (sThemeSets.empty())
        getThemeSets();

    if (sThemeSets.empty())
        // No theme sets available.
        return "";

    std::map<std::string, ThemeSet, StringComparator>::const_iterator set {
        sThemeSets.find(Settings::getInstance()->getString("ThemeSet"))};
    if (set == sThemeSets.cend()) {
        // Currently configured theme set is missing, attempt to load the default theme set
        // slate-es-de instead, and if that's also missing then pick the first available set.
        bool defaultSetFound {true};

        set = sThemeSets.find("slate-es-de");

        if (set == sThemeSets.cend()) {
            set = sThemeSets.cbegin();
            defaultSetFound = false;
        }

        LOG(LogWarning) << "Configured theme set \""
                        << Settings::getInstance()->getString("ThemeSet")
                        << "\" does not exist, loading" << (defaultSetFound ? " default " : " ")
                        << "theme set \"" << set->first << "\" instead";

        Settings::getInstance()->setString("ThemeSet", set->first);
        sCurrentThemeSet = sThemeSets.find(Settings::getInstance()->getString("ThemeSet"));
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

void ThemeData::setThemeTransitions()
{
    auto setTransitionsFunc = [](int transitionAnim) {
        Settings::getInstance()->setInt("TransitionsSystemToSystem", transitionAnim);
        Settings::getInstance()->setInt("TransitionsSystemToGamelist", transitionAnim);
        Settings::getInstance()->setInt("TransitionsGamelistToGamelist", transitionAnim);
        Settings::getInstance()->setInt("TransitionsGamelistToSystem", transitionAnim);
        Settings::getInstance()->setInt("TransitionsStartupToSystem", transitionAnim);
        Settings::getInstance()->setInt("TransitionsStartupToGamelist", transitionAnim);
    };

    int transitionAnim {ViewTransitionAnimation::INSTANT};
    setTransitionsFunc(transitionAnim);

    if (sCurrentThemeSet->second.capabilities.legacyTheme) {
        const std::string& legacyTransitionsSetting {
            Settings::getInstance()->getString("LegacyThemeTransitions")};
        if (legacyTransitionsSetting == "builtin-slide")
            transitionAnim = static_cast<int>(ViewTransitionAnimation::SLIDE);
        else if (legacyTransitionsSetting == "builtin-fade")
            transitionAnim = static_cast<int>(ViewTransitionAnimation::FADE);
        setTransitionsFunc(transitionAnim);
    }
    else {
        const std::string& transitionsSetting {
            Settings::getInstance()->getString("ThemeTransitions")};
        std::string profile;
        size_t profileEntry {0};

        if (transitionsSetting == "automatic") {
            if (sVariantDefinedTransitions != "")
                profile = sVariantDefinedTransitions;
            else if (!sCurrentThemeSet->second.capabilities.transitions.empty())
                profile = sCurrentThemeSet->second.capabilities.transitions.front().name;
        }
        else {
            profile = transitionsSetting;
        }

        auto it = std::find_if(
            sCurrentThemeSet->second.capabilities.transitions.cbegin(),
            sCurrentThemeSet->second.capabilities.transitions.cend(),
            [&profile](const ThemeTransitions transitions) { return transitions.name == profile; });
        if (it != sCurrentThemeSet->second.capabilities.transitions.cend())
            profileEntry = static_cast<size_t>(
                std::distance(sCurrentThemeSet->second.capabilities.transitions.cbegin(), it) + 1);

        if (profileEntry != 0 &&
            sCurrentThemeSet->second.capabilities.transitions.size() > profileEntry - 1) {
            auto transitionMap =
                sCurrentThemeSet->second.capabilities.transitions[profileEntry - 1].animations;
            if (transitionMap.find(ViewTransition::SYSTEM_TO_SYSTEM) != transitionMap.end())
                Settings::getInstance()->setInt("TransitionsSystemToSystem",
                                                transitionMap[ViewTransition::SYSTEM_TO_SYSTEM]);
            if (transitionMap.find(ViewTransition::SYSTEM_TO_GAMELIST) != transitionMap.end())
                Settings::getInstance()->setInt("TransitionsSystemToGamelist",
                                                transitionMap[ViewTransition::SYSTEM_TO_GAMELIST]);
            if (transitionMap.find(ViewTransition::GAMELIST_TO_GAMELIST) != transitionMap.end())
                Settings::getInstance()->setInt(
                    "TransitionsGamelistToGamelist",
                    transitionMap[ViewTransition::GAMELIST_TO_GAMELIST]);
            if (transitionMap.find(ViewTransition::GAMELIST_TO_SYSTEM) != transitionMap.end())
                Settings::getInstance()->setInt("TransitionsGamelistToSystem",
                                                transitionMap[ViewTransition::GAMELIST_TO_SYSTEM]);
            if (transitionMap.find(ViewTransition::STARTUP_TO_SYSTEM) != transitionMap.end())
                Settings::getInstance()->setInt("TransitionsStartupToSystem",
                                                transitionMap[ViewTransition::STARTUP_TO_SYSTEM]);
            if (transitionMap.find(ViewTransition::STARTUP_TO_GAMELIST) != transitionMap.end())
                Settings::getInstance()->setInt("TransitionsStartupToGamelist",
                                                transitionMap[ViewTransition::STARTUP_TO_GAMELIST]);
        }
        else if (transitionsSetting == "builtin-slide" || transitionsSetting == "builtin-fade") {
            if (std::find(
                    sCurrentThemeSet->second.capabilities.suppressedTransitionProfiles.cbegin(),
                    sCurrentThemeSet->second.capabilities.suppressedTransitionProfiles.cend(),
                    transitionsSetting) ==
                sCurrentThemeSet->second.capabilities.suppressedTransitionProfiles.cend()) {
                if (transitionsSetting == "builtin-slide") {
                    transitionAnim = static_cast<int>(ViewTransitionAnimation::SLIDE);
                }
                else if (transitionsSetting == "builtin-fade") {
                    transitionAnim = static_cast<int>(ViewTransitionAnimation::FADE);
                }
                setTransitionsFunc(transitionAnim);
            }
        }
    }
}

const std::map<ThemeTriggers::TriggerType, std::pair<std::string, std::vector<std::string>>>
ThemeData::getCurrentThemeSetSelectedVariantOverrides()
{
    const auto variantIter = std::find_if(
        sCurrentThemeSet->second.capabilities.variants.cbegin(),
        sCurrentThemeSet->second.capabilities.variants.cend(),
        [this](ThemeVariant currVariant) { return currVariant.name == mSelectedVariant; });

    if (variantIter != sCurrentThemeSet->second.capabilities.variants.cend() &&
        !(*variantIter).overrides.empty())
        return (*variantIter).overrides;
    else
        return ThemeVariant().overrides;
}

const void ThemeData::themeLoadedLogOutput()
{
    if (sCurrentThemeSet->second.capabilities.legacyTheme) {
        LOG(LogInfo) << "Finished loading legacy theme set \"" << sCurrentThemeSet->first << "\"";
    }
    else {
        LOG(LogInfo) << "Finished loading theme set \"" << sCurrentThemeSet->first << "\"";
        if (sSelectedAspectRatio != "") {
            const bool autoDetect {Settings::getInstance()->getString("ThemeAspectRatio") ==
                                   "automatic"};
            const std::string match {sAspectRatioMatch ? "exact match " : "closest match "};

            LOG(LogInfo) << "Aspect ratio " << (autoDetect ? "automatically " : "manually ")
                         << "set to " << (autoDetect ? match : "") << "\""
                         << Utils::String::replace(sSelectedAspectRatio, "_", " ") << "\"";
        }
    }
}

unsigned int ThemeData::getHexColor(const std::string& str)
{
    ThemeException error;

    if (str == "")
        throw error << "Empty color property";

    const size_t length {str.size()};
    if (length != 6 && length != 8)
        throw error << "Invalid color property \"" << str
                    << "\" (must be 6 or 8 characters in length)";

    unsigned int value;
    std::stringstream ss;
    ss << str;
    ss >> std::hex >> value;

    if (length == 6)
        value = (value << 8) | 0xFF;

    return value;
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
    bool hasTriggers {false};

    const std::string capFile {path + "/capabilities.xml"};

    if (Utils::FileSystem::isRegularFile(capFile) || Utils::FileSystem::isSymlink(capFile)) {
        capabilities.legacyTheme = false;

        pugi::xml_document doc;
#if defined(_WIN64)
        const pugi::xml_parse_result& res {
            doc.load_file(Utils::String::stringToWideString(capFile).c_str())};
#else
        const pugi::xml_parse_result& res {doc.load_file(capFile.c_str())};
#endif
        if (res.status == pugi::status_no_document_element) {
            LOG(LogDebug) << "Found a capabilities.xml file with no configuration";
        }
        else if (!res) {
            LOG(LogError) << "Couldn't parse capabilities.xml: " << res.description();
            return capabilities;
        }
        const pugi::xml_node& themeCapabilities {doc.child("themeCapabilities")};
        if (!themeCapabilities) {
            LOG(LogError) << "Missing <themeCapabilities> tag in capabilities.xml";
            return capabilities;
        }

        const pugi::xml_node& themeName {themeCapabilities.child("themeName")};
        if (themeName != nullptr)
            capabilities.themeName = themeName.text().get();

        for (pugi::xml_node aspectRatio {themeCapabilities.child("aspectRatio")}; aspectRatio;
             aspectRatio = aspectRatio.next_sibling("aspectRatio")) {
            const std::string& value {aspectRatio.text().get()};
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

        for (pugi::xml_node variant {themeCapabilities.child("variant")}; variant;
             variant = variant.next_sibling("variant")) {
            ThemeVariant readVariant;
            const std::string& name {variant.attribute("name").as_string()};
            if (name.empty()) {
                LOG(LogWarning)
                    << "Found <variant> tag without name attribute, ignoring entry in \"" << capFile
                    << "\"";
            }
            else if (name == "all") {
                LOG(LogWarning)
                    << "Found <variant> tag using reserved name \"all\", ignoring entry in \""
                    << capFile << "\"";
            }
            else {
                readVariant.name = name;
            }

            const pugi::xml_node& labelTag {variant.child("label")};
            if (labelTag == nullptr) {
                LOG(LogDebug)
                    << "No variant <label> tag found, setting label value to the variant name \""
                    << name << "\" for \"" << capFile << "\"";
                readVariant.label = name;
            }
            else {
                const std::string& labelValue {labelTag.text().as_string()};
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

            const pugi::xml_node& selectableTag {variant.child("selectable")};
            if (selectableTag != nullptr) {
                const std::string& value {selectableTag.text().as_string()};
                if (value.front() == '0' || value.front() == 'f' || value.front() == 'F' ||
                    value.front() == 'n' || value.front() == 'N')
                    readVariant.selectable = false;
                else
                    readVariant.selectable = true;
            }

            for (pugi::xml_node overrideTag {variant.child("override")}; overrideTag;
                 overrideTag = overrideTag.next_sibling("override")) {
                if (overrideTag != nullptr) {
                    std::vector<std::string> mediaTypes;
                    const pugi::xml_node& mediaTypeTag {overrideTag.child("mediaType")};
                    if (mediaTypeTag != nullptr) {
                        std::string mediaTypeValue {mediaTypeTag.text().as_string()};
                        for (auto& character : mediaTypeValue) {
                            if (std::isspace(character))
                                character = ',';
                        }
                        mediaTypeValue = Utils::String::replace(mediaTypeValue, ",,", ",");
                        mediaTypes = Utils::String::delimitedStringToVector(mediaTypeValue, ",");

                        for (std::string& type : mediaTypes) {
                            if (std::find(sSupportedMediaTypes.cbegin(),
                                          sSupportedMediaTypes.cend(),
                                          type) == sSupportedMediaTypes.cend()) {
                                LOG(LogError) << "ThemeData::parseThemeCapabilities(): Invalid "
                                                 "override configuration, unsupported "
                                                 "\"mediaType\" value \""
                                              << type << "\"";
                                mediaTypes.clear();
                                break;
                            }
                        }
                    }

                    const pugi::xml_node& triggerTag {overrideTag.child("trigger")};
                    if (triggerTag != nullptr) {
                        const std::string& triggerValue {triggerTag.text().as_string()};
                        if (triggerValue == "") {
                            LOG(LogWarning) << "No <trigger> tag value defined for variant \""
                                            << readVariant.name << "\", ignoring entry in \""
                                            << capFile << "\"";
                        }
                        else if (triggerValue != "noVideos" && triggerValue != "noMedia") {
                            LOG(LogWarning) << "Invalid <useVariant> tag value \"" << triggerValue
                                            << "\" defined for variant \"" << readVariant.name
                                            << "\", ignoring entry in \"" << capFile << "\"";
                        }
                        else {
                            const pugi::xml_node& useVariantTag {overrideTag.child("useVariant")};
                            if (useVariantTag != nullptr) {
                                const std::string& useVariantValue {
                                    useVariantTag.text().as_string()};
                                if (useVariantValue == "") {
                                    LOG(LogWarning)
                                        << "No <useVariant> tag value defined for variant \""
                                        << readVariant.name << "\", ignoring entry in \"" << capFile
                                        << "\"";
                                }
                                else {
                                    hasTriggers = true;
                                    if (triggerValue == "noVideos") {
                                        readVariant
                                            .overrides[ThemeTriggers::TriggerType::NO_VIDEOS] =
                                            std::make_pair(useVariantValue,
                                                           std::vector<std::string>());
                                    }
                                    else if (triggerValue == "noMedia") {
                                        if (mediaTypes.empty())
                                            mediaTypes.emplace_back("miximage");
                                        readVariant
                                            .overrides[ThemeTriggers::TriggerType::NO_MEDIA] =
                                            std::make_pair(useVariantValue, mediaTypes);
                                    }
                                }
                            }
                            else {
                                LOG(LogWarning)
                                    << "Found an <override> tag without a corresponding "
                                       "<useVariant> tag, "
                                    << "ignoring entry for variant \"" << readVariant.name
                                    << "\" in \"" << capFile << "\"";
                            }
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

        for (pugi::xml_node colorScheme {themeCapabilities.child("colorScheme")}; colorScheme;
             colorScheme = colorScheme.next_sibling("colorScheme")) {
            ThemeColorScheme readColorScheme;
            const std::string& name {colorScheme.attribute("name").as_string()};
            if (name.empty()) {
                LOG(LogWarning)
                    << "Found <colorScheme> tag without name attribute, ignoring entry in \""
                    << capFile << "\"";
            }
            else {
                readColorScheme.name = name;
            }

            const pugi::xml_node& labelTag {colorScheme.child("label")};
            if (labelTag == nullptr) {
                LOG(LogDebug) << "No colorScheme <label> tag found, setting label value to the "
                                 "color scheme name \""
                              << name << "\" for \"" << capFile << "\"";
                readColorScheme.label = name;
            }
            else {
                const std::string& labelValue {labelTag.text().as_string()};
                if (labelValue == "") {
                    LOG(LogWarning) << "No colorScheme <label> value defined, setting value to "
                                       "the color scheme name \""
                                    << name << "\" for \"" << capFile << "\"";
                    readColorScheme.label = name;
                }
                else {
                    readColorScheme.label = labelValue;
                }
            }

            if (readColorScheme.name != "") {
                bool duplicate {false};
                for (auto& colorScheme : capabilities.colorSchemes) {
                    if (colorScheme.name == readColorScheme.name) {
                        LOG(LogWarning) << "Color scheme \"" << readColorScheme.name
                                        << "\" is declared multiple times, ignoring entry in \""
                                        << capFile << "\"";
                        duplicate = true;
                        break;
                    }
                }
                if (!duplicate)
                    capabilities.colorSchemes.emplace_back(readColorScheme);
            }
        }

        for (pugi::xml_node transitions {themeCapabilities.child("transitions")}; transitions;
             transitions = transitions.next_sibling("transitions")) {
            std::map<ViewTransition, ViewTransitionAnimation> readTransitions;
            std::string name {transitions.attribute("name").as_string()};
            std::string label;
            bool selectable {true};

            if (name.empty()) {
                LOG(LogWarning)
                    << "Found <transitions> tag without name attribute, ignoring entry in \""
                    << capFile << "\"";
                name.clear();
            }
            else {
                if (std::find(sSupportedTransitionAnimations.cbegin(),
                              sSupportedTransitionAnimations.cend(),
                              name) != sSupportedTransitionAnimations.cend()) {
                    LOG(LogWarning)
                        << "Found <transitions> tag using reserved name attribute value \"" << name
                        << "\", ignoring entry in \"" << capFile << "\"";
                    name.clear();
                }
                else {
                    for (auto& transitionEntry : capabilities.transitions) {
                        if (transitionEntry.name == name) {
                            LOG(LogWarning)
                                << "Found <transitions> tag with previously used name attribute "
                                   "value \""
                                << name << "\", ignoring entry in \"" << capFile << "\"";
                            name.clear();
                        }
                    }
                }
            }

            if (name == "")
                continue;

            const pugi::xml_node& labelTag {transitions.child("label")};
            if (labelTag != nullptr)
                label = labelTag.text().as_string();

            const pugi::xml_node& selectableTag {transitions.child("selectable")};
            if (selectableTag != nullptr) {
                const std::string& value {selectableTag.text().as_string()};
                if (value.front() == '0' || value.front() == 'f' || value.front() == 'F' ||
                    value.front() == 'n' || value.front() == 'N')
                    selectable = false;
            }

            for (auto& currTransition : sSupportedTransitions) {
                const pugi::xml_node& transitionTag {transitions.child(currTransition.c_str())};
                if (transitionTag != nullptr) {
                    const std::string& transitionValue {transitionTag.text().as_string()};
                    if (transitionValue.empty()) {
                        LOG(LogWarning) << "Found <" << currTransition
                                        << "> transition tag without any value, "
                                           "ignoring entry in \""
                                        << capFile << "\"";
                    }
                    else if (std::find(sSupportedTransitionAnimations.cbegin(),
                                       sSupportedTransitionAnimations.cend(),
                                       currTransition) != sSupportedTransitionAnimations.cend()) {
                        LOG(LogWarning)
                            << "Invalid <" << currTransition << "> transition tag value \""
                            << transitionValue << "\", ignoring entry in \"" << capFile << "\"";
                    }
                    else {
                        ViewTransitionAnimation transitionAnim {ViewTransitionAnimation::INSTANT};
                        if (transitionValue == "slide")
                            transitionAnim = ViewTransitionAnimation::SLIDE;
                        else if (transitionValue == "fade")
                            transitionAnim = ViewTransitionAnimation::FADE;

                        if (currTransition == "systemToSystem")
                            readTransitions[ViewTransition::SYSTEM_TO_SYSTEM] = transitionAnim;
                        else if (currTransition == "systemToGamelist")
                            readTransitions[ViewTransition::SYSTEM_TO_GAMELIST] = transitionAnim;
                        else if (currTransition == "gamelistToGamelist")
                            readTransitions[ViewTransition::GAMELIST_TO_GAMELIST] = transitionAnim;
                        else if (currTransition == "gamelistToSystem")
                            readTransitions[ViewTransition::GAMELIST_TO_SYSTEM] = transitionAnim;
                        else if (currTransition == "startupToSystem")
                            readTransitions[ViewTransition::STARTUP_TO_SYSTEM] = transitionAnim;
                        else if (currTransition == "startupToGamelist")
                            readTransitions[ViewTransition::STARTUP_TO_GAMELIST] = transitionAnim;
                    }
                }
            }

            if (!readTransitions.empty()) {
                // If startupToSystem and startupToGamelist are not defined, then set them
                // to the same values as systemToSystem and gamelistToGamelist respectively,
                // assuming those transitions have been defined.
                if (readTransitions.find(ViewTransition::STARTUP_TO_SYSTEM) ==
                    readTransitions.cend()) {
                    if (readTransitions.find(ViewTransition::SYSTEM_TO_SYSTEM) !=
                        readTransitions.cend())
                        readTransitions[ViewTransition::STARTUP_TO_SYSTEM] =
                            readTransitions[ViewTransition::SYSTEM_TO_SYSTEM];
                }
                if (readTransitions.find(ViewTransition::STARTUP_TO_GAMELIST) ==
                    readTransitions.cend()) {
                    if (readTransitions.find(ViewTransition::GAMELIST_TO_GAMELIST) !=
                        readTransitions.cend())
                        readTransitions[ViewTransition::STARTUP_TO_GAMELIST] =
                            readTransitions[ViewTransition::GAMELIST_TO_GAMELIST];
                }

                ThemeTransitions transition;
                transition.name = name;
                transition.label = label;
                transition.selectable = selectable;
                transition.animations = std::move(readTransitions);
                capabilities.transitions.emplace_back(std::move(transition));
            }
        }

        for (pugi::xml_node suppressTransitionProfiles {
                 themeCapabilities.child("suppressTransitionProfiles")};
             suppressTransitionProfiles;
             suppressTransitionProfiles =
                 suppressTransitionProfiles.next_sibling("suppressTransitionProfiles")) {
            std::vector<std::string> readSuppressProfiles;

            for (pugi::xml_node entries {suppressTransitionProfiles.child("entry")}; entries;
                 entries = entries.next_sibling("entry")) {
                const std::string& entryValue {entries.text().as_string()};

                if (std::find(sSupportedTransitionAnimations.cbegin(),
                              sSupportedTransitionAnimations.cend(),
                              entryValue) != sSupportedTransitionAnimations.cend()) {
                    capabilities.suppressedTransitionProfiles.emplace_back(entryValue);
                }
                else {
                    LOG(LogWarning)
                        << "Found suppressTransitionProfiles <entry> tag with invalid value \""
                        << entryValue << "\", ignoring entry in \"" << capFile << "\"";
                }
            }

            // Sort and remove any duplicates.
            if (capabilities.suppressedTransitionProfiles.size() > 1) {
                std::sort(capabilities.suppressedTransitionProfiles.begin(),
                          capabilities.suppressedTransitionProfiles.end());
                auto last = std::unique(capabilities.suppressedTransitionProfiles.begin(),
                                        capabilities.suppressedTransitionProfiles.end());
                capabilities.suppressedTransitionProfiles.erase(
                    last, capabilities.suppressedTransitionProfiles.end());
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
        // Add the "automatic" aspect ratio if there is at least one entry.
        if (!aspectRatiosTemp.empty())
            capabilities.aspectRatios.emplace_back(sSupportedAspectRatios.front().first);
        for (auto& aspectRatio : sSupportedAspectRatios) {
            if (std::find(aspectRatiosTemp.cbegin(), aspectRatiosTemp.cend(), aspectRatio.first) !=
                aspectRatiosTemp.cend()) {
                capabilities.aspectRatios.emplace_back(aspectRatio.first);
            }
        }
    }

    if (hasTriggers) {
        for (auto& variant : capabilities.variants) {
            for (auto it = variant.overrides.begin(); it != variant.overrides.end();) {
                const auto variantIter =
                    std::find_if(capabilities.variants.begin(), capabilities.variants.end(),
                                 [it](ThemeVariant currVariant) {
                                     return currVariant.name == (*it).second.first;
                                 });
                if (variantIter == capabilities.variants.end()) {
                    LOG(LogWarning)
                        << "The <useVariant> tag value \"" << (*it).second.first
                        << "\" does not match any defined variants, ignoring entry in \"" << capFile
                        << "\"";
                    it = variant.overrides.erase(it);
                }
                else {
                    ++it;
                }
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

    for (pugi::xml_node node {root.child("include")}; node; node = node.next_sibling("include")) {
        std::string relPath {resolvePlaceholders(node.text().as_string())};
        std::string path {Utils::FileSystem::resolveRelativePath(relPath, mPaths.back(), true)};

        if (!ResourceManager::getInstance().fileExists(path)) {
            // For explicit paths, throw an error if the file couldn't be found, but only
            // print a debug message if it was set using a variable.
            if (relPath == node.text().get()) {
                throw error << " -> \"" << relPath << "\" not found (resolved to \"" << path
                            << "\")";
            }
            else {
                if (!(Settings::getInstance()->getBool("DebugSkipMissingThemeFiles") ||
                      (mCustomCollection && Settings::getInstance()->getBool(
                                                "DebugSkipMissingThemeFilesCustomCollections")))) {
#if defined(_WIN64)
                    LOG(LogDebug) << Utils::String::replace(error.message, "/", "\\")
                                  << ": Couldn't find file \"" << node.text().get() << "\" "
                                  << ((node.text().get() != path) ?
                                          "which resolves to \"" +
                                              Utils::String::replace(path, "/", "\\") + "\"" :
#else
                    LOG(LogDebug) << error.message << ": Couldn't find file \"" << node.text().get()
                                  << "\" "
                                  << ((node.text().get() != path) ?
                                          "which resolves to \"" + path + "\"" :
#endif
                                          "");
                }
                return;
            }
        }

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

        if (!mLegacyTheme)
            parseTransitions(theme);
        parseVariables(theme);
        if (!mLegacyTheme)
            parseColorSchemes(theme);

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

    for (pugi::xml_node node {root.child("feature")}; node; node = node.next_sibling("feature")) {
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
    if (sCurrentThemeSet == sThemeSets.end())
        return;

    if (mSelectedVariant == "")
        return;

    ThemeException error;
    error << "ThemeData::parseVariants(): ";
    error.setFiles(mPaths);

    for (pugi::xml_node node {root.child("variant")}; node; node = node.next_sibling("variant")) {
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

            const std::string variant {mOverrideVariant.empty() ? mSelectedVariant :
                                                                  mOverrideVariant};

            if (variant == viewKey || viewKey == "all") {
                parseTransitions(node);
                parseVariables(node);
                parseColorSchemes(node);
                parseIncludes(node);
                parseViews(node);
                parseAspectRatios(node);
            }
        }
    }
}

void ThemeData::parseColorSchemes(const pugi::xml_node& root)
{
    if (sCurrentThemeSet == sThemeSets.end())
        return;

    if (mSelectedColorScheme == "")
        return;

    ThemeException error;
    error << "ThemeData::parseColorSchemes(): ";
    error.setFiles(mPaths);

    for (pugi::xml_node node {root.child("colorScheme")}; node;
         node = node.next_sibling("colorScheme")) {
        if (!node.attribute("name"))
            throw error << ": <colorScheme> tag missing \"name\" attribute";

        const std::string delim {" \t\r\n,"};
        const std::string nameAttr {node.attribute("name").as_string()};
        size_t prevOff {nameAttr.find_first_not_of(delim, 0)};
        size_t off {nameAttr.find_first_of(delim, prevOff)};
        std::string viewKey;
        while (off != std::string::npos || prevOff != std::string::npos) {
            viewKey = nameAttr.substr(prevOff, off - prevOff);
            prevOff = nameAttr.find_first_not_of(delim, off);
            off = nameAttr.find_first_of(delim, prevOff);

            if (std::find(mColorSchemes.cbegin(), mColorSchemes.cend(), viewKey) ==
                mColorSchemes.cend()) {
                throw error << ": <colorScheme> value \"" << viewKey
                            << "\" is not defined in capabilities.xml";
            }

            if (mSelectedColorScheme == viewKey)
                parseVariables(node);
        }
    }
}

void ThemeData::parseAspectRatios(const pugi::xml_node& root)
{
    if (sCurrentThemeSet == sThemeSets.end())
        return;

    if (sSelectedAspectRatio == "")
        return;

    ThemeException error;
    error << "ThemeData::parseAspectRatios(): ";
    error.setFiles(mPaths);

    for (pugi::xml_node node {root.child("aspectRatio")}; node;
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

            if (std::find(sCurrentThemeSet->second.capabilities.aspectRatios.cbegin(),
                          sCurrentThemeSet->second.capabilities.aspectRatios.cend(),
                          viewKey) == sCurrentThemeSet->second.capabilities.aspectRatios.cend()) {
                throw error << ": <aspectRatio> value \"" << viewKey
                            << "\" is not defined in capabilities.xml";
            }

            if (sSelectedAspectRatio == viewKey) {
                parseVariables(node);
                parseColorSchemes(node);
                parseIncludes(node);
                parseViews(node);
            }
        }
    }
}

void ThemeData::parseTransitions(const pugi::xml_node& root)
{
    ThemeException error;
    error << "ThemeData::parseTransitions(): ";
    error.setFiles(mPaths);

    const pugi::xml_node& transitions {root.child("transitions")};
    if (transitions != nullptr) {
        const std::string& transitionsValue {transitions.text().as_string()};
        if (std::find_if(sCurrentThemeSet->second.capabilities.transitions.cbegin(),
                         sCurrentThemeSet->second.capabilities.transitions.cend(),
                         [&transitionsValue](const ThemeTransitions transitions) {
                             return transitions.name == transitionsValue;
                         }) == sCurrentThemeSet->second.capabilities.transitions.cend()) {
            throw error << ": <transitions> value \"" << transitionsValue
                        << "\" is not matching any defined transitions";
        }
        sVariantDefinedTransitions = transitionsValue;
    }
}

void ThemeData::parseVariables(const pugi::xml_node& root)
{
    ThemeException error;
    error.setFiles(mPaths);

    for (pugi::xml_node node {root.child("variables")}; node;
         node = node.next_sibling("variables")) {

        for (pugi::xml_node_iterator it = node.begin(); it != node.end(); ++it) {
            const std::string key {it->name()};
            const std::string val {resolvePlaceholders(it->text().as_string())};

            if (!val.empty()) {
                // Overriding existing variables is not allowed for legacy themes.
                if (!mLegacyTheme && mVariables.find(key) != mVariables.end())
                    mVariables[key] = val;
                else
                    mVariables.insert(std::pair<std::string, std::string>(key, val));
            }
        }
    }
}

void ThemeData::parseViews(const pugi::xml_node& root)
{
    ThemeException error;
    error << "ThemeData::parseViews(): ";
    error.setFiles(mPaths);

    // Parse views.
    for (pugi::xml_node node {root.child("view")}; node; node = node.next_sibling("view")) {
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

    for (pugi::xml_node node {root.first_child()}; node; node = node.next_sibling()) {
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
            // component types. Also include workarounds for legacy theme sets for when the
            // fixed labels have been defined with the wrong element type.
            const std::string elementType {node.name()};
            LegacyWorkaround legacyWorkaround {LegacyWorkaround::NONE};

            if (mLegacyTheme && elementType == "text" &&
                (elemKey == "md_releasedate" || elemKey == "md_lastplayed")) {
                LOG(LogDebug) << "ThemeData::parseView(): Element type for \"" << elemKey
                              << "\" incorrectly set to \"text\" "
                                 "instead of \"datetime\", applying workaround";
                legacyWorkaround = LegacyWorkaround::DATETIME;
                elemKey = "datetime_" + elemKey;
            }
            else if (mLegacyTheme && elementType == "datetime" &&
                     (elemKey == "md_lbl_releasedate" || elemKey == "md_lbl_lastplayed")) {
                LOG(LogDebug) << "ThemeData::parseView(): Element type for \"" << elemKey
                              << "\" incorrectly set to \"datetime\" "
                                 "instead of \"text\", applying workaround";
                legacyWorkaround = LegacyWorkaround::TEXT;
                elemKey = "text_" + elemKey;
            }
            else if (mLegacyTheme && elementType == "text" && elemKey == "md_rating") {
                LOG(LogDebug) << "ThemeData::parseView(): Element type for \"" << elemKey
                              << "\" incorrectly set to \"text\" "
                                 "instead of \"rating\", applying workaround";
                legacyWorkaround = LegacyWorkaround::RATING;
                elemKey = "rating_" + elemKey;
            }
            else {
                elemKey = elementType + "_" + elemKey;
            }

            parseElement(
                node, elemTypeIt->second,
                view.elements.insert(std::pair<std::string, ThemeElement>(elemKey, ThemeElement()))
                    .first->second,
                legacyWorkaround);

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
                             const LegacyWorkaround legacyWorkaround)
{
    ThemeException error;
    error << "ThemeData::parseElement(): ";
    error.setFiles(mPaths);

    if (legacyWorkaround == LegacyWorkaround::DATETIME)
        element.type = "datetime";
    else if (legacyWorkaround == LegacyWorkaround::TEXT)
        element.type = "text";
    else if (legacyWorkaround == LegacyWorkaround::RATING)
        element.type = "rating";
    else
        element.type = root.name();

    element.extra = root.attribute("extra").as_bool(false);
    if (mLegacyTheme)
        element.extra = root.attribute("extra").as_bool(false);
    else if (!mLegacyTheme && std::string(root.attribute("extra").as_string("")) != "")
        throw error << ": Legacy \"extra\" attribute found for non-legacy theme set";

    for (pugi::xml_node node {root.first_child()}; node; node = node.next_sibling()) {
        auto typeIt = typeMap.find(node.name());
        if (typeIt == typeMap.cend())
            throw error << ": Unknown property type \"" << node.name()
                        << "\" for element of type \"" << root.name() << "\"";

        std::string str {resolvePlaceholders(node.text().as_string())};

        // Handle the special case with mutually exclusive system variables, for example
        // system.fullName.autoCollections and system.fullName.noCollections which can never
        // exist at the same time. A backspace is assigned in SystemData to flag the
        // variables that do not apply and if it's encountered here we simply skip the
        // property.
        if (!mLegacyTheme && str == "\b")
            continue;

        // Skip this check for legacy themes to not break backward compatibility with some
        // themes sets that include empty property values.
        if (!mLegacyTheme && str == "")
            throw error << ": Property \"" << typeIt->first << "\" for element \"" << element.type
                        << "\" has no value defined";

        std::string nodeName = node.name();

        // Strictly enforce removal of legacy properties for non-legacy theme sets by creating
        // an unthemed system if they're present in the configuration.
        if (!mLegacyTheme) {
            for (auto& legacyProperty : sLegacyProperties) {
                if (nodeName == legacyProperty) {
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
                    // For explicit paths, print a warning if the file couldn't be found, but
                    // only print a debug message if it was set using a variable.
                    if (str == node.text().as_string()) {
#if defined(_WIN64)
                        LOG(LogWarning) << Utils::String::replace(error.message, "/", "\\")
                                        << ": Couldn't find file \"" << node.text().get() << "\" "
                                        << ((node.text().get() != path) ?
                                                "which resolves to \"" +
                                                    Utils::String::replace(path, "/", "\\") + "\"" :
#else
                        LOG(LogWarning)
                            << error.message << ": Couldn't find file \"" << node.text().get()
                            << "\" "
                            << ((node.text().get() != path) ? "which resolves to \"" + path + "\"" :
#endif
                                                "")
                                        << " (element type \"" << element.type << "\", name \""
                                        << root.attribute("name").as_string() << "\", property \""
                                        << nodeName << "\")";
                    }
                    else if (!(Settings::getInstance()->getBool("DebugSkipMissingThemeFiles") ||
                               (mCustomCollection &&
                                Settings::getInstance()->getBool(
                                    "DebugSkipMissingThemeFilesCustomCollections")))) {
#if defined(_WIN64)
                        LOG(LogDebug) << Utils::String::replace(error.message, "/", "\\")
                                      << ": Couldn't find file \"" << node.text().get() << "\" "
                                      << ((node.text().get() != path) ?
                                              "which resolves to \"" +
                                                  Utils::String::replace(path, "/", "\\") + "\"" :
#else
                        LOG(LogDebug)
                            << error.message << ": Couldn't find file \"" << node.text().get()
                            << "\" "
                            << ((node.text().get() != path) ? "which resolves to \"" + path + "\"" :
#endif
                                              "")
                                      << " (element type \"" << element.type << "\", name \""
                                      << root.attribute("name").as_string() << "\", property \""
                                      << nodeName << "\")";
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
