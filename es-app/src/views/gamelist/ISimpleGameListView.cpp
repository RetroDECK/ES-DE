//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ISimpleGameListView.cpp
//
//  Interface that defines a simple gamelist view.
//

#include "views/gamelist/ISimpleGameListView.h"

#include "guis/GuiInfoPopup.h"
#include "utils/StringUtil.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "CollectionSystemsManager.h"
#include "FileFilterIndex.h"
#include "Settings.h"
#include "Sound.h"
#include "SystemData.h"

#include "Log.h"

ISimpleGameListView::ISimpleGameListView(
        Window* window,
        FileData* root)
        : IGameListView(window, root),
        mHeaderText(window),
        mHeaderImage(window),
        mBackground(window),
        mRandomGame(nullptr)
{
    mHeaderText.setText("Logo Text");
    mHeaderText.setSize(mSize.x(), 0);
    mHeaderText.setPosition(0, 0);
    mHeaderText.setHorizontalAlignment(ALIGN_CENTER);
    mHeaderText.setDefaultZIndex(50);

    mHeaderImage.setResize(0, mSize.y() * 0.185f);
    mHeaderImage.setOrigin(0.5f, 0.0f);
    mHeaderImage.setPosition(mSize.x() / 2, 0);
    mHeaderImage.setDefaultZIndex(50);

    mBackground.setResize(mSize.x(), mSize.y());
    mBackground.setDefaultZIndex(0);

    addChild(&mHeaderText);
    addChild(&mBackground);
}

ISimpleGameListView::~ISimpleGameListView()
{
    // Remove theme extras.
    for (auto extra : mThemeExtras) {
        removeChild(extra);
        delete extra;
    }
    mThemeExtras.clear();
}

void ISimpleGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
    using namespace ThemeFlags;
    mBackground.applyTheme(theme, getName(), "background", ALL);
    mHeaderImage.applyTheme(theme, getName(), "logo", ALL);
    mHeaderText.applyTheme(theme, getName(), "logoText", ALL);

    // Remove old theme extras.
    for (auto extra : mThemeExtras) {
        removeChild(extra);
        delete extra;
    }
    mThemeExtras.clear();

    // Add new theme extras.
    mThemeExtras = ThemeData::makeExtras(theme, getName(), mWindow);
    for (auto extra : mThemeExtras)
        addChild(extra);

    if (mHeaderImage.hasImage()) {
        removeChild(&mHeaderText);
        addChild(&mHeaderImage);
    }
    else {
        addChild(&mHeaderText);
        removeChild(&mHeaderImage);
    }
}

void ISimpleGameListView::onFileChanged(FileData* file, bool reloadGameList)
{
    // We could be tricky here to be efficient;
    // but this shouldn't happen very often so we'll just always repopulate.
    FileData* cursor = getCursor();
    if (!cursor->isPlaceHolder()) {
        populateList(cursor->getParent()->getChildrenListToDisplay(), cursor->getParent());
        setCursor(cursor);
    }
    else {
        populateList(mRoot->getChildrenListToDisplay(), mRoot);
        setCursor(cursor);
    }
}

