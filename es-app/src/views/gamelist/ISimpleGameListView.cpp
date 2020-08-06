//
//  ISimpleGameListView.cpp
//
//  Interface that defines a simple GameListView.
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

void ISimpleGameListView::onFileChanged(FileData* /*file*/, FileChangeType /*change*/)
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
                launch(cursor);
            }
            else {
                // It's a folder.
                if (cursor->getChildren().size() > 0) {
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
            if (mCursorStack.size()) {
                NavigationSounds::getInstance()->playThemeNavigationSound(BACKSOUND);
                populateList(mCursorStack.top()->getParent()->getChildren());
                setCursor(mCursorStack.top());
                mCursorStack.pop();
            }
            else {
                NavigationSounds::getInstance()->playThemeNavigationSound(BACKSOUND);
                onFocusLost();
                SystemData* systemToView = getCursor()->getSystem();
                if (systemToView->isCollection())
                    systemToView = CollectionSystemManager::get()->getSystemToView(systemToView);

                ViewController::get()->goToSystemView(systemToView);
            }

            return true;
        }
        else if (config->isMappedLike(getQuickSystemSelectRightButton(), input)) {
            if (Settings::getInstance()->getBool("QuickSystemSelect")) {
                onFocusLost();
                ViewController::get()->goToNextGameList();
                return true;
            }
        }
        else if (config->isMappedLike(getQuickSystemSelectLeftButton(), input)) {
            if (Settings::getInstance()->getBool("QuickSystemSelect")) {
                onFocusLost();
                ViewController::get()->goToPrevGameList();
                return true;
            }
        }
        else if (config->isMappedTo("x", input)) {
            if (mRoot->getSystem()->isGameSystem() && getCursor()->getType() != PLACEHOLDER) {
                // Go to random system game.
                NavigationSounds::getInstance()->playThemeNavigationSound(SCROLLSOUND);
                FileData* randomGame = getCursor()->getSystem()->getRandomGame(getCursor());
                if (randomGame)
                    setCursor(randomGame);
                // If it's not a game, maybe it's a folder for an unthemed collection.
                else if (getCursor()->getSystem()->isCollection()) {
                    FileData* randomFolder =
                            mRoot->getSystem()->getRandomCollectionFolder(getCursor());
                    if (randomFolder)
                        setCursor(randomFolder);
                }

                return true;
            }
        }
        else if (config->isMappedTo("y", input) &&
                !UIModeController::getInstance()->isUIModeKid()) {
            if (mRoot->getSystem()->isGameSystem()) {
                if (getCursor()->getType() == GAME || getCursor()->getType() == FOLDER)
                    NavigationSounds::getInstance()->playThemeNavigationSound(FAVORITESOUND);

                // Marking folders as favorites is only cosmetic as they're not sorted
                // differently and they're not part of any collections. So it makes more
                // sense to do it here than to add the function to CollectionSystemManager.
                if (getCursor()->getType() == FOLDER) {
                    GuiInfoPopup* s;
                    MetaDataList* md = &getCursor()->getSourceFileData()->metadata;
                    if (md->get("favorite") == "false") {
                        md->set("favorite", "true");
                        s = new GuiInfoPopup(mWindow, "Marked folder '" +
                                Utils::String::removeParenthesis(getCursor()->getName()) +
                                "' as favorite", 4000);
                    }
                    else {
                        md->set("favorite", "false");
                        s = new GuiInfoPopup(mWindow, "Removed favorite marking for folder '" +
                                Utils::String::removeParenthesis(getCursor()->getName()) +
                                "'", 4000);
                    }

                    mWindow->setInfoPopup(s);
                    getCursor()->getSourceFileData()->getSystem()->onMetaDataSavePoint();

                    if (!Settings::getInstance()->getBool("FoldersOnTop"))
                        mRoot->sort(getSortTypeFromString(mRoot->getSortTypeString()),
                                Settings::getInstance()->getBool("FavoritesFirst"));

                    ViewController::get()->onFileChanged(getCursor(), FILE_METADATA_CHANGED);
                    return true;
                }
                else if (CollectionSystemManager::get()->toggleGameInCollection(getCursor())) {
                    return true;
                }
            }
        }
    }

    return IGameListView::input(config, input);
}
