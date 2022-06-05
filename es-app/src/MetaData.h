//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  MetaData.h
//
//  Static data for default metadata values as well as functions
//  to read and write metadata from the gamelist files.
//

#ifndef ES_APP_META_DATA_H
#define ES_APP_META_DATA_H

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#include <sstream>
#endif

#include <map>
#include <string>
#include <vector>

namespace pugi
{
    class xml_node;
}

enum MetaDataType {
    // Generic types.
    MD_STRING,
    MD_INT,
    MD_FLOAT,
    MD_BOOL,

    // Specialized types.
    MD_MULTILINE_STRING,
    MD_CONTROLLER,
    MD_ALT_EMULATOR,
    MD_FOLDER_LINK,
    MD_PATH,
    MD_RATING,
    MD_DATE,
    MD_TIME // Used for lastplayed.
};

struct MetaDataDecl {
    std::string key;
    MetaDataType type;
    std::string defaultValue;
    // If true, ignore values for this metadata.
    bool isStatistic;
    // Displayed as this in editors.
    std::string displayName;
    // Phrase displayed in editors when prompted to enter value (currently only for strings).
    std::string displayPrompt;
    // If set to false, the scraper will not overwrite this metadata.
    bool shouldScrape;
};

enum MetaDataListType {
    GAME_METADATA,
    FOLDER_METADATA
};

const std::vector<MetaDataDecl>& getMDDByType(MetaDataListType type);

class MetaDataList
{
public:
    static MetaDataList createFromXML(MetaDataListType type,
                                      pugi::xml_node& node,
                                      const std::string& relativeTo);
    void appendToXML(pugi::xml_node& parent,
                     bool ignoreDefaults,
                     const std::string& relativeTo) const;

    MetaDataList(MetaDataListType type);

    void set(const std::string& key, const std::string& value);

    const std::string& get(const std::string& key) const;
    int getInt(const std::string& key) const;
    float getFloat(const std::string& key) const;

    bool wasChanged() const;
    void resetChangedFlag();

    MetaDataListType getType() const { return mType; }
    const std::vector<MetaDataDecl>& getMDD() const { return getMDDByType(getType()); }
    const std::vector<MetaDataDecl>& getMDD(MetaDataListType type) const
    {
        return getMDDByType(type);
    }

private:
    MetaDataListType mType;
    std::map<std::string, std::string> mMap;
    std::string mNoResult = "";
    bool mWasChanged;
};

#endif // ES_APP_META_DATA_H
