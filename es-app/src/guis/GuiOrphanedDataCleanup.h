//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiOrphanedDataCleanup.h
//
//  Removes orphaned game media, gamelist.xml entries and custom collections entries.
//

#ifndef ES_APP_GUIS_GUI_ORPHANED_DATA_CLEANUP_H
#define ES_APP_GUIS_GUI_ORPHANED_DATA_CLEANUP_H

#include "GuiComponent.h"
#include "components/BusyComponent.h"
#include "guis/GuiSettings.h"
#include "views/ViewController.h"

#include <atomic>
#include <mutex>
#include <thread>

class GuiOrphanedDataCleanup : public GuiComponent
{
public:
    GuiOrphanedDataCleanup(std::function<void()> reloadCallback);
    ~GuiOrphanedDataCleanup();

    void cleanupMediaFiles();
    void cleanupGamelists();
    void cleanupCollections();

    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;

private:
    void onSizeChanged() override;
    bool input(InputConfig* config, Input input) override;

    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override { return ViewController::getInstance()->getViewHelpStyle(); }

    Renderer* mRenderer;
    NinePatchComponent mBackground;
    ComponentGrid mGrid;
    BusyComponent mBusyAnim;
    std::function<void()> mReloadCallback;

    std::shared_ptr<ComponentGrid> mButtons;
    std::shared_ptr<ButtonComponent> mButton1;
    std::shared_ptr<ButtonComponent> mButton2;
    std::shared_ptr<ButtonComponent> mButton3;
    std::shared_ptr<ButtonComponent> mButton4;

    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<TextComponent> mStatus;
    std::shared_ptr<TextComponent> mDescriptionHeader;
    std::shared_ptr<TextComponent> mDescription;
    std::shared_ptr<TextComponent> mSystemProcessingHeader;
    std::shared_ptr<TextComponent> mEntryCountHeader;
    std::shared_ptr<TextComponent> mSystemProcessing;
    std::shared_ptr<TextComponent> mEntryCount;
    std::shared_ptr<TextComponent> mErrorHeader;
    std::shared_ptr<TextComponent> mError;

    std::unique_ptr<std::thread> mThread;
    std::mutex mMutex;
    int mCursorPos;

    std::string mMediaDescription;
    std::string mGamelistDescription;
    std::string mCollectionsDescription;
    std::string mCurrentSystem;
    std::string mErrorMessage;

    std::string mMediaDirectory;
    std::vector<std::string> mMediaTypes;

    std::atomic<bool> mIsProcessing;
    std::atomic<bool> mStopProcessing;
    std::atomic<bool> mCompleted;
    std::atomic<bool> mFailed;
    std::atomic<bool> mNeedsReloading;
    std::atomic<int> mProcessedCount;
    bool mHasCustomCollections;
    bool mCaseSensitiveFilesystem;

    enum class CleanupType {
        MEDIA,
        GAMELISTS,
        COLLECTIONS
    };

    CleanupType mCleanupType;
};

#endif // ES_APP_GUIS_GUI_ORPHANED_DATA_CLEANUP_H
