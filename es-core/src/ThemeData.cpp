//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ThemeData.cpp
//
//  Finds available themes on the file system and loads these,
//  including the parsing of individual theme components
//  (includes, features, variables, views, elements).
//

#include "ThemeData.h"

#include "Log.h"
#include "Platform.h"
#include "Settings.h"
#include "components/ImageComponent.h"
#include "components/TextComponent.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

#include <algorithm>
#include <pugixml.hpp>

std::vector<std::string> ThemeData::sSupportedViews{{"all"},      {"system"}, {"basic"},
                                                    {"detailed"}, {"grid"},   {"video"}};
std::vector<std::string> ThemeData::sSupportedFeatures{
    {"navigationsounds"}, {"video"}, {"carousel"}, {"z-index"}, {"visible"}};

std::map<std::string, std::map<std::string, ThemeData::ElementPropertyType>> ThemeData::sElementMap{
    {"image",
     {{"pos", NORMALIZED_PAIR},
      {"size", NORMALIZED_PAIR},
      {"maxSize", NORMALIZED_PAIR},
      {"origin", NORMALIZED_PAIR},
      {"rotation", FLOAT},
      {"rotationOrigin", NORMALIZED_PAIR},
      {"path", PATH},
      {"default", PATH},
      {"tile", BOOLEAN},
      {"color", COLOR},
      {"colorEnd", COLOR},
      {"gradientType", STRING},
      {"visible", BOOLEAN},
      {"zIndex", FLOAT}}},
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
    {"text",
     {{"pos", NORMALIZED_PAIR},
      {"size", NORMALIZED_PAIR},
      {"origin", NORMALIZED_PAIR},
      {"rotation", FLOAT},
      {"rotationOrigin", NORMALIZED_PAIR},
      {"text", STRING},
      {"backgroundColor", COLOR},
      {"fontPath", PATH},
      {"fontSize", FLOAT},
      {"color", COLOR},
      {"alignment", STRING},
      {"forceUppercase", BOOLEAN},
      {"lineSpacing", FLOAT},
      {"value", STRING},
      {"visible", BOOLEAN},
      {"zIndex", FLOAT}}},
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
      {"scrollSound", PATH}, // For backward compatibility with old themes.
      {"alignment", STRING},
      {"horizontalMargin", FLOAT},
      {"forceUppercase", BOOLEAN},
      {"lineSpacing", FLOAT},
      {"zIndex", FLOAT}}},
    {"container",
     {{"pos", NORMALIZED_PAIR},
      {"size", NORMALIZED_PAIR},
      {"origin", NORMALIZED_PAIR},
      {"visible", BOOLEAN},
      {"zIndex", FLOAT}}},
    {"ninepatch",
     {{"pos", NORMALIZED_PAIR},
      {"size", NORMALIZED_PAIR},
      {"path", PATH},
      {"visible", BOOLEAN},
      {"zIndex", FLOAT}}},
    {"datetime",
     {{"pos", NORMALIZED_PAIR},
      {"size", NORMALIZED_PAIR},
      {"origin", NORMALIZED_PAIR},
      {"rotation", FLOAT},
      {"rotationOrigin", NORMALIZED_PAIR},
      {"backgroundColor", COLOR},
      {"fontPath", PATH},
      {"fontSize", FLOAT},
      {"color", COLOR},
      {"alignment", STRING},
      {"forceUppercase", BOOLEAN},
      {"lineSpacing", FLOAT},
      {"value", STRING},
      {"format", STRING},
      {"displayRelative", BOOLEAN},
      {"visible", BOOLEAN},
      {"zIndex", FLOAT}}},
    {"rating",
     {{"pos", NORMALIZED_PAIR},
      {"size", NORMALIZED_PAIR},
      {"origin", NORMALIZED_PAIR},
      {"rotation", FLOAT},
      {"rotationOrigin", NORMALIZED_PAIR},
      {"color", COLOR},
      {"filledPath", PATH},
      {"unfilledPath", PATH},
      {"visible", BOOLEAN},
      {"zIndex", FLOAT}}},
    {"badges",
     {{"pos", NORMALIZED_PAIR},
      {"size", NORMALIZED_PAIR},
      {"origin", NORMALIZED_PAIR},
      {"rotation", FLOAT},
      {"rotationOrigin", NORMALIZED_PAIR},
      {"alignment", STRING},
      {"direction", STRING},
      {"lines", FLOAT},
      {"itemsPerLine", FLOAT},
      {"itemMargin", NORMALIZED_PAIR},
      {"slots", STRING},
      {"controllerPos", NORMALIZED_PAIR},
      {"controllerSize", FLOAT},
      {"customBadgeIcon", PATH},
      {"customControllerIcon", PATH},
      {"visible", BOOLEAN},
      {"zIndex", FLOAT}}},
    {"sound", {{"path", PATH}}},
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
      {"textStyle", STRING},
      {"customButtonIcon", PATH}}},
    {"navigationsounds",
     {{"systembrowseSound", PATH},
      {"quicksysselectSound", PATH},
      {"selectSound", PATH},
      {"backSound", PATH},
      {"scrollSound", PATH},
      {"favoriteSound", PATH},
      {"launchSound", PATH}}},
    {"video",
     {{"pos", NORMALIZED_PAIR},
      {"size", NORMALIZED_PAIR},
      {"maxSize", NORMALIZED_PAIR},
      {"origin", NORMALIZED_PAIR},
      {"rotation", FLOAT},
      {"rotationOrigin", NORMALIZED_PAIR},
      {"default", PATH},
      {"delay", FLOAT},
      {"visible", BOOLEAN},
      {"zIndex", FLOAT},
      {"showSnapshotNoVideo", BOOLEAN},
      {"showSnapshotDelay", BOOLEAN}}},
    {"carousel",
     {{"type", STRING},
      {"size", NORMALIZED_PAIR},
      {"pos", NORMALIZED_PAIR},
      {"origin", NORMALIZED_PAIR},
      {"color", COLOR},
      {"colorEnd", COLOR},
      {"gradientType", STRING},
      {"logoScale", FLOAT},
      {"logoRotation", FLOAT},
      {"logoRotationOrigin", NORMALIZED_PAIR},
      {"logoSize", NORMALIZED_PAIR},
      {"logoAlignment", STRING},
      {"maxLogoCount", FLOAT},
      {"zIndex", FLOAT},
      {"legacyZIndexMode", STRING}}}};

