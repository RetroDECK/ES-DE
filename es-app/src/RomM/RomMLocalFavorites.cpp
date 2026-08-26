//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMLocalFavorites.cpp
//

#include "RomM/RomMLocalFavorites.h"

#include "Log.h"
#include "utils/FileSystemUtil.h"
#if defined(_WIN64)
#include "utils/StringUtil.h"
#endif

#include <pugixml.hpp>

namespace
{
    std::string getFilePath()
    {
        return Utils::FileSystem::getAppDataDirectory() + "/settings/romm_local_favorites.xml";
    }
} // namespace

RomMLocalFavorites& RomMLocalFavorites::getInstance()
{
    static RomMLocalFavorites instance;
    return instance;
}

RomMLocalFavorites::RomMLocalFavorites() { loadFile(); }

bool RomMLocalFavorites::isFavorite(int rommId) const
{
    return mFavoriteIds.find(rommId) != mFavoriteIds.cend();
}

void RomMLocalFavorites::setFavorite(int rommId, bool favorite)
{
    if (favorite)
        mFavoriteIds.insert(rommId);
    else
        mFavoriteIds.erase(rommId);
    flush();
}

void RomMLocalFavorites::clearAll()
{
    mFavoriteIds.clear();
    flush();
}

void RomMLocalFavorites::loadFile()
{
    mFavoriteIds.clear();

    const std::string path {getFilePath()};
    if (!Utils::FileSystem::exists(path))
        return;

    pugi::xml_document doc;
#if defined(_WIN64)
    pugi::xml_parse_result result {doc.load_file(Utils::String::stringToWideString(path).c_str())};
#else
    pugi::xml_parse_result result {doc.load_file(path.c_str())};
#endif
    if (!result) {
        LOG(LogError) << "Couldn't parse the romm_local_favorites.xml file: "
                      << result.description() << " - no local RomM favorites will be shown";
        return;
    }

    for (pugi::xml_node romNode {doc.child("favorites").child("rom")}; romNode;
         romNode = romNode.next_sibling("rom")) {
        const int rommId {romNode.attribute("id").as_int(-1)};
        if (rommId >= 0)
            mFavoriteIds.insert(rommId);
    }
}

void RomMLocalFavorites::flush()
{
    const std::string path {getFilePath()};

    pugi::xml_document doc;
    pugi::xml_node favoritesNode {doc.append_child("favorites")};
    for (const int rommId : mFavoriteIds)
        favoritesNode.append_child("rom").append_attribute("id").set_value(rommId);

#if defined(_WIN64)
    doc.save_file(Utils::String::stringToWideString(path).c_str());
#else
    doc.save_file(path.c_str());
#endif
}
