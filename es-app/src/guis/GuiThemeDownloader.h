//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
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
    GuiThemeDownloader(std::function<void()> updateCallback);
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
        std::vector<std::string> fontSizes;
        std::vector<std::string> transitions;
        std::vector<Screenshot> screenshots;
        bool newEntry;
        bool deprecated;
        bool invalidRepository;
        bool shallowRepository;
        bool corruptRepository;
        bool wrongUrl;
        bool manuallyDownloaded;
        bool hasLocalChanges;
        bool isCloned;
        ThemeEntry()
            : newEntry {false}
            , deprecated {false}
            , invalidRepository {false}
            , shallowRepository {false}
            , corruptRepository {false}
            , wrongUrl {false}
            , manuallyDownloaded {false}
            , hasLocalChanges {false}
            , isCloned {false}
        {
        }
    };

    void removeDisabledRepositories();
    bool fetchThemesList();
    bool fetchRepository(const std::string& repositoryName, bool allowReset = false);
    bool cloneRepository(const std::string& repositoryName, const std::string& url);

    bool checkLocalChanges(git_repository* repository);
    bool checkCorruptRepository(git_repository* repository);
    void resetRepository(git_repository* repository);
    void makeInventory();
    bool renameDirectory(const std::string& path, const std::string& extension);
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
    std::function<void()> mUpdateCallback;

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
        NOT_A_REPOSITORY,
        INVALID_ORIGIN,
        HAS_DIVERGED,
        CLONE_ERROR,
        FETCH_ERROR
    };

    RepositoryError mRepositoryError;
    std::string mThemeDirectory;
    std::string mMessage;
    std::thread mFetchThread;
    std::promise<bool> mPromise;
    std::future<bool> mFuture;
    std::atomic<bool> mFetching;
    std::atomic<bool> mLatestThemesList;
    bool mAttemptedFetch;
    bool mHasThemeUpdates;
    static inline std::atomic<float> mReceivedObjectsProgress {0.0f};
    static inline std::atomic<float> mResolveDeltaProgress {0.0f};
    std::vector<ThemeEntry> mThemes;
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
    std::shared_ptr<TextComponent> mInfoField;
    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<TextComponent> mVariantsLabel;
    std::shared_ptr<TextComponent> mColorSchemesLabel;
    std::shared_ptr<TextComponent> mAspectRatiosLabel;
    std::shared_ptr<TextComponent> mFontSizesLabel;
    std::shared_ptr<TextComponent> mAuthor;
    std::shared_ptr<TextComponent> mVariantCount;
    std::shared_ptr<TextComponent> mColorSchemesCount;
    std::shared_ptr<TextComponent> mAspectRatiosCount;
    std::shared_ptr<TextComponent> mFontSizesCount;
};

#endif // ES_APP_GUIS_GUI_THEME_DOWNLOADER_H
