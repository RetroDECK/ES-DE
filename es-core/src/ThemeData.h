//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ThemeData.h
//
//  Finds available themes on the file system and loads and parses these.
//  Basic error checking for valid elements and data types is done here,
//  with additional validation handled by the individual components.
//

#ifndef ES_CORE_THEME_DATA_H
#define ES_CORE_THEME_DATA_H

#include "GuiComponent.h"
#include "utils/FileSystemUtil.h"
#include "utils/MathUtil.h"
#include "utils/StringUtil.h"

#include <algorithm>
#include <any>
#include <deque>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

namespace pugi
{
    class xml_node;
}

class GuiComponent;
class ImageComponent;
class NinePatchComponent;
class Sound;
class TextComponent;
class Window;

namespace ThemeFlags
{
    // clang-format off
    // These are only the most common flags shared accross multiple components, in addition
    // to these there are many component-specific options.
    enum PropertyFlags : unsigned int {
        PATH            = 0x00000001,
        POSITION        = 0x00000002,
        SIZE            = 0x00000004,
        ORIGIN          = 0x00000008,
        COLOR           = 0x00000010,
        FONT_PATH       = 0x00000020,
        FONT_SIZE       = 0x00000040,
        ALIGNMENT       = 0x00000080,
        TEXT            = 0x00000100,
        METADATA        = 0x00000200,
        LETTER_CASE     = 0x00000400,
        FORCE_UPPERCASE = 0x00000800, // For backward compatibility with legacy themes.
        LINE_SPACING    = 0x00001000,
        DELAY           = 0x00002000,
        Z_INDEX         = 0x00004000,
        ROTATION        = 0x00008000,
        BRIGHTNESS      = 0x00010000,
        OPACITY         = 0x00020000,
        SATURATION      = 0x00040000,
        VISIBLE         = 0x00080000,
        ALL             = 0xFFFFFFFF
    };
    // clang-format on
} // namespace ThemeFlags

namespace ThemeTriggers
{
    enum class TriggerType {
        NONE,
        NO_VIDEOS,
        NO_MEDIA
    };
} // namespace ThemeTriggers

class ThemeException : public std::exception
{
public:
    std::string message;

    const char* what() const throw() { return message.c_str(); }

    template <typename T> friend ThemeException& operator<<(ThemeException& e, T message)
    {
        std::stringstream ss;
        ss << e.message << message;
        e.message = ss.str();
        return e;
    }

    void setFiles(const std::deque<std::string>& deque)
    {
        // Add all paths to the error message, separated by -> so it's easy to read the log
        // output in case of theme loading errors.
        *this << "\"" << deque.front() << "\"";
        for (auto it = deque.cbegin() + 1; it != deque.cend(); ++it)
            *this << " -> \"" << (*it) << "\"";
    }
};

class ThemeData
{
public:
    class ThemeElement
    {
    public:
        bool extra;
        std::string type;

        struct Property {
            void operator=(const glm::vec4& value)
            {
                r = value;
                const glm::vec4 initVector {value};
                v = glm::vec2 {initVector.x, initVector.y};
            }
            void operator=(const glm::vec2& value) { v = value; }
            void operator=(const std::string& value) { s = value; }
            void operator=(const unsigned int& value) { i = value; }
            void operator=(const float& value) { f = value; }
            void operator=(const bool& value) { b = value; }

            glm::vec4 r;
            glm::vec2 v;
            std::string s;
            unsigned int i;
            float f;
            bool b;
        };

        std::map<std::string, Property> properties;

        template <typename T> const T get(const std::string& prop) const
        {
#if defined(LEGACY_MACOS)
            if (std::is_same<T, glm::vec2>::value)
                return *(const T*)&properties.at(prop).v;
            else if (std::is_same<T, std::string>::value)
                return *(const T*)&properties.at(prop).s;
            else if (std::is_same<T, unsigned int>::value)
                return *(const T*)&properties.at(prop).i;
            else if (std::is_same<T, float>::value)
                return *(const T*)&properties.at(prop).f;
            else if (std::is_same<T, bool>::value)
                return *(const T*)&properties.at(prop).b;
            else if (std::is_same<T, glm::vec4>::value)
                return *(const T*)&properties.at(prop).r;
            return T();
#else
            if (std::is_same<T, glm::vec2>::value)
                return std::any_cast<const T>(properties.at(prop).v);
            else if (std::is_same<T, std::string>::value)
                return std::any_cast<const T>(properties.at(prop).s);
            else if (std::is_same<T, unsigned int>::value)
                return std::any_cast<const T>(properties.at(prop).i);
            else if (std::is_same<T, float>::value)
                return std::any_cast<const T>(properties.at(prop).f);
            else if (std::is_same<T, bool>::value)
                return std::any_cast<const T>(properties.at(prop).b);
            else if (std::is_same<T, glm::vec4>::value)
                return std::any_cast<const T>(properties.at(prop).r);
            return T();
#endif
        }

        bool has(const std::string& prop) const
        {
            return (properties.find(prop) != properties.cend());
        }
    };

    ThemeData();

    class ThemeView
    {
    public:
        std::map<std::string, ThemeElement> elements;
        std::vector<std::string> legacyOrderedKeys;
    };

    struct ThemeVariant {
        std::string name;
        std::string label;
        bool selectable;
        std::map<ThemeTriggers::TriggerType, std::pair<std::string, std::vector<std::string>>>
            overrides;

