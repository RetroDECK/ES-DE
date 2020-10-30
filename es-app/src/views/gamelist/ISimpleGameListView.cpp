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
#include "CollectionSystemManager.h"
#include "Settings.h"
#include "Sound.h"
#include "SystemData.h"

ISimpleGameListView::ISimpleGameListView(
        Window* window,
        FileData* root)
        : IGameListView(window, root),
        mHeaderText(window),
        mHeaderImage(window),
        mBackground(window)
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
        populateList(cursor->getParent()->getChildrenListToDisplay());
        setCursor(cursor);
    }
    else {
        populateList(mRoot->getChildrenListToDisplay());
        setCursor(cursor);
    }
}

bool ISimpleGameListView::input(InputConfig* config, Input input)
{
    if (input.value != 0) {
        if (config->isMappedTo("a", input)) {
            FileData* cursor = getCursor();
            if (cursor->getType() == GAME) {
                ViewController::get()->resetMovingCamera();
                stopListScrolling();
                launch(cursor);
            }
            else {
                // It's a folder.
                if (cursor->getChildren().size() > 0) {
                    ViewController::get()->resetMovingCamera();
                    NavigationSounds::getInstance()->playThemeNavigationSound(SELECTSOUND);
                    mCursorStack.push(cursor);
                    populateList(cursor->getChildrenListToDisplay());
                    FileData* cursor = getCursor();
                    setCursor(cursor);
                }
            }

            return true;
        }
        else if (config->isMappedTo("b", input)) {
            ViewController::get()->resetMovingCamera();
            if (mCursorStack.size()) {
                NavigationSounds::getInstance()->playThemeNavigationSound(BACKSOUND);
                populateList(mCursorStack.top()->getParent()->getChildrenListToDisplay());
                setCursor(mCursorStack.top());
                if (mCursorStack.size() > 0)
                    mCursorStack.pop();
                onFileChanged(getCursor(), false);
            }
            else {
                NavigationSounds::getInstance()->playThemeNavigationSound(BACKSOUND);
                onFocusLost();
                stopListScrolling();
                SystemData* systemToView = getCursor()->getSystem();
                if (systemToView->isCustomCollection() &&
                        systemToView->getRootFolder()->getParent())
                    ViewController::get()->goToSystemView(
                            systemToView->getRootFolder()->getParent()->getSystem());
                else
                    ViewController::get()->goToSystemView(systemToView);
            }

            return true;
        }
        else if (config->isMappedLike(getQuickSystemSelectRightButton(), input)) {
            if (Settings::getInstance()->getBool("QuickSystemSelect")) {
                onFocusLost();
                stopListScrolling();
                ViewController::get()->goToNextGameList();
                return true;
            }
        }
        else if (config->isMappedLike(getQuickSystemSelectLeftButton(), input)) {
            if (Settings::getInstance()->getBool("QuickSystemSelect")) {
                onFocusLost();
                stopListScrolling();
                ViewController::get()->goToPrevGameList();
                return true;
            }
        }
        else if (config->isMappedTo("x", input)) {
            if (mRoot->getSystem()->isGameSystem() && getCursor()->getType() != PLACEHOLDER) {
                stopListScrolling();
                // Go to random system game.
                NavigationSounds::getInstance()->playThemeNavigationSound(SCROLLSOUND);
                FileData* randomGame = getCursor()->getSystem()->getRandomGame(getCursor());
                if (randomGame)
                    setCursor(randomGame);
                return true;
            }
        }
        else if (config->isMappedTo("y", input) &&
                !UIModeController::getInstance()->isUIModeKid()) {
            if (mRoot->getSystem()->isGameSystem() && getCursor()->getType() != PLACEHOLDER &&
                    getCursor()->getParent()->getPath() != "collections") {
                if (getCursor()->getType() == GAME || getCursor()->getType() == FOLDER)
                    NavigationSounds::getInstance()->playThemeNavigationSound(FAVORITESOUND);
                // When marking or unmarking a game as favorite, don't jump to the new position
                // it gets after the gamelist sorting. Instead retain the cursor position in the
                // list using the logic below.
                FileData* entryToUpdate = getCursor();
                bool favoritesSorting;
                bool removedLastFavorite = false;
                bool isEditing = CollectionSystemManager::get()->isEditing();
                bool foldersOnTop = Settings::getInstance()->getBool("FoldersOnTop");
                // If the current list only contains folders, then treat it as if the folders
                // are not sorted on top, this way the logic should work exactly as for mixed
                // lists or files-only lists.
                if (getCursor()->getType() == FOLDER && foldersOnTop == true)
                    foldersOnTop = !getCursor()->viewHasOnlyFolders();

                if (mRoot->getSystem()->isCustomCollection())
                    favoritesSorting = Settings::getInstance()->getBool("FavFirstCustom");
                else
                    favoritesSorting = Settings::getInstance()->getBool("FavoritesFirst");

                if (favoritesSorting && static_cast<std::string>(
                        mRoot->getSystem()->getName()) != "recent" && !isEditing) {
                    FileData* entryToSelect;
                    // Add favorite flag.
                    if (!getCursor()->getFavorite()) {
                        // If it's a folder and folders are sorted on top, select the current entry.
                        if (foldersOnTop && getCursor()->getType() == FOLDER)
                            entryToSelect = getCursor();
                        // If it's the first entry to be marked as favorite, select the next entry.
                        else if (getCursor() == getFirstEntry())
                            entryToSelect = getNextEntry();
                        // If we are on the favorite marking boundary, select the next entry.
                        else if (getCursor()->getFavorite() != getPreviousEntry()->getFavorite())
                            entryToSelect = getNextEntry();
                        // If we mark the second entry as favorite and the first entry is not a
                        // favorite, then select this entry if they are of the same type.
                        else if (getPreviousEntry() == getFirstEntry() &&
                                getCursor()->getType() == getPreviousEntry()->getType())
                            entryToSelect = getPreviousEntry();
                        // For all other scenarios try to select the next entry, and if it doesn't
                        // exist, select the previous entry.
                        else
                            entryToSelect = getCursor() != getNextEntry() ?
                                    getNextEntry() : getPreviousEntry();
                    }
                    // Remove favorite flag.
                    else {
                        // If it's a folder and folders are sorted on top, select the current entry.
                        if (foldersOnTop && getCursor()->getType() == FOLDER)
                            entryToSelect = getCursor();
                        // If it's the last entry, select the previous entry.
                        else if (getCursor() == getLastEntry())
                            entryToSelect = getPreviousEntry();
                        // If we are on the favorite marking boundary, select the previous entry,
                        // unless folders are sorted on top and the previous entry is a folder.
                        else if (foldersOnTop &&
                                getCursor()->getFavorite() != getNextEntry()->getFavorite())
                            entryToSelect = getPreviousEntry()->getType() == FOLDER ?
                                    getCursor() : getPreviousEntry();
                        // If we are on the favorite marking boundary, select the previous entry.
                        else if (getCursor()->getFavorite() != getNextEntry()->getFavorite())
                            entryToSelect = getPreviousEntry();
                        // For all other scenarios try to select the next entry, and if it doesn't
                        // exist, select the previous entry.
                        else
                            entryToSelect = getCursor() != getNextEntry() ?
                                    getNextEntry() : getPreviousEntry();

                        // If we removed the last favorite marking, set the flag to jump to the
                        // first list entry after the sorting has been performed.
                        if (foldersOnTop && getCursor() == getFirstGameEntry() &&
                                !getNextEntry()->getFavorite())
                            removedLastFavorite = true;
                        else if (getCursor() == getFirstEntry() && !getNextEntry()->getFavorite())
                                removedLastFavorite = true;
                    }

                    setCursor(entryToSelect);
                }

                // Marking folders as favorites don't make them part of any collections,
                // so it makes more sense to handle it here than to add the function to
                // CollectionSystemManager.
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
                else if (CollectionSystemManager::get()->toggleGameInCollection(entryToUpdate)) {
                    // Jump to the first entry in the gamelist if the last favorite was unmarked.
                    if (foldersOnTop && removedLastFavorite &&
                            !entryToUpdate->getSystem()->isCustomCollection())
                        ViewController::get()->getGameListView(entryToUpdate->getSystem())->
                                setCursor(ViewController::get()->getGameListView(entryToUpdate->
                                getSystem())->getFirstGameEntry());
                    else if (removedLastFavorite &&
                            !entryToUpdate->getSystem()->isCustomCollection())
                        setCursor(getFirstEntry());
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
