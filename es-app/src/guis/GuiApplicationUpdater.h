//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiApplicationUpdater.h
//
//  Installs application updates.
//  Used in conjunction with ApplicationUpdater.
//

#ifndef ES_APP_GUIS_GUI_APPLICATION_UPDATER_H
#define ES_APP_GUIS_GUI_APPLICATION_UPDATER_H

#include "ApplicationUpdater.h"
#include "AsyncHandle.h"
#include "GuiComponent.h"
#include "HttpReq.h"
#include "components/BusyComponent.h"
#include "guis/GuiSettings.h"
#include "views/ViewController.h"

#include <atomic>
#include <mutex>
#include <thread>

class GuiApplicationUpdater : public GuiComponent
{
public:
    GuiApplicationUpdater();
    ~GuiApplicationUpdater();

    void setDownloadPath();
    bool downloadPackage();
    bool installAppImage();

    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;

private:
    void onSizeChanged() override;
    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override
    {
        if (ViewController::getInstance()->getState().viewing == ViewController::ViewMode::NOTHING)
            return HelpStyle();
        else
            return ViewController::getInstance()->getViewHelpStyle();
    }

    Renderer* mRenderer;
    BusyComponent mBusyAnim;

    NinePatchComponent mBackground;
    ComponentGrid mGrid;
    std::shared_ptr<ComponentGrid> mButtons;
    std::shared_ptr<ButtonComponent> mButton1;
    std::shared_ptr<ButtonComponent> mButton2;
    std::shared_ptr<ButtonComponent> mButton3;

    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<TextComponent> mStatusHeader;
    std::shared_ptr<TextComponent> mProcessStep1;
    std::shared_ptr<TextComponent> mProcessStep2;
    std::shared_ptr<TextComponent> mProcessStep3;
    std::shared_ptr<TextComponent> mStatusMessageHeader;
    std::shared_ptr<TextComponent> mStatusMessage;
    std::shared_ptr<TextComponent> mChangelogMessage;

    std::unique_ptr<std::thread> mThread;
    std::unique_ptr<HttpReq> mRequest;
    AsyncHandleStatus mStatus;
    std::mutex mMutex;
    std::string mMessage;

    ApplicationUpdater::Package mPackage;
    std::string mDownloadPackageFilename;
    std::atomic<int> mDownloadPercentage;
    std::atomic<bool> mLinuxAppImage;
    std::atomic<bool> mAbortDownload;
    std::atomic<bool> mDownloading;
    std::atomic<bool> mReadyToInstall;
    bool mHasDownloaded;
    bool mInstalling;
    bool mHasInstalled;
};

#endif // ES_APP_GUIS_GUI_APPLICATION_UPDATER_H
