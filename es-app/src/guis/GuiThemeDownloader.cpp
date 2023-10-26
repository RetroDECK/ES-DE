//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiThemeDownloader.cpp
//
//  Theme downloader.
//

#include "guis/GuiThemeDownloader.h"

#include "EmulationStation.h"
#include "ThemeData.h"
#include "components/MenuComponent.h"
#include "resources/ResourceManager.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#define LOCAL_TESTING_FILE false
#define DEBUG_CLONING false

GuiThemeDownloader::GuiThemeDownloader(std::function<void()> updateCallback)
    : mRenderer {Renderer::getInstance()}
    , mBackground {":/graphics/frame.svg"}
    , mGrid {glm::ivec2 {2, 4}}
    , mUpdateCallback(updateCallback)
    , mRepositoryError {RepositoryError::NO_REPO_ERROR}
    , mFetching {false}
    , mLatestThemesList {false}
    , mAttemptedFetch {false}
    , mHasThemeUpdates {false}
    , mFullscreenViewing {false}
    , mFullscreenViewerIndex {0}
{
    addChild(&mBackground);
    addChild(&mGrid);

#if defined(_WIN64)
    // Required due to the idiotic file locking that exists on this operating system.
    ViewController::getInstance()->stopViewVideos();
#endif

    const float fontSizeSmall {mRenderer->getIsVerticalOrientation() ? FONT_SIZE_MINI :
                                                                       FONT_SIZE_SMALL};

    // Set up main grid.
    mTitle = std::make_shared<TextComponent>("THEME DOWNLOADER", Font::get(FONT_SIZE_LARGE),
                                             mMenuColorTitle, ALIGN_CENTER);
    mGrid.setEntry(mTitle, glm::ivec2 {0, 0}, false, true, glm::ivec2 {2, 2},
                   GridFlags::BORDER_BOTTOM);

    // We need a center grid embedded within the main grid in order for navigation and helpsystem
    // entries to work and display correctly.
    mCenterGrid = std::make_shared<ComponentGrid>(glm::ivec2 {8, 5});
    mCenterGrid->setEntry(std::make_shared<GuiComponent>(), glm::ivec2 {0, 0}, false, false,
                          glm::ivec2 {1, 5});

    mVariantsLabel =
        std::make_shared<TextComponent>("", Font::get(fontSizeSmall), mMenuColorTitle, ALIGN_LEFT);
    mCenterGrid->setEntry(mVariantsLabel, glm::ivec2 {1, 0}, false, true, glm::ivec2 {1, 1});

    mColorSchemesLabel =
        std::make_shared<TextComponent>("", Font::get(fontSizeSmall), mMenuColorTitle, ALIGN_LEFT);
    mCenterGrid->setEntry(mColorSchemesLabel, glm::ivec2 {1, 1}, false, true, glm::ivec2 {1, 1});

    mAspectRatiosLabel =
        std::make_shared<TextComponent>("", Font::get(fontSizeSmall), mMenuColorTitle, ALIGN_LEFT);
    mCenterGrid->setEntry(mAspectRatiosLabel, glm::ivec2 {3, 0}, false, true, glm::ivec2 {1, 1});

    mFutureUseLabel =
        std::make_shared<TextComponent>("", Font::get(fontSizeSmall), mMenuColorTitle, ALIGN_LEFT);
    mCenterGrid->setEntry(mFutureUseLabel, glm::ivec2 {3, 1}, false, true, glm::ivec2 {1, 1});

    mCenterGrid->setEntry(std::make_shared<GuiComponent>(), glm::ivec2 {5, 0}, false, false,
                          glm::ivec2 {1, 5});

    mVariantCount = std::make_shared<TextComponent>("", Font::get(fontSizeSmall, FONT_PATH_LIGHT),
                                                    mMenuColorTitle, ALIGN_LEFT);
    mCenterGrid->setEntry(mVariantCount, glm::ivec2 {2, 0}, false, true, glm::ivec2 {1, 1});

    mColorSchemesCount = std::make_shared<TextComponent>(
        "", Font::get(fontSizeSmall, FONT_PATH_LIGHT), mMenuColorTitle, ALIGN_LEFT);
    mCenterGrid->setEntry(mColorSchemesCount, glm::ivec2 {2, 1}, false, true, glm::ivec2 {1, 1});

    mAspectRatiosCount = std::make_shared<TextComponent>(
        "", Font::get(fontSizeSmall, FONT_PATH_LIGHT), mMenuColorTitle, ALIGN_LEFT);
    mCenterGrid->setEntry(mAspectRatiosCount, glm::ivec2 {4, 0}, false, true, glm::ivec2 {1, 1});

    mFutureUseCount = std::make_shared<TextComponent>("", Font::get(fontSizeSmall, FONT_PATH_LIGHT),
                                                      mMenuColorTitle, ALIGN_LEFT);
    mCenterGrid->setEntry(mFutureUseCount, glm::ivec2 {4, 1}, false, true, glm::ivec2 {1, 1});

    mDownloadStatus = std::make_shared<TextComponent>("", Font::get(fontSizeSmall, FONT_PATH_BOLD),
                                                      mMenuColorTitle, ALIGN_LEFT);
    mCenterGrid->setEntry(mDownloadStatus, glm::ivec2 {1, 2}, false, true, glm::ivec2 {2, 1});

    mLocalChanges = std::make_shared<TextComponent>("", Font::get(fontSizeSmall, FONT_PATH_BOLD),
                                                    mMenuColorTitle, ALIGN_LEFT);
    mCenterGrid->setEntry(mLocalChanges, glm::ivec2 {3, 2}, false, true, glm::ivec2 {2, 1});

    mScreenshot = std::make_shared<ImageComponent>();
    mScreenshot->setLinearInterpolation(true);
    mCenterGrid->setEntry(mScreenshot, glm::ivec2 {1, 3}, false, true, glm::ivec2 {4, 1});

    mAuthor = std::make_shared<TextComponent>("", Font::get(FONT_SIZE_MINI * 0.9f, FONT_PATH_LIGHT),
                                              mMenuColorTitle, ALIGN_LEFT);
    mCenterGrid->setEntry(mAuthor, glm::ivec2 {1, 4}, false, true, glm::ivec2 {4, 1});

    mList = std::make_shared<ComponentList>();
    mCenterGrid->setEntry(mList, glm::ivec2 {6, 0}, true, true, glm::ivec2 {2, 5},
                          GridFlags::BORDER_LEFT);

    mGrid.setEntry(mCenterGrid, glm::ivec2 {0, 2}, true, false, glm::ivec2 {2, 1});

    // Set up scroll indicators.
    mScrollUp = std::make_shared<ImageComponent>();
    mScrollDown = std::make_shared<ImageComponent>();

    mScrollUp->setResize(0.0f, mTitle->getFont()->getLetterHeight() / 2.0f);
    mScrollUp->setOrigin(0.0f, -0.35f);

    mScrollDown->setResize(0.0f, mTitle->getFont()->getLetterHeight() / 2.0f);
    mScrollDown->setOrigin(0.0f, 0.35f);

    mScrollIndicator = std::make_shared<ScrollIndicatorComponent>(mList, mScrollUp, mScrollDown);

    mGrid.setEntry(mScrollUp, glm::ivec2 {1, 0}, false, false, glm::ivec2 {1, 1});
    mGrid.setEntry(mScrollDown, glm::ivec2 {1, 1}, false, false, glm::ivec2 {1, 1});

    std::vector<std::shared_ptr<ButtonComponent>> buttons;
    buttons.push_back(std::make_shared<ButtonComponent>("CLOSE", "CLOSE", [&] { delete this; }));
    mButtons = MenuComponent::makeButtonGrid(buttons);
    mGrid.setEntry(mButtons, glm::ivec2 {0, 3}, true, false, glm::ivec2 {2, 1},
                   GridFlags::BORDER_TOP);

    // Limit the width of the GUI on ultrawide monitors. The 1.778 aspect ratio value is
    // the 16:9 reference.
    const float aspectValue {1.778f / Renderer::getScreenAspectRatio()};
    const float width {glm::clamp(0.95f * aspectValue, 0.45f, 0.98f) * mRenderer->getScreenWidth()};
    setSize(width,
            mTitle->getSize().y + (mList->getRowHeight() * 9.0f) + mButtons->getSize().y * 1.1f);

    setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f,
                (mRenderer->getScreenHeight() - mSize.y) / 2.0f);

    mBusyAnim.setSize(mSize);
    mBusyAnim.setText("DOWNLOADING THEMES LIST 100%");
    mBusyAnim.onSizeChanged();

    mList->setCursorChangedCallback([this](CursorState state) {
        if (state == CursorState::CURSOR_SCROLLING || state == CursorState::CURSOR_STOPPED)
            updateInfoPane();
    });

    mViewerIndicatorLeft = std::make_shared<TextComponent>(
        ViewController::ARROW_LEFT_CHAR, Font::get(FONT_SIZE_LARGE * 1.2f, FONT_PATH_BOLD),
        0xCCCCCCFF, ALIGN_CENTER);

    mViewerIndicatorRight = std::make_shared<TextComponent>(
        ViewController::ARROW_RIGHT_CHAR, Font::get(FONT_SIZE_LARGE * 1.2f, FONT_PATH_BOLD),
        0xCCCCCCFF, ALIGN_CENTER);

    git_libgit2_init();

    // The promise/future mechanism is used as signaling for the thread to indicate that
    // repository fetching has been completed.
    std::promise<bool>().swap(mPromise);
    mFuture = mPromise.get_future();

    const std::string defaultUserThemeDir {Utils::FileSystem::getHomePath() +
                                           "/.emulationstation/themes"};
    std::string userThemeDirSetting {Utils::FileSystem::expandHomePath(
        Settings::getInstance()->getString("UserThemeDirectory"))};
