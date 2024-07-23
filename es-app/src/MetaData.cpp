//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  MetaData.cpp
//
//  Static data for default metadata values as well as functions
//  to read and write metadata from the gamelist files.
//

#include "MetaData.h"

#include "Log.h"
#include "utils/FileSystemUtil.h"
#include "utils/LocalizationUtil.h"

#include <pugixml.hpp>

namespace
{
    // clang-format off
    // The statistic entries must be placed at the bottom or otherwise there will be problems with
    // saving the values in GuiMetaDataEd.
    MetaDataDecl gameDecls[] {
    // Key                 Type                 Default value      Statistic  Name in GuiMetaDataEd          Prompt in GuiMetaDataEd             Scrape
    {"name",               MD_STRING,           "",                false,     "NAME",                        "ENTER NAME",                       true},
    {"sortname",           MD_STRING,           "",                false,     "SORTNAME",                    "ENTER SORTNAME",                   false},
    {"collectionsortname", MD_STRING,           "",                false,     "CUSTOM COLLECTIONS SORTNAME", "ENTER COLLECTIONS SORTNAME",       false},
    {"desc",               MD_MULTILINE_STRING, "",                false,     "DESCRIPTION",                 "ENTER DESCRIPTION",                true},
    {"rating",             MD_RATING,           "0",               false,     "RATING",                      "ENTER RATING",                     true},
    {"releasedate",        MD_DATE,             "19700101T000000", false,     "RELEASE DATE",                "ENTER RELEASE DATE",               true},
    {"developer",          MD_STRING,           "unknown",         false,     "DEVELOPER",                   "ENTER DEVELOPER",                  true},
    {"publisher",          MD_STRING,           "unknown",         false,     "PUBLISHER",                   "ENTER PUBLISHER",                  true},
    {"genre",              MD_STRING,           "unknown",         false,     "GENRE",                       "ENTER GENRE",                      true},
    {"players",            MD_STRING,           "unknown",         false,     "PLAYERS",                     "ENTER NUMBER OF PLAYERS",          true},
    {"favorite",           MD_BOOL,             "false",           false,     "FAVORITE",                    "ENTER FAVORITE OFF/ON",            false},
    {"completed",          MD_BOOL,             "false",           false,     "COMPLETED",                   "ENTER COMPLETED OFF/ON",           false},
    {"kidgame",            MD_BOOL,             "false",           false,     "KIDGAME",                     "ENTER KIDGAME OFF/ON",             false},
    {"hidden",             MD_BOOL,             "false",           false,     "HIDDEN",                      "ENTER HIDDEN OFF/ON",              false},
    {"broken",             MD_BOOL,             "false",           false,     "BROKEN/NOT WORKING",          "ENTER BROKEN OFF/ON",              false},
    {"nogamecount",        MD_BOOL,             "false",           false,     "EXCLUDE FROM GAME COUNTER",   "ENTER DON'T COUNT AS GAME OFF/ON", false},
    {"nomultiscrape",      MD_BOOL,             "false",           false,     "EXCLUDE FROM MULTI-SCRAPER",  "ENTER NO MULTI-SCRAPE OFF/ON",     false},
    {"hidemetadata",       MD_BOOL,             "false",           false,     "HIDE METADATA FIELDS",        "ENTER HIDE METADATA OFF/ON",       false},
    {"playcount",          MD_INT,              "0",               false,     "TIMES PLAYED",                "ENTER NUMBER OF TIMES PLAYED",     false},
    {"controller",         MD_CONTROLLER,       "",                false,     "CONTROLLER",                  "SELECT CONTROLLER",                true},
    {"altemulator",        MD_ALT_EMULATOR,     "",                false,     "ALTERNATIVE EMULATOR",        "SELECT ALTERNATIVE EMULATOR",      false},
    {"lastplayed",         MD_TIME,             "0",               true,      "LAST PLAYED",                 "ENTER LAST PLAYED DATE",           false}
    };

