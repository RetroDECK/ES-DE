//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ThemeData.h
//
//  Finds available themes on the file system and loads these,
//  including the parsing of individual theme components
//  (includes, features, variables, views, elements).
//

#ifndef ES_CORE_THEME_DATA_H
#define ES_CORE_THEME_DATA_H

#include "utils/FileSystemUtil.h"
#include "utils/MathUtil.h"

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

template <typename T> class TextListComponent;

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
        PATH =            0x00000001,
        POSITION =        0x00000002,
        SIZE =            0x00000004,
        ORIGIN =          0x00000008,
        COLOR =           0x00000010,
        FONT_PATH =       0x00000020,
        FONT_SIZE =       0x00000040,
        ALIGNMENT =       0x00000080,
        TEXT =            0x00000100,
        METADATA =        0x00000200,
        FORCE_UPPERCASE = 0x00000400,
        LINE_SPACING =    0x00000800,
        DELAY =           0x00001000,
        Z_INDEX =         0x00002000,
        ROTATION =        0x00004000,
        VISIBLE =         0x00008000,
        ALL = 0xFFFFFFFF
    };
    // clang-format on
} // namespace ThemeFlags

class ThemeException : public std::exception
{
public:
    std::string msg;

    const char* what() const throw() { return msg.c_str(); }

    template <typename T> friend ThemeException& operator<<(ThemeException& e, T msg);

    void setFiles(const std::deque<std::string>& deque)
    {
        *this << "\"" << deque.front() << "\"";
        for (auto it = deque.cbegin() + 1; it != deque.cend(); ++it)
            *this << " -> \"" << (*it) << "\"";
    }
};

template <typename T> ThemeException& operator<<(ThemeException& e, T appendMsg)
{
    std::stringstream ss;
    ss << e.msg << appendMsg;
    e.msg = ss.str();
    return e;
}

struct ThemeSet {
    std::string path;

    std::string getName() const { return Utils::FileSystem::getStem(path); }
    std::string getThemePath(const std::string& system) const
    {
        return path + "/" + system + "/theme.xml";
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

private:
    class ThemeView
    {
    public:
        std::map<std::string, ThemeElement> elements;
        std::vector<std::string> orderedKeys;
    };

public:
    ThemeData();

    // Throws ThemeException.
    void loadFile(const std::map<std::string, std::string>& sysDataMap, const std::string& path);

    enum ElementPropertyType {
        NORMALIZED_RECT,
        NORMALIZED_PAIR,
        PATH,
        STRING,
        COLOR,
        FLOAT,
        BOOLEAN
    };

    bool hasView(const std::string& view);

    // If expectedType is an empty string, will do no type checking.
    const ThemeElement* getElement(const std::string& view,
                                   const std::string& element,
                                   const std::string& expectedType) const;

    static std::vector<GuiComponent*> makeExtras(const std::shared_ptr<ThemeData>& theme,
                                                 const std::string& view);

    static const std::shared_ptr<ThemeData> getDefault();

    static std::map<std::string, ThemeSet> getThemeSets();
    static std::string getThemeFromCurrentSet(const std::string& system);

private:
    static std::map<std::string, std::map<std::string, ElementPropertyType>> sElementMap;
    static std::vector<std::string> sSupportedFeatures;
    static std::vector<std::string> sSupportedViews;

    std::deque<std::string> mPaths;
    float mVersion;

    void parseFeatures(const pugi::xml_node& themeRoot);
    void parseIncludes(const pugi::xml_node& themeRoot);
    void parseVariables(const pugi::xml_node& root);
    void parseViews(const pugi::xml_node& themeRoot);
    void parseView(const pugi::xml_node& viewNode, ThemeView& view);
    void parseElement(const pugi::xml_node& elementNode,
                      const std::map<std::string, ElementPropertyType>& typeMap,
                      ThemeElement& element);

    std::map<std::string, ThemeView> mViews;
};

#endif // ES_CORE_THEME_DATA_H