#if defined(_WIN64)
    mThemeDirectory = Utils::String::replace(mThemeDirectory, "\\", "/");
#endif

    if (userThemeDirSetting == "") {
        mThemeDirectory = defaultUserThemeDir;
    }
    else if (Utils::FileSystem::isDirectory(userThemeDirSetting) ||
             Utils::FileSystem::isSymlink(userThemeDirSetting)) {
        mThemeDirectory = userThemeDirSetting;
    }
    else {
        LOG(LogWarning) << "GuiThemeDownloader: Requested user theme directory \""
                        << userThemeDirSetting
                        << "\" does not exist or is not a directory, reverting to \""
                        << defaultUserThemeDir << "\"";
        mThemeDirectory = defaultUserThemeDir;
    }

    if (mThemeDirectory.back() != '/')
        mThemeDirectory.append("/");
}

GuiThemeDownloader::~GuiThemeDownloader()
{
    if (mFetchThread.joinable())
        mFetchThread.join();

    git_libgit2_shutdown();

    if (mHasThemeUpdates) {
        LOG(LogInfo) << "GuiThemeDownloader: There are updates, repopulating the themes";
        ThemeData::populateThemes();
        ViewController::getInstance()->reloadAll();
        if (mUpdateCallback)
            mUpdateCallback();
    }

    mWindow->stopInfoPopup();
}

bool GuiThemeDownloader::fetchRepository(const std::string& repositoryName, bool allowReset)
{
    int errorCode {0};
    const std::string path {mThemeDirectory + repositoryName};
    mRepositoryError = RepositoryError::NO_REPO_ERROR;
    mMessage = "";

    const bool isThemesList {repositoryName == "themes-list"};
    git_repository* repository {nullptr};
    git_remote* gitRemote {nullptr};

    try {
        mFetching = true;
        errorCode = git_repository_open(&repository, &path[0]);

        if (errorCode != 0) {
            mRepositoryError = RepositoryError::NOT_A_REPOSITORY;
            throw std::runtime_error("Couldn't open local repository, ");
        }
        errorCode = git_remote_lookup(&gitRemote, repository, "origin");
        if (errorCode != 0) {
            mRepositoryError = RepositoryError::INVALID_ORIGIN;
            throw std::runtime_error("Couldn't get information about origin, ");
        }

#if LIBGIT2_VER_MAJOR >= 1
        git_fetch_options fetchOptions;
        git_fetch_options_init(&fetchOptions, GIT_FETCH_OPTIONS_VERSION);
#else
        git_fetch_options fetchOptions = GIT_FETCH_OPTIONS_INIT;
#endif
        // Prune branches that are no longer present on remote.
        fetchOptions.prune = GIT_FETCH_PRUNE;

        errorCode = git_remote_fetch(gitRemote, nullptr, &fetchOptions, nullptr);

        if (errorCode != 0)
            throw std::runtime_error("Couldn't fetch latest commits for \"" + repositoryName +
                                     "\", ");

        git_annotated_commit* annotated {nullptr};
        git_object* object {nullptr};

        if (git_repository_head_detached(repository)) {
            LOG(LogWarning) << "GuiThemeDownloader: Repository \"" << repositoryName
                            << "\" has HEAD detached, resetting it";
            git_buf buffer {};
            errorCode = git_remote_default_branch(&buffer, gitRemote);
            if (errorCode == 0) {
                git_reference* oldTargetRef;
                git_repository_head(&oldTargetRef, repository);

                const std::string branchName {buffer.ptr, buffer.size};
                errorCode = git_revparse_single(&object, repository, branchName.c_str());
#if LIBGIT2_VER_MAJOR >= 1
                git_checkout_options checkoutOptions;
                git_checkout_options_init(&checkoutOptions, GIT_CHECKOUT_OPTIONS_VERSION);
#else
                git_checkout_options checkoutOptions = GIT_CHECKOUT_OPTIONS_INIT;
#endif
                checkoutOptions.checkout_strategy = GIT_CHECKOUT_FORCE;
                errorCode = git_checkout_tree(repository, object, &checkoutOptions);
                errorCode = git_repository_set_head(repository, branchName.c_str());

                git_reference_free(oldTargetRef);
            }
            git_buf_dispose(&buffer);
            if (repositoryName != "themes-list")
                mHasThemeUpdates = true;
        }

        errorCode = git_revparse_single(&object, repository, "FETCH_HEAD");
        errorCode = git_annotated_commit_lookup(&annotated, repository, git_object_id(object));

        git_merge_analysis_t mergeAnalysis {};
        git_merge_preference_t mergePreference {};

        errorCode = git_merge_analysis(&mergeAnalysis, &mergePreference, repository,
                                       (const git_annotated_commit**)(&annotated), 1);

        if (errorCode != 0) {
            git_object_free(object);
            git_annotated_commit_free(annotated);
            throw std::runtime_error("GuiThemeDownloader: Couldn't run Git merge analysis, ");
        }

        if (!(mergeAnalysis & GIT_MERGE_ANALYSIS_UP_TO_DATE) &&
            !(mergeAnalysis & GIT_MERGE_ANALYSIS_FASTFORWARD)) {
            if (allowReset) {
                LOG(LogWarning) << "GuiThemeDownloader: Repository \"" << repositoryName
                                << "\" has diverged from origin, performing hard reset";
                git_object* objectHead {nullptr};
                errorCode = git_revparse_single(&objectHead, repository, "HEAD");
                errorCode = git_reset(repository, objectHead, GIT_RESET_HARD, nullptr);
                git_object_free(objectHead);
                if (repositoryName != "themes-list")
                    mHasThemeUpdates = true;
            }
            else {
                LOG(LogWarning) << "GuiThemeDownloader: Repository \"" << repositoryName
                                << "\" has diverged from origin, can't fast-forward";
                git_annotated_commit_free(annotated);
                git_object_free(object);
                mPromise.set_value(true);
                mRepositoryError = RepositoryError::HAS_DIVERGED;
                return true;
            }
        }

        if (allowReset && checkLocalChanges(repository)) {
            LOG(LogWarning) << "GuiThemeDownloader: Repository \"" << repositoryName
                            << "\" contains local changes, performing hard reset";
            resetRepository(repository);
            if (repositoryName != "themes-list")
                mHasThemeUpdates = true;
        }

        if (mergeAnalysis & GIT_MERGE_ANALYSIS_UP_TO_DATE) {
            LOG(LogInfo) << "GuiThemeDownloader: Repository \"" << repositoryName
                         << "\" already up to date";
            if (repositoryName != "themes-list")
                mMessage = "THEME ALREADY UP TO DATE";
            git_annotated_commit_free(annotated);
            git_object_free(object);
            git_remote_free(gitRemote);
            git_repository_free(repository);
            mPromise.set_value(true);
            if (isThemesList)
                mLatestThemesList = true;
            return false;
        }

        LOG(LogInfo) << "GuiThemeDownloader: Performing fast-forward of repository \""
                     << repositoryName << "\"";

        git_reference* oldTargetRef {nullptr};
        git_repository_head(&oldTargetRef, repository);

        const git_oid* objectID {nullptr};
        objectID = git_annotated_commit_id(annotated);

        git_object_lookup(&object, repository, objectID, GIT_OBJECT_COMMIT);
        git_reference* newTargetRef {nullptr};

#if LIBGIT2_VER_MAJOR >= 1
        git_checkout_options checkoutOptions;
        git_checkout_options_init(&checkoutOptions, GIT_CHECKOUT_OPTIONS_VERSION);
#else
        git_checkout_options checkoutOptions = GIT_CHECKOUT_OPTIONS_INIT;
#endif
        checkoutOptions.checkout_strategy = GIT_CHECKOUT_FORCE;

        git_checkout_tree(repository, object, &checkoutOptions);
        errorCode = git_reference_set_target(&newTargetRef, oldTargetRef, objectID, nullptr);

        git_reference_free(oldTargetRef);
        git_reference_free(newTargetRef);
        git_annotated_commit_free(annotated);
        // Not sure why you need to run this twice, but if you don't there will be a memory leak.
        git_object_free(object);
        git_object_free(object);

        if (errorCode != 0)
            throw std::runtime_error("Couldn't fast-forward repository, ");

        if (isThemesList)
            mLatestThemesList = true;
    }
    catch (std::runtime_error& runtimeError) {
        const git_error* gitError {git_error_last()};
        LOG(LogError) << "GuiThemeDownloader: " << runtimeError.what() << gitError->message;
        mRepositoryError = RepositoryError::FETCH_ERROR;
        mMessage = gitError->message;
        git_error_clear();
        git_remote_free(gitRemote);
        git_repository_free(repository);
        mPromise.set_value(true);
        return true;
    }

    if (repositoryName != "themes-list") {
        mMessage = "THEME HAS BEEN UPDATED";
        mHasThemeUpdates = true;
    }

    git_remote_free(gitRemote);
    git_repository_free(repository);
    mPromise.set_value(true);
    return false;
}

