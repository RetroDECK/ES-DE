//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  ApplicationUpdater.cpp
//
//  Checks for application updates.
//  Used in conjunction with GuiApplicationUpdater.
//

#include "ApplicationUpdater.h"

#include "ApplicationVersion.h"
#include "Log.h"
#include "Settings.h"
#include "resources/ResourceManager.h"
#include "utils/LocalizationUtil.h"
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
#endif
}

ApplicationUpdater::~ApplicationUpdater()
{
    // This is needed if getResults() was never called.
    mApplicationShutdown = true;

    if (mThread)
        mThread->join();

    HttpReq::cleanupCurlMulti();
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
#if defined(_WIN64)
    // Workaround for a bug in the libintl library.
    Utils::Localization::setThreadLocale();
#endif

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
    mRequest = std::unique_ptr<HttpReq>(std::make_unique<HttpReq>(mUrl, false));

    while (mTimer < mMaxTime || !mAbortDownload) {
        // Add a small delay so we don't eat all CPU cycles checking for status updates.
        SDL_Delay(5);
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
    std::string errorMessage {_("Network error (status:")};
    errorMessage.append(" ")
        .append(std::to_string(reqStatus))
        .append(") - ")
        .append(mRequest->getErrorMsg());
    throw std::runtime_error(errorMessage);
}

void ApplicationUpdater::parseFile()
{
    assert(mRequest->status() == HttpReq::REQ_SUCCESS);
    rapidjson::Document doc;

#if (LOCAL_TESTING_FILE)
    LOG(LogWarning) << "ApplicationUpdater: Using local \"latest_release.json\" testing file";

    const std::string localReleaseFile {Utils::FileSystem::getAppDataDirectory() +
                                        "/latest_release.json"};

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

#if defined(__ANDROID__)
    const std::vector<std::string> releaseTypes {"stable"};
#else
    const std::vector<std::string> releaseTypes {"stable", "prerelease"};
#endif

    for (auto& releaseType : releaseTypes) {
        Release release;
        if (doc.HasMember(releaseType.c_str())) {
            release.releaseType = releaseType.c_str();
            const rapidjson::Value& releaseTypeEntry {doc[releaseType.c_str()]};

#if defined(__ANDROID__)
            if (releaseTypeEntry.HasMember("androidVersionName") &&
                releaseTypeEntry["androidVersionName"].IsString())
                release.version = releaseTypeEntry["androidVersionName"].GetString();
            else
                throw std::runtime_error(
                    "Invalid file structure, \"androidVersionName\" key missing");
#else
            if (releaseTypeEntry.HasMember("version") && releaseTypeEntry["version"].IsString())
                release.version = releaseTypeEntry["version"].GetString();
            else
                throw std::runtime_error("Invalid file structure, \"version\" key missing");
#endif
            // There may not be a prerelease available.
            if (releaseType == "prerelease" && release.version == "")
                continue;

            if (releaseTypeEntry.HasMember("release") && releaseTypeEntry["release"].IsString())
                release.releaseNum = releaseTypeEntry["release"].GetString();
            else
                throw std::runtime_error("Invalid file structure, \"release\" key missing");

#if defined(__ANDROID__)
            if (releaseTypeEntry.HasMember("androidVersionCode") &&
                releaseTypeEntry["androidVersionCode"].IsString())
                release.androidVersionCode = releaseTypeEntry["androidVersionCode"].GetString();
            else
                throw std::runtime_error(
                    "Invalid file structure, \"androidVersionCode\" key missing");

            if (releaseTypeEntry.HasMember("androidDate") &&
                releaseTypeEntry["androidDate"].IsString())
                release.date = releaseTypeEntry["androidDate"].GetString();
            else
                throw std::runtime_error("Invalid file structure, \"androidDate\" key missing");
#else
            if (releaseTypeEntry.HasMember("date") && releaseTypeEntry["date"].IsString())
                release.date = releaseTypeEntry["date"].GetString();
            else
                throw std::runtime_error("Invalid file structure, \"date\" key missing");
#endif
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

                    if (package.name == "")
                        throw std::runtime_error(
                            "Invalid file contents, package \"name\" key is blank");

                    if (packageEntry.HasMember("filename") && packageEntry["filename"].IsString())
                        package.filename = packageEntry["filename"].GetString();
                    else
                        throw std::runtime_error(
                            "Invalid file structure, package \"filename\" key missing");

                    if (package.filename == "")
                        throw std::runtime_error(
                            "Invalid file contents, package \"filename\" key is blank");

                    if (packageEntry.HasMember("url") && packageEntry["url"].IsString())
                        package.url = packageEntry["url"].GetString();
                    else
                        throw std::runtime_error(
                            "Invalid file structure, package \"url\" key missing");

                    if (package.url == "")
                        throw std::runtime_error(
                            "Invalid file contents, package \"url\" key is blank");

                    if (packageEntry.HasMember("md5") && packageEntry["md5"].IsString())
                        package.md5 = packageEntry["md5"].GetString();
                    else
                        throw std::runtime_error(
                            "Invalid file structure, package \"md5\" key missing");

                    if (package.md5 == "")
                        throw std::runtime_error(
                            "Invalid file contents, package \"md5\" key is blank");

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

#if !defined(__ANDROID__)
    if (mPrerelease.releaseNum != "") {
#if defined(IS_PRERELEASE)
        releaseTypes.emplace_front(&mPrerelease);
#else
        if (Settings::getInstance()->getBool("ApplicationUpdaterPrereleases"))
            releaseTypes.emplace_front(&mPrerelease);
#endif
    }
#endif

    mNewVersion = false;

    for (auto& releaseType : releaseTypes) {
        // That these keys are blank is not technically wrong as the prerelease is optional,
        // therefore we just check them here and not in the parseFile() function.
        if (releaseType->version == "" || releaseType->releaseNum == "" || releaseType->date == "")
            continue;

#if defined(__ANDROID__)
        // This should hopefully never happen.
        if (releaseType->androidVersionCode == "")
            continue;

        mNewVersion = (std::stoi(releaseType->androidVersionCode) > ANDROID_VERSION_CODE);
#else
        mNewVersion = (std::stoi(releaseType->releaseNum) > PROGRAM_RELEASE_NUMBER);
#endif

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
                else if (mPackageType == PackageType::LINUX_APPIMAGE &&
                         package.name == "LinuxAppImage")
                    mPackage = package;
                else if (mPackageType == PackageType::LINUX_STEAM_DECK_APPIMAGE &&
                         package.name == "LinuxSteamDeckAppImage")
                    mPackage = package;
            }

            mPackage.version = releaseType->version;

            // Cut the message to 280 characters so we don't make the message box exceedingly large.
            mPackage.message = mPackage.message.substr(0, 280);

            mLogInfo = "A new ";
            mLogInfo
#if defined(__ANDROID__)
                .append("release is available: ")
#else
                .append(releaseType == &mStableRelease ? "stable release" : "prerelease")
                .append(" is available for download at https://es-de.org: ")
#endif
                .append(releaseType->version)
                .append(" (r")
                .append(releaseType->releaseNum)
                .append("), release date: ")
                .append(releaseType->date);

            if (releaseType == &mPrerelease) {
                mResults.append(_("New prerelease available:"))
                    .append("\n")
                    .append(releaseType->version)
                    .append(" (")
                    .append(releaseType->date)
                    .append(")");
            }
            else {
                mResults.append(_("New release available:"))
                    .append(" ")
                    .append(releaseType->version);
            }

            if (mPackageType == PackageType::UNKNOWN)
                mResults.append("\n")
                    .append(_("For more information visit"))
                    .append("\n")
                    .append("https://es-de.org");

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

#if !defined(__ANDROID__)
    if (mNewVersion && mPackage.name == "") {
        LOG(LogDebug) << "ApplicationUpdater::getResults(): Couldn't find a package type matching "
                         "current build";
    }
#endif

    return mNewVersion;
}