#define MINIMUM_THEME_FORMAT_VERSION 3
#define CURRENT_THEME_FORMAT_VERSION 7

// Helper.
unsigned int getHexColor(const std::string& str)
{
    ThemeException error;
    if (str == "")
        throw error << "Empty color";

    size_t len = str.size();
    if (len != 6 && len != 8)
        throw error << "Invalid color (bad length, \"" << str << "\" - must be 6 or 8)";

    unsigned int val;
    std::stringstream ss;
    ss << str;
    ss >> std::hex >> val;

    if (len == 6)
        val = (val << 8) | 0xFF;

    return val;
}

std::map<std::string, std::string> mVariables;

std::string resolvePlaceholders(const std::string& in)
{
    if (in.empty())
        return in;

    const size_t variableBegin = in.find("${");
    const size_t variableEnd = in.find("}", variableBegin);

    if ((variableBegin == std::string::npos) || (variableEnd == std::string::npos))
        return in;

    std::string prefix = in.substr(0, variableBegin);
    std::string replace = in.substr(variableBegin + 2, variableEnd - (variableBegin + 2));
    std::string suffix = resolvePlaceholders(in.substr(variableEnd + 1).c_str());

    return prefix + mVariables[replace] + suffix;
}

ThemeData::ThemeData()
{
    // The version will be loaded from the theme file.
    mVersion = 0;
}