bool GuiThemeDownloader::checkLocalChanges(git_repository* repository)
{
    git_status_list* status {nullptr};
    size_t statusEntryCount {0};
    int errorCode {0};

#if LIBGIT2_VER_MAJOR >= 1
    git_status_options statusOptions;
    git_status_options_init(&statusOptions, GIT_STATUS_OPTIONS_VERSION);
#else
    git_status_options statusOptions = GIT_STATUS_OPTIONS_INIT;
#endif
    // We don't include untracked files (GIT_STATUS_OPT_INCLUDE_UNTRACKED) as this makes
    // it possible to add custom files to the repository without overwriting these when
    // pulling theme updates.
    statusOptions.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    statusOptions.flags =
        GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX | GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;

    errorCode = git_status_list_new(&status, repository, &statusOptions);
    if (errorCode == 0)
        statusEntryCount = git_status_list_entrycount(status);

    git_status_list_free(status);
    // TODO: Also check if there are any local commits not on origin.

    return (statusEntryCount != 0);
}

bool GuiThemeDownloader::checkCorruptRepository(git_repository* repository)
{
    // For the time being we only check if there are no tracked files in the repository. If there
    // are none then it would indicate that it has not been properly cloned (for example if the
    // ES-DE process was killed during the clone operation).
    git_status_list* status {nullptr};
    size_t statusEntryCount {0};
    int errorCode {0};

#if LIBGIT2_VER_MAJOR >= 1
    git_status_options statusOptions;
    git_status_options_init(&statusOptions, GIT_STATUS_OPTIONS_VERSION);
#else
    git_status_options statusOptions = GIT_STATUS_OPTIONS_INIT;
#endif
    statusOptions.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    statusOptions.flags = GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX |
                          GIT_STATUS_OPT_SORT_CASE_SENSITIVELY | GIT_STATUS_OPT_INCLUDE_UNMODIFIED;

    errorCode = git_status_list_new(&status, repository, &statusOptions);
    if (errorCode == 0)
        statusEntryCount = git_status_list_entrycount(status);

    git_status_list_free(status);

    return (statusEntryCount == 0);
}

void GuiThemeDownloader::resetRepository(git_repository* repository)
{
    git_object* objectHead {nullptr};
    if (git_revparse_single(&objectHead, repository, "HEAD") == 0)
        git_reset(repository, objectHead, GIT_RESET_HARD, nullptr);
    git_object_free(objectHead);
}

void GuiThemeDownloader::makeInventory()
{
    for (auto& theme : mThemes) {
        const std::string path {mThemeDirectory + theme.reponame};
        theme.invalidRepository = false;
        theme.corruptRepository = false;
        theme.shallowRepository = false;
        theme.manuallyDownloaded = false;
        theme.hasLocalChanges = false;
        theme.isCloned = false;

        if (Utils::FileSystem::exists(path + "-main")) {
            theme.manuallyDownloaded = true;
            theme.manualExtension = "-main";
        }
        else if (Utils::FileSystem::exists(path + "-master")) {
            theme.manuallyDownloaded = true;
            theme.manualExtension = "-master";
        }

        if (Utils::FileSystem::exists(path)) {
            git_repository* repository {nullptr};
            int errorCode {0};

            errorCode = git_repository_open(&repository, &path[0]);
            if (errorCode != 0) {
                theme.invalidRepository = true;
                git_repository_free(repository);
                continue;
            }

            if (git_repository_is_shallow(repository)) {
                theme.shallowRepository = true;
                git_repository_free(repository);
                continue;
            }

            if (checkCorruptRepository(repository)) {
                theme.corruptRepository = true;
                git_repository_free(repository);
                continue;
            }

            theme.isCloned = true;

            if (checkLocalChanges(repository))
                theme.hasLocalChanges = true;
            else if (git_repository_head_detached(repository))
                theme.hasLocalChanges = true;

            git_repository_free(repository);
        }
    }
}