    MetaDataDecl folderDecls[] {
    // Key            Type                 Default value      Statistic  Name in GuiMetaDataEd            Prompt in GuiMetaDataEd             Scrape
    {"name",          MD_STRING,           "",                false,     "NAME",                          "ENTER NAME",                       true},
    {"desc",          MD_MULTILINE_STRING, "",                false,     "DESCRIPTION",                   "ENTER DESCRIPTION",                true},
    {"rating",        MD_RATING,           "0",               false,     "RATING",                        "ENTER RATING",                     true},
    {"releasedate",   MD_DATE,             "19700101T000000", false,     "RELEASE DATE",                  "ENTER RELEASE DATE",               true},
    {"developer",     MD_STRING,           "unknown",         false,     "DEVELOPER",                     "ENTER DEVELOPER",                  true},
    {"publisher",     MD_STRING,           "unknown",         false,     "PUBLISHER",                     "ENTER PUBLISHER",                  true},
    {"genre",         MD_STRING,           "unknown",         false,     "GENRE",                         "ENTER GENRE",                      true},
    {"players",       MD_STRING,           "unknown",         false,     "PLAYERS",                       "ENTER NUMBER OF PLAYERS",          true},
    {"favorite",      MD_BOOL,             "false",           false,     "FAVORITE",                      "ENTER FAVORITE OFF/ON",            false},
    {"completed",     MD_BOOL,             "false",           false,     "COMPLETED",                     "ENTER COMPLETED OFF/ON",           false},
    {"kidgame",       MD_BOOL,             "false",           false,     "KIDGAME (ONLY AFFECTS BADGES)", "ENTER KIDGAME OFF/ON",             false},
    {"hidden",        MD_BOOL,             "false",           false,     "HIDDEN",                        "ENTER HIDDEN OFF/ON",              false},
    {"broken",        MD_BOOL,             "false",           false,     "BROKEN/NOT WORKING",            "ENTER BROKEN OFF/ON",              false},
    {"nomultiscrape", MD_BOOL,             "false",           false,     "EXCLUDE FROM MULTI-SCRAPER",    "ENTER NO MULTI-SCRAPE OFF/ON",     false},
    {"hidemetadata",  MD_BOOL,             "false",           false,     "HIDE METADATA FIELDS",          "ENTER HIDE METADATA OFF/ON",       false},
    {"controller",    MD_CONTROLLER,       "",                false,     "CONTROLLER",                    "SELECT CONTROLLER",                true},
    {"folderlink",    MD_FOLDER_LINK,      "",                false,     "FOLDER LINK",                   "SELECT FOLDER LINK",               false},
    {"lastplayed",    MD_TIME,             "0",               true,      "LAST PLAYED",                   "ENTER LAST PLAYED DATE",           false}
    };
    // clang-format on

    const std::vector<MetaDataDecl> gameMDD {gameDecls,
                                             gameDecls + sizeof(gameDecls) / sizeof(gameDecls[0])};

    const std::vector<MetaDataDecl> folderMDD {
        folderDecls, folderDecls + sizeof(folderDecls) / sizeof(folderDecls[0])};

} // namespace

const std::vector<MetaDataDecl>& getMDDByType(MetaDataListType type)
{
    switch (type) {
        case GAME_METADATA:
            return gameMDD;
        case FOLDER_METADATA:
            return folderMDD;
    }

    LOG(LogError) << "Invalid MDD type";
    return gameMDD;
}

MetaDataList::MetaDataList(MetaDataListType type)
    : mType(type)
    , mWasChanged(false)
{
    const std::vector<MetaDataDecl>& mdd = getMDD();
    for (auto it = mdd.cbegin(); it != mdd.cend(); ++it)
        set(it->key, it->defaultValue);
}

MetaDataList MetaDataList::createFromXML(MetaDataListType type,
                                         pugi::xml_node& node,
                                         const std::string& relativeTo)
{
    MetaDataList mdl(type);

    const std::vector<MetaDataDecl>& mdd = mdl.getMDD();

    for (auto it = mdd.cbegin(); it != mdd.cend(); ++it) {
        pugi::xml_node md = node.child(it->key.c_str());
        if (md && !md.text().empty()) {
            // If it's a path, resolve relative paths.
            std::string value = md.text().get();
            if (it->type == MD_PATH)
                value = Utils::FileSystem::resolveRelativePath(value, relativeTo, true);
            mdl.set(it->key, value);
        }
        else {
            mdl.set(it->key, it->defaultValue);
        }
    }
    return mdl;
}

