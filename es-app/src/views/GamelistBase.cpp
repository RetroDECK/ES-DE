//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GamelistBase.cpp
//
//  Gamelist base class with utility functions and other low-level logic.
//

#if defined(_WIN64)
// Why this is needed here is anyone's guess but without it the compilation fails.
#include <winsock2.h>
#endif

#include "views/GamelistBase.h"

#include "CollectionSystemsManager.h"
#include "FileFilterIndex.h"
#include "UIModeController.h"
#include "guis/GuiGamelistOptions.h"
#include "utils/LocalizationUtil.h"
#include "views/ViewController.h"

GamelistBase::GamelistBase(FileData* root)
    : mRoot {root}
    , mPrimary {nullptr}
    , mRandomGame {nullptr}
    , mLastUpdated {nullptr}
    , mGameCount {0}
    , mFavoritesGameCount {0}
    , mFilteredGameCount {0}
    , mFilteredGameCountAll {0}
    , mIsFiltered {false}
    , mIsFolder {false}
    , mVideoPlaying {false}
    , mLeftRightAvailable {true}
    , mSystemNameSuffix {false}
{
    setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
}

void GamelistBase::setCursor(FileData* cursor)
{
    if (!mPrimary->setCursor(cursor) && (!cursor->isPlaceHolder())) {
        populateList(cursor->getParent()->getChildrenListToDisplay(), cursor->getParent());
        mPrimary->setCursor(cursor);

        // Update our cursor stack in case our cursor just got set to some folder
        // we weren't in before.
        if (mCursorStack.empty() || mCursorStack.top() != cursor->getParent()) {
            std::stack<FileData*> tmp;
            FileData* ptr {cursor->getParent()};

            while (ptr && ptr != mRoot) {
                tmp.push(ptr);
                ptr = ptr->getParent();
            }

            // Flip the stack and put it in mCursorStack.
            mCursorStack = std::stack<FileData*>();
            while (!tmp.empty()) {
                mCursorStack.push(tmp.top());
                tmp.pop();
            }
        }
    }
}