bool GuiThemeDownloader::renameDirectory(const std::string& path, const std::string& extension)
{
    LOG(LogInfo) << "Renaming directory " << path;
    int index {1};
    bool renameStatus {false};

    if (!Utils::FileSystem::exists(path + extension)) {
        renameStatus = Utils::FileSystem::renameFile(path, path + extension, false);
    }
    else {
        // This will hopefully never be needed as it should only occur if a theme has been
        // downloaded manually multiple times and the theme downloader has been ran multiple times
        // as well.
        for (; index < 10; ++index) {
            if (!Utils::FileSystem::exists(path + "_" + std::to_string(index) + extension)) {
                renameStatus = Utils::FileSystem::renameFile(
                    path, path + "_" + std::to_string(index) + extension, false);
                break;
            }
        }
    }

    if (renameStatus) {
        mWindow->pushGui(new GuiMsgBox(
            getHelpStyle(), "COULDN'T RENAME DIRECTORY \"" + path + "\", PERMISSION PROBLEMS?",
            "OK", [] { return; }, "", nullptr, "", nullptr, nullptr, true));
        return true;
    }
    else {
        return false;
    }
}

void GuiThemeDownloader::parseThemesList()
{
#if (LOCAL_TESTING_FILE)
    LOG(LogWarning) << "GuiThemeDownloader: Using local \"themes.json\" testing file";

    const std::string themesFile {Utils::FileSystem::getHomePath() +
                                  "/.emulationstation/themes.json"};
#else
    const std::string themesFile {mThemeDirectory + "themes-list/themes.json"};
#endif

    if (!Utils::FileSystem::exists(themesFile)) {
        LOG(LogError) << "GuiThemeDownloader: No themes.json file found";
        mWindow->pushGui(new GuiMsgBox(
            getHelpStyle(), "COULDN'T FIND THE THEMES LIST CONFIGURATION FILE", "OK",
            [] { return; }, "", nullptr, "", nullptr, nullptr, true));
        mGrid.removeEntry(mCenterGrid);
        mGrid.setCursorTo(mButtons);
        return;
    }

    const ResourceData& themesFileData {ResourceManager::getInstance().getFileData(themesFile)};
    rapidjson::Document doc;
    doc.Parse(reinterpret_cast<const char*>(themesFileData.ptr.get()), themesFileData.length);

    if (doc.HasParseError()) {
        LOG(LogError) << "GuiThemeDownloader: Couldn't parse the themes.json file";
        mWindow->pushGui(new GuiMsgBox(
            getHelpStyle(),
            "COULDN'T PARSE THE THEMES LIST CONFIGURATION FILE, MAYBE THE LOCAL REPOSITORY IS "
            "CORRUPT?",
            "OK", [] { return; }, "", nullptr, "", nullptr, nullptr, true));
        mGrid.removeEntry(mCenterGrid);
        mGrid.setCursorTo(mButtons);
        return;
    }

    if (doc.HasMember("latestStableRelease") && doc["latestStableRelease"].IsString()) {
        const int latestStableRelease {std::stoi(doc["latestStableRelease"].GetString())};
        if (latestStableRelease > PROGRAM_RELEASE_NUMBER) {
            LOG(LogWarning) << "Not running the most current application release, theme "
                               "downloading is not recommended";
            mWindow->pushGui(new GuiMsgBox(
                getHelpStyle(),
                "IT SEEMS AS IF YOU'RE NOT RUNNING THE LATEST ES-DE RELEASE, PLEASE UPGRADE BEFORE "
                "PROCEEDING AS THESE THEMES MAY NOT BE COMPATIBLE WITH YOUR VERSION",
                "OK", [] { return; }, "", nullptr, "", nullptr, nullptr, true));
        }
    }

    if (doc.HasMember("themes") && doc["themes"].IsArray()) {
        const rapidjson::Value& themes {doc["themes"]};
        for (int i {0}; i < static_cast<int>(themes.Size()); ++i) {
            ThemeEntry themeEntry;
            const rapidjson::Value& theme {themes[i]};

            if (theme.HasMember("name") && theme["name"].IsString())
                themeEntry.name = theme["name"].GetString();

            if (theme.HasMember("reponame") && theme["reponame"].IsString())
                themeEntry.reponame = theme["reponame"].GetString();

            if (theme.HasMember("url") && theme["url"].IsString())
                themeEntry.url = theme["url"].GetString();

            if (theme.HasMember("author") && theme["author"].IsString())
                themeEntry.author = theme["author"].GetString();

            if (theme.HasMember("newEntry") && theme["newEntry"].IsBool())
                themeEntry.newEntry = theme["newEntry"].GetBool();

            if (theme.HasMember("variants") && theme["variants"].IsArray()) {
                const rapidjson::Value& variants {theme["variants"]};
                for (int i {0}; i < static_cast<int>(variants.Size()); ++i)
                    themeEntry.variants.emplace_back(variants[i].GetString());
            }

            if (theme.HasMember("colorSchemes") && theme["colorSchemes"].IsArray()) {
                const rapidjson::Value& colorSchemes {theme["colorSchemes"]};
                for (int i {0}; i < static_cast<int>(colorSchemes.Size()); ++i)
                    themeEntry.colorSchemes.emplace_back(colorSchemes[i].GetString());
            }

            if (theme.HasMember("aspectRatios") && theme["aspectRatios"].IsArray()) {
                const rapidjson::Value& aspectRatios {theme["aspectRatios"]};
                for (int i {0}; i < static_cast<int>(aspectRatios.Size()); ++i)
                    themeEntry.aspectRatios.emplace_back(aspectRatios[i].GetString());
            }

            if (theme.HasMember("transitions") && theme["transitions"].IsArray()) {
                const rapidjson::Value& transitions {theme["transitions"]};
                for (int i {0}; i < static_cast<int>(transitions.Size()); ++i)
                    themeEntry.transitions.emplace_back(transitions[i].GetString());
            }

            if (theme.HasMember("screenshots") && theme["screenshots"].IsArray()) {
                const rapidjson::Value& screenshots {theme["screenshots"]};
                for (int i {0}; i < static_cast<int>(screenshots.Size()); ++i) {
                    Screenshot screenshotEntry;
                    if (screenshots[i].HasMember("image") && screenshots[i]["image"].IsString())
                        screenshotEntry.image = screenshots[i]["image"].GetString();

                    if (screenshots[i].HasMember("caption") && screenshots[i]["caption"].IsString())
                        screenshotEntry.caption = screenshots[i]["caption"].GetString();

                    if (screenshotEntry.image != "" && screenshotEntry.caption != "")
                        themeEntry.screenshots.emplace_back(screenshotEntry);
                }
            }

            mThemes.emplace_back(themeEntry);
        }
    }

    LOG(LogDebug) << "GuiThemeDownloader::parseThemesList(): Parsed " << mThemes.size()
                  << " themes";
}