void ThemeData::loadFile(std::map<std::string, std::string> sysDataMap, const std::string& path)
{
    mPaths.push_back(path);

    ThemeException error;
    error.setFiles(mPaths);

    if (!Utils::FileSystem::exists(path))
        throw error << "File does not exist";

    mVersion = 0;
    mViews.clear();
    mVariables.clear();

    mVariables.insert(sysDataMap.cbegin(), sysDataMap.cend());

    pugi::xml_document doc;
#if defined(_WIN64)
    pugi::xml_parse_result res = doc.load_file(Utils::String::stringToWideString(path).c_str());
#else
    pugi::xml_parse_result res = doc.load_file(path.c_str());
#endif
    if (!res)
        throw error << "XML parsing error: \n    " << res.description();

    pugi::xml_node root = doc.child("theme");
    if (!root)
        throw error << "Missing <theme> tag!";

    // parse version
    mVersion = root.child("formatVersion").text().as_float(-404);
    if (mVersion == -404)
        throw error << "<formatVersion> tag missing\n   "
                       "It's either out of date or you need to add <formatVersion>"
                    << CURRENT_THEME_FORMAT_VERSION << "</formatVersion> inside your <theme> tag.";

    if (mVersion < MINIMUM_THEME_FORMAT_VERSION)
        throw error << "Theme uses format version " << mVersion << ". Minimum supported version is "
                    << MINIMUM_THEME_FORMAT_VERSION << ".";

    parseVariables(root);
    parseIncludes(root);
    parseViews(root);
    parseFeatures(root);
}

void ThemeData::parseIncludes(const pugi::xml_node& root)
{
    ThemeException error;
    error.setFiles(mPaths);

    for (pugi::xml_node node = root.child("include"); node; node = node.next_sibling("include")) {
        std::string relPath = resolvePlaceholders(node.text().as_string());
        std::string path = Utils::FileSystem::resolveRelativePath(relPath, mPaths.back(), true);
        if (!ResourceManager::getInstance()->fileExists(path))
            throw error << " -> \"" << relPath << "\" not found (resolved to \"" << path << "\")";
        error << " -> \"" << relPath << "\"";

        mPaths.push_back(path);

        pugi::xml_document includeDoc;
#if defined(_WIN64)
        pugi::xml_parse_result result =
            includeDoc.load_file(Utils::String::stringToWideString(path).c_str());
#else
        pugi::xml_parse_result result = includeDoc.load_file(path.c_str());
#endif
        if (!result)
            throw error << ": Error parsing file: " << result.description();

        pugi::xml_node theme = includeDoc.child("theme");
        if (!theme)
            throw error << "Missing <theme> tag ";

        parseVariables(theme);
        parseIncludes(theme);
        parseViews(theme);
        parseFeatures(theme);

        mPaths.pop_back();
    }
}

void ThemeData::parseFeatures(const pugi::xml_node& root)
{
    ThemeException error;
    error.setFiles(mPaths);

    for (pugi::xml_node node = root.child("feature"); node; node = node.next_sibling("feature")) {
        if (!node.attribute("supported"))
            throw error << "Feature missing \"supported\" attribute!";

        const std::string supportedAttr = node.attribute("supported").as_string();

        if (std::find(sSupportedFeatures.cbegin(), sSupportedFeatures.cend(), supportedAttr) !=
            sSupportedFeatures.cend()) {
            parseViews(node);
        }
    }
}

void ThemeData::parseVariables(const pugi::xml_node& root)
{
    ThemeException error;
    error.setFiles(mPaths);

    pugi::xml_node variables = root.child("variables");

    if (!variables)
        return;

    for (pugi::xml_node_iterator it = variables.begin(); it != variables.end(); it++) {
        std::string key = it->name();
        std::string val = it->text().as_string();

        if (!val.empty())
            mVariables.insert(std::pair<std::string, std::string>(key, val));
    }
}