bool GamelistBase::input(InputConfig* config, Input input)
{
    if (input.value != 0) {
        if (config->isMappedTo("a", input)) {
            FileData* cursor {getCursor()};
            if (cursor->getType() == GAME) {
                pauseViewVideos();
                ViewController::getInstance()->cancelViewTransitions();
                stopListScrolling();
                stopGamelistFadeAnimations();
                launch(cursor);
            }
            else {
                // It's a folder.
                if (cursor->getChildren().size() > 0) {
                    ViewController::getInstance()->cancelViewTransitions();
                    // If a folder link entry has been set on the folder, then check if it
                    // corresponds to an actual child entry, and if so then launch this child
                    // instead of entering the folder.
                    if (!CollectionSystemsManager::getInstance()->isEditing() &&
                        cursor->metadata.get("folderlink") != "") {
                        std::string folderLink;
                        folderLink.append(cursor->getPath())
                            .append("/")
                            .append(Utils::String::replace(cursor->metadata.get("folderlink"), "\\",
                                                           "/"));
                        for (auto child : cursor->getChildrenRecursive()) {
                            if (child->getPath() == folderLink) {
                                pauseViewVideos();
                                ViewController::getInstance()->cancelViewTransitions();
                                stopListScrolling();
                                stopGamelistFadeAnimations();
                                launch(child);
                                return true;
                            }
                        }
                    }

                    NavigationSounds::getInstance().playThemeNavigationSound(SELECTSOUND);
                    mCursorStack.push(cursor);
                    populateList(cursor->getChildrenListToDisplay(), cursor);

                    FileData* newCursor {nullptr};
                    std::vector<FileData*> listEntries {cursor->getChildrenListToDisplay()};
                    // Check if there is an entry in the cursor stack history matching any entry
                    // in the currect folder. If so, select that entry.
                    for (auto it = mCursorStackHistory.begin(); // Line break.
                         it != mCursorStackHistory.end(); ++it) {
                        if (std::find(listEntries.begin(), listEntries.end(), *it) !=
                            listEntries.end()) {
                            newCursor = *it;
                            mCursorStackHistory.erase(it);
                            break;
                        }
                    }

                    // If there was no match in the cursor history, simply select the first entry.
                    if (!newCursor)
                        newCursor = getCursor();
                    setCursor(newCursor);
                    stopListScrolling();
                    if (mRoot->getSystem()->getThemeFolder() == "custom-collections")
                        updateHelpPrompts();
                }
                else {
                    NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
                }
            }

            return true;
        }
        else if (config->isMappedTo("b", input)) {
            ViewController::getInstance()->cancelViewTransitions();
            if (mCursorStack.size()) {
                // Save the position to the cursor stack history.
                mCursorStackHistory.push_back(getCursor());
                NavigationSounds::getInstance().playThemeNavigationSound(BACKSOUND);
                populateList(mCursorStack.top()->getParent()->getChildrenListToDisplay(),
                             mCursorStack.top()->getParent());
                setCursor(mCursorStack.top());
                stopListScrolling();
                stopGamelistFadeAnimations();
                if (mCursorStack.size() > 0)
                    mCursorStack.pop();
                if (mRoot->getSystem()->getThemeFolder() == "custom-collections")
                    updateHelpPrompts();
            }
            else {
                NavigationSounds::getInstance().playThemeNavigationSound(BACKSOUND);
                muteViewVideos();
                onFocusLost();
                stopListScrolling();
                stopGamelistFadeAnimations();
                SystemData* systemToView = getCursor()->getSystem();
                if (systemToView->isCustomCollection() &&
                    systemToView->getRootFolder()->getParent())
                    ViewController::getInstance()->goToSystemView(
                        systemToView->getRootFolder()->getParent()->getSystem(), true);
                else
                    ViewController::getInstance()->goToSystemView(systemToView, true);
            }

            return true;
        }
        else if (config->isMappedTo("x", input)) {
            if (getCursor()->getType() == PLACEHOLDER) {
                NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
                return true;
            }
            else if (config->isMappedTo("x", input) &&
                     mRoot->getSystem()->getThemeFolder() == "custom-collections" &&
                     mCursorStack.empty() &&
                     ViewController::getInstance()->getState().viewing ==
                         ViewController::ViewMode::GAMELIST) {
                NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
                // Jump to the randomly selected game.
                if (mRandomGame) {
                    stopListScrolling();
                    ViewController::getInstance()->cancelViewTransitions();
                    mWindow->startMediaViewer(mRandomGame);
                    return true;
                }
            }
            else if (mRoot->getSystem()->isGameSystem()) {
                stopListScrolling();
                stopGamelistFadeAnimations();
                ViewController::getInstance()->cancelViewTransitions();
                NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
                mWindow->startMediaViewer(getCursor());
                return true;
            }
        }
        else if (config->isMappedLike(getQuickSystemSelectRightButton(), input)) {
            if (SystemData::sSystemVector.size() > 1) {
                muteViewVideos();
                onFocusLost();
                stopListScrolling();
                stopGamelistFadeAnimations();
                ViewController::getInstance()->goToNextGamelist();
                return true;
            }
        }
        else if (config->isMappedLike(getQuickSystemSelectLeftButton(), input)) {
            if (SystemData::sSystemVector.size() > 1) {
                muteViewVideos();
                onFocusLost();
                stopListScrolling();
                stopGamelistFadeAnimations();
                ViewController::getInstance()->goToPrevGamelist();
                return true;
            }
        }
        else if ((Settings::getInstance()->getString("RandomEntryButton") == "games" ||
                  Settings::getInstance()->getString("RandomEntryButton") == "gamessystems") &&
                 (config->isMappedTo("leftthumbstickclick", input) ||
                  config->isMappedTo("rightthumbstickclick", input))) {
            if (mRoot->getSystem()->isGameSystem() && getCursor()->getType() != PLACEHOLDER) {
                stopListScrolling();
                // Jump to a random game.
                NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
                FileData* randomGame {getCursor()->getSystem()->getRandomGame(getCursor())};
                if (randomGame)
                    setCursor(randomGame);
                return true;
            }
        }
        else if (config->isMappedTo("y", input) &&
                 mRoot->getSystem()->getThemeFolder() == "custom-collections" &&
                 !CollectionSystemsManager::getInstance()->isEditing() && mCursorStack.empty() &&
                 ViewController::getInstance()->getState().viewing ==
                     ViewController::ViewMode::GAMELIST) {
            // Jump to the randomly selected game.
            if (mRandomGame) {
                NavigationSounds::getInstance().playThemeNavigationSound(SELECTSOUND);
                // If there is already an mCursorStackHistory entry for the collection, then
                // remove it so we don't get multiple entries.
                std::vector<FileData*> listEntries {
                    mRandomGame->getSystem()->getRootFolder()->getChildrenListToDisplay()};
                for (auto it = mCursorStackHistory.begin(); it != mCursorStackHistory.end(); ++it) {
                    if (std::find(listEntries.begin(), listEntries.end(), *it) !=
                        listEntries.end()) {
                        mCursorStackHistory.erase(it);
                        break;
                    }
                }
                setCursor(mRandomGame);
                stopListScrolling();
                updateHelpPrompts();
            }
            else {
                NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
            }
        }
        else if (config->isMappedTo("y", input) &&
                 !Settings::getInstance()->getBool("FavoritesAddButton") &&
                 !CollectionSystemsManager::getInstance()->isEditing()) {
            return true;
        }
        else if (config->isMappedTo("y", input) &&
                 !UIModeController::getInstance()->isUIModeKid() &&
                 !UIModeController::getInstance()->isUIModeKiosk()) {
            // Notify the user if attempting to add a custom collection to a custom collection.
            if (CollectionSystemsManager::getInstance()->isEditing() &&
                mRoot->getSystem()->isGameSystem() && getCursor()->getType() != PLACEHOLDER &&
                getCursor()->getParent()->getPath() == "collections") {
                NavigationSounds::getInstance().playThemeNavigationSound(FAVORITESOUND);
                mWindow->queueInfoPopup(_("CAN'T ADD CUSTOM COLLECTIONS TO CUSTOM COLLECTIONS"),
                                        4000);
            }
            // Notify the user if attempting to add a placeholder to a custom collection.
            if (CollectionSystemsManager::getInstance()->isEditing() &&
                mRoot->getSystem()->isGameSystem() && getCursor()->getType() == PLACEHOLDER) {
                NavigationSounds::getInstance().playThemeNavigationSound(FAVORITESOUND);
                mWindow->queueInfoPopup(_("CAN'T ADD PLACEHOLDERS TO CUSTOM COLLECTIONS"), 4000);
            }
            else if (mRoot->getSystem()->isGameSystem() && getCursor()->getType() != PLACEHOLDER &&
                     getCursor()->getParent()->getPath() != "collections") {
                if (getCursor()->getType() == GAME || getCursor()->getType() == FOLDER)
                    NavigationSounds::getInstance().playThemeNavigationSound(FAVORITESOUND);
                // When marking or unmarking a game as favorite, don't jump to the new position
                // it gets after the gamelist sorting. Instead retain the cursor position in the
                // list using the logic below.
                FileData* entryToUpdate {getCursor()};
                SystemData* system {getCursor()->getSystem()};
                bool favoritesSorting;
                bool removedLastFavorite {false};
                bool selectLastEntry {false};
                bool isEditing {CollectionSystemsManager::getInstance()->isEditing()};
                bool foldersOnTop {Settings::getInstance()->getBool("FoldersOnTop")};
                // If the current list only contains folders, then treat it as if the folders
                // are not sorted on top, this way the logic should work exactly as for mixed
                // lists or files-only lists.
                if (getCursor()->getType() == FOLDER && foldersOnTop == true)
                    foldersOnTop = !getCursor()->getParent()->getOnlyFoldersFlag();

                if (mRoot->getSystem()->isCustomCollection() ||
                    mRoot->getSystem()->getThemeFolder() == "custom-collections")
                    favoritesSorting = Settings::getInstance()->getBool("FavFirstCustom");
                else
                    favoritesSorting = Settings::getInstance()->getBool("FavoritesFirst");

                if (favoritesSorting && mRoot->getSystem()->getName() != "recent" && !isEditing) {
                    FileData* entryToSelect;
                    // Add favorite flag.
                    if (!getCursor()->getFavorite()) {
                        // If it's a folder and folders are sorted on top, select the current entry.
                        if (foldersOnTop && getCursor()->getType() == FOLDER) {
                            entryToSelect = getCursor();
                        }
                        // If it's the first entry to be marked as favorite, select the next entry.
                        else if (getCursor() == getFirstEntry()) {
                            entryToSelect = getNextEntry();
                        }
                        else if (getCursor() == getLastEntry() &&
                                 getPreviousEntry()->getFavorite()) {
                            entryToSelect = getLastEntry();
                            selectLastEntry = true;
                        }
                        // If we are on the favorite marking boundary, select the next entry.
                        else if (getCursor()->getFavorite() != getPreviousEntry()->getFavorite()) {
                            entryToSelect = getNextEntry();
                        }
                        // If we mark the second entry as favorite and the first entry is not a
                        // favorite, then select this entry if they are of the same type.
                        else if (getPreviousEntry() == getFirstEntry() &&
                                 getCursor()->getType() == getPreviousEntry()->getType()) {
                            entryToSelect = getPreviousEntry();
                        }
                        // For all other scenarios try to select the next entry, and if it doesn't
                        // exist, select the previous entry.
                        else {
                            entryToSelect =
                                getCursor() != getNextEntry() ? getNextEntry() : getPreviousEntry();
                        }
                    }
                    // Remove favorite flag.
                    else {
                        // If it's a folder and folders are sorted on top, select the current entry.
                        if (foldersOnTop && getCursor()->getType() == FOLDER) {
                            entryToSelect = getCursor();
                        }
                        // If it's the last entry, select the previous entry.
                        else if (getCursor() == getLastEntry()) {
                            entryToSelect = getPreviousEntry();
                        }
                        // If we are on the favorite marking boundary, select the previous entry,
                        // unless folders are sorted on top and the previous entry is a folder.
                        else if (foldersOnTop &&
                                 getCursor()->getFavorite() != getNextEntry()->getFavorite()) {
                            entryToSelect = getPreviousEntry()->getType() == FOLDER ?
                                                getCursor() :
                                                getPreviousEntry();
                        }
                        // If we are on the favorite marking boundary, select the previous entry.
                        else if (getCursor()->getFavorite() != getNextEntry()->getFavorite()) {
                            entryToSelect = getPreviousEntry();
                        }
                        // For all other scenarios try to select the next entry, and if it doesn't
                        // exist, select the previous entry.
                        else {
                            entryToSelect =
                                getCursor() != getNextEntry() ? getNextEntry() : getPreviousEntry();
                        }

                        // If we removed the last favorite marking, set the flag to jump to the
                        // first list entry after the sorting has been performed.
                        if (foldersOnTop && getCursor() == getFirstGameEntry() &&
                            !getNextEntry()->getFavorite())
                            removedLastFavorite = true;
                        else if (getCursor() == getFirstEntry() && !getNextEntry()->getFavorite())
                            removedLastFavorite = true;
                    }

                    setCursor(entryToSelect);
                    system = entryToUpdate->getSystem();
                }

                // Marking folders as favorites don't make them part of any collections,
                // so it makes more sense to handle it here than to add the function to
                // CollectionSystemsManager.
                if (entryToUpdate->getType() == FOLDER) {
                    if (isEditing) {
                        mWindow->queueInfoPopup(_("CAN'T ADD FOLDERS TO CUSTOM COLLECTIONS"), 4000);
                    }
                    else {
                        MetaDataList* md {&entryToUpdate->getSourceFileData()->metadata};
                        if (md->get("favorite") == "false") {
                            md->set("favorite", "true");
                            mWindow->queueInfoPopup(
                                Utils::String::format(
                                    _("MARKED FOLDER '%s' AS FAVORITE"),
                                    Utils::String::toUpper(
                                        Utils::String::removeParenthesis(entryToUpdate->getName()))
                                        .c_str()),
                                4000);
                        }
                        else {
                            md->set("favorite", "false");
                            mWindow->queueInfoPopup(
                                Utils::String::format(
                                    _("REMOVED FAVORITE MARKING FOR FOLDER '%s'"),
                                    Utils::String::toUpper(
                                        Utils::String::removeParenthesis(entryToUpdate->getName()))
                                        .c_str()),
                                4000);
                        }
                    }

                    entryToUpdate->getSourceFileData()->getSystem()->onMetaDataSavePoint();

                    getCursor()->getParent()->sort(
                        mRoot->getSortTypeFromString(mRoot->getSortTypeString()),
                        Settings::getInstance()->getBool("FavoritesFirst"));

                    ViewController::getInstance()->onFileChanged(getCursor(), false);

                    // Always jump to the first entry in the gamelist if the last favorite
                    // was unmarked. We couldn't do this earlier as we didn't have the list
                    // sorted yet.
                    if (removedLastFavorite) {
                        ViewController::getInstance()
                            ->getGamelistView(entryToUpdate->getSystem())
                            ->setCursor(ViewController::getInstance()
                                            ->getGamelistView(entryToUpdate->getSystem())
                                            ->getFirstEntry());
                    }
                    return true;
                }
                else if (isEditing && entryToUpdate->metadata.get("nogamecount") == "true") {
                    mWindow->queueInfoPopup(_("CAN'T ADD ENTRIES THAT ARE NOT COUNTED "
                                              "AS GAMES TO CUSTOM COLLECTIONS"),
                                            4000);
                }
                else if (CollectionSystemsManager::getInstance()->toggleGameInCollection(
                             entryToUpdate)) {
                    // Needed to avoid some minor transition animation glitches.
                    auto grid =
                        ViewController::getInstance()->getGamelistView(system).get()->mGrid.get();
                    if (grid != nullptr)
                        grid->setSuppressTransitions(true);

                    // As the toggling of the game destroyed this object, we need to get the view
                    // from ViewController instead of using the reference that existed before the
                    // destruction. Otherwise we get random crashes.
                    GamelistView* view {
                        ViewController::getInstance()->getGamelistView(system).get()};

                    // Jump to the first entry in the gamelist if the last favorite was unmarked.
                    if (foldersOnTop && removedLastFavorite &&
                        !entryToUpdate->getSystem()->isCustomCollection()) {
                        ViewController::getInstance()
                            ->getGamelistView(entryToUpdate->getSystem())
                            ->setCursor(ViewController::getInstance()
                                            ->getGamelistView(entryToUpdate->getSystem())
                                            ->getFirstGameEntry());
                    }
                    else if (removedLastFavorite &&
                             !entryToUpdate->getSystem()->isCustomCollection()) {
                        setCursor(getFirstEntry());
                        view->setCursor(view->getFirstEntry());
                    }
                    else if (selectLastEntry && view->getPrimary()->size() > 0) {
                        view->setCursor(view->getLastEntry());
                    }

                    if (grid != nullptr)
                        grid->setSuppressTransitions(false);

                    // Display the indication icons which show what games are part of the
                    // custom collection currently being edited. This is done cheaply using
                    // onFileChanged() which will trigger populateList().
                    if (isEditing) {
                        for (auto it = SystemData::sSystemVector.begin();
                             it != SystemData::sSystemVector.end(); ++it) {
                            ViewController::getInstance()->getGamelistView((*it))->onFileChanged(
                                ViewController::getInstance()->getGamelistView((*it))->getCursor(),
                                false);
                        }
                    }
                    return true;
                }
            }
            else if (config->isMappedTo("y", input) && getCursor()->isPlaceHolder()) {
                NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
            }
        }
    }

    // Select button opens GuiGamelistOptions.
    if (!UIModeController::getInstance()->isUIModeKid() && // Line break.
        config->isMappedTo("back", input) && input.value) {
        ViewController::getInstance()->cancelViewTransitions();
        stopListScrolling();
        pauseViewVideos();
        stopGamelistFadeAnimations();
        mWindow->setAllowTextScrolling(false);
        mWindow->setAllowFileAnimation(false);
        mWindow->pushGui(new GuiGamelistOptions(this->mRoot->getSystem()));
        return true;
    }

    // Ctrl-R reloads the view when debugging.
    else if (Settings::getInstance()->getBool("Debug") &&
             config->getDeviceId() == DEVICE_KEYBOARD &&
             (SDL_GetModState() & (KMOD_LCTRL | KMOD_RCTRL)) && input.id == SDLK_r &&
             input.value != 0) {
        LOG(LogDebug) << "GamelistView::input(): Reloading view";
        ViewController::getInstance()->reloadGamelistView(this->mRoot->getSystem(), true);
        return true;
    }

    return GuiComponent::input(config, input);
}