void GuiThemeDownloader::populateGUI()
{
    if (mThemes.empty())
        return;

    for (auto& theme : mThemes) {
        std::string themeName {Utils::String::toUpper(theme.name)};
        if (theme.newEntry && !theme.isCloned)
            themeName.append(" ").append(ViewController::BRANCH_CHAR);
        if (theme.isCloned)
            themeName.append(" ").append(ViewController::TICKMARK_CHAR);
        if (theme.manuallyDownloaded || theme.invalidRepository || theme.corruptRepository ||
            theme.shallowRepository)
            themeName.append(" ").append(ViewController::CROSSEDCIRCLE_CHAR);
        if (theme.hasLocalChanges)
            themeName.append(" ").append(ViewController::EXCLAMATION_CHAR);

        ComponentListRow row;
        std::shared_ptr<TextComponent> themeNameElement {std::make_shared<TextComponent>(
            themeName, Font::get(FONT_SIZE_MEDIUM), mMenuColorPrimary)};

        ThemeGUIEntry guiEntry;
        guiEntry.themeName = themeNameElement;
        mThemeGUIEntries.emplace_back(guiEntry);
        row.addElement(themeNameElement, false);

        row.makeAcceptInputHandler([this, &theme] {
            std::promise<bool>().swap(mPromise);
            if (theme.manuallyDownloaded || theme.invalidRepository) {
                mWindow->pushGui(new GuiMsgBox(
                    getHelpStyle(),
                    "IT SEEMS AS IF THIS THEME HAS BEEN MANUALLY DOWNLOADED INSTEAD OF VIA "
                    "THIS THEME DOWNLOADER. A FRESH DOWNLOAD IS REQUIRED AND THE OLD THEME "
                    "DIRECTORY \"" +
                        theme.reponame + theme.manualExtension + "\" WILL BE RENAMED TO \"" +
                        theme.reponame + theme.manualExtension + "_DISABLED\"",
                    "PROCEED",
                    [this, theme] {
                        if (renameDirectory(mThemeDirectory + theme.reponame +
                                                theme.manualExtension,
                                            "_DISABLED")) {
                            return;
                        }
                        std::promise<bool>().swap(mPromise);
                        mFuture = mPromise.get_future();
                        mFetchThread = std::thread(&GuiThemeDownloader::cloneRepository, this,
                                                   theme.reponame, theme.url);
                        mStatusType = StatusType::STATUS_DOWNLOADING;
                        mStatusText = "DOWNLOADING THEME";
                    },
                    "CANCEL", [] { return; }, "", nullptr, nullptr, false, true,
                    (mRenderer->getIsVerticalOrientation() ?
                         0.75f :
                         0.46f * (1.778f / mRenderer->getScreenAspectRatio()))));
            }
            else if (theme.corruptRepository) {
                mWindow->pushGui(new GuiMsgBox(
                    getHelpStyle(),
                    "IT SEEMS AS IF THIS THEME REPOSITORY IS CORRUPT, WHICH COULD HAVE BEEN CAUSED "
                    "BY AN INTERRUPTION OF A PREVIOUS DOWNLOAD OR UPDATE, FOR EXAMPLE IF THE ES-DE "
                    "PROCESS WAS KILLED. A FRESH DOWNLOAD IS REQUIRED AND THE OLD THEME DIRECTORY "
                    "\"" +
                        theme.reponame + theme.manualExtension + "\" WILL BE RENAMED TO \"" +
                        theme.reponame + theme.manualExtension + "_CORRUPT_DISABLED\"",
                    "PROCEED",
                    [this, theme] {
                        if (renameDirectory(mThemeDirectory + theme.reponame +
                                                theme.manualExtension,
                                            "_CORRUPT_DISABLED")) {
                            return;
                        }
                        std::promise<bool>().swap(mPromise);
                        mFuture = mPromise.get_future();
                        mFetchThread = std::thread(&GuiThemeDownloader::cloneRepository, this,
                                                   theme.reponame, theme.url);
                        mStatusType = StatusType::STATUS_DOWNLOADING;
                        mStatusText = "DOWNLOADING THEME";
                    },
                    "CANCEL", [] { return; }, "", nullptr, nullptr, false, true,
                    (mRenderer->getIsVerticalOrientation() ?
                         0.75f :
                         0.46f * (1.778f / mRenderer->getScreenAspectRatio()))));
            }
            else if (theme.shallowRepository) {
                mWindow->pushGui(new GuiMsgBox(
                    getHelpStyle(),
                    "IT SEEMS AS IF THIS IS A SHALLOW REPOSITORY WHICH MEANS THAT IT HAS BEEN "
                    "DOWNLOADED USING SOME OTHER TOOL THAN THIS THEME DOWNLOADER. A FRESH DOWNLOAD "
                    "IS REQUIRED AND THE OLD THEME DIRECTORY \"" +
                        theme.reponame + theme.manualExtension + "\" WILL BE RENAMED TO \"" +
                        theme.reponame + theme.manualExtension + "_DISABLED\"",
                    "PROCEED",
                    [this, theme] {
                        if (renameDirectory(mThemeDirectory + theme.reponame +
                                                theme.manualExtension,
                                            "_DISABLED")) {
                            return;
                        }
                        std::promise<bool>().swap(mPromise);
                        mFuture = mPromise.get_future();
                        mFetchThread = std::thread(&GuiThemeDownloader::cloneRepository, this,
                                                   theme.reponame, theme.url);
                        mStatusType = StatusType::STATUS_DOWNLOADING;
                        mStatusText = "DOWNLOADING THEME";
                    },
                    "CANCEL", [] { return; }, "", nullptr, nullptr, false, true,
                    (mRenderer->getIsVerticalOrientation() ?
                         0.75f :
                         0.46f * (1.778f / mRenderer->getScreenAspectRatio()))));
            }
            else if (theme.hasLocalChanges) {
                mWindow->pushGui(new GuiMsgBox(
                    getHelpStyle(),
                    "THEME REPOSITORY \"" + theme.reponame +
                        "\" CONTAINS LOCAL CHANGES. PROCEED TO OVERWRITE YOUR CHANGES "
                        "OR CANCEL TO SKIP ALL UPDATES FOR THIS THEME",
                    "PROCEED",
                    [this, theme] {
                        std::promise<bool>().swap(mPromise);
                        mFuture = mPromise.get_future();
                        mFetchThread = std::thread(&GuiThemeDownloader::fetchRepository, this,
                                                   theme.reponame, true);
                        mStatusType = StatusType::STATUS_UPDATING;
                        mStatusText = "UPDATING THEME";
                    },
                    "CANCEL", [] { return; }, "", nullptr, nullptr, false, true,
                    (mRenderer->getIsVerticalOrientation() ?
                         0.75f :
                         0.45f * (1.778f / mRenderer->getScreenAspectRatio()))));
            }
            else if (theme.isCloned) {
                mFuture = mPromise.get_future();
                mFetchThread =
                    std::thread(&GuiThemeDownloader::fetchRepository, this, theme.reponame, false);
                mStatusType = StatusType::STATUS_UPDATING;
                mStatusText = "UPDATING THEME";
            }
            else {
                mFuture = mPromise.get_future();
                mFetchThread = std::thread(&GuiThemeDownloader::cloneRepository, this,
                                           theme.reponame, theme.url);
                mStatusType = StatusType::STATUS_DOWNLOADING;
                mStatusText = "DOWNLOADING THEME";
            }
            mWindow->stopInfoPopup();
        });
        mList->addRow(row);
    }

    mVariantsLabel->setText("VARIANTS:");
    mColorSchemesLabel->setText("COLOR SCHEMES:");
    mAspectRatiosLabel->setText("ASPECT RATIOS:");

    updateInfoPane();
    updateHelpPrompts();
}

void GuiThemeDownloader::updateGUI()
{
    updateInfoPane();
    updateHelpPrompts();

    for (size_t i {0}; i < mThemes.size(); ++i) {
        std::string themeName {Utils::String::toUpper(mThemes[i].name)};
        if (mThemes[i].newEntry && !mThemes[i].isCloned)
            themeName.append(" ").append(ViewController::BRANCH_CHAR);
        if (mThemes[i].isCloned)
            themeName.append(" ").append(ViewController::TICKMARK_CHAR);
        if (mThemes[i].manuallyDownloaded || mThemes[i].invalidRepository ||
            mThemes[i].corruptRepository || mThemes[i].shallowRepository)
            themeName.append(" ").append(ViewController::CROSSEDCIRCLE_CHAR);
        if (mThemes[i].hasLocalChanges)
            themeName.append(" ").append(ViewController::EXCLAMATION_CHAR);

        mThemeGUIEntries[i].themeName->setText(themeName);
    }
}

