//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ApplicationUpdater.h
//
//  Checks for application updates.
//  In the future updates will also be downloaded and possibly installed.
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
    ApplicationUpdater();
    ~ApplicationUpdater();

    void checkForUpdates();
    void updaterThread();
    bool downloadFile();
    void update();
    void parseFile();
    void compareVersions();
    void getResults(std::string& results);

private:
    struct Package {
        std::string name;
        std::string filename;
        std::string url;
        std::string md5;
        std::string message;
    };

    struct Release {
        std::string releaseType;
        std::string version;
        std::string releaseNum;
        std::string date;
        std::vector<Package> packages;
    };

    std::string mUrl;
    std::string mResults;
    unsigned int mTimer;
    unsigned int mMaxTime;
    std::atomic<bool> mAbortDownload;
    bool mCheckedForUpdate;

    std::unique_ptr<std::thread> mThread;
    std::unique_ptr<HttpReq> mRequest;
    AsyncHandleStatus mStatus;

    Release mStableRelease;
    Release mPrerelease;
};

#endif // ES_APP_APPLICATION_UPDATER_H
