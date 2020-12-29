//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  BasicGameListView.cpp
//
//  Interface that defines a GameListView of the type 'Basic'.
//

#include "views/gamelist/BasicGameListView.h"

#include "utils/FileSystemUtil.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "CollectionSystemsManager.h"
#include "Settings.h"
#include "SystemData.h"

BasicGameListView::BasicGameListView(Window* window, FileData* root)
        : ISimpleGameListView(window, root), mList(window)
{
    mList.setSize(mSize.x(), mSize.y() * 0.8f);
    mList.setPosition(0, mSize.y() * 0.2f);
    mList.setDefaultZIndex(20);
    addChild(&mList);

    populateList(root->getChildrenListToDisplay(), root);
}

void BasicGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
    ISimpleGameListView::onThemeChanged(theme);
    using namespace ThemeFlags;
    mList.applyTheme(theme, getName(), "gamelist", ALL);

    sortChildren();
}

void BasicGameListView::onFileChanged(FileData* file, bool reloadGameList)
{
    if (reloadGameList) {
        // Might switch to a detailed view.
        ViewController::get()->reloadGameListView(this);
        return;
    }

    ISimpleGameListView::onFileChanged(file, reloadGameList);
}

void BasicGameListView::populateList(const std::vector<FileData*>& files, FileData* firstEntry)
{
    mFirstGameEntry = nullptr;
    bool favoriteStar = true;
    bool isEditing = false;
    std::string editingCollection;
    std::string inCollectionPrefix;

    if (CollectionSystemsManager::get()->isEditing()) {
        editingCollection = CollectionSystemsManager::get()->getEditingCollection();
        isEditing = true;
    }

    // Read the settings that control whether a unicode star character should be added
    // as a prefix to the game name.
    if (files.size() > 0) {
        if (files.front()->getSystem()->isCustomCollection())
            favoriteStar = Settings::getInstance()->getBool("FavStarCustom");
        else
            favoriteStar = Settings::getInstance()->getBool("FavoritesStar");
    }

    mList.clear();
    mHeaderText.setText(mRoot->getSystem()->getFullName());
    if (files.size() > 0) {
        for (auto it = files.cbegin(); it != files.cend(); it++) {
            if (!mFirstGameEntry && (*it)->getType() == GAME)
                mFirstGameEntry = (*it);
            // Add a leading tick mark icon to the game name if it's part of the custom collection
            // currently being edited.
            if (isEditing && (*it)->getType() == GAME) {
                if (CollectionSystemsManager::get()->inCustomCollection(editingCollection, (*it)))
                    inCollectionPrefix = ViewController::TICKMARK_CHAR + "  ";
                else
                    inCollectionPrefix = "";
            }
            if ((*it)->getFavorite() && favoriteStar &&
                    mRoot->getSystem()->getName() != "favorites") {
                mList.add(inCollectionPrefix + ViewController::FAVORITE_CHAR + "  " +
                    (*it)->getName(), *it, ((*it)->getType() == FOLDER));
            }
            else if ((*it)->getType() == FOLDER &&
                    mRoot->getSystem()->getName() != "collections") {
                mList.add(ViewController::FOLDER_CHAR + "  " + (*it)->getName(), *it, true);
            }
            else {
                mList.add(inCollectionPrefix + (*it)->getName(), *it, ((*it)->getType() == FOLDER));
            }
        }
    }
    else {
        addPlaceholder(firstEntry);
    }

    generateGamelistInfo(getCursor(), firstEntry);
    generateFirstLetterIndex(files);
}

FileData* BasicGameListView::getCursor()
{
    return mList.getSelected();
}

