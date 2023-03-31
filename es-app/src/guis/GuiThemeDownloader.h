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
#include "components/ImageComponent.h"
#include "components/NinePatchComponent.h"
#include "components/ScrollIndicatorComponent.h"
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

    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;

    void onSizeChanged() override;
    bool input(InputConfig* config, Input input) override;

    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override { return ViewController::getInstance()->getViewHelpStyle(); }

private:
    struct Screenshot {
        std::string image;
        std::string caption;
    };

    struct ThemeEntry {
        std::string name;
        std::string reponame;
        std::string url;
        std::string manualExtension;
        std::string author;
        std::vector<std::string> variants;
        std::vector<std::string> colorSchemes;
        std::vector<std::string> aspectRatios;
        std::vector<std::string> transitions;
        std::vector<Screenshot> screenshots;
        bool newEntry;
        bool invalidRepository;
        bool manuallyDownloaded;
        bool hasLocalChanges;
        bool isCloned;
        ThemeEntry()
            : newEntry {false}
            , invalidRepository {false}
            , manuallyDownloaded {false}
            , hasLocalChanges {false}
            , isCloned {false}
        {
        }
    };

    bool fetchThemesList();
    bool fetchRepository(const std::string& repositoryName,
                         const std::string& url,
                         bool allowReset = false);
    bool cloneRepository(const std::string& repositoryName, const std::string& url);

    bool checkLocalChanges(git_repository* repository, bool hasFetched = false);
    void resetRepository(git_repository* repository);
    void makeInventory();
    bool renameDirectory(const std::string& path);
    void parseThemesList();

    void populateGUI();
    void updateGUI();
    void updateInfoPane();
    void setupFullscreenViewer();

    Renderer* mRenderer;
    NinePatchComponent mBackground;
    ComponentGrid mGrid;
    std::shared_ptr<ComponentGrid> mCenterGrid;
    std::shared_ptr<ComponentList> mList;
    std::shared_ptr<ComponentGrid> mButtons;
    BusyComponent mBusyAnim;

    struct ThemeGUIEntry {
        std::shared_ptr<TextComponent> themeName;
    };

    std::vector<ThemeGUIEntry> mThemeGUIEntries;

    enum class StatusType {
        STATUS_NO_CHANGE,
        STATUS_DOWNLOADING,
        STATUS_UPDATING
    };

    enum class RepositoryError {
        NO_REPO_ERROR,
        MANUALLY_DOWNLOADED,
        NOT_A_REPOSITORY,
        INVALID_ORIGIN,
        HAS_DIVERGED,
        HAS_LOCAL_CHANGES
    };

    RepositoryError mRepositoryError;
    std::string mThemeDirectory;
    std::string mMessage;
    std::thread mFetchThread;
    std::promise<bool> mPromise;
    std::future<bool> mFuture;
    std::atomic<bool> mFetching;
    std::atomic<bool> mLatestThemesList;
    static inline std::atomic<float> mReceivedObjectsProgress {0.0f};
    static inline std::atomic<float> mResolveDeltaProgress {0.0f};
    std::vector<ThemeEntry> mThemeSets;
    StatusType mStatusType;
    std::string mStatusText;
    bool mFullscreenViewing;
    size_t mFullscreenViewerIndex;

    std::shared_ptr<ImageComponent> mScrollUp;
    std::shared_ptr<ImageComponent> mScrollDown;
    std::shared_ptr<ScrollIndicatorComponent> mScrollIndicator;
    std::vector<float> mGrayRectangleCoords;

    std::shared_ptr<ImageComponent> mScreenshot;
    std::vector<std::shared_ptr<ImageComponent>> mViewerScreenshots;
    std::vector<std::shared_ptr<TextComponent>> mViewerCaptions;
    std::shared_ptr<TextComponent> mViewerIndicatorLeft;
    std::shared_ptr<TextComponent> mViewerIndicatorRight;
    std::shared_ptr<TextComponent> mDownloadStatus;
    std::shared_ptr<TextComponent> mLocalChanges;
    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<TextComponent> mVariantsLabel;
    std::shared_ptr<TextComponent> mColorSchemesLabel;
    std::shared_ptr<TextComponent> mAspectRatiosLabel;
    std::shared_ptr<TextComponent> mFutureUseLabel;
    std::shared_ptr<TextComponent> mAuthor;
    std::shared_ptr<TextComponent> mVariantCount;
    std::shared_ptr<TextComponent> mColorSchemesCount;
    std::shared_ptr<TextComponent> mAspectRatiosCount;
    std::shared_ptr<TextComponent> mFutureUseCount;
};

#endif // ES_APP_GUIS_GUI_THEME_DOWNLOADER_H