void ThemeData::parseViews(const pugi::xml_node& root)
{
    ThemeException error;
    error.setFiles(mPaths);

    // Parse views.
    for (pugi::xml_node node = root.child("view"); node; node = node.next_sibling("view")) {
        if (!node.attribute("name"))
            throw error << "View missing \"name\" attribute!";

        const std::string delim = " \t\r\n,";
        const std::string nameAttr = node.attribute("name").as_string();
        size_t prevOff = nameAttr.find_first_not_of(delim, 0);
        size_t off = nameAttr.find_first_of(delim, prevOff);
        std::string viewKey;
        while (off != std::string::npos || prevOff != std::string::npos) {
            viewKey = nameAttr.substr(prevOff, off - prevOff);
            prevOff = nameAttr.find_first_not_of(delim, off);
            off = nameAttr.find_first_of(delim, prevOff);

            if (std::find(sSupportedViews.cbegin(), sSupportedViews.cend(), viewKey) !=
                sSupportedViews.cend()) {
                ThemeView& view =
                    mViews.insert(std::pair<std::string, ThemeView>(viewKey, ThemeView()))
                        .first->second;
                parseView(node, view);
            }
        }
    }
}

void ThemeData::parseView(const pugi::xml_node& root, ThemeView& view)
{
    ThemeException error;
    error.setFiles(mPaths);

    for (pugi::xml_node node = root.first_child(); node; node = node.next_sibling()) {
        if (!node.attribute("name"))
            throw error << "Element of type \"" << node.name() << "\" missing \"name\" attribute!";

        auto elemTypeIt = sElementMap.find(node.name());
        if (elemTypeIt == sElementMap.cend())
            throw error << "Unknown element of type \"" << node.name() << "\"!";

        const std::string delim = " \t\r\n,";
        const std::string nameAttr = node.attribute("name").as_string();
        size_t prevOff = nameAttr.find_first_not_of(delim, 0);
        size_t off = nameAttr.find_first_of(delim, prevOff);
        while (off != std::string::npos || prevOff != std::string::npos) {
            std::string elemKey = nameAttr.substr(prevOff, off - prevOff);
            prevOff = nameAttr.find_first_not_of(delim, off);
            off = nameAttr.find_first_of(delim, prevOff);

            parseElement(
                node, elemTypeIt->second,
                view.elements.insert(std::pair<std::string, ThemeElement>(elemKey, ThemeElement()))
                    .first->second);

            if (std::find(view.orderedKeys.cbegin(), view.orderedKeys.cend(), elemKey) ==
                view.orderedKeys.cend())
                view.orderedKeys.push_back(elemKey);
        }
    }
}

