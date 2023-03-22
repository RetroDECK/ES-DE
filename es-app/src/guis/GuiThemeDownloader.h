//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiThemeDownloader.h
//
//  Theme downloader.
//

#ifndef ES_APP_GUIS_GUI_THEME_DOWNLOADER_H
#define ES_APP_GUIS_GUI_THEME_DOWNLOADER_H

#include "GuiComponent.h"
#include "components/BusyComponent.h"
#include "components/ButtonComponent.h"
#include "components/ComponentGrid.h"
#include "components/ComponentList.h"
#include "components/NinePatchComponent.h"
#include "components/TextComponent.h"
#include "renderers/Renderer.h"
#include "views/ViewController.h"

#include <git2/clone.h>
#include <git2/errors.h>
#include <git2/global.h>
#include <git2/merge.h>
#include <git2/reset.h>
#include <git2/revparse.h>
#include <git2/status.h>
#include <git2/version.h>

#include <atomic>
#include <future>
#include <thread>

class GuiThemeDownloader : public GuiComponent
{
public:
    GuiThemeDownloader();
    ~GuiThemeDownloader();

    bool fetchRepository(std::pair<std::string, std::string> repoInfo, bool allowReset = false);
    bool renameDirectory(const std::string& path);
    void parseThemesList();

    void populateGUI();

    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;

    void onSizeChanged() override;
    bool input(InputConfig* config, Input input) override;

    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override { return ViewController::getInstance()->getViewHelpStyle(); }

private:
    Renderer* mRenderer;
    NinePatchComponent mBackground;
    ComponentGrid mGrid;
    std::shared_ptr<ComponentList> mList;
    std::shared_ptr<ComponentGrid> mButtons;
    BusyComponent mBusyAnim;

    std::string mErrorMessage;
    std::thread mFetchThread;
    std::promise<bool> mPromise;
    std::future<bool> mFuture;
    std::atomic<bool> mFetching;
    std::atomic<bool> mLatestThemesList;
    bool mHasLocalChanges;
    static inline std::atomic<float> mReceivedObjectsProgress {0.0f};
    static inline std::atomic<float> mResolveDeltaProgress {0.0f};

    struct Screenshot {
        std::string image;
        std::string caption;
    };

    struct ThemeEntry {
        std::string name;
        std::string reponame;
        std::string url;
        std::vector<std::string> variants;
        std::vector<std::string> colorSchemes;
        std::vector<std::string> aspectRatios;
        std::vector<std::string> transitions;
        std::vector<Screenshot> screenshots;
    };

    std::shared_ptr<TextComponent> mTitle;
    std::vector<ThemeEntry> mThemeSets;
};

#endif // ES_APP_GUIS_GUI_THEME_DOWNLOADER_H