bool ISimpleGameListView::input(InputConfig* config, Input input)
{
    if (input.value != 0) {
        if (config->isMappedTo("a", input)) {
            FileData* cursor = getCursor();
            if (cursor->getType() == GAME) {
                onPauseVideo();
                ViewController::get()->cancelViewTransitions();
                stopListScrolling();
                launch(cursor);
            }
            else {
                // It's a folder.
                if (cursor->getChildren().size() > 0) {
                    ViewController::get()->cancelViewTransitions();
                    NavigationSounds::getInstance()->playThemeNavigationSound(SELECTSOUND);
                    mCursorStack.push(cursor);
                    populateList(cursor->getChildrenListToDisplay(), cursor);

                    FileData* newCursor = nullptr;
                    std::vector<FileData*> listEntries = cursor->getChildrenListToDisplay();
                    // Check if there is an entry in the cursor stack history matching any entry
                    // in the currect folder. If so, select that entry.
                    for (auto it = mCursorStackHistory.begin();
                            it != mCursorStackHistory.end(); it++) {
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
                    if (mRoot->getSystem()->getThemeFolder() == "custom-collections")
                        updateHelpPrompts();
                }
                else {
                    NavigationSounds::getInstance()->playThemeNavigationSound(SCROLLSOUND);
                }
            }

            return true;
        }
        else if (config->isMappedTo("b", input)) {
            ViewController::get()->cancelViewTransitions();
            if (mCursorStack.size()) {
                // Save the position to the cursor stack history.
                mCursorStackHistory.push_back(getCursor());
                NavigationSounds::getInstance()->playThemeNavigationSound(BACKSOUND);
                populateList(mCursorStack.top()->getParent()->getChildrenListToDisplay(),
                        mCursorStack.top()->getParent());
                setCursor(mCursorStack.top());
                if (mCursorStack.size() > 0)
                    mCursorStack.pop();
                if (mRoot->getSystem()->getThemeFolder() == "custom-collections")
                    updateHelpPrompts();
            }
            else {
                NavigationSounds::getInstance()->playThemeNavigationSound(BACKSOUND);
                onPauseVideo();
                onFocusLost();
                stopListScrolling();
                SystemData* systemToView = getCursor()->getSystem();
                if (systemToView->isCustomCollection() &&
                        systemToView->getRootFolder()->getParent())
                    ViewController::get()->goToSystemView(
                            systemToView->getRootFolder()->getParent()->getSystem(), true);
                else
                    ViewController::get()->goToSystemView(systemToView, true);
            }

            return true;
        }
        else if (config->isMappedTo("x", input)) {
            if (getCursor()->getType() == PLACEHOLDER) {
                NavigationSounds::getInstance()->playThemeNavigationSound(SCROLLSOUND);
                return true;
            }
            else if (config->isMappedTo("x", input) &&
                    mRoot->getSystem()->getThemeFolder() == "custom-collections" &&
                    mCursorStack.empty() && ViewController::get()->getState().viewing ==
                    ViewController::GAME_LIST) {
                NavigationSounds::getInstance()->playThemeNavigationSound(SCROLLSOUND);
                // Jump to the randomly selected game.
                if (mRandomGame) {
                    stopListScrolling();
                    ViewController::get()->cancelViewTransitions();
                    mWindow->startMediaViewer(mRandomGame);
                    return true;
                }
            }
            else if (mRoot->getSystem()->isGameSystem()) {
                stopListScrolling();
                ViewController::get()->cancelViewTransitions();
                NavigationSounds::getInstance()->playThemeNavigationSound(SCROLLSOUND);
                mWindow->startMediaViewer(getCursor());
                return true;
            }
        }
        else if (config->isMappedLike(getQuickSystemSelectRightButton(), input)) {
            if (Settings::getInstance()->getBool("QuickSystemSelect") &&
                    SystemData::sSystemVector.size() > 1) {
                onPauseVideo();
                onFocusLost();
                stopListScrolling();
                ViewController::get()->goToNextGameList();
                return true;
            }
        }
        else if (config->isMappedLike(getQuickSystemSelectLeftButton(), input)) {
            if (Settings::getInstance()->getBool("QuickSystemSelect") &&
                    SystemData::sSystemVector.size() > 1) {
                onPauseVideo();
                onFocusLost();
                stopListScrolling();
                ViewController::get()->goToPrevGameList();
                return true;
            }
        }
        else if (Settings::getInstance()->getBool("RandomAddButton") &&
                (config->isMappedTo("leftthumbstickclick", input) ||
                config->isMappedTo("rightthumbstickclick", input))) {
            if (mRoot->getSystem()->isGameSystem() && getCursor()->getType() != PLACEHOLDER) {
                stopListScrolling();
                // Jump to a random game.
                NavigationSounds::getInstance()->playThemeNavigationSound(SCROLLSOUND);
                FileData* randomGame = getCursor()->getSystem()->getRandomGame(getCursor());
                if (randomGame)
                    setCursor(randomGame);
                return true;
            }
        }
        else if (config->isMappedTo("y", input) &&
                mRoot->getSystem()->getThemeFolder() == "custom-collections" &&
                !CollectionSystemsManager::get()->isEditing() &&
                mCursorStack.empty() && ViewController::get()->getState().viewing ==
                ViewController::GAME_LIST) {
            // Jump to the randomly selected game.
            if (mRandomGame) {
                NavigationSounds::getInstance()->playThemeNavigationSound(SELECTSOUND);
                // If there is already an mCursorStackHistory entry for the collection, then
                // remove it so we don't get multiple entries.
                std::vector<FileData*> listEntries =
                        mRandomGame->getSystem()->getRootFolder()->getChildrenListToDisplay();
                for (auto it = mCursorStackHistory.begin();
                        it != mCursorStackHistory.end(); it++) {
                    if (std::find(listEntries.begin(), listEntries.end(), *it) !=
                            listEntries.end()) {
                        mCursorStackHistory.erase(it);
                        break;
                    }
                }
                setCursor(mRandomGame);
                updateHelpPrompts();
            }
            else {
                NavigationSounds::getInstance()->playThemeNavigationSound(SCROLLSOUND);
            }
        }
        else if (config->isMappedTo("y", input) &&
                !Settings::getInstance()->getBool("FavoritesAddButton") &&
                !CollectionSystemsManager::get()->isEditing()) {
            return true;
        }
        else if (config->isMappedTo("y", input) &&
                !UIModeController::getInstance()->isUIModeKid() &&
                !UIModeController::getInstance()->isUIModeKiosk()) {
            // Notify the user if attempting to add a custom collection to a custom collection.
            if (CollectionSystemsManager::get()->isEditing() &&
                    mRoot->getSystem()->isGameSystem() && getCursor()->getType() != PLACEHOLDER &&
                    getCursor()->getParent()->getPath() == "collections") {
                NavigationSounds::getInstance()->playThemeNavigationSound(FAVORITESOUND);
                GuiInfoPopup* s;
                s = new GuiInfoPopup(mWindow,
                        "CAN'T ADD CUSTOM COLLECTIONS TO CUSTOM COLLECTIONS", 4000);
                mWindow->setInfoPopup(s);
            }
            // Notify the user if attempting to add a placeholder to a custom collection.
            if (CollectionSystemsManager::get()->isEditing() &&
                    mRoot->getSystem()->isGameSystem() && getCursor()->getType() == PLACEHOLDER) {
                NavigationSounds::getInstance()->playThemeNavigationSound(FAVORITESOUND);
                GuiInfoPopup* s;
                s = new GuiInfoPopup(mWindow,
                        "CAN'T ADD PLACEHOLDERS TO CUSTOM COLLECTIONS", 4000);
                mWindow->setInfoPopup(s);
            }
            else if (mRoot->getSystem()->isGameSystem() && getCursor()->getType() != PLACEHOLDER &&
                    getCursor()->getParent()->getPath() != "collections") {
                if (getCursor()->getType() == GAME || getCursor()->getType() == FOLDER)
                    NavigationSounds::getInstance()->playThemeNavigationSound(FAVORITESOUND);
                // When marking or unmarking a game as favorite, don't jump to the new position
                // it gets after the gamelist sorting. Instead retain the cursor position in the
                // list using the logic below.
                FileData* entryToUpdate = getCursor();
                SystemData* system = getCursor()->getSystem();
                bool favoritesSorting;
                bool removedLastFavorite = false;
                bool selectLastEntry = false;
                bool isEditing = CollectionSystemsManager::get()->isEditing();
                bool foldersOnTop = Settings::getInstance()->getBool("FoldersOnTop");
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

                if (favoritesSorting && static_cast<std::string>(
                        mRoot->getSystem()->getName()) != "recent" && !isEditing) {
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
                            entryToSelect = getCursor() != getNextEntry() ?
                                    getNextEntry() : getPreviousEntry();
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
                                    getCursor() : getPreviousEntry();
                        }
                        // If we are on the favorite marking boundary, select the previous entry.
                        else if (getCursor()->getFavorite() != getNextEntry()->getFavorite()) {
                            entryToSelect = getPreviousEntry();
                        }
                        // For all other scenarios try to select the next entry, and if it doesn't
                        // exist, select the previous entry.
                        else {
                            entryToSelect = getCursor() != getNextEntry() ?
                                    getNextEntry() : getPreviousEntry();
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
                    GuiInfoPopup* s;
                    if (isEditing) {
                        s = new GuiInfoPopup(mWindow,
                                "CAN'T ADD FOLDERS TO CUSTOM COLLECTIONS", 4000);
                    }
                    else {
                        MetaDataList* md = &entryToUpdate->getSourceFileData()->metadata;
                        if (md->get("favorite") == "false") {
                            md->set("favorite", "true");
                            s = new GuiInfoPopup(mWindow, "MARKED FOLDER '" +
                                    Utils::String::toUpper(Utils::String::removeParenthesis(
                                    entryToUpdate->getName())) + "' AS FAVORITE", 4000);
                        }
                        else {
                            md->set("favorite", "false");
                            s = new GuiInfoPopup(mWindow, "REMOVED FAVORITE MARKING FOR FOLDER '" +
                                    Utils::String::toUpper(Utils::String::removeParenthesis(
                                    entryToUpdate->getName())) + "'", 4000);
                        }
                    }

                    mWindow->setInfoPopup(s);
                    entryToUpdate->getSourceFileData()->getSystem()->onMetaDataSavePoint();

                    getCursor()->getParent()->sort(
                            mRoot->getSortTypeFromString(mRoot->getSortTypeString()),
                            Settings::getInstance()->getBool("FavoritesFirst"));

                    ViewController::get()->onFileChanged(getCursor(), false);

                    // Always jump to the first entry in the gamelist if the last favorite
                    // was unmarked. We couldn't do this earlier as we didn't have the list
                    // sorted yet.
                    if (removedLastFavorite) {
                         ViewController::get()->getGameListView(entryToUpdate->
                                getSystem())->setCursor(ViewController::get()->
                                getGameListView(entryToUpdate->getSystem())->getFirstEntry());
                    }
                    return true;
                }
                else if (isEditing && entryToUpdate->metadata.get("nogamecount") == "true") {
                    GuiInfoPopup* s = new GuiInfoPopup(mWindow,
                            "CAN'T ADD ENTRIES THAT ARE NOT COUNTED "
                            "AS GAMES TO CUSTOM COLLECTIONS", 4000);
                    mWindow->setInfoPopup(s);
                }
                else if (CollectionSystemsManager::get()->toggleGameInCollection(entryToUpdate)) {
                    // As the toggling of the game destroyed this object, we need to get the view
                    // from ViewController instead of using the reference that existed before the
                    // destruction. Otherwise we get random crashes.
                    IGameListView* view = ViewController::get()->getGameListView(system).get();
                    // Jump to the first entry in the gamelist if the last favorite was unmarked.
                    if (foldersOnTop && removedLastFavorite &&
                            !entryToUpdate->getSystem()->isCustomCollection()) {
                        ViewController::get()->getGameListView(entryToUpdate->getSystem())->
                                setCursor(ViewController::get()->getGameListView(entryToUpdate->
                                getSystem())->getFirstGameEntry());
                    }
                    else if (removedLastFavorite &&
                            !entryToUpdate->getSystem()->isCustomCollection()) {
                        view->setCursor(view->getFirstEntry());
                    }
                    else if (selectLastEntry) {
                        view->setCursor(view->getLastEntry());
                    }
                    // Display the indication icons which show what games are part of the
                    // custom collection currently being edited. This is done cheaply using
                    // onFileChanged() which will trigger populateList().
                    if (isEditing) {
                        for (auto it = SystemData::sSystemVector.begin();
                                it != SystemData::sSystemVector.end(); it++) {
                            ViewController::get()->getGameListView((*it))->onFileChanged(
                                    ViewController::get()->getGameListView((*it))->
                                    getCursor(), false);
                        }
                    }
                    return true;
                }
            }
        }
    }

    return IGameListView::input(config, input);
}

void ISimpleGameListView::generateGamelistInfo(FileData* cursor, FileData* firstEntry)
{
    // Generate data needed for the gamelistInfo field, which is displayed from the
    // gamelist interfaces (Detailed/Video/Grid).
    mIsFiltered = false;
    mIsFolder = false;
    FileData* rootFolder = firstEntry->getSystem()->getRootFolder();

    std::pair<unsigned int, unsigned int> gameCount;
    FileFilterIndex* idx = rootFolder->getSystem()->getIndex();

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
        mFilteredGameCount = static_cast<unsigned int>(rootFolder->
                getFilesRecursive(GAME, true, false).size());
        // Also count the games that are set to not be counted as games, as the filter may
        // apply to such entries as well and this will be indicated with a separate '+ XX'
        // in the GamelistInfo field.
        mFilteredGameCountAll = static_cast<unsigned int>(rootFolder->
                getFilesRecursive(GAME, true, true).size());
    }

    if (firstEntry->getParent() && firstEntry->getParent()->getType() == FOLDER)
        mIsFolder = true;
}

