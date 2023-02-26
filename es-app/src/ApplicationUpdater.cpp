//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ApplicationUpdater.cpp
//
//  Checks for application updates.
//  In the future updates will also be downloaded, and installed on some platforms.
//

#include "ApplicationUpdater.h"

#include "EmulationStation.h"
#include "Log.h"
#include "Settings.h"
#include "resources/ResourceManager.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include <SDL2/SDL_timer.h>

#include <algorithm>
#include <deque>

#define LOCAL_TESTING_FILE false
#define MAX_DOWNLOAD_TIME 1

ApplicationUpdater::ApplicationUpdater()
    : mPackageType {PackageType::UNKNOWN}
    , mTimer {0}
    , mMaxTime {0}
    , mAbortDownload {false}
    , mApplicationShutdown {false}
    , mCheckedForUpdate {false}
    , mNewVersion {false}
{
    mUrl = "https://gitlab.com/api/v4/projects/18817634/repository/files/latest_release.json/"
           "raw?ref=master";

#if defined(_WIN64)
    if (Settings::getInstance()->getBool("PortableMode"))
        mPackageType = PackageType::WINDOWS_PORTABLE;
    else
        mPackageType = PackageType::WINDOWS_INSTALLER;
#elif defined(MACOS_APPLE_CPU)
    mPackageType = PackageType::MACOS_APPLE;
#elif defined(MACOS_INTEL_CPU)
    mPackageType = PackageType::MACOS_INTEL;
#elif defined(STEAM_DECK)
    mPackageType = PackageType::LINUX_STEAM_DECK_APPIMAGE;
#elif defined(APPIMAGE_BUILD)
    mPackageType = PackageType::LINUX_APPIMAGE;
#elif defined(LINUX_RPM_PACKAGE)
    mPackageType = PackageType::LINUX_RPM;
#elif defined(LINUX_DEB_PACKAGE)
    mPackageType = PackageType::LINUX_DEB;
#endif
}

ApplicationUpdater::~ApplicationUpdater()
{
    // This is needed if getResults() was never called.
    mApplicationShutdown = true;
    if (mThread)
        mThread->join();
}

ApplicationUpdater& ApplicationUpdater::getInstance()
{
    static ApplicationUpdater instance;
    return instance;
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
    else {
        LOG(LogInfo) << "Skipping application update check as not enough time has passed "
                        "since the last run (configured to check \""
                     << updateFrequency << "\")";
    }
}