void MetaDataList::appendToXML(pugi::xml_node& parent,
                               bool ignoreDefaults,
                               const std::string& relativeTo) const
{
    const std::vector<MetaDataDecl>& mdd = getMDD();

    for (auto it = mdd.cbegin(); it != mdd.cend(); ++it) {
        auto mapIter = mMap.find(it->key);
        if (mapIter != mMap.cend()) {
            // We have this value!
            // If it's just the default (and we ignore defaults), don't write it.
            if (ignoreDefaults && mapIter->second == it->defaultValue)
                continue;

            // Try and make paths relative if we can.
            std::string value = mapIter->second;
            if (it->type == MD_PATH)
                value = Utils::FileSystem::createRelativePath(value, relativeTo, true);

            parent.append_child(mapIter->first.c_str()).text().set(value.c_str());
        }
    }
}

void MetaDataList::set(const std::string& key, const std::string& value)
{
    mMap[key] = value;
    mWasChanged = true;
}

const std::string& MetaDataList::get(const std::string& key) const
{
    // Check that the key actually exists, otherwise return an empty string.
    if (mMap.count(key) > 0)
        return mMap.at(key);
    else
        return mNoResult;
}

int MetaDataList::getInt(const std::string& key) const
{
    // Return integer value.
    return atoi(get(key).c_str());
}

float MetaDataList::getFloat(const std::string& key) const
{
    // Return float value.
    return static_cast<float>(atof(get(key).c_str()));
}

bool MetaDataList::wasChanged() const
{
    // Return whether the metadata was changed.
    return mWasChanged;
}

void MetaDataList::resetChangedFlag()
{
    // Reset the change flag.
    mWasChanged = false;
}

#if defined(GETTEXT_DUMMY_ENTRIES)
void gettextMessageCatalogEntries()
{
    _p("metadata", "NAME");
    _p("metadata", "ENTER NAME");
    _p("metadata", "SORTNAME");
    _p("metadata", "ENTER SORTNAME");
    _p("metadata", "CUSTOM COLLECTIONS SORTNAME");
    _p("metadata", "ENTER COLLECTIONS SORTNAME");
    _p("metadata", "DESCRIPTION");
    _p("metadata", "ENTER DESCRIPTION");
    _p("metadata", "RATING");
    _p("metadata", "RELEASE DATE");
    _p("metadata", "DEVELOPER");
    _p("metadata", "ENTER DEVELOPER");
    _p("metadata", "PUBLISHER");
    _p("metadata", "ENTER PUBLISHER");
    _p("metadata", "GENRE");
    _p("metadata", "ENTER GENRE");
    _p("metadata", "PLAYERS");
    _p("metadata", "ENTER NUMBER OF PLAYERS");
    _p("metadata", "FAVORITE");
    _p("metadata", "COMPLETED");
    _p("metadata", "KIDGAME");
    _p("metadata", "KIDGAME (ONLY AFFECTS BADGES)");
    _p("metadata", "HIDDEN");
    _p("metadata", "BROKEN/NOT WORKING");
    _p("metadata", "EXCLUDE FROM GAME COUNTER");
    _p("metadata", "EXCLUDE FROM MULTI-SCRAPER");
    _p("metadata", "HIDE METADATA FIELDS");
    _p("metadata", "TIMES PLAYED");
    _p("metadata", "ENTER NUMBER OF TIMES PLAYED");
    _p("metadata", "CONTROLLER");
    _p("metadata", "SELECT CONTROLLER");
    _p("metadata", "ALTERNATIVE EMULATOR");
    _p("metadata", "SELECT ALTERNATIVE EMULATOR");
    _p("metadata", "FOLDER LINK");
    _p("metadata", "SELECT FOLDER LINK");
    _p("metadata", "LAST PLAYED");
    _p("metadata", "ENTER LAST PLAYED DATE");
}
#endif
