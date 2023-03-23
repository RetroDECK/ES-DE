//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiThemeDownloader.cpp
//
//  Theme downloader.
//

#include "guis/GuiThemeDownloader.h"

#include "EmulationStation.h"
#include "components/MenuComponent.h"
#include "resources/ResourceManager.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#define DEBUG_CLONING false

GuiThemeDownloader::GuiThemeDownloader()
    : mRenderer {Renderer::getInstance()}
    , mBackground {":/graphics/frame.svg"}
    , mGrid {glm::ivec2 {3, 3}}
    , mRepositoryError {RepositoryError::NO_ERROR}
    , mFetching {false}
    , mLatestThemesList {false}
{
    addChild(&mBackground);
    addChild(&mGrid);

    // Set up grid.
    mTitle = std::make_shared<TextComponent>("THEME DOWNLOADER", Font::get(FONT_SIZE_LARGE),
                                             0x555555FF, ALIGN_CENTER);
    mGrid.setEntry(mTitle, glm::ivec2 {1, 0}, false, true, glm::ivec2 {1, 1});

    mList = std::make_shared<ComponentList>();
    mGrid.setEntry(mList, glm::ivec2 {0, 1}, true, true, glm::ivec2 {3, 1});

    std::vector<std::shared_ptr<ButtonComponent>> buttons;
    buttons.push_back(std::make_shared<ButtonComponent>("Exit", "Exit", [&] { delete this; }));
    mButtons = makeButtonGrid(buttons);
    mGrid.setEntry(mButtons, glm::ivec2 {1, 2}, true, false, glm::ivec2 {1, 1});

    float width {mRenderer->getScreenWidth() * 0.85f};
    float height {mRenderer->getScreenHeight() * 0.90f};

    setSize(width, height);
    setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f,
                (mRenderer->getScreenHeight() - mSize.y) / 2.0f);

    mBusyAnim.setSize(mSize);
    mBusyAnim.setText("100%");
    mBusyAnim.onSizeChanged();

    git_libgit2_init();

    // The promise/future mechanism is used as signaling for the thread to indicate that
    // repository fetching has been completed.
    std::promise<bool>().swap(mPromise);
    mFuture = mPromise.get_future();

    std::string repositoryName {"themes-list"};
    std::string url {"https://gitlab.com/es-de/themes/themes-list.git"};

    mFetchThread = std::thread(&GuiThemeDownloader::fetchRepository, this,
                               std::make_pair(repositoryName, url), true);
}

GuiThemeDownloader::~GuiThemeDownloader()
{
    if (mFetchThread.joinable())
        mFetchThread.join();

    git_libgit2_shutdown();
}

