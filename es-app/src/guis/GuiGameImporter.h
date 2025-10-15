//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiGameImporter.h
//
//  Game import utility.
//

#ifndef ES_APP_GUIS_GUI_GAME_IMPORTER_H
#define ES_APP_GUIS_GUI_GAME_IMPORTER_H

#include "GuiComponent.h"
#include "components/BusyComponent.h"
#include "components/MenuComponent.h"
#include "components/OptionListComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiSettings.h"
#include "views/ViewController.h"

#include <atomic>
#include <thread>

template <typename T> class OptionListComponent;

class GuiGameImporter : public GuiComponent
{
public:
    GuiGameImporter(std::string title, std::function<void()> updateCallback);
    ~GuiGameImporter();

    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;

    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    void pressedStart();
    void mainWindow();
    void selectorWindow();

    void androidpackageRule(std::vector<std::pair<std::string, std::string>> appList);
#if defined(__APPLE__)
    void macosbundleRule(std::pair<const std::string, ImportRules::ImportRule> importRule);
#else
    void fileRule(std::pair<const std::string, ImportRules::ImportRule> importRule);
#endif
    void desktopshortcutRule(std::pair<const std::string, ImportRules::ImportRule> importRule);

    bool input(InputConfig* config, Input input) override;

    Renderer* mRenderer;
    BusyComponent mBusyAnim;
    std::function<void()> mUpdateCallback;

    MenuComponent mMenu;
    std::unique_ptr<MenuComponent> mSelectorMenu;

    std::shared_ptr<OptionListComponent<std::string>> mTargetSystem;
    std::shared_ptr<OptionListComponent<std::string>> mMediaTarget;
    std::shared_ptr<OptionListComponent<std::string>> mRemoveEntries;
    std::shared_ptr<SwitchComponent> mStripSpecialChars;
    std::shared_ptr<SwitchComponent> mImportMedia;
    std::shared_ptr<SwitchComponent> mImportMediaAdditional;
    std::shared_ptr<SwitchComponent> mImportMediaOverwrite;
    std::shared_ptr<SwitchComponent> mGamesOnly;
    std::vector<std::shared_ptr<ImageComponent>> mCheckboxes;

    std::unique_ptr<std::thread> mImportThread;

    std::string mTempDir;
    std::string mTargetSystemDir;
    std::string mFileExtension;
    std::string mMediaTargetDir;
    std::string mMediaFileExtension;

    bool mNoConfig;
    bool mSelectorWindow;
    bool mHasUpdates;
    bool mAndroidGetApps;

    std::atomic<bool> mIsInventorying;
    std::atomic<bool> mDoneInventorying;
    std::atomic<bool> mHasEntries;
};

#endif // ES_APP_GUIS_GUI_GAME_IMPORTER_H