void ThemeData::parseElement(const pugi::xml_node& root,
                             const std::map<std::string, ElementPropertyType>& typeMap,
                             ThemeElement& element)
{
    ThemeException error;
    error.setFiles(mPaths);

    element.type = root.name();
    element.extra = root.attribute("extra").as_bool(false);

    for (pugi::xml_node node = root.first_child(); node; node = node.next_sibling()) {
        auto typeIt = typeMap.find(node.name());
        if (typeIt == typeMap.cend())
            throw error << ": Unknown property type \"" << node.name() << "\" (for element of type "
                        << root.name() << ")";

        std::string str = resolvePlaceholders(node.text().as_string());

        switch (typeIt->second) {
            case NORMALIZED_RECT: {
                glm::vec4 val;

                auto splits = Utils::String::delimitedStringToVector(str, " ");
                if (splits.size() == 2) {
                    val = glm::vec4{static_cast<float>(atof(splits.at(0).c_str())),
                                    static_cast<float>(atof(splits.at(1).c_str())),
                                    static_cast<float>(atof(splits.at(0).c_str())),
                                    static_cast<float>(atof(splits.at(1).c_str()))};
                }
                else if (splits.size() == 4) {
                    val = glm::vec4{static_cast<float>(atof(splits.at(0).c_str())),
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
                    throw error << "invalid normalized pair (property \"" << node.name()
                                << "\", value \"" << str.c_str() << "\")";

                std::string first{str.substr(0, divider)};
                std::string second{str.substr(divider, std::string::npos)};

                glm::vec2 val{static_cast<float>(atof(first.c_str())),
                              static_cast<float>(atof(second.c_str()))};

                element.properties[node.name()] = val;
                break;
            }
            case STRING: {
                element.properties[node.name()] = str;
                break;
            }
            case PATH: {
                std::string path = Utils::FileSystem::resolveRelativePath(str, mPaths.back(), true);
                if (!ResourceManager::getInstance()->fileExists(path)) {
                    std::stringstream ss;
                    // "From theme yadda yadda, included file yadda yadda.
                    LOG(LogWarning) << error.msg << ":";
                    LOG(LogWarning)
                        << "Could not find file \"" << node.text().get() << "\" "
                        << ((node.text().get() != path) ? "which resolves to \"" + path + "\"" :
                                                          "");
                }

                // Special parsing instruction for recurring options.
                // Store as its attribute to prevent nodes overwriting each other.
                if (strcmp(node.name(), "customButtonIcon") == 0) {
                    const auto button = node.attribute("button").as_string("");
                    if (strcmp(button, "") == 0)
                        LOG(LogError)
                            << "<customButtonIcon> element requires the `button` property.";
                    else
                        element.properties[button] = path;
                }
                else if (strcmp(node.name(), "customBadgeIcon") == 0) {
                    const auto badge = node.attribute("badge").as_string("");
                    if (strcmp(badge, "") == 0)
                        LOG(LogError) << "<customBadgeIcon> element requires the `badge` property.";
                    else
                        element.properties[badge] = path;
                }
                else if (strcmp(node.name(), "customControllerIcon") == 0) {
                    const auto controller = node.attribute("controller").as_string("");
                    if (strcmp(controller, "") == 0)
                        LOG(LogError)
                            << "<customControllerIcon> element requires the `controller` property.";
                    else
                        element.properties[controller] = path;
                }
                else
                    element.properties[node.name()] = path;

                break;
            }
            case COLOR: {
                element.properties[node.name()] = getHexColor(str);
                break;
            }
            case FLOAT: {
                float floatVal = static_cast<float>(strtod(str.c_str(), 0));
                element.properties[node.name()] = floatVal;
                break;
            }
            case BOOLEAN: {
                // Only look at first char.
                char first = str[0];
                // 1*, t* (true), T* (True), y* (yes), Y* (YES)
                bool boolVal =
                    (first == '1' || first == 't' || first == 'T' || first == 'y' || first == 'Y');

                element.properties[node.name()] = boolVal;
                break;
            }
            default: {
                throw error << "Unknown ElementPropertyType for \""
                            << root.attribute("name").as_string() << "\", property " << node.name();
            }
        }
    }
}

bool ThemeData::hasView(const std::string& view)
{
    auto viewIt = mViews.find(view);
    return (viewIt != mViews.cend());
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

    if (elemIt->second.type != expectedType && !expectedType.empty()) {
        LOG(LogWarning) << " requested mismatched theme type for [" << view << "." << element
                        << "] - expected \"" << expectedType << "\", got \"" << elemIt->second.type
                        << "\"";
        return nullptr;
    }

    return &elemIt->second;
}

const std::shared_ptr<ThemeData>& ThemeData::getDefault()
{
    static std::shared_ptr<ThemeData> theme = nullptr;
    if (theme == nullptr) {
        theme = std::shared_ptr<ThemeData>(new ThemeData());

        const std::string path =
            Utils::FileSystem::getHomePath() + "/.emulationstation/es_theme_default.xml";
        if (Utils::FileSystem::exists(path)) {
            try {
                std::map<std::string, std::string> emptyMap;
                theme->loadFile(emptyMap, path);
            }
            catch (ThemeException& e) {
                LOG(LogError) << e.what();
                theme = std::shared_ptr<ThemeData>(new ThemeData()); // Reset to empty.
            }
        }
    }

    return theme;
}

std::vector<GuiComponent*> ThemeData::makeExtras(const std::shared_ptr<ThemeData>& theme,
                                                 const std::string& view,
                                                 Window* window)
{
    std::vector<GuiComponent*> comps;

    auto viewIt = theme->mViews.find(view);
    if (viewIt == theme->mViews.cend())
        return comps;

    for (auto it = viewIt->second.orderedKeys.cbegin(); // Line break.
         it != viewIt->second.orderedKeys.cend(); it++) {
        ThemeElement& elem = viewIt->second.elements.at(*it);
        if (elem.extra) {
            GuiComponent* comp = nullptr;
            const std::string& t = elem.type;
            if (t == "image")
                comp = new ImageComponent(window);
            else if (t == "text")
                comp = new TextComponent(window);

            if (comp) {
                comp->setDefaultZIndex(10.0f);
                comp->applyTheme(theme, view, *it, ThemeFlags::ALL);
                comps.push_back(comp);
            }
        }
    }

    return comps;
}

std::map<std::string, ThemeSet> ThemeData::getThemeSets()
{
    std::map<std::string, ThemeSet> sets;

    // Check for themes first under the home directory, then under the data installation
    // directory (Unix only) and last under the ES-DE binary directory.

#if defined(__unix__) || defined(__APPLE__)
    static const size_t pathCount = 3;
#else
    static const size_t pathCount = 2;
#endif
    std::string paths[pathCount] = {
        Utils::FileSystem::getExePath() + "/themes",
#if defined(__APPLE__)
        Utils::FileSystem::getExePath() + "/../Resources/themes",
#elif defined(__unix__)
        Utils::FileSystem::getProgramDataPath() + "/themes",
#endif
        Utils::FileSystem::getHomePath() + "/.emulationstation/themes"
    };

    for (size_t i = 0; i < pathCount; i++) {
        if (!Utils::FileSystem::isDirectory(paths[i]))
            continue;

        Utils::FileSystem::stringList dirContent = Utils::FileSystem::getDirContent(paths[i]);

        for (Utils::FileSystem::stringList::const_iterator it = dirContent.cbegin();
             it != dirContent.cend(); it++) {
            if (Utils::FileSystem::isDirectory(*it)) {
                ThemeSet set = {*it};
                sets[set.getName()] = set;
            }
        }
    }

    return sets;
}

std::string ThemeData::getThemeFromCurrentSet(const std::string& system)
{
    std::map<std::string, ThemeSet> themeSets = ThemeData::getThemeSets();
    if (themeSets.empty())
        // No theme sets available.
        return "";

    std::map<std::string, ThemeSet>::const_iterator set =
        themeSets.find(Settings::getInstance()->getString("ThemeSet"));
    if (set == themeSets.cend()) {
        // Currently configured theme set is missing, attempt to load the default theme set
        // rbsimple-DE instead, and if that's also missing then pick the first available set.
        bool defaultSetFound = true;

        set = themeSets.find("rbsimple-DE");

        if (set == themeSets.cend()) {
            set = themeSets.cbegin();
            defaultSetFound = false;
        }

        LOG(LogWarning) << "Configured theme set \""
                        << Settings::getInstance()->getString("ThemeSet")
                        << "\" does not exist, loading" << (defaultSetFound ? " default " : " ")
                        << "theme set \"" << set->first << "\" instead";

        Settings::getInstance()->setString("ThemeSet", set->first);
    }

    return set->second.getThemePath(system);
}