void GamelistBase::enterDirectory(FileData* cursor)
{
    assert(cursor->getType() == FOLDER);

    if (cursor->getChildren().size() > 0) {
        ViewController::getInstance()->cancelViewTransitions();
        NavigationSounds::getInstance().playThemeNavigationSound(SELECTSOUND);
        mCursorStack.push(cursor);
        populateList(cursor->getChildrenListToDisplay(), cursor);

        FileData* newCursor {nullptr};
        std::vector<FileData*> listEntries = cursor->getChildrenListToDisplay();
        // Check if there is an entry in the cursor stack history matching any entry
        // in the currect folder. If so, select that entry.
        for (auto it = mCursorStackHistory.begin(); it != mCursorStackHistory.end(); ++it) {
            if (std::find(listEntries.begin(), listEntries.end(), *it) != listEntries.end()) {
                newCursor = *it;
                mCursorStackHistory.erase(it);
                break;
            }
        }

        // If there was no match in the cursor history, simply select the first entry.
        if (!newCursor)
            newCursor = getCursor();
        setCursor(newCursor);
        stopListScrolling();
        if (mRoot->getSystem()->getThemeFolder() == "custom-collections")
            updateHelpPrompts();
    }
    else {
        NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
    }
}

void GamelistBase::addPlaceholder(FileData* firstEntry)
{
    // Empty list, add a placeholder.
    FileData* placeholder {nullptr};

    if (firstEntry && firstEntry->getSystem()->isGroupedCustomCollection())
        placeholder = firstEntry->getSystem()->getPlaceholder();
    else
        placeholder = this->mRoot->getSystem()->getPlaceholder();

    auto letterCaseFunc = [this](std::string& name) {
        const LetterCase letterCase {mPrimary->getLetterCase()};
        if (letterCase == LetterCase::UPPERCASE)
            name = Utils::String::toUpper(name);
        else if (letterCase == LetterCase::LOWERCASE)
            name = Utils::String::toLower(name);
        else if (letterCase == LetterCase::CAPITALIZE)
            name = Utils::String::toCapitalized(name);
    };

    if (mTextList != nullptr) {
        TextListComponent<FileData*>::Entry textListEntry;
        textListEntry.name = placeholder->getName();
        letterCaseFunc(textListEntry.name);
        textListEntry.object = placeholder;
        textListEntry.data.entryType = TextListEntryType::SECONDARY;
        mTextList->addEntry(textListEntry);
    }
    else if (mCarousel != nullptr) {
        CarouselComponent<FileData*>::Entry carouselEntry;
        carouselEntry.name = placeholder->getName();
        letterCaseFunc(carouselEntry.name);
        carouselEntry.object = placeholder;
        mCarousel->addEntry(carouselEntry, mRoot->getSystem()->getTheme());
    }
    else if (mGrid != nullptr) {
        GridComponent<FileData*>::Entry gridEntry;
        gridEntry.name = placeholder->getName();
        letterCaseFunc(gridEntry.name);
        gridEntry.object = placeholder;
        mGrid->addEntry(gridEntry, mRoot->getSystem()->getTheme());
    }
}