bool GuiThemeDownloader::fetchRepository(std::pair<std::string, std::string> repoInfo,
                                         bool allowReset)
{
    int errorCode {0};
    mRepositoryName = repoInfo.first;
    mUrl = repoInfo.second;
    mPath = Utils::FileSystem::getHomePath() + "/.emulationstation/themes/" + mRepositoryName;
    mRepositoryError = RepositoryError::NO_ERROR;
    mErrorMessage = "";
    mManualPathSuffix = "";

    const bool isThemesList {mRepositoryName == "themes-list"};

    if (!isThemesList && (Utils::FileSystem::exists(mPath + "-main") ||
                          Utils::FileSystem::exists(mPath + "-master"))) {
        mRepositoryError = RepositoryError::MANUALLY_DOWNLOADED;
        if (Utils::FileSystem::exists(mPath + "-main"))
            mManualPathSuffix = "-main";
        else
            mManualPathSuffix = "-master";
        mPromise.set_value(true);
        return true;
    }

    git_repository* repository {nullptr};

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

    if (Utils::FileSystem::exists(mPath)) {
        errorCode = git_repository_open(&repository, &mPath[0]);
        if (errorCode != 0 && isThemesList) {
            if (renameDirectory(mPath)) {
                LOG(LogError) << "Couldn't rename \"" << mPath << "\", permission problems?";
                mWindow->pushGui(new GuiMsgBox(
                    getHelpStyle(),
                    "COULDN'T RENAME DIRECTORY\n" + mPath + "\nPERMISSION PROBLEMS?", "OK",
                    [] { return; }, "", nullptr, "", nullptr, true));
                return true;
            }
        }
    }

    if (!Utils::FileSystem::exists(mPath)) {
        // Repository does not exist, so clone it.
#if LIBGIT2_VER_MAJOR >= 1
        git_clone_options cloneOptions;
        git_clone_options_init(&cloneOptions, GIT_CLONE_OPTIONS_VERSION);
#else
        git_clone_options cloneOptions = GIT_CLONE_OPTIONS_INIT;
#endif

        cloneOptions.checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
        cloneOptions.fetch_opts.callbacks.transfer_progress = fetchProgressFunc;

        mReceivedObjectsProgress = 0.0f;
        mResolveDeltaProgress = 0.0f;

        mFetching = true;
        errorCode = git_clone(&repository, &mUrl[0], &mPath[0], &cloneOptions);
        git_repository_free(repository);

        if (errorCode != 0) {
            const git_error* gitError {git_error_last()};
            LOG(LogWarning) << "GuiThemeDownloader: Git returned error code " << errorCode
                            << ", error message: \"" << gitError->message << "\"";
            mErrorMessage = gitError->message;
            git_error_clear();
            mPromise.set_value(true);
            return true;
        }

        if (isThemesList)
            mLatestThemesList = true;
    }
    else {
        git_remote* gitRemote {nullptr};

        try {
            mFetching = true;
            errorCode = git_repository_open(&repository, &mPath[0]);

            if (errorCode != 0) {
                mRepositoryError = RepositoryError::NOT_A_REPOSITORY;
                throw std::runtime_error("Couldn't open local repository, ");
            }
            errorCode = git_remote_lookup(&gitRemote, repository, "origin");
            if (errorCode != 0) {
                mRepositoryError = RepositoryError::INVALID_ORIGIN;
                throw std::runtime_error("Couldn't get information about origin, ");
            }

            // int state {git_repository_state(repository)};
            // if (state != GIT_REPOSITORY_STATE_NONE) {
            //     //
            // }

#if LIBGIT2_VER_MAJOR >= 1
            git_fetch_options fetchOptions;
            git_fetch_options_init(&fetchOptions, GIT_FETCH_OPTIONS_VERSION);
#else
            git_fetch_options fetchOptions = GIT_FETCH_OPTIONS_INIT;
#endif
            errorCode = git_remote_fetch(gitRemote, nullptr, &fetchOptions, nullptr);

            if (errorCode != 0)
                throw std::runtime_error("Couldn't pull latest commits, ");

            git_annotated_commit* annotated {nullptr};
            git_object* object {nullptr};

            if (git_repository_head_detached(repository)) {
                LOG(LogWarning) << "Repository \"" << mRepositoryName
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
                    checkoutOptions.checkout_strategy = GIT_CHECKOUT_SAFE;
                    errorCode = git_checkout_tree(repository, object, &checkoutOptions);
                    errorCode = git_repository_set_head(repository, branchName.c_str());

                    git_reference_free(oldTargetRef);
                }
                git_buf_dispose(&buffer);
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
                throw std::runtime_error("Couldn't run Git merge analysis, ");
            }

            if (mergeAnalysis & GIT_MERGE_ANALYSIS_UP_TO_DATE) {
                LOG(LogInfo) << "Repository \"" << mRepositoryName << "\" already up to date";
                git_annotated_commit_free(annotated);
                git_object_free(object);
                mPromise.set_value(true);
                if (isThemesList)
                    mLatestThemesList = true;
                return false;
            }

            if (!(mergeAnalysis & GIT_MERGE_ANALYSIS_FASTFORWARD)) {
                if (allowReset) {
                    LOG(LogWarning) << "Repository \"" << mRepositoryName
                                    << "\" has diverged from origin, performing hard reset";
                    git_object* objectHead {nullptr};
                    errorCode = git_revparse_single(&objectHead, repository, "HEAD");
                    errorCode = git_reset(repository, objectHead, GIT_RESET_HARD, nullptr);
                    git_object_free(objectHead);
                }
                else {
                    LOG(LogWarning) << "Repository \"" << mRepositoryName
                                    << "\" has diverged from origin, can't fast-forward";
                    git_annotated_commit_free(annotated);
                    git_object_free(object);
                    mPromise.set_value(true);
                    mRepositoryError = RepositoryError::HAS_DIVERGED;
                    return true;
                }
            }

            git_status_list* status {nullptr};
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
            const size_t statusEntryCount {git_status_list_entrycount(status)};

            if (statusEntryCount != 0) {
                if (allowReset) {
                    LOG(LogWarning) << "Repository \"" << mRepositoryName
                                    << "\" contains local changes, performing hard reset";
                    git_object* objectHead {nullptr};
                    errorCode = git_revparse_single(&objectHead, repository, "HEAD");
                    errorCode = git_reset(repository, objectHead, GIT_RESET_HARD, nullptr);
                    git_object_free(objectHead);
                }
                else {
                    LOG(LogWarning) << "Repository \"" << mRepositoryName
                                    << "\" contains local changes, can't fast-forward";
                    git_status_list_free(status);
                    git_annotated_commit_free(annotated);
                    git_object_free(object);
                    mPromise.set_value(true);
                    mRepositoryError = RepositoryError::HAS_LOCAL_CHANGES;
                    return true;
                }
            }

            git_status_list_free(status);

            LOG(LogInfo) << "Performing Git fast-forward of repository \"" << mRepositoryName
                         << "\"";

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
            checkoutOptions.checkout_strategy = GIT_CHECKOUT_SAFE;

            git_checkout_tree(repository, object, &checkoutOptions);
            errorCode = git_reference_set_target(&newTargetRef, oldTargetRef, objectID, nullptr);

            git_reference_free(oldTargetRef);
            git_reference_free(newTargetRef);
            git_annotated_commit_free(annotated);
            git_object_free(object);

            if (errorCode != 0)
                throw std::runtime_error("Couldn't fast-forward repository, ");

            if (isThemesList)
                mLatestThemesList = true;

            if (gitRemote != nullptr)
                git_remote_disconnect(gitRemote);
        }
        catch (std::runtime_error& runtimeError) {
            const git_error* gitError {git_error_last()};
            LOG(LogError) << "GuiThemeDownloader: " << runtimeError.what() << gitError->message;
            mErrorMessage = gitError->message;
            git_error_clear();
            if (gitRemote != nullptr)
                git_remote_disconnect(gitRemote);
            mPromise.set_value(true);
            return true;
        }
    }

    mPromise.set_value(true);
    mRepositoryName = "";
    mUrl = "";
    mPath = "";

    return false;
}