void BasicGameListView::setCursor(FileData* cursor)
{
    if (!mList.setCursor(cursor) && (!cursor->isPlaceHolder())) {
        populateList(cursor->getParent()->getChildrenListToDisplay(), cursor->getParent());
        mList.setCursor(cursor);

        // Update our cursor stack in case our cursor just
        // got set to some folder we weren't in before.
        if (mCursorStack.empty() || mCursorStack.top() != cursor->getParent()) {
            std::stack<FileData*> tmp;
            FileData* ptr = cursor->getParent();
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

FileData* BasicGameListView::getNextEntry()
{
    return mList.getNext();
}

FileData* BasicGameListView::getPreviousEntry()
{
    return mList.getPrevious();
}

FileData* BasicGameListView::getFirstEntry()
{
    return mList.getFirst();
}

FileData* BasicGameListView::getLastEntry()
{
    return mList.getLast();
}

FileData* BasicGameListView::getFirstGameEntry()
{
    return mFirstGameEntry;
}

void BasicGameListView::addPlaceholder(FileData* firstEntry)
{
    // Empty list - add a placeholder.
    SystemData* system;
    if (firstEntry && firstEntry->getSystem()->isGroupedCustomCollection())
        system = firstEntry->getSystem();
    else
        system = this->mRoot->getSystem();

    FileData* placeholder = new FileData(PLACEHOLDER, "<No Entries Found>",
            this->mRoot->getSystem()->getSystemEnvData(), system);
    mList.add(placeholder->getName(), placeholder, (placeholder->getType() == PLACEHOLDER));
}

std::string BasicGameListView::getQuickSystemSelectRightButton()
{
    return "right";
}

std::string BasicGameListView::getQuickSystemSelectLeftButton()
{
    return "left";
}

void BasicGameListView::launch(FileData* game)
{
    ViewController::get()->triggerGameLaunch(game);
}

void BasicGameListView::remove(FileData *game, bool deleteFile)
{
    // Delete the game file on the filesystem.
    if (deleteFile)
        Utils::FileSystem::removeFile(game->getPath());

    FileData* parent = game->getParent();
    // Select next element in list, or previous if none.
    if (getCursor() == game) {
        std::vector<FileData*> siblings = parent->getChildrenListToDisplay();
        auto gameIter = std::find(siblings.cbegin(), siblings.cend(), game);
        unsigned int gamePos = static_cast<int>(std::distance(siblings.cbegin(), gameIter));
        if (gameIter != siblings.cend()) {
            if ((gamePos + 1) < siblings.size())
                setCursor(siblings.at(gamePos + 1));
            else if (gamePos > 1)
                setCursor(siblings.at(gamePos - 1));
        }
    }
    mList.remove(game);

    if (mList.size() == 0)
        addPlaceholder();

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

void BasicGameListView::removeMedia(FileData *game)
{
    // Remove all game media files on the filesystem.
    if (Utils::FileSystem::exists(game->getVideoPath()))
        Utils::FileSystem::removeFile(game->getVideoPath());

    if (Utils::FileSystem::exists(game->getMiximagePath()))
        Utils::FileSystem::removeFile(game->getMiximagePath());

    if (Utils::FileSystem::exists(game->getScreenshotPath()))
        Utils::FileSystem::removeFile(game->getScreenshotPath());

    if (Utils::FileSystem::exists(game->getCoverPath()))
        Utils::FileSystem::removeFile(game->getCoverPath());

    if (Utils::FileSystem::exists(game->getMarqueePath()))
        Utils::FileSystem::removeFile(game->getMarqueePath());

    if (Utils::FileSystem::exists(game->get3DBoxPath()))
        Utils::FileSystem::removeFile(game->get3DBoxPath());

    if (Utils::FileSystem::exists(game->getThumbnailPath()))
        Utils::FileSystem::removeFile(game->getThumbnailPath());
}

std::vector<HelpPrompt> BasicGameListView::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;

    if (Settings::getInstance()->getBool("QuickSystemSelect"))
        prompts.push_back(HelpPrompt("left/right", "system"));
    prompts.push_back(HelpPrompt("up/down", "choose"));
    prompts.push_back(HelpPrompt("a", "launch"));
    prompts.push_back(HelpPrompt("b", "back"));
    if (!UIModeController::getInstance()->isUIModeKid())
        prompts.push_back(HelpPrompt("select", "options"));
    if (mRoot->getSystem()->isGameSystem())
        prompts.push_back(HelpPrompt("x", "random"));
    if (mRoot->getSystem()->isGameSystem() && !UIModeController::getInstance()->isUIModeKid() &&
                !UIModeController::getInstance()->isUIModeKiosk() &&
                (Settings::getInstance()->getBool("FavoritesAddButton") ||
                CollectionSystemsManager::get()->isEditing())) {
        std::string prompt = CollectionSystemsManager::get()->getEditingCollection();
        prompts.push_back(HelpPrompt("y", prompt));
    }
    return prompts;
}