void GamelistBase::remove(FileData* game, bool deleteFile)
{
    // Optionally delete the game file on the filesystem.
    if (deleteFile)
        Utils::FileSystem::removeFile(game->getPath());

    FileData* parent {game->getParent()};
    // Select next element in list, or previous if none.
    if (getCursor() == game) {
        std::vector<FileData*> siblings {parent->getChildrenListToDisplay()};
        auto gameIter = std::find(siblings.cbegin(), siblings.cend(), game);
        unsigned int gamePos {
            static_cast<unsigned int>(std::distance(siblings.cbegin(), gameIter))};
        if (gameIter != siblings.cend()) {
            if ((gamePos + 1) < siblings.size())
                setCursor(siblings.at(gamePos + 1));
            else if (gamePos > 1)
                setCursor(siblings.at(gamePos - 1));
        }
    }

    mPrimary->remove(game);

    if (mPrimary->size() == 0)
        addPlaceholder(nullptr);

    // If a game has been deleted, immediately remove the entry from gamelist.xml
    // regardless of the value of the setting SaveGamelistsMode.
    game->setDeletionFlag(true);
    parent->getSystem()->writeMetaData();

    // Remove before repopulating (removes from parent), then update the view.
    delete game;

    if (deleteFile) {
        parent->sort(parent->getSortTypeFromString(parent->getSortTypeString()),
                     Settings::getInstance()->getBool("FavoritesFirst"));
        onFileChanged(parent, false);
    }
}