void GuiThemeDownloader::updateInfoPane()
{
    assert(static_cast<size_t>(mList->size()) == mThemes.size());
    if (!mThemes[mList->getCursorId()].screenshots.empty())
        mScreenshot->setImage(mThemeDirectory + "themes-list/" +
                              mThemes[mList->getCursorId()].screenshots.front().image);
    else
        mScreenshot->setImage("");

    if (mThemes[mList->getCursorId()].isCloned) {
        mDownloadStatus->setText(ViewController::TICKMARK_CHAR + " INSTALLED");
        mDownloadStatus->setColor(mMenuColorGreen);
        mDownloadStatus->setOpacity(1.0f);
    }
    else if (mThemes[mList->getCursorId()].invalidRepository ||
             mThemes[mList->getCursorId()].manuallyDownloaded) {
        mDownloadStatus->setText(ViewController::CROSSEDCIRCLE_CHAR + " MANUAL DOWNLOAD");
        mDownloadStatus->setColor(mMenuColorRed);
        mDownloadStatus->setOpacity(1.0f);
    }
    else if (mThemes[mList->getCursorId()].corruptRepository) {
        mDownloadStatus->setText(ViewController::CROSSEDCIRCLE_CHAR + " CORRUPT");
        mDownloadStatus->setColor(mMenuColorRed);
        mDownloadStatus->setOpacity(1.0f);
    }
    else if (mThemes[mList->getCursorId()].shallowRepository) {
        mDownloadStatus->setText(ViewController::CROSSEDCIRCLE_CHAR + " SHALLOW");
        mDownloadStatus->setColor(mMenuColorRed);
        mDownloadStatus->setOpacity(1.0f);
    }
    else {
        if (mThemes[mList->getCursorId()].newEntry)
            mDownloadStatus->setText("NOT INSTALLED (NEW)");
        else
            mDownloadStatus->setText("NOT INSTALLED");
        mDownloadStatus->setColor(mMenuColorPrimary);
        mDownloadStatus->setOpacity(0.7f);
    }
    if (mThemes[mList->getCursorId()].hasLocalChanges) {
        mLocalChanges->setText(ViewController::EXCLAMATION_CHAR + " LOCAL CHANGES");
        mLocalChanges->setColor(mMenuColorRed);
    }
    else {
        mLocalChanges->setText("");
    }

    mVariantCount->setText(std::to_string(mThemes[mList->getCursorId()].variants.size()));
    mColorSchemesCount->setText(std::to_string(mThemes[mList->getCursorId()].colorSchemes.size()));
    mAspectRatiosCount->setText(std::to_string(mThemes[mList->getCursorId()].aspectRatios.size()));
    mAuthor->setText("CREATED BY " + Utils::String::toUpper(mThemes[mList->getCursorId()].author));
}

void GuiThemeDownloader::setupFullscreenViewer()
{
    if (mThemes.empty())
        return;

    mViewerScreenshots.clear();
    mViewerCaptions.clear();
    mFullscreenViewerIndex = 0;
    mFullscreenViewing = true;

    for (auto& screenshot : mThemes[mList->getCursorId()].screenshots) {
        auto image = std::make_shared<ImageComponent>(false, false);
        image->setLinearInterpolation(true);
        image->setMaxSize(mRenderer->getScreenWidth() * 0.86f,
                          mRenderer->getScreenHeight() * 0.86f);
        if (!Utils::FileSystem::exists(mThemeDirectory + "themes-list/" + screenshot.image))
            continue;
        image->setImage(mThemeDirectory + "themes-list/" + screenshot.image);
        // Center image on screen.
        glm::vec3 imagePos {image->getPosition()};
        imagePos.x = (mRenderer->getScreenWidth() - image->getSize().x) / 2.0f;
        imagePos.y = (mRenderer->getScreenHeight() - image->getSize().y) / 2.0f;
        image->setPosition(imagePos);
        mViewerScreenshots.emplace_back(image);
        auto caption = std::make_shared<TextComponent>(screenshot.caption,
                                                       Font::get(FONT_SIZE_MINI, FONT_PATH_REGULAR),
                                                       0xCCCCCCFF, ALIGN_LEFT);
        glm::vec3 textPos {image->getPosition()};
        textPos.y += image->getSize().y;
        caption->setPosition(textPos);
        mViewerCaptions.emplace_back(caption);
    }

    if (mViewerScreenshots.size() > 0) {
        // Navigation indicators to the left and right of the screenshot.
        glm::vec3 indicatorPos {mViewerScreenshots.front()->getPosition()};
        indicatorPos.x -= mViewerIndicatorLeft->getSize().x * 2.0f;
        indicatorPos.y += (mViewerScreenshots.front()->getSize().y / 2.0f) -
                          (mViewerIndicatorLeft->getSize().y / 2.0f);
        mViewerIndicatorLeft->setPosition(indicatorPos);
        indicatorPos.x +=
            mViewerScreenshots.front()->getSize().x + (mViewerIndicatorRight->getSize().x * 3.0f);
        mViewerIndicatorRight->setPosition(indicatorPos);
    }
    else {
        mFullscreenViewing = false;
    }
}

void GuiThemeDownloader::update(int deltaTime)
{
    if (!mAttemptedFetch) {
        // We need to run this here instead of from the constructor so that GuiMsgBox will be
        // on top of the GUI stack if it needs to be displayed.
        mAttemptedFetch = true;
        fetchThemesList();
    }

    if (mFuture.valid()) {
        // Only wait one millisecond as this update() function runs very frequently.
        if (mFuture.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready) {
            if (mFetchThread.joinable()) {
                mFetchThread.join();
                mFetching = false;
                if (mRepositoryError != RepositoryError::NO_REPO_ERROR) {
                    std::string errorMessage {"ERROR: "};
                    if (mThemes.empty()) {
                        errorMessage.append("COULDN'T DOWNLOAD THEMES LIST, ");
                        mGrid.removeEntry(mCenterGrid);
                        mGrid.setCursorTo(mButtons);
                    }
                    errorMessage.append(Utils::String::toUpper(mMessage));
                    mWindow->pushGui(new GuiMsgBox(
                        getHelpStyle(), errorMessage, "OK", [] { return; }, "", nullptr, "",
                        nullptr, nullptr, true));
                    mMessage = "";
                    getHelpPrompts();
                }
                if (mThemes.empty() && mLatestThemesList) {
                    parseThemesList();
                    makeInventory();
                    populateGUI();
                }
                else if (!mThemes.empty()) {
                    makeInventory();
                    updateGUI();
                }
            }
        }
    }

    if (mFetching) {
        int progress {mReceivedObjectsProgress != 1.0f ? 0 : 100};
        if (mStatusType != StatusType::STATUS_NO_CHANGE) {
            if (mStatusType == StatusType::STATUS_DOWNLOADING)
                mBusyAnim.setText(mStatusText + " 100%");
            else if (mStatusType == StatusType::STATUS_UPDATING)
                mBusyAnim.setText(mStatusText);
            mBusyAnim.onSizeChanged();
            mStatusType = StatusType::STATUS_NO_CHANGE;
        }
        if (mReceivedObjectsProgress != 1.0f) {
            progress = static_cast<int>(
                std::round(glm::mix(0.0f, 100.0f, static_cast<float>(mReceivedObjectsProgress))));
            if (mStatusText.substr(0, 11) == "DOWNLOADING")
                mBusyAnim.setText(mStatusText + " " + std::to_string(progress) + "%");
            else
                mBusyAnim.setText(mStatusText);
        }
        else if (mReceivedObjectsProgress != 0.0f) {
            progress = static_cast<int>(
                std::round(glm::mix(0.0f, 100.0f, static_cast<float>(mResolveDeltaProgress))));
            if (mStatusText.substr(0, 11) == "DOWNLOADING")
                mBusyAnim.setText(mStatusText + " " + std::to_string(progress) + "%");
            else
                mBusyAnim.setText(mStatusText);
        }
        mBusyAnim.update(deltaTime);
    }

    if (mRepositoryError == RepositoryError::NO_REPO_ERROR && mMessage != "") {
        mWindow->queueInfoPopup(mMessage, 6000);
        mMessage = "";
    }

    GuiComponent::update(deltaTime);
}

