//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ApplicationUpdater.cpp
//
//  Checks for application updates.
//  In the future updates will also be downloaded and possibly installed.
//

#include "ApplicationUpdater.h"

#include "EmulationStation.h"
#include "Log.h"
#include "Settings.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include <SDL2/SDL_timer.h>

#include <algorithm>
#include <deque>

#define MAX_DOWNLOAD_TIME 1

ApplicationUpdater::ApplicationUpdater()
    : mTimer {0}
    , mMaxTime {0}
    , mAbortDownload {false}
    , mCheckedForUpdate {false}
{
    mUrl = "https://gitlab.com/api/v4/projects/18817634/repository/files/latest_release.json/"
           "raw?ref=master";
}

ApplicationUpdater::~ApplicationUpdater()
{
    // This is needed if getResults() was never called.
    if (mThread)
        mThread->join();
}

void ApplicationUpdater::checkForUpdates()
{
    const std::string updateFrequency {
        Settings::getInstance()->getString("ApplicationUpdaterFrequency")};
    if (updateFrequency == "never")
        return;

    const std::string lastCheck {Settings::getInstance()->getString("ApplicationUpdaterLastCheck")};
    unsigned int frequencyDays {0};
    bool checkForUpdate {false};

    if (updateFrequency == "daily")
        frequencyDays = 1;
    else if (updateFrequency == "weekly")
        frequencyDays = 7;
    else if (updateFrequency == "monthly")
        frequencyDays = 30;

    // Frequency set to "always" or it's the first time we check for updates.
    if (frequencyDays == 0 || lastCheck == "") {
        checkForUpdate = true;
    }
    else {
        const Utils::Time::DateTime now {Utils::Time::now()};
        const Utils::Time::DateTime lastTime {lastCheck};
        const Utils::Time::Duration dur {now.getTime() - lastTime.getTime()};
        if (dur.getDays() >= frequencyDays)
            checkForUpdate = true;
    }

    if (checkForUpdate) {
        LOG(LogInfo) << "Checking for application updates...";
        mThread = std::make_unique<std::thread>(&ApplicationUpdater::updaterThread, this);
    }
}

void ApplicationUpdater::updaterThread()
{
    if (!downloadFile()) {
        compareVersions();
    }
}

bool ApplicationUpdater::downloadFile()
{
    const unsigned int startTime {SDL_GetTicks()};
    mTimer = startTime;
    mMaxTime = mTimer + (MAX_DOWNLOAD_TIME * 1000);

    mStatus = ASYNC_IN_PROGRESS;
    mRequest = std::unique_ptr<HttpReq>(std::make_unique<HttpReq>(mUrl));

    while (mTimer < mMaxTime || !mAbortDownload) {
        SDL_Delay(10);
        try {
            update();
        }
        catch (std::runtime_error& e) {
            LOG(LogWarning) << "ApplicationUpdater: Couldn't download \"latest_release.json\": "
                            << e.what();
            return true;
        }
        if (mStatus == ASYNC_DONE)
            break;
        mTimer = SDL_GetTicks();
    };

    if (mStatus == ASYNC_DONE) {
        rapidjson::Document doc;
        const std::string& fileContents {mRequest->getContent()};
        doc.Parse(&fileContents[0], fileContents.length());
        if (doc.HasMember("error") && doc["error"].IsString()) {
            LOG(LogWarning)
                << "ApplicationUpdater: Couldn't download \"latest_release.json\", received "
                   "server error response \""
                << doc["error"].GetString() << "\"";
            return true;
        }
        LOG(LogDebug)
            << "ApplicationUpdater::downloadFile(): Downloaded \"latest_release.json\" in "
            << mTimer - startTime << " milliseconds";
        try {
            parseFile();
        }
        catch (std::runtime_error& e) {
            LOG(LogError) << "ApplicationUpdater: Couldn't parse \"latest_release.json\": "
                          << e.what();
            return true;
        }
    }
    else if (mAbortDownload) {
        LOG(LogWarning) << "ApplicationUpdater: Aborted download of \"latest_release.json\" after "
                        << mTimer - startTime << " milliseconds as the application has started up";
        return true;
    }
    else {
        LOG(LogWarning) << "ApplicationUpdater: Couldn't download \"latest_release.json\" within "
                        << MAX_DOWNLOAD_TIME << " second time limit";
        return true;
    }

    return false;
}

