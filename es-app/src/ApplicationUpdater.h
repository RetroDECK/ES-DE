//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ApplicationUpdater.h
//
//  Checks for application updates.
//  In the future updates will also be downloaded, and installed on some platforms.
//

#ifndef ES_APP_APPLICATION_UPDATER_H
#define ES_APP_APPLICATION_UPDATER_H

#include "AsyncHandle.h"
#include "HttpReq.h"

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

class ApplicationUpdater : public AsyncHandle
{
public:
    virtual ~ApplicationUpdater();
    static ApplicationUpdater& getInstance();

    void checkForUpdates();
    void updaterThread();
    bool downloadFile();
    void update();
    void parseFile();
    void compareVersions();
    bool getResults();

    struct Package {
        std::string name;
        std::string filename;
        std::string url;
        std::string md5;
        std::string message;
    };

    const std::string& getResultsString() { return mResults; }
    const Package& getPackageInfo() { return mPackage; }

private:
    ApplicationUpdater();

    struct Release {
        std::string releaseType;
        std::string version;
        std::string releaseNum;
        std::string date;
        std::vector<Package> packages;
    };

    enum class PackageType {
        WINDOWS_PORTABLE,
        WINDOWS_INSTALLER,
        MACOS_APPLE,
        MACOS_INTEL,
        LINUX_DEB,
        LINUX_RPM,
        LINUX_APPIMAGE,
        LINUX_STEAM_DECK_APPIMAGE,
        UNKNOWN
    };

    PackageType mPackageType;
    Package mPackage;
    std::string mUrl;
    std::string mResults;
    std::string mLogInfo;
    std::string mLogWarning;
    std::string mLogError;
    unsigned int mTimer;
    unsigned int mMaxTime;
    std::atomic<bool> mAbortDownload;
    std::atomic<bool> mApplicationShutdown;
    bool mCheckedForUpdate;
    bool mNewVersion;

    std::unique_ptr<std::thread> mThread;
    std::unique_ptr<HttpReq> mRequest;
    AsyncHandleStatus mStatus;

    Release mStableRelease;
    Release mPrerelease;
};

#endif // ES_APP_APPLICATION_UPDATER_H