void GamelistBase::removeMedia(FileData* game)
{
    std::string systemMediaDir {FileData::getMediaDirectory() + game->getSystem()->getName()};
    std::string mediaType;
    std::string path;

    // Stop the video player, especially important on Windows as the file would otherwise be locked.
    stopViewVideos();

    // If there are no media files left in the directory after the deletion, then remove
    // the directory too. Remove any empty parent directories as well.
    auto removeEmptyDirFunc = [](std::string systemMediaDir, std::string mediaType,
                                 std::string path) {
        std::string parentPath {Utils::FileSystem::getParent(path)};
        while (parentPath != systemMediaDir + "/" + mediaType) {
            if (Utils::FileSystem::getDirContent(parentPath).size() == 0) {
                Utils::FileSystem::removeDirectory(parentPath, false);
                parentPath = Utils::FileSystem::getParent(parentPath);
            }
            else {
                break;
            }
        }
    };

    // Remove all game media files on the filesystem.
    while (Utils::FileSystem::exists(game->getVideoPath())) {
        mediaType = "videos";
        path = game->getVideoPath();
        if (!Utils::FileSystem::removeFile(path))
            break;
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    while (Utils::FileSystem::exists(game->getManualPath())) {
        mediaType = "manuals";
        path = game->getManualPath();
        if (!Utils::FileSystem::removeFile(path))
            break;
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    while (Utils::FileSystem::exists(game->getMiximagePath())) {
        mediaType = "miximages";
        path = game->getMiximagePath();
        if (!Utils::FileSystem::removeFile(path))
            break;
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    while (Utils::FileSystem::exists(game->getScreenshotPath())) {
        mediaType = "screenshots";
        path = game->getScreenshotPath();
        if (!Utils::FileSystem::removeFile(path))
            break;
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    while (Utils::FileSystem::exists(game->getTitleScreenPath())) {
        mediaType = "titlescreens";
        path = game->getTitleScreenPath();
        if (!Utils::FileSystem::removeFile(path))
            break;
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    while (Utils::FileSystem::exists(game->getCoverPath())) {
        mediaType = "covers";
        path = game->getCoverPath();
        if (!Utils::FileSystem::removeFile(path))
            break;
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    while (Utils::FileSystem::exists(game->getBackCoverPath())) {
        mediaType = "backcovers";
        path = game->getBackCoverPath();
        if (!Utils::FileSystem::removeFile(path))
            break;
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    while (Utils::FileSystem::exists(game->getFanArtPath())) {
        mediaType = "fanart";
        path = game->getFanArtPath();
        if (!Utils::FileSystem::removeFile(path))
            break;
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    while (Utils::FileSystem::exists(game->getMarqueePath())) {
        mediaType = "marquees";
        path = game->getMarqueePath();
        if (!Utils::FileSystem::removeFile(path))
            break;
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    while (Utils::FileSystem::exists(game->get3DBoxPath())) {
        mediaType = "3dboxes";
        path = game->get3DBoxPath();
        if (!Utils::FileSystem::removeFile(path))
            break;
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    while (Utils::FileSystem::exists(game->getPhysicalMediaPath())) {
        mediaType = "physicalmedia";
        path = game->getPhysicalMediaPath();
        if (!Utils::FileSystem::removeFile(path))
            break;
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }
}

void GamelistBase::populateList(const std::vector<FileData*>& files, FileData* firstEntry)
{
    mFirstGameEntry = nullptr;
    bool favoriteStar {true};
    bool isEditing {false};
    bool isCollection {false};
    bool isCustomCollection {false};
    std::string editingCollection;
    std::string inCollectionPrefix;
    LetterCase letterCase {LetterCase::NONE};

    if (CollectionSystemsManager::getInstance()->isEditing()) {
        editingCollection = CollectionSystemsManager::getInstance()->getEditingCollection();
        isEditing = true;
    }

    if (files.size() > 0) {
        isCollection = files.front()->getSystem()->isCollection();
        isCustomCollection = files.front()->getSystem()->isCustomCollection();
        // Read the settings that control whether a unicode star character should be added
        // as a prefix to the game name.
        if (isCustomCollection)
            favoriteStar = Settings::getInstance()->getBool("FavStarCustom");
        else
            favoriteStar = Settings::getInstance()->getBool("FavoritesStar");
    }

    if (mPrimary != nullptr)
        mPrimary->clear();

    auto nameSuffixFunc = [this](std::vector<FileData*>::const_iterator it, std::string& name) {
        if ((*it)->getType() == GAME) {
            const LetterCase letterCase {mPrimary->getLetterCaseSystemNameSuffix()};
            name.append(" [");
            if (letterCase == LetterCase::UPPERCASE) {
                name.append(
                    Utils::String::toUpper((*it)->getSourceFileData()->getSystem()->getName()));
            }
            else if (letterCase == LetterCase::CAPITALIZE) {
                name.append(Utils::String::toCapitalized(
                    (*it)->getSourceFileData()->getSystem()->getName()));
            }
            else {
                name.append((*it)->getSourceFileData()->getSystem()->getName());
            }
            name.append("]");
        }
    };

    auto theme = mRoot->getSystem()->getTheme();
    std::string name;
    std::string defaultImage;
    std::string defaultFolderImage;

    if (mCarousel != nullptr) {
        defaultImage = mCarousel->getDefaultCarouselImage();
        defaultFolderImage = mCarousel->getDefaultCarouselFolderImage();
        if (!ResourceManager::getInstance().fileExists(defaultImage))
            defaultImage = "";
    }
    else if (mGrid != nullptr) {
        defaultImage = mGrid->getDefaultGridImage();
        defaultFolderImage = mGrid->getDefaultGridFolderImage();
        if (!ResourceManager::getInstance().fileExists(defaultImage))
            defaultImage = "";
    }

    if (files.size() > 0) {
        for (auto it = files.cbegin(); it != files.cend(); ++it) {

            if (!mFirstGameEntry && (*it)->getType() == GAME)
                mFirstGameEntry = (*it);

            if (isCustomCollection && (*it)->getType() == FOLDER) {
                letterCase = mPrimary->getLetterCaseCustomCollections();
                if (letterCase == LetterCase::UNDEFINED)
                    letterCase = mPrimary->getLetterCase();
            }
            else {
                letterCase = mPrimary->getLetterCase();
            }

            if (mCarousel != nullptr) {
                CarouselComponent<FileData*>::Entry carouselEntry;
                carouselEntry.name = (*it)->getName();
                carouselEntry.object = *it;

                if (letterCase == LetterCase::UPPERCASE)
                    carouselEntry.name = Utils::String::toUpper(carouselEntry.name);
                else if (letterCase == LetterCase::LOWERCASE)
                    carouselEntry.name = Utils::String::toLower(carouselEntry.name);
                else if (letterCase == LetterCase::CAPITALIZE)
                    carouselEntry.name = Utils::String::toCapitalized(carouselEntry.name);

                if (isCollection && mSystemNameSuffix)
                    nameSuffixFunc(it, carouselEntry.name);

                if ((*it)->getType() == FOLDER) {
                    if (defaultFolderImage != "")
                        carouselEntry.data.defaultImagePath = defaultFolderImage;
                    else if (defaultImage != "")
                        carouselEntry.data.defaultImagePath = defaultImage;
                }
                else if (defaultImage != "") {
                    carouselEntry.data.defaultImagePath = defaultImage;
                }

                mCarousel->addEntry(carouselEntry, theme);
            }
            else if (mGrid != nullptr) {
                GridComponent<FileData*>::Entry gridEntry;
                gridEntry.name = (*it)->getName();
                gridEntry.object = *it;

                if (letterCase == LetterCase::UPPERCASE)
                    gridEntry.name = Utils::String::toUpper(gridEntry.name);
                else if (letterCase == LetterCase::LOWERCASE)
                    gridEntry.name = Utils::String::toLower(gridEntry.name);
                else if (letterCase == LetterCase::CAPITALIZE)
                    gridEntry.name = Utils::String::toCapitalized(gridEntry.name);

                if (isCollection && mSystemNameSuffix)
                    nameSuffixFunc(it, gridEntry.name);

                if ((*it)->getType() == FOLDER) {
                    if (defaultFolderImage != "")
                        gridEntry.data.defaultImagePath = defaultFolderImage;
                    else if (defaultImage != "")
                        gridEntry.data.defaultImagePath = defaultImage;
                }
                else if (defaultImage != "") {
                    gridEntry.data.defaultImagePath = defaultImage;
                }

                mGrid->addEntry(gridEntry, theme);
            }
            else if (mTextList != nullptr) {
                TextListComponent<FileData*>::Entry textListEntry;
                std::string indicators {mTextList->getIndicators()};
                std::string collectionIndicators {mTextList->getCollectionIndicators()};

                // Add a leading tick mark icon to the game name if it's part of the custom
                // collection currently being edited.
                if (isEditing && (*it)->getType() == GAME) {
                    if (CollectionSystemsManager::getInstance()->inCustomCollection(
                            editingCollection, (*it))) {
                        if (collectionIndicators == "ascii")
                            inCollectionPrefix = "! ";
                        else
                            inCollectionPrefix = ViewController::TICKMARK_CHAR + "  ";
                    }
                    else {
                        inCollectionPrefix = "";
                    }
                }

                if (indicators == "none") {
                    name = inCollectionPrefix + (*it)->getName();
                }
                else {
                    if ((*it)->getFavorite() && favoriteStar &&
                        mRoot->getSystem()->getName() != "favorites") {
                        if (indicators == "ascii")
                            name = inCollectionPrefix + "* " + (*it)->getName();
                        else
                            name = inCollectionPrefix + ViewController::FAVORITE_CHAR + "  " +
                                   (*it)->getName();
                    }
                    else if ((*it)->getType() == FOLDER &&
                             mRoot->getSystem()->getName() != "collections") {
                        if (indicators == "ascii") {
                            if ((*it)->metadata.get("folderlink") != "")
                                name = "> " + (*it)->getName();
                            else
                                name = "# " + (*it)->getName();
                        }
                        else {
                            if ((*it)->metadata.get("folderlink") != "")
                                name = ViewController::FOLDERLINK_CHAR + "  " + (*it)->getName();
                            else
                                name = ViewController::FOLDER_CHAR + "  " + (*it)->getName();
                        }
                    }
                    else {
                        name = inCollectionPrefix + (*it)->getName();
                    }
                }

                if (letterCase == LetterCase::UPPERCASE)
                    name = Utils::String::toUpper(name);
                else if (letterCase == LetterCase::LOWERCASE)
                    name = Utils::String::toLower(name);
                else if (letterCase == LetterCase::CAPITALIZE)
                    name = Utils::String::toCapitalized(name);

                if (isCollection && mSystemNameSuffix)
                    nameSuffixFunc(it, name);

                textListEntry.name = name;
                textListEntry.object = *it;
                if ((*it)->getType() == FOLDER)
                    textListEntry.data.entryType = TextListEntryType::SECONDARY;
                else
                    textListEntry.data.entryType = TextListEntryType::PRIMARY;
                mTextList->addEntry(textListEntry);
            }
        }
    }
    else {
        addPlaceholder(firstEntry);
    }

    if (mGrid != nullptr)
        mGrid->calculateLayout();

    generateGamelistInfo(getCursor(), firstEntry);
    generateFirstLetterIndex(files);
}

void GamelistBase::generateFirstLetterIndex(const std::vector<FileData*>& files)
{
    std::string firstChar;

    bool onlyFavorites {true};
    bool onlyFolders {true};
    bool hasFavorites {false};
    bool hasFolderFavorites {false};
    bool hasFolders {false};
    bool favoritesSorting {false};

    mFirstLetterIndex.clear();

    if (files.size() > 0 && files.front()->getSystem()->isCustomCollection())
        favoritesSorting = Settings::getInstance()->getBool("FavFirstCustom");
    else
        favoritesSorting = Settings::getInstance()->getBool("FavoritesFirst");

    bool foldersOnTop {Settings::getInstance()->getBool("FoldersOnTop")};

    // Find out if there are only favorites and/or only folders in the list.
    for (auto it = files.begin(); it != files.end(); ++it) {
        if (!((*it)->getFavorite()))
            onlyFavorites = false;
        if (!((*it)->getType() == FOLDER))
            onlyFolders = false;
    }

    // Build the index.
    for (auto it = files.begin(); it != files.end(); ++it) {
        if ((*it)->getType() == FOLDER && (*it)->getFavorite() && favoritesSorting &&
            !onlyFavorites) {
            hasFolderFavorites = true;
        }
        else if ((*it)->getType() == FOLDER && foldersOnTop && !onlyFolders) {
            hasFolders = true;
        }
        else if ((*it)->getType() == GAME && (*it)->getFavorite() && favoritesSorting &&
                 !onlyFavorites) {
            hasFavorites = true;
        }
        else {
            mFirstLetterIndex.push_back(Utils::String::getFirstCharacter((*it)->getSortName()));
        }
    }

    // Sort and make each entry unique.
    std::sort(mFirstLetterIndex.begin(), mFirstLetterIndex.end());
    auto last = std::unique(mFirstLetterIndex.begin(), mFirstLetterIndex.end());
    mFirstLetterIndex.erase(last, mFirstLetterIndex.end());

    // If there are any favorites and/or folders in the list, insert their respective
    // Unicode characters at the beginning of the vector.
    if (hasFavorites || (hasFolderFavorites && onlyFolders))
        mFirstLetterIndex.insert(mFirstLetterIndex.begin(), ViewController::FAVORITE_CHAR);

    if (hasFolders)
        mFirstLetterIndex.insert(mFirstLetterIndex.begin(), ViewController::FOLDER_CHAR);
}

void GamelistBase::generateGamelistInfo(FileData* cursor, FileData* firstEntry)
{
    // Generate data needed for the gamelistInfo field, which is displayed from the
    // gamelist interfaces.
    mIsFiltered = false;
    mIsFolder = false;
    FileData* rootFolder {firstEntry->getSystem()->getRootFolder()};

    std::pair<unsigned int, unsigned int> gameCount;
    FileFilterIndex* idx {rootFolder->getSystem()->getIndex()};

    // For the 'recent' collection we need to recount the games as the collection was
    // trimmed down to 50 items. If we don't do this, the game count will not be correct
    // as it would include all the games prior to trimming.
    if (mRoot->getPath() == "recent")
        mRoot->countGames(gameCount);

    gameCount = rootFolder->getGameCount();

    mGameCount = gameCount.first;
    mFavoritesGameCount = gameCount.second;
    mFilteredGameCount = 0;
    mFilteredGameCountAll = 0;

    if (idx->isFiltered()) {
        mIsFiltered = true;
        mFilteredGameCount =
            static_cast<unsigned int>(rootFolder->getFilesRecursive(GAME, true, false).size());
        // Also count the games that are set to not be counted as games, as the filter may
        // apply to such entries as well and this will be indicated with a separate '+ XX'
        // in the GamelistInfo field.
        mFilteredGameCountAll =
            static_cast<unsigned int>(rootFolder->getFilesRecursive(GAME, true, true).size());
    }

    if (firstEntry->getParent() && firstEntry->getParent()->getType() == FOLDER)
        mIsFolder = true;
}

std::string GamelistBase::getQuickSystemSelectLeftButton()
{
    if (Settings::getInstance()->getString("QuickSystemSelect") == "leftrightshoulders") {
        if (mLeftRightAvailable)
            return "left";
        else
            return "leftshoulder";
    }

    if (Settings::getInstance()->getString("QuickSystemSelect") == "leftrighttriggers") {
        if (mLeftRightAvailable)
            return "left";
        else
            return "lefttrigger";
    }

    if (Settings::getInstance()->getString("QuickSystemSelect") == "shoulders")
        return "leftshoulder";

    if (Settings::getInstance()->getString("QuickSystemSelect") == "triggers")
        return "lefttrigger";

    if (Settings::getInstance()->getString("QuickSystemSelect") == "leftright") {
        if (mLeftRightAvailable)
            return "left";
        else
            return "";
    }

    if (Settings::getInstance()->getString("QuickSystemSelect") == "disabled")
        return "";

    // This should only happen if there is an invalid value in es_settings.xml.
    if (mLeftRightAvailable)
        return "left";
    else
        return "leftshoulder";
}
std::string GamelistBase::getQuickSystemSelectRightButton()
{
    if (Settings::getInstance()->getString("QuickSystemSelect") == "leftrightshoulders") {
        if (mLeftRightAvailable)
            return "right";
        else
            return "rightshoulder";
    }

    if (Settings::getInstance()->getString("QuickSystemSelect") == "leftrighttriggers") {
        if (mLeftRightAvailable)
            return "right";
        else
            return "righttrigger";
    }

    if (Settings::getInstance()->getString("QuickSystemSelect") == "shoulders")
        return "rightshoulder";

    if (Settings::getInstance()->getString("QuickSystemSelect") == "triggers")
        return "righttrigger";

    if (Settings::getInstance()->getString("QuickSystemSelect") == "leftright") {
        if (mLeftRightAvailable)
            return "right";
        else
            return "";
    }

    if (Settings::getInstance()->getString("QuickSystemSelect") == "disabled")
        return "";

    if (mLeftRightAvailable)
        return "right";
    else
        return "rightshoulder";
}