void ApplicationUpdater::update()
{
    HttpReq::Status reqStatus {mRequest->status()};
    if (reqStatus == HttpReq::REQ_SUCCESS) {
        mStatus = ASYNC_DONE;
        return;
    }

    // Not ready yet.
    if (reqStatus == HttpReq::REQ_IN_PROGRESS)
        return;

    // Everything else is some sort of error.
    std::string errorMessage {"Network error (status: "};
    errorMessage.append(std::to_string(reqStatus)).append(") - ").append(mRequest->getErrorMsg());
    throw std::runtime_error(errorMessage);
}

void ApplicationUpdater::parseFile()
{
    assert(mRequest->status() == HttpReq::REQ_SUCCESS);

    const std::string fileContents {mRequest->getContent()};
    rapidjson::Document doc;
    doc.Parse(&fileContents[0], fileContents.length());

    if (doc.HasParseError())
        throw std::runtime_error(rapidjson::GetParseError_En(doc.GetParseError()));

    const std::vector<std::string> releaseTypes {"stable", "prerelease"};

    for (auto& releaseType : releaseTypes) {
        Release release;
        if (doc.HasMember(releaseType.c_str())) {
            release.releaseType = releaseType.c_str();
            const rapidjson::Value& releaseTypeEntry {doc[releaseType.c_str()]};

            if (releaseTypeEntry.HasMember("version") && releaseTypeEntry["version"].IsString())
                release.version = releaseTypeEntry["version"].GetString();
            else
                throw std::runtime_error("Invalid file structure, \"version\" key missing");

            // There may not be a prerelease available.
            if (releaseType == "prerelease" && release.version == "")
                continue;

            if (releaseTypeEntry.HasMember("release") && releaseTypeEntry["release"].IsString())
                release.releaseNum = releaseTypeEntry["release"].GetString();
            else
                throw std::runtime_error("Invalid file structure, \"release\" key missing");

            if (releaseTypeEntry.HasMember("date") && releaseTypeEntry["date"].IsString())
                release.date = releaseTypeEntry["date"].GetString();
            else
                throw std::runtime_error("Invalid file structure, \"date\" key missing");

            if (releaseTypeEntry.HasMember("packages") && releaseTypeEntry["packages"].IsArray()) {
                const rapidjson::Value& packages {releaseTypeEntry["packages"]};
                for (int i {0}; i < static_cast<int>(packages.Size()); ++i) {
                    Package package;
                    const rapidjson::Value& packageEntry {packages[i]};

                    if (packageEntry.HasMember("name") && packageEntry["name"].IsString())
                        package.name = packageEntry["name"].GetString();
                    else
                        throw std::runtime_error(
                            "Invalid file structure, package \"name\" key missing");

                    if (packageEntry.HasMember("filename") && packageEntry["filename"].IsString())
                        package.filename = packageEntry["filename"].GetString();
                    else
                        throw std::runtime_error(
                            "Invalid file structure, package \"filename\" key missing");

                    if (packageEntry.HasMember("url") && packageEntry["url"].IsString())
                        package.url = packageEntry["url"].GetString();
                    else
                        throw std::runtime_error(
                            "Invalid file structure, package \"url\" key missing");

                    if (packageEntry.HasMember("md5") && packageEntry["md5"].IsString())
                        package.md5 = packageEntry["md5"].GetString();
                    else
                        throw std::runtime_error(
                            "Invalid file structure, package \"md5\" key missing");

                    if (packageEntry.HasMember("message") && packageEntry["message"].IsString())
                        package.message = packageEntry["message"].GetString();
                    else
                        throw std::runtime_error(
                            "Invalid file structure, package \"message\" key missing");

                    release.packages.emplace_back(package);
                }
            }
            else {
                throw std::runtime_error("Invalid file structure");
            }
            if (releaseType == "stable")
                mStableRelease = std::move(release);
            else
                mPrerelease = std::move(release);
        }
        else {
            throw std::runtime_error("Invalid file structure, release type \"" + releaseType +
                                     "\" missing");
        }
    }
    if (mPrerelease.version == "") {
        LOG(LogDebug) << "ApplicationUpdater::parseFile(): Latest stable release is "
                      << mStableRelease.version << " (r" << mStableRelease.releaseNum
                      << "), no prerelease currently available";
    }
    else {
        LOG(LogDebug) << "ApplicationUpdater::parseFile(): Latest stable release is "
                      << mStableRelease.version << " (r" << mStableRelease.releaseNum
                      << ") and latest prerelease is " << mPrerelease.version << " (r"
                      << mPrerelease.releaseNum << ")";
    }
}