void GuiThemeDownloader::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans {parentTrans * getTransform()};
    renderChildren(trans);

    if (mGrayRectangleCoords.size() == 4) {
        mRenderer->setMatrix(parentTrans * getTransform());
        mRenderer->drawRect(mGrayRectangleCoords[0], mGrayRectangleCoords[1],
                            mGrayRectangleCoords[2], mGrayRectangleCoords[3], mMenuColorPanelDimmed,
                            mMenuColorPanelDimmed);
    }

    if (mFetching)
        mBusyAnim.render(trans);

    if (mFullscreenViewing && mViewerScreenshots.size() > 0) {
        mRenderer->setMatrix(parentTrans);
        mRenderer->drawRect(0.0f, 0.0f, mRenderer->getScreenWidth(), mRenderer->getScreenHeight(),
                            0x222222FF, 0x222222FF);
        mViewerScreenshots[mFullscreenViewerIndex]->render(parentTrans);
        mViewerCaptions[mFullscreenViewerIndex]->render(parentTrans);
        if (mFullscreenViewerIndex != 0)
            mViewerIndicatorLeft->render(parentTrans);
        if (mFullscreenViewerIndex != mViewerCaptions.size() - 1)
            mViewerIndicatorRight->render(parentTrans);
    }
}

void GuiThemeDownloader::onSizeChanged()
{
    const float screenSize {mRenderer->getIsVerticalOrientation() ? mRenderer->getScreenWidth() :
                                                                    mRenderer->getScreenHeight()};
    mGrid.setRowHeightPerc(0, (mTitle->getFont()->getLetterHeight() + screenSize * 0.2f) / mSize.y /
                                  4.0f);
    mGrid.setRowHeightPerc(1, (mTitle->getFont()->getLetterHeight() + screenSize * 0.2f) / mSize.y /
                                  4.0f);
    mGrid.setRowHeightPerc(2, (mList->getRowHeight() * 9.0f) / mSize.y);

    mCenterGrid->setRowHeightPerc(
        0, (mVariantsLabel->getFont()->getLetterHeight() + screenSize * 0.115f) / mSize.y / 2.0f);
    mCenterGrid->setRowHeightPerc(
        1,
        (mColorSchemesLabel->getFont()->getLetterHeight() + screenSize * 0.09f) / mSize.y / 2.0f);
    mCenterGrid->setRowHeightPerc(
        2, (mDownloadStatus->getFont()->getLetterHeight() + screenSize * 0.115f) / mSize.y / 2.0f);
    mCenterGrid->setRowHeightPerc(3, 0.7f);

    mGrid.setColWidthPerc(1, 0.04f);
    mCenterGrid->setColWidthPerc(0, 0.01f);
    mCenterGrid->setColWidthPerc(1, (mRenderer->getScreenAspectRatio() < 1.6f ? 0.21f : 0.18f));
    mCenterGrid->setColWidthPerc(2, 0.05f);
    mCenterGrid->setColWidthPerc(3, 0.18f);
    mCenterGrid->setColWidthPerc(4, 0.04f);
    mCenterGrid->setColWidthPerc(5, 0.005f);
    mCenterGrid->setColWidthPerc(7, 0.04f);

    mGrid.setSize(mSize);

    mCenterGrid->setSize(
        glm::vec2 {std::round(mSize.x), (mList->getRowHeight() * 9.0f) +
                                            std::round(mRenderer->getScreenHeightModifier())});
    mCenterGrid->setPosition(glm::vec3 {0.0f, mGrid.getRowHeight(0) + mGrid.getRowHeight(1), 0.0f});
    mBackground.fitTo(mSize);
    mScreenshot->setMaxSize(mCenterGrid->getColWidth(1) + mCenterGrid->getColWidth(2) +
                                mCenterGrid->getColWidth(3) + mCenterGrid->getColWidth(4),
                            mCenterGrid->getRowHeight(3));

    mGrayRectangleCoords.clear();
    mGrayRectangleCoords.emplace_back(0.0f);
    mGrayRectangleCoords.emplace_back(mCenterGrid->getPosition().y);
    mGrayRectangleCoords.emplace_back(mSize.x);
    mGrayRectangleCoords.emplace_back(mList->getRowHeight() * 9.0f);
}

bool GuiThemeDownloader::input(InputConfig* config, Input input)
{
    if (mFetching && input.value)
        return false;

    if (mFullscreenViewing && input.value) {
        if (config->isMappedLike("left", input)) {
            if (mFullscreenViewerIndex > 0)
                --mFullscreenViewerIndex;
            return true;
        }
        else if (config->isMappedLike("right", input)) {
            if (mViewerScreenshots.size() > mFullscreenViewerIndex + 1)
                ++mFullscreenViewerIndex;
            return true;
        }
        else if (config->isMappedLike("lefttrigger", input)) {
            mFullscreenViewerIndex = 0;
            return true;
        }
        else if (config->isMappedLike("righttrigger", input)) {
            mFullscreenViewerIndex = mViewerScreenshots.size() - 1;
            return true;
        }
        else {
            mViewerScreenshots.clear();
            mViewerCaptions.clear();
            mFullscreenViewing = false;
            mFullscreenViewerIndex = 0;
            return true;
        }
    }

    if (config->isMappedTo("b", input) && input.value) {
        delete this;
        return true;
    }

    if (config->isMappedTo("x", input) && input.value &&
        mGrid.getSelectedComponent() == mCenterGrid) {
        setupFullscreenViewer();
        return true;
    }

    if (config->isMappedTo("y", input) && input.value &&
        mGrid.getSelectedComponent() == mCenterGrid && mThemes[mList->getCursorId()].isCloned) {
        mWindow->pushGui(new GuiMsgBox(
            getHelpStyle(),
            "THIS WILL COMPLETELY DELETE THE THEME INCLUDING ANY "
            "LOCAL CUSTOMIZATIONS",
            "PROCEED",
            [this] {
                const std::filesystem::path themeDirectory {mThemeDirectory +
                                                            mThemes[mList->getCursorId()].reponame};
                LOG(LogInfo) << "Deleting theme directory \"" << themeDirectory.string() << "\"";
                if (!Utils::FileSystem::removeDirectory(themeDirectory.string(), true)) {
                    mWindow->pushGui(new GuiMsgBox(
                        getHelpStyle(), "COULDN'T DELETE THEME, PERMISSION PROBLEMS?", "OK",
                        [] { return; }, "", nullptr, "", nullptr, nullptr, true));
                }
                else {
                    mMessage = "THEME WAS DELETED";
                }
                mHasThemeUpdates = true;
                makeInventory();
                updateGUI();
            },
            "CANCEL", nullptr, "", nullptr, nullptr, false, true,
            (mRenderer->getIsVerticalOrientation() ?
                 0.70f :
                 0.44f * (1.778f / mRenderer->getScreenAspectRatio()))));
        return true;
    }

    return GuiComponent::input(config, input);
}

