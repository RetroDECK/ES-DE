//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  MetaData.cpp
//
//  Static data for default metadata values as well as functions
//  to read and write metadata from the gamelist files.
//

#include "MetaData.h"

#include "utils/FileSystemUtil.h"
#include "Log.h"

#include <pugixml.hpp>

MetaDataDecl gameDecls[] = {
// key,           type,                default,           statistic, name in GuiMetaDataEd,        prompt in GuiMetaDataEd,            shouldScrape
{"name",          MD_STRING,           "",                false,     "name",                       "enter game name",                  true},
{"sortname",      MD_STRING,           "",                false,     "sortname",                   "enter game sort name",             false},
{"desc",          MD_MULTILINE_STRING, "",                false,     "description",                "enter description",                true},
{"rating",        MD_RATING,           "0",               false,     "rating",                     "enter rating",                     true},
{"releasedate",   MD_DATE,             "19700101T010000", false,     "release date",               "enter release date",               true},
{"developer",     MD_STRING,           "unknown",         false,     "developer",                  "enter game developer",             true},
{"publisher",     MD_STRING,           "unknown",         false,     "publisher",                  "enter game publisher",             true},
{"genre",         MD_STRING,           "unknown",         false,     "genre",                      "enter game genre",                 true},
{"players",       MD_INT,              "unknown",         false,     "players",                    "enter number of players",          true},
{"favorite",      MD_BOOL,             "false",           false,     "favorite",                   "enter favorite off/on",            false},
{"completed",     MD_BOOL,             "false",           false,     "completed",                  "enter completed off/on",           false},
{"kidgame",       MD_BOOL,             "false",           false,     "kidgame",                    "enter kidgame off/on",             false},
{"hidden",        MD_BOOL,             "false",           false,     "hidden",                     "enter hidden off/on",              false},
{"broken",        MD_BOOL,             "false",           false,     "broken/not working",         "enter broken off/on",              false},
{"nogamecount",   MD_BOOL,             "false",           false,     "exclude from game counter",  "enter don't count as game off/on", false},
{"nomultiscrape", MD_BOOL,             "false",           false,     "exclude from multi-scraper", "enter no multi-scrape off/on",     false},
{"hidemetadata",  MD_BOOL,             "false",           false,     "hide metadata fields",       "enter hide metadata off/on",       false},
{"launchcommand", MD_LAUNCHCOMMAND,    "",                false,     "launch command",             "enter game launch command "
                                                                                                   "(emulator override)",              false},
{"playcount",     MD_INT,              "0",               false,     "play count",                 "enter number of times played",     false},
{"lastplayed",    MD_TIME,             "0",               true,      "last played",                "enter last played date",           false}
};

const std::vector<MetaDataDecl> gameMDD(gameDecls, gameDecls +
        sizeof(gameDecls) / sizeof(gameDecls[0]));

MetaDataDecl folderDecls[] = {
{"name",          MD_STRING,           "",                false,     "name",                       "enter game name",                  true},
{"desc",          MD_MULTILINE_STRING, "",                false,     "description",                "enter description",                true},
{"rating",        MD_RATING,           "0",               false,     "rating",                     "enter rating",                     true},
{"releasedate",   MD_DATE,             "19700101T010000", false,     "release date",               "enter release date",               true},
{"developer",     MD_STRING,           "unknown",         false,     "developer",                  "enter game developer",             true},
{"publisher",     MD_STRING,           "unknown",         false,     "publisher",                  "enter game publisher",             true},
{"genre",         MD_STRING,           "unknown",         false,     "genre",                      "enter game genre",                 true},
{"players",       MD_INT,              "unknown",         false,     "players",                    "enter number of players",          true},
{"favorite",      MD_BOOL,             "false",           false,     "favorite",                   "enter favorite off/on",            false},
{"completed",     MD_BOOL,             "false",           false,     "completed",                  "enter completed off/on",           false},
{"hidden",        MD_BOOL,             "false",           false,     "hidden",                     "enter hidden off/on",              false},
{"broken",        MD_BOOL,             "false",           false,     "broken/not working",         "enter broken off/on",              false},
{"nomultiscrape", MD_BOOL,             "false",           false,     "exclude from multi-scraper", "enter no multi-scrape off/on",     false},
{"hidemetadata",  MD_BOOL,             "false",           false,     "hide metadata fields",       "enter hide metadata off/on",       false},
{"lastplayed",    MD_TIME,             "0",               true,      "last played",                "enter last played date",           false}
};

const std::vector<MetaDataDecl> folderMDD(folderDecls, folderDecls +
        sizeof(folderDecls) / sizeof(folderDecls[0]));

const std::vector<MetaDataDecl>& getMDDByType(MetaDataListType type)
{
    switch(type) {
    case GAME_METADATA:
        return gameMDD;
    case FOLDER_METADATA:
        return folderMDD;
    }

    LOG(LogError) << "Invalid MDD type";
    return gameMDD;
}

MetaDataList::MetaDataList(MetaDataListType type)
        : mType(type), mWasChanged(false)
{
    const std::vector<MetaDataDecl>& mdd = getMDD();
    for (auto iter = mdd.cbegin(); iter != mdd.cend(); iter++)
        set(iter->key, iter->defaultValue);
}

MetaDataList MetaDataList::createFromXML(MetaDataListType type,
        pugi::xml_node& node, const std::string& relativeTo)
{
    MetaDataList mdl(type);

    const std::vector<MetaDataDecl>& mdd = mdl.getMDD();

    for (auto iter = mdd.cbegin(); iter != mdd.cend(); iter++) {
        pugi::xml_node md = node.child(iter->key.c_str());
        if (md && !md.text().empty()) {
            // If it's a path, resolve relative paths.
            std::string value = md.text().get();
            if (iter->type == MD_PATH)
                value = Utils::FileSystem::resolveRelativePath(value, relativeTo, true);
            mdl.set(iter->key, value);
        }
        else {
            mdl.set(iter->key, iter->defaultValue);
        }
    }
    return mdl;
}

void MetaDataList::appendToXML(pugi::xml_node& parent, bool ignoreDefaults,
        const std::string& relativeTo) const
{
    const std::vector<MetaDataDecl>& mdd = getMDD();

    for (auto mddIter = mdd.cbegin(); mddIter != mdd.cend(); mddIter++) {
        auto mapIter = mMap.find(mddIter->key);
        if (mapIter != mMap.cend()) {
            // We have this value!
            // If it's just the default (and we ignore defaults), don't write it.
            if (ignoreDefaults && mapIter->second == mddIter->defaultValue)
                continue;

            // Try and make paths relative if we can.
            std::string value = mapIter->second;
            if (mddIter->type == MD_PATH)
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
    // Check that the key actually exists, otherwise return empty string.
    if (mMap.count(key) > 0)
        return mMap.at(key);
    else

    return mNoResult;
}

int MetaDataList::getInt(const std::string& key) const
{
    return atoi(get(key).c_str());
}

float MetaDataList::getFloat(const std::string& key) const
{
    return (float)atof(get(key).c_str());
}

bool MetaDataList::wasChanged() const
{
    return mWasChanged;
}

void MetaDataList::resetChangedFlag()
{
    mWasChanged = false;
}