void ApplicationUpdater::updaterThread()
{
    if (!downloadFile())
        compareVersions();

    mRequest.reset();
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
            mLogWarning = "ApplicationUpdater: Couldn't download \"latest_release.json\": " +
                          std::string {e.what()};
            return true;
        }
        if (mStatus == ASYNC_DONE || mApplicationShutdown)
            break;
        mTimer = SDL_GetTicks();
    };

    if (mStatus == ASYNC_DONE) {
        rapidjson::Document doc;
        const std::string& fileContents {mRequest->getContent()};
        doc.Parse(&fileContents[0], fileContents.length());
        if (!doc.HasParseError() && doc.HasMember("message") && doc["message"].IsString()) {
            mLogWarning = "ApplicationUpdater: Couldn't download \"latest_release.json\", received "
                          "server response \"" +
                          std::string {doc["message"].GetString()} + "\"";
            return true;
        }
        LOG(LogDebug)
            << "ApplicationUpdater::downloadFile(): Downloaded \"latest_release.json\" in "
            << mTimer - startTime << " milliseconds";
        try {
            parseFile();
        }
        catch (std::runtime_error& e) {
            mLogError = "ApplicationUpdater: Couldn't parse \"latest_release.json\": " +
                        std::string {e.what()};
            return true;
        }
    }
    else if (mApplicationShutdown) {
        return true;
    }
    else if (mTimer - startTime - 10 > MAX_DOWNLOAD_TIME * 1000) {
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
    rapidjson::Document doc;

#if (LOCAL_TESTING_FILE)
    LOG(LogWarning) << "ApplicationUpdater: Using local \"latest_release.json\" testing file";

    const std::string localReleaseFile {Utils::FileSystem::getHomePath() +
                                        "/.emulationstation/latest_release.json"};

    if (!Utils::FileSystem::exists(localReleaseFile))
        throw std::runtime_error("Local testing file not found");

    const ResourceData& localReleaseFileData {
        ResourceManager::getInstance().getFileData(localReleaseFile)};
    doc.Parse(reinterpret_cast<const char*>(localReleaseFileData.ptr.get()),
              localReleaseFileData.length);
#else
    const std::string fileContents {mRequest->getContent()};
    doc.Parse(&fileContents[0], fileContents.length());
#endif

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

    mNewVersion = false;

    for (auto& releaseType : releaseTypes) {
        // If the version does not follow the semantic versioning scheme then always consider it to
        // be a new release as perhaps the version scheme will be changed sometime in the future.
        if (count_if(releaseType->version.cbegin(), releaseType->version.cend(),
                     [](char c) { return c == '.'; }) != 2) {
            mNewVersion = true;
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
                mNewVersion = true;
        }

        if (mNewVersion) {
            for (auto& package : releaseType->packages) {
                if (mPackageType == PackageType::WINDOWS_PORTABLE &&
                    package.name == "WindowsPortable")
                    mPackage = package;
                else if (mPackageType == PackageType::WINDOWS_INSTALLER &&
                         package.name == "WindowsInstaller")
                    mPackage = package;
                else if (mPackageType == PackageType::MACOS_APPLE && package.name == "macOSApple")
                    mPackage = package;
                else if (mPackageType == PackageType::MACOS_INTEL && package.name == "macOSIntel")
                    mPackage = package;
                else if (mPackageType == PackageType::LINUX_DEB && package.name == "LinuxDEB")
                    mPackage = package;
                else if (mPackageType == PackageType::LINUX_RPM && package.name == "LinuxRPM")
                    mPackage = package;
                else if (mPackageType == PackageType::LINUX_APPIMAGE &&
                         package.name == "LinuxAppImage")
                    mPackage = package;
                else if (mPackageType == PackageType::LINUX_STEAM_DECK_APPIMAGE &&
                         package.name == "LinuxSteamDeckAppImage")
                    mPackage = package;
            }

            // Cut the message to 280 characters so we don't make the message box exceedingly large.
            mPackage.message = mPackage.message.substr(0, 280);

            mLogInfo = "A new ";
            mLogInfo.append(releaseType == &mStableRelease ? "stable release" : "prerelease")
                .append(" is available for download at https://es-de.org: ")
                .append(releaseType->version)
                .append(" (r")
                .append(releaseType->releaseNum)
                .append("), release date: ")
                .append(releaseType->date);

            mResults.append("New ")
                .append(releaseType == &mStableRelease ? "release " : "prerelease ")
                .append("available!\n")
                .append(releaseType->version)
                .append(" (")
                .append(releaseType->date)
                .append(")\n")
                .append("can now be downloaded from\n")
                .append("https://es-de.org/");

            if (mPackage.message != "")
                mResults.append("\n").append(mPackage.message);

            mResults = Utils::String::toUpper(mResults);
            break;
        }
    }
    if (!mNewVersion) {
        mLogInfo = "No application updates available";
    }
    mCheckedForUpdate = true;
}

bool ApplicationUpdater::getResults()
{
    mAbortDownload = true;

    if (mThread) {
        mThread->join();
        mThread.reset();
        if (mCheckedForUpdate) {
            Settings::getInstance()->setString(
                "ApplicationUpdaterLastCheck",
                Utils::Time::DateTime(Utils::Time::now()).getIsoString());
            Settings::getInstance()->saveFile();
        }
    }

    // We output these messages here instead of immediately when they occur so that they will
    // always be printed at the end of the application startup.
    if (mLogError != "") {
        LOG(LogError) << mLogError;
    }
    if (mLogWarning != "") {
        LOG(LogWarning) << mLogWarning;
    }
    if (mLogInfo != "") {
        LOG(LogInfo) << mLogInfo;
    }

    if (mNewVersion && mPackage.name == "") {
        LOG(LogWarning)
            << "ApplicationUpdater: Couldn't find a package type matching current platform";
    }

    return mNewVersion;
}
