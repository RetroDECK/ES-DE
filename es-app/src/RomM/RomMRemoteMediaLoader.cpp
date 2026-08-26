//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMRemoteMediaLoader.cpp
//

#include "RomM/RomMRemoteMediaLoader.h"

#include "HttpReq.h"
#include "Log.h"

namespace
{
    const std::string sEmptyString;
}

RomMRemoteMediaLoader& RomMRemoteMediaLoader::getInstance()
{
    static RomMRemoteMediaLoader instance;
    return instance;
}

void RomMRemoteMediaLoader::setCoverSource(int rommId,
                                           const std::string& url,
                                           const std::string& format)
{
    auto it {mCovers.find(rommId)};
    if (it == mCovers.cend()) {
        Entry entry;
        entry.sourceUrl = url;
        entry.format = format;
        entry.state = url.empty() ? State::NO_MEDIA : State::NOT_REQUESTED;
        mCovers.emplace(rommId, std::move(entry));
        return;
    }

    Entry& entry {it->second};
    if (entry.sourceUrl == url)
        return;
    if (entry.state == State::IN_PROGRESS)
        return;

    entry.sourceUrl = url;
    entry.format = format;
    entry.bytes.clear();
    entry.state = url.empty() ? State::NO_MEDIA : State::NOT_REQUESTED;
}

RomMRemoteMediaLoader::State RomMRemoteMediaLoader::requestCover(int rommId)
{
    auto it {mCovers.find(rommId)};
    if (it == mCovers.cend())
        return State::NO_MEDIA; // No source was ever recorded for this rom.

    Entry& entry {it->second};
    if (entry.state != State::NOT_REQUESTED)
        return entry.state;

    entry.state = State::IN_PROGRESS;
    entry.req = std::make_unique<HttpReq>(entry.sourceUrl, true);
    return entry.state;
}

RomMRemoteMediaLoader::State RomMRemoteMediaLoader::getCoverState(int rommId) const
{
    const auto it {mCovers.find(rommId)};
    return it == mCovers.cend() ? State::NOT_REQUESTED : it->second.state;
}

const std::string* RomMRemoteMediaLoader::getCoverBytes(int rommId) const
{
    const auto it {mCovers.find(rommId)};
    if (it == mCovers.cend() || it->second.state != State::READY)
        return nullptr;
    return &it->second.bytes;
}

const std::string& RomMRemoteMediaLoader::getCoverFormat(int rommId) const
{
    const auto it {mCovers.find(rommId)};
    return it == mCovers.cend() ? sEmptyString : it->second.format;
}

void RomMRemoteMediaLoader::update()
{
    for (auto& [rommId, entry] : mCovers) {
        if (entry.state != State::IN_PROGRESS || entry.req == nullptr)
            continue;

        const HttpReq::Status status {entry.req->status()};
        if (status == HttpReq::REQ_IN_PROGRESS)
            continue;

        if (status == HttpReq::REQ_SUCCESS) {
            entry.bytes = entry.req->getContent();
            entry.state = State::READY;
        }
        else {
            entry.state = State::FAILED;
            LOG(LogWarning) << "RomM remote media: fetch failed for rom " << rommId << " from \""
                            << entry.sourceUrl << "\" (HttpReq status " << status << ": "
                            << entry.req->getErrorMsg() << ")";
        }
        entry.req.reset();
    }
}

void RomMRemoteMediaLoader::forget(int rommId) { mCovers.erase(rommId); }