void ApplicationUpdater::compareVersions()
{
    std::deque<Release*> releaseTypes {&mStableRelease};

    if (mPrerelease.version != "") {
#if defined(IS_PRERELEASE)
        releaseTypes.emplace_front(&mPrerelease);
#else
        if (Settings::getInstance()->getBool("ApplicationUpdaterPrereleases"))
            releaseTypes.emplace_front(&mPrerelease);
#endif
    }

    for (auto& releaseType : releaseTypes) {
        bool newVersion {false};
        // If the version does not follow the semantic versioning scheme then always consider it to
        // be a new release as perhaps the version scheme will be changed sometime in the future.
        if (count_if(releaseType->version.cbegin(), releaseType->version.cend(),
                     [](char c) { return c == '.'; }) != 2) {
            newVersion = true;
        }
        else {
            std::vector<std::string> fileVersion {
                Utils::String::delimitedStringToVector(releaseType->version, ".")};

            const size_t dashPos {fileVersion.back().find('-')};
            if (dashPos != std::string::npos)
                fileVersion.back() = fileVersion.back().substr(0, dashPos);

            int versionWeight {0};

            if (std::stoi(fileVersion.at(0)) > PROGRAM_VERSION_MAJOR)
                versionWeight += 8;
            else if (std::stoi(fileVersion.at(0)) < PROGRAM_VERSION_MAJOR)
                versionWeight -= 8;

            if (std::stoi(fileVersion.at(1)) > PROGRAM_VERSION_MINOR)
                versionWeight += 4;
            else if (std::stoi(fileVersion.at(1)) < PROGRAM_VERSION_MINOR)
                versionWeight -= 4;

            if (std::stoi(fileVersion.at(2)) > PROGRAM_VERSION_MAINTENANCE)
                versionWeight += 2;
            else if (std::stoi(fileVersion.at(2)) < PROGRAM_VERSION_MAINTENANCE)
                versionWeight -= 2;

            // If versions match precisely then fall back to using the release number.
            if (versionWeight == 0 && std::stoi(releaseType->releaseNum) > PROGRAM_RELEASE_NUMBER)
                ++versionWeight;

            if (versionWeight > 0)
                newVersion = true;
        }
        if (newVersion) {
            std::string message;

            for (auto& package : releaseType->packages) {
#if defined(_WIN64)
                if (Settings::getInstance()->getBool("PortableMode")) {
                    if (package.name == "WindowsPortable")
                        message = package.message;
                }
                else {
                    if (package.name == "WindowsInstaller")
                        message = package.message;
                }
#elif defined(MACOS_APPLE_CPU)
                if (package.name == "macOSApple")
                    message = package.message;
#elif defined(MACOS_INTEL_CPU)
                if (package.name == "macOSIntel")
                    message = package.message;
#elif defined(STEAM_DECK)
                if (package.name == "LinuxSteamDeckAppImage")
                    message = package.message;
#elif defined(APPIMAGE_BUILD)
                if (package.name == "LinuxAppImage")
                    message = package.message;
#elif defined(LINUX_DEB_PACKAGE)
                if (package.name == "LinuxDEB")
                    message = package.message;
#elif defined(LINUX_RPM_PACKAGE)
                if (package.name == "LinuxRPM")
                    message = package.message;
#endif
                auto tempVar = package;
            }

            // Cut the message to 280 characters so we don't make the message box exceedingly large.
            message = message.substr(0, 280);

            LOG(LogInfo) << "ApplicationUpdater: A new "
                         << (releaseType == &mStableRelease ? "stable release" : "prerelease")
                         << " is available for download at https://es-de.org: "
                         << releaseType->version << " (r" << releaseType->releaseNum
                         << "), release date: " << releaseType->date;

            mResults.append("New ")
                .append(releaseType == &mStableRelease ? "release " : "prerelease ")
                .append("available!\n")
                .append(releaseType->version)
                .append(" (")
                .append(releaseType->date)
                .append(")\n")
                .append("can now be downloaded from\n")
                .append("https://es-de.org/");

            if (message != "")
                mResults.append("\n").append(message);

            mResults = Utils::String::toUpper(mResults);
            break;
        }
    }
    mCheckedForUpdate = true;
}

void ApplicationUpdater::getResults(std::string& results)
{
    mAbortDownload = true;

    if (mThread) {
        mThread->join();
        mThread.reset();
        if (mCheckedForUpdate) {
            if (mResults != "")
                results = mResults;
            Settings::getInstance()->setString(
                "ApplicationUpdaterLastCheck",
                Utils::Time::DateTime(Utils::Time::now()).getIsoString());
            Settings::getInstance()->saveFile();
        }
    }
}