        ThemeVariant()
            : selectable {false}
        {
        }
    };

    struct ThemeColorScheme {
        std::string name;
        std::string label;
    };

    struct ThemeTransitions {
        std::string name;
        std::string label;
        bool selectable;
        std::map<ViewTransition, ViewTransitionAnimation> animations;

        ThemeTransitions()
            : selectable {true}
        {
        }
    };

    struct ThemeCapability {
        std::string themeName;
        std::vector<ThemeVariant> variants;
        std::vector<ThemeColorScheme> colorSchemes;
        std::vector<std::string> aspectRatios;
        std::vector<ThemeTransitions> transitions;
        std::vector<std::string> suppressedTransitionProfiles;
        bool legacyTheme;
    };

    struct ThemeSet {
        std::string path;
        ThemeCapability capabilities;

        std::string getName() const { return Utils::FileSystem::getStem(path); }
        std::string getThemePath(const std::string& system) const
        {
            return path + "/" + system + "/theme.xml";
        }
    };

    struct StringComparator {
        bool operator()(const std::string& a, const std::string& b) const
        {
            return Utils::String::toUpper(a) < Utils::String::toUpper(b);
        }
    };

    void loadFile(const std::map<std::string, std::string>& sysDataMap,
                  const std::string& path,
                  const ThemeTriggers::TriggerType trigger,
                  const bool customCollection);
    bool hasView(const std::string& view);
    ThemeView& getViewElements(std::string view) { return mViews[view]; }

    static std::vector<GuiComponent*> makeExtras(const std::shared_ptr<ThemeData>& theme,
                                                 const std::string& view);

    const ThemeElement* getElement(const std::string& view,
                                   const std::string& element,
                                   const std::string& expectedType) const;

    static void populateThemeSets();
    const static std::map<std::string, ThemeSet, StringComparator>& getThemeSets()
    {
        return sThemeSets;
    }
    const static std::string getThemeFromCurrentSet(const std::string& system);
    const static std::string getAspectRatioLabel(const std::string& aspectRatio);
    const static std::string getCurrentThemeSetName() { return sCurrentThemeSet->first; }
    static void setThemeTransitions();

    const bool isLegacyTheme() { return mLegacyTheme; }
    const std::map<ThemeTriggers::TriggerType, std::pair<std::string, std::vector<std::string>>>
    getCurrentThemeSetSelectedVariantOverrides();
    const static void themeLoadedLogOutput();

    enum ElementPropertyType {
        NORMALIZED_RECT,
        NORMALIZED_PAIR,
        PATH,
        STRING,
        COLOR,
        UNSIGNED_INTEGER,
        FLOAT,
        BOOLEAN
    };

    std::map<std::string, std::string> mVariables;

private:
    enum class LegacyWorkaround {
        NONE = 0x00000000,
        TEXT = 0x00000001,
        DATETIME = 0x00000002,
        RATING = 0x00000004
    };

    unsigned int getHexColor(const std::string& str);
    std::string resolvePlaceholders(const std::string& in);

    static ThemeCapability parseThemeCapabilities(const std::string& path);

    void parseIncludes(const pugi::xml_node& root);
    void parseFeatures(const pugi::xml_node& root);
    void parseVariants(const pugi::xml_node& root);
    void parseColorSchemes(const pugi::xml_node& root);
    void parseAspectRatios(const pugi::xml_node& root);
    void parseTransitions(const pugi::xml_node& root);
    void parseVariables(const pugi::xml_node& root);
    void parseViews(const pugi::xml_node& root);
    void parseView(const pugi::xml_node& root, ThemeView& view);
    void parseElement(const pugi::xml_node& root,
                      const std::map<std::string, ElementPropertyType>& typeMap,
                      ThemeElement& element,
                      const LegacyWorkaround legacyWorkaround);

    static std::vector<std::string> sSupportedViews;
    static std::vector<std::string> sSupportedMediaTypes;
    static std::vector<std::string> sSupportedTransitions;
    static std::vector<std::string> sSupportedTransitionAnimations;
    static std::vector<std::string> sLegacySupportedViews;
    static std::vector<std::string> sLegacySupportedFeatures;
    static std::vector<std::string> sLegacyProperties;
    static std::vector<std::pair<std::string, std::string>> sSupportedAspectRatios;
    static std::map<std::string, float> sAspectRatioMap;

    static std::map<std::string, std::map<std::string, std::string>> sPropertyAttributeMap;
    static std::map<std::string, std::map<std::string, ElementPropertyType>> sElementMap;

    static inline std::map<std::string, ThemeSet, StringComparator> sThemeSets;
    static inline std::map<std::string, ThemeSet, StringComparator>::iterator sCurrentThemeSet {};
    static inline std::string sVariantDefinedTransitions;

    std::map<std::string, ThemeView> mViews;
    std::deque<std::string> mPaths;
    std::vector<std::string> mVariants;
    std::vector<std::string> mColorSchemes;
    std::string mSelectedVariant;
    std::string mOverrideVariant;
    std::string mSelectedColorScheme;
    static inline std::string sSelectedAspectRatio;
    static inline bool sAspectRatioMatch {false};
    bool mLegacyTheme;
    bool mCustomCollection;
};

#endif // ES_CORE_THEME_DATA_H