bool GuiThemeDownloader::renameDirectory(const std::string& path)
{
    LOG(LogInfo) << "Renaming directory " << path;
    int index {1};

    if (!Utils::FileSystem::exists(path + "_DISABLED"))
        return Utils::FileSystem::renameFile(path, path + "_DISABLED", false);

    // This will hopefully never be needed as it should only occur if a theme has been downloaded
    // manually multiple times and the theme downloader has been ran multiple times as well.
    for (; index < 10; ++index) {
        if (!Utils::FileSystem::exists(path + "_" + std::to_string(index) + "_DISABLED"))
            return Utils::FileSystem::renameFile(
                path, path + "_" + std::to_string(index) + "_DISABLED", false);
    }

    return true;
}

void GuiThemeDownloader::parseThemesList()
{
    // Temporary location for testing purposes.
    //    const std::string themesFile {Utils::FileSystem::getHomePath() +
    //                                  "/.emulationstation/themes.json"};

    const std::string themesFile {Utils::FileSystem::getHomePath() +
                                  "/.emulationstation/themes/themes-list/themes.json"};

    if (!Utils::FileSystem::exists(themesFile)) {
        LOG(LogInfo) << "GuiThemeDownloader: No themes.json file found";
        return;
    }

    const ResourceData& themesFileData {ResourceManager::getInstance().getFileData(themesFile)};
    rapidjson::Document doc;
    doc.Parse(reinterpret_cast<const char*>(themesFileData.ptr.get()), themesFileData.length);

    if (doc.HasParseError()) {
        LOG(LogWarning) << "GuiThemeDownloader: Couldn't parse the themes.json file";
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
                "OK", [] { return; }, "", nullptr, "", nullptr, true));
        }
    }

    if (doc.HasMember("themeSets") && doc["themeSets"].IsArray()) {
        const rapidjson::Value& themeSets {doc["themeSets"]};
        for (int i {0}; i < static_cast<int>(themeSets.Size()); ++i) {
            ThemeEntry themeEntry;
            const rapidjson::Value& theme {themeSets[i]};

            if (theme.HasMember("name") && theme["name"].IsString())
                themeEntry.name = theme["name"].GetString();

            if (theme.HasMember("reponame") && theme["reponame"].IsString())
                themeEntry.reponame = theme["reponame"].GetString();

            if (theme.HasMember("url") && theme["url"].IsString())
                themeEntry.url = theme["url"].GetString();

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

            mThemeSets.emplace_back(themeEntry);
        }
    }

    mWindow->queueInfoPopup("PARSED " + std::to_string(mThemeSets.size()) + " THEME SETS", 6000);
    LOG(LogInfo) << "GuiThemeDownloader: Parsed " << mThemeSets.size() << " theme sets";

    populateGUI();
}

void GuiThemeDownloader::populateGUI()
{
    for (auto& theme : mThemeSets) {
        ComponentListRow row;
        std::shared_ptr<TextComponent> themeName {std::make_shared<TextComponent>(
            Utils::String::toUpper(theme.name), Font::get(FONT_SIZE_MEDIUM), 0x777777FF)};

        row.addElement(themeName, false);

        row.makeAcceptInputHandler([this, theme] {
            std::promise<bool>().swap(mPromise);
            mFuture = mPromise.get_future();
            mFetchThread = std::thread(&GuiThemeDownloader::fetchRepository, this,
                                       std::make_pair(theme.reponame, theme.url), true);
        });
        mList->addRow(row);
    }
    updateHelpPrompts();
}