std::vector<HelpPrompt> GuiThemeDownloader::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;

    if (mList->size() > 0) {
        prompts = mGrid.getHelpPrompts();
        prompts.push_back(HelpPrompt("b", "close"));

        if (mGrid.getSelectedComponent() == mCenterGrid)
            prompts.push_back(HelpPrompt("x", "view screenshots"));

        if (mThemes[mList->getCursorId()].isCloned) {
            prompts.push_back(HelpPrompt("a", "fetch updates"));
            if (mGrid.getSelectedComponent() == mCenterGrid)
                prompts.push_back(HelpPrompt("y", "delete"));
        }
        else {
            prompts.push_back(HelpPrompt("a", "download"));
        }
    }
    else {
        prompts.push_back(HelpPrompt("b", "close"));
    }

    return prompts;
}

bool GuiThemeDownloader::fetchThemesList()
{
    const std::string repositoryName {"themes-list"};
    const std::string url {"https://gitlab.com/es-de/themes/themes-list.git"};
    const std::string path {mThemeDirectory + "themes-list"};

    if (Utils::FileSystem::exists(path)) {
        git_repository* repository {nullptr};
        int errorCode {git_repository_open(&repository, &path[0])};

        if (errorCode != 0 || checkCorruptRepository(repository)) {
            mWindow->pushGui(new GuiMsgBox(
                getHelpStyle(),
                "IT SEEMS AS IF THE THEMES LIST REPOSITORY IS CORRUPT, WHICH COULD HAVE BEEN "
                "CAUSED BY AN INTERRUPTION OF A PREVIOUS DOWNLOAD OR UPDATE, FOR EXAMPLE IF THE "
                "ES-DE PROCESS WAS KILLED. A FRESH DOWNLOAD IS REQUIRED AND THE OLD DIRECTORY "
                "\"themes-list\" WILL BE RENAMED TO \"themes-list_CORRUPT_DISABLED\"",
                "PROCEED",
                [this, repositoryName, url] {
                    if (renameDirectory(mThemeDirectory + "themes-list", "_CORRUPT_DISABLED")) {
                        mGrid.removeEntry(mCenterGrid);
                        mGrid.setCursorTo(mButtons);
                        return true;
                    }
                    LOG(LogInfo)
                        << "GuiThemeDownloader: Creating initial themes list repository clone";
                    mFetchThread = std::thread(&GuiThemeDownloader::cloneRepository, this,
                                               repositoryName, url);
                    mStatusType = StatusType::STATUS_DOWNLOADING;
                    mStatusText = "DOWNLOADING THEMES LIST";
                    return false;
                },
                "CANCEL",
                [&] {
                    delete this;
                    return false;
                },
                "", nullptr, nullptr, true, true,
                (mRenderer->getIsVerticalOrientation() ?
                     0.75f :
                     0.50f * (1.778f / mRenderer->getScreenAspectRatio()))));
        }
        else {
            // We always hard reset the themes list as it should never contain any local changes.
            resetRepository(repository);

            mFetchThread =
                std::thread(&GuiThemeDownloader::fetchRepository, this, repositoryName, false);
            mStatusType = StatusType::STATUS_UPDATING;
            mStatusText = "UPDATING THEMES LIST";
        }
        git_repository_free(repository);
    }
    else {
        mWindow->pushGui(new GuiMsgBox(
            getHelpStyle(),
            "IT SEEMS AS IF YOU'RE USING THE THEME DOWNLOADER FOR THE FIRST TIME. "
            "AS SUCH THE THEMES LIST REPOSITORY WILL BE DOWNLOADED WHICH WILL TAKE A LITTLE "
            "WHILE. SUBSEQUENT RUNS WILL HOWEVER BE MUCH FASTER AS ONLY NEW OR MODIFIED FILES "
            "WILL BE FETCHED. THE SAME IS TRUE FOR ANY THEMES YOU DOWNLOAD. NOTE THAT YOU CAN'T "
            "ABORT AN ONGOING DOWNLOAD AS THAT COULD LEAD TO DATA CORRUPTION.",
            "PROCEED",
            [this, repositoryName, url] {
                LOG(LogInfo) << "GuiThemeDownloader: Creating initial themes list repository clone";
                mFetchThread =
                    std::thread(&GuiThemeDownloader::cloneRepository, this, repositoryName, url);
                mStatusType = StatusType::STATUS_DOWNLOADING;
                mStatusText = "DOWNLOADING THEMES LIST";
                return false;
            },
            "CANCEL",
            [&] {
                delete this;
                return false;
            },
            "", nullptr, nullptr, true, true,
            (mRenderer->getIsVerticalOrientation() ?
                 0.85f :
                 0.54f * (1.778f / mRenderer->getScreenAspectRatio()))));
    }

    return false;
}

bool GuiThemeDownloader::cloneRepository(const std::string& repositoryName, const std::string& url)
{
    int errorCode {0};
    git_repository* repository {nullptr};
    const std::string path {mThemeDirectory + repositoryName};

#if LIBGIT2_VER_MAJOR >= 1
    auto fetchProgressFunc = [](const git_indexer_progress* stats, void* payload) -> int {
#else
    auto fetchProgressFunc = [](const git_transfer_progress* stats, void* payload) -> int {
#endif
        (void)payload;
        if (stats->received_objects == stats->total_objects) {
#if (DEBUG_CLONING)
            LOG(LogDebug) << "Indexed deltas: " << stats->indexed_deltas
                          << " Total deltas: " << stats->total_deltas;
#endif
            mReceivedObjectsProgress = 1.0f;
            if (stats->total_deltas > 0) {
                mResolveDeltaProgress = static_cast<float>(stats->indexed_deltas) /
                                        static_cast<float>(stats->total_deltas);
            }
        }
        else if (stats->total_objects > 0) {
#if (DEBUG_CLONING)
            LOG(LogDebug) << "Received objects: " << stats->received_objects
                          << " Total objects: " << stats->total_objects
                          << " Indexed objects: " << stats->indexed_objects
                          << " Received bytes: " << stats->received_bytes;
#endif
            if (stats->total_objects > 0) {
                mReceivedObjectsProgress = static_cast<float>(stats->received_objects) /
                                           static_cast<float>(stats->total_objects);
            }
        }
        return 0;
    };

#if LIBGIT2_VER_MAJOR >= 1
    git_clone_options cloneOptions;
    git_clone_options_init(&cloneOptions, GIT_CLONE_OPTIONS_VERSION);
#else
    git_clone_options cloneOptions = GIT_CLONE_OPTIONS_INIT;
#endif

    cloneOptions.checkout_opts.checkout_strategy = GIT_CHECKOUT_FORCE;
    cloneOptions.fetch_opts.callbacks.transfer_progress = fetchProgressFunc;

    mReceivedObjectsProgress = 0.0f;
    mResolveDeltaProgress = 0.0f;

    mFetching = true;
    errorCode = git_clone(&repository, &url[0], &path[0], &cloneOptions);
    git_repository_free(repository);

    if (errorCode != 0) {
        const git_error* gitError {git_error_last()};
        LOG(LogError) << "GuiThemeDownloader: Couldn't clone repository \"" << repositoryName
                      << "\", error code: " << errorCode << ", error message: \""
                      << gitError->message << "\"";
        mRepositoryError = RepositoryError::CLONE_ERROR;
        mMessage = gitError->message;
        git_error_clear();
        mPromise.set_value(true);
        return true;
    }

    if (repositoryName != "themes-list") {
        LOG(LogInfo) << "GuiThemeDownloader: Downloaded theme \"" << repositoryName << "\"";
        mHasThemeUpdates = true;
    }

    mLatestThemesList = true;
    mPromise.set_value(true);
    return false;
}