void ISimpleGameListView::generateFirstLetterIndex(const std::vector<FileData*>& files)
{
    std::string firstChar;

    bool onlyFavorites = true;
    bool onlyFolders = true;
    bool hasFavorites = false;
    bool hasFolders = false;
    bool favoritesSorting = false;

    mFirstLetterIndex.clear();

    if (files.size() > 0 && files.front()->getSystem()->isCustomCollection())
        favoritesSorting = Settings::getInstance()->getBool("FavFirstCustom");
    else
        favoritesSorting = Settings::getInstance()->getBool("FavoritesFirst");

     bool foldersOnTop = Settings::getInstance()->getBool("FoldersOnTop");

    // Find out if there are only favorites and/or only folders in the list.
    for (auto it = files.begin(); it != files.end(); it++) {
        if (!((*it)->getFavorite()))
            onlyFavorites = false;
        if (!((*it)->getType() == FOLDER))
            onlyFolders = false;
    }

    // Build the index.
    for (auto it = files.begin(); it != files.end(); it++) {
        if ((*it)->getType() == FOLDER && (*it)->getFavorite() &&
                favoritesSorting && !onlyFavorites)
            hasFavorites = true;
        else if ((*it)->getType() == FOLDER && foldersOnTop && !onlyFolders)
            hasFolders = true;
        else if ((*it)->getType() == GAME && (*it)->getFavorite() &&
                favoritesSorting && !onlyFavorites)
            hasFavorites = true;
        else
            mFirstLetterIndex.push_back(Utils::String::getFirstCharacter((*it)->getSortName()));
    }

    // Sort and make each entry unique.
    std::sort(mFirstLetterIndex.begin(), mFirstLetterIndex.end());
    auto last = std::unique(mFirstLetterIndex.begin(), mFirstLetterIndex.end());
    mFirstLetterIndex.erase(last, mFirstLetterIndex.end());

    // If there are any favorites and/or folders in the list, insert their respective
    // Unicode characters at the beginning of the vector.
    if (hasFavorites)
        mFirstLetterIndex.insert(mFirstLetterIndex.begin(), ViewController::FAVORITE_CHAR);

    if (hasFolders)
        mFirstLetterIndex.insert(mFirstLetterIndex.begin(), ViewController::FOLDER_CHAR);
}