void GuiThemeDownloader::update(int deltaTime)
{
    if (mFuture.valid()) {
        // Only wait one millisecond as this update() function runs very frequently.
        if (mFuture.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready) {
            if (mFetchThread.joinable()) {
                mFetchThread.join();
                mFetching = false;
                if (mRepositoryError == RepositoryError::HAS_LOCAL_CHANGES) {
                    LOG(LogError) << "Repository has local changes";
                }
                if (mRepositoryError != RepositoryError::NO_ERROR) {
                    if (mRepositoryError == RepositoryError::NOT_A_REPOSITORY ||
                        mRepositoryError == RepositoryError::MANUALLY_DOWNLOADED) {
                        mWindow->pushGui(new GuiMsgBox(
                            getHelpStyle(),
                            "IT SEEMS AS IF THIS THEME HAS BEEN MANUALLY DOWNLOADED INSTEAD OF VIA "
                            "THIS THEME DOWNLOADER. A FRESH DOWNLOAD IS REQUIRED AND THE OLD THEME "
                            "DIRECTORY \"" +
                                mRepositoryName + mManualPathSuffix + "\" WILL BE RENAMED TO \"" +
                                mRepositoryName + mManualPathSuffix + "_DISABLED\"",
                            "PROCEED",
                            [this] {
                                renameDirectory(mPath + mManualPathSuffix);
                                std::promise<bool>().swap(mPromise);
                                mFuture = mPromise.get_future();
                                mFetchThread =
                                    std::thread(&GuiThemeDownloader::fetchRepository, this,
                                                std::make_pair(mRepositoryName, mUrl), false);
                            },
                            "ABORT", [] { return; }, "", nullptr, true, true,
                            (mRenderer->getIsVerticalOrientation() ?
                                 0.75f :
                                 0.45f * (1.778f / mRenderer->getScreenAspectRatio()))));
                    }
                    else {
                        std::string errorMessage {"ERROR: "};
                        errorMessage.append(Utils::String::toUpper(mErrorMessage));
                        mWindow->queueInfoPopup(errorMessage, 6000);
                        LOG(LogError) << "Error: " << mErrorMessage;
                    }
                }
                if (mThemeSets.empty() && mLatestThemesList)
                    parseThemesList();
            }
        }
    }

    if (mFetching) {
        int progress {mReceivedObjectsProgress != 1.0f ? 0 : 100};
        if (mReceivedObjectsProgress != 1.0f) {
            progress = static_cast<int>(
                std::round(glm::mix(0.0f, 100.0f, static_cast<float>(mReceivedObjectsProgress))));
            const std::string progressText {std::to_string(progress) + "%"};
            mBusyAnim.setText(progressText);
        }
        else if (mReceivedObjectsProgress != 0.0f) {
            progress = static_cast<int>(
                std::round(glm::mix(0.0f, 100.0f, static_cast<float>(mResolveDeltaProgress))));
            const std::string progressText {std::to_string(progress) + "%"};
            mBusyAnim.setText(progressText);
        }
        mBusyAnim.update(deltaTime);
    }
}

void GuiThemeDownloader::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans {parentTrans * getTransform()};

    renderChildren(trans);
    if (mFetching)
        mBusyAnim.render(trans);
}

void GuiThemeDownloader::onSizeChanged()
{
    const float screenSize {mRenderer->getIsVerticalOrientation() ? mRenderer->getScreenWidth() :
                                                                    mRenderer->getScreenHeight()};
    mGrid.setRowHeightPerc(0, (mTitle->getFont()->getLetterHeight() + screenSize * 0.2f) / mSize.y /
                                  2.0f);
    mGrid.setRowHeightPerc(1, 0.7f);
    // mGrid.setRowHeightPerc(1, ((mList->getRowHeight(0) * 5.0f)) / mSize.y);

    mGrid.setColWidthPerc(0, 0.04f);
    mGrid.setColWidthPerc(2, 0.04f);

    mGrid.setSize(mSize);
    mBackground.fitTo(mSize, glm::vec3 {0.0f, 0.0f, 0.0f}, glm::vec2 {-32.0f, -32.0f});
}

bool GuiThemeDownloader::input(InputConfig* config, Input input)
{
    if (config->isMappedTo("b", input) && input.value) {
        delete this;
        return true;
    }

    return mGrid.input(config, input);
}

std::vector<HelpPrompt> GuiThemeDownloader::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts {mGrid.getHelpPrompts()};
    prompts.push_back(HelpPrompt("b", "exit"));

    return prompts;
}
