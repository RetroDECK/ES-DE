//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  BasicGamelistView.cpp
//
//  Interface that defines a GamelistView of the type 'Basic'.
//

#include "views/gamelist/BasicGamelistView.h"

#include "CollectionSystemsManager.h"
#include "Settings.h"
#include "SystemData.h"
#include "utils/FileSystemUtil.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"

BasicGamelistView::BasicGamelistView(Window* window, FileData* root)
    : ISimpleGamelistView(window, root)
    , mList(window)
{
    mList.setSize(mSize.x, mSize.y * 0.8f);
    mList.setPosition(0.0f, mSize.y * 0.2f);
    mList.setDefaultZIndex(20.0f);
    addChild(&mList);

    populateList(root->getChildrenListToDisplay(), root);
}

void BasicGamelistView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
    ISimpleGamelistView::onThemeChanged(theme);
    using namespace ThemeFlags;
    mList.applyTheme(theme, getName(), "gamelist", ALL);

    sortChildren();
}

void BasicGamelistView::onFileChanged(FileData* file, bool reloadGamelist)
{
    if (reloadGamelist) {
        // Might switch to a detailed view.
        ViewController::getInstance()->reloadGamelistView(this);
        return;
    }

    ISimpleGamelistView::onFileChanged(file, reloadGamelist);
}

void BasicGamelistView::populateList(const std::vector<FileData*>& files, FileData* firstEntry)
{
    mFirstGameEntry = nullptr;
    bool favoriteStar = true;
    bool isEditing = false;
    std::string editingCollection;
    std::string inCollectionPrefix;

    if (CollectionSystemsManager::getInstance()->isEditing()) {
        editingCollection = CollectionSystemsManager::getInstance()->getEditingCollection();
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
        for (auto it = files.cbegin(); it != files.cend(); ++it) {
            if (!mFirstGameEntry && (*it)->getType() == GAME)
                mFirstGameEntry = (*it);
            // Add a leading tick mark icon to the game name if it's part of the custom collection
            // currently being edited.
            if (isEditing && (*it)->getType() == GAME) {
                if (CollectionSystemsManager::getInstance()->inCustomCollection(editingCollection,
                                                                                (*it))) {
                    if (Settings::getInstance()->getBool("SpecialCharsASCII"))
                        inCollectionPrefix = "! ";
                    else
                        inCollectionPrefix = ViewController::TICKMARK_CHAR + "  ";
                }
                else {
                    inCollectionPrefix = "";
                }
            }

            if ((*it)->getFavorite() && favoriteStar &&
                mRoot->getSystem()->getName() != "favorites") {
                if (Settings::getInstance()->getBool("SpecialCharsASCII"))
                    mList.add(inCollectionPrefix + "* " + (*it)->getName(), *it,
                              ((*it)->getType() == FOLDER));
                else
                    mList.add(inCollectionPrefix + ViewController::FAVORITE_CHAR + "  " +
                                  (*it)->getName(),
                              *it, ((*it)->getType() == FOLDER));
            }
            else if ((*it)->getType() == FOLDER && mRoot->getSystem()->getName() != "collections") {
                if (Settings::getInstance()->getBool("SpecialCharsASCII"))
                    mList.add("# " + (*it)->getName(), *it, true);
                else
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

void BasicGamelistView::setCursor(FileData* cursor)
{
    if (!mList.setCursor(cursor) && (!cursor->isPlaceHolder())) {
        populateList(cursor->getParent()->getChildrenListToDisplay(), cursor->getParent());
        mList.setCursor(cursor);

        // Update our cursor stack in case our cursor just got set to some folder
        // we weren't in before.
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

void BasicGamelistView::addPlaceholder(FileData* firstEntry)
{
    // Empty list, add a placeholder.
    FileData* placeholder;

    if (firstEntry && firstEntry->getSystem()->isGroupedCustomCollection())
        placeholder = firstEntry->getSystem()->getPlaceholder();
    else
        placeholder = this->mRoot->getSystem()->getPlaceholder();

    mList.add(placeholder->getName(), placeholder, (placeholder->getType() == PLACEHOLDER));
}

void BasicGamelistView::launch(FileData* game)
{
    // This triggers ViewController to launch the game.
    ViewController::getInstance()->triggerGameLaunch(game);
}

void BasicGamelistView::remove(FileData* game, bool deleteFile)
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

void BasicGamelistView::removeMedia(FileData* game)
{
    std::string systemMediaDir = FileData::getMediaDirectory() + game->getSystem()->getName();
    std::string mediaType;
    std::string path;

    // Stop the video player, especially important on Windows as the file would otherwise be locked.
    onStopVideo();

    // If there are no media files left in the directory after the deletion, then remove
    // the directory too. Remove any empty parent directories as well.
    auto removeEmptyDirFunc = [](std::string systemMediaDir, std::string mediaType,
                                 std::string path) {
        std::string parentPath = Utils::FileSystem::getParent(path);
        while (parentPath != systemMediaDir + "/" + mediaType) {
            if (Utils::FileSystem::getDirContent(parentPath).size() == 0) {
                Utils::FileSystem::removeDirectory(parentPath);
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
        Utils::FileSystem::removeFile(path);
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    while (Utils::FileSystem::exists(game->getMiximagePath())) {
        mediaType = "miximages";
        path = game->getMiximagePath();
        Utils::FileSystem::removeFile(path);
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    while (Utils::FileSystem::exists(game->getScreenshotPath())) {
        mediaType = "screenshots";
        path = game->getScreenshotPath();
        Utils::FileSystem::removeFile(path);
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    while (Utils::FileSystem::exists(game->getTitleScreenPath())) {
        mediaType = "titlescreens";
        path = game->getTitleScreenPath();
        Utils::FileSystem::removeFile(path);
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    while (Utils::FileSystem::exists(game->getCoverPath())) {
        mediaType = "covers";
        path = game->getCoverPath();
        Utils::FileSystem::removeFile(path);
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    while (Utils::FileSystem::exists(game->getBackCoverPath())) {
        mediaType = "backcovers";
        path = game->getBackCoverPath();
        Utils::FileSystem::removeFile(path);
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    while (Utils::FileSystem::exists(game->getFanArtPath())) {
        mediaType = "fanart";
        path = game->getFanArtPath();
        Utils::FileSystem::removeFile(path);
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    while (Utils::FileSystem::exists(game->getMarqueePath())) {
        mediaType = "marquees";
        path = game->getMarqueePath();
        Utils::FileSystem::removeFile(path);
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    while (Utils::FileSystem::exists(game->get3DBoxPath())) {
        mediaType = "3dboxes";
        path = game->get3DBoxPath();
        Utils::FileSystem::removeFile(path);
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    while (Utils::FileSystem::exists(game->getPhysicalMediaPath())) {
        mediaType = "physicalmedia";
        path = game->getPhysicalMediaPath();
        Utils::FileSystem::removeFile(path);
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    while (Utils::FileSystem::exists(game->getThumbnailPath())) {
        mediaType = "thumbnails";
        path = game->getThumbnailPath();
        Utils::FileSystem::removeFile(path);
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }
}

std::vector<HelpPrompt> BasicGamelistView::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;

    if (Settings::getInstance()->getBool("QuickSystemSelect") &&
        SystemData::sSystemVector.size() > 1)
        prompts.push_back(HelpPrompt("left/right", "system"));

    if (mRoot->getSystem()->getThemeFolder() == "custom-collections" && mCursorStack.empty() &&
        ViewController::getInstance()->getState().viewing == ViewController::GAME_LIST)
        prompts.push_back(HelpPrompt("a", "enter"));
    else
        prompts.push_back(HelpPrompt("a", "launch"));

    prompts.push_back(HelpPrompt("b", "back"));
    prompts.push_back(HelpPrompt("x", "view media"));

    if (!UIModeController::getInstance()->isUIModeKid())
        prompts.push_back(HelpPrompt("back", "options"));
    if (mRoot->getSystem()->isGameSystem() && Settings::getInstance()->getBool("RandomAddButton"))
        prompts.push_back(HelpPrompt("thumbstickclick", "random"));

    if (mRoot->getSystem()->getThemeFolder() == "custom-collections" &&
        !CollectionSystemsManager::getInstance()->isEditing() && mCursorStack.empty() &&
        ViewController::getInstance()->getState().viewing == ViewController::GAME_LIST &&
        ViewController::getInstance()->getState().viewstyle != ViewController::BASIC) {
        prompts.push_back(HelpPrompt("y", "jump to game"));
    }
    else if (mRoot->getSystem()->isGameSystem() &&
             (mRoot->getSystem()->getThemeFolder() != "custom-collections" ||
              !mCursorStack.empty()) &&
             !UIModeController::getInstance()->isUIModeKid() &&
             !UIModeController::getInstance()->isUIModeKiosk() &&
             (Settings::getInstance()->getBool("FavoritesAddButton") ||
              CollectionSystemsManager::getInstance()->isEditing())) {
        std::string prompt = CollectionSystemsManager::getInstance()->getEditingCollection();
        prompts.push_back(HelpPrompt("y", prompt));
    }
    else if (mRoot->getSystem()->isGameSystem() &&
             mRoot->getSystem()->getThemeFolder() == "custom-collections" &&
             CollectionSystemsManager::getInstance()->isEditing()) {
        std::string prompt = CollectionSystemsManager::getInstance()->getEditingCollection();
        prompts.push_back(HelpPrompt("y", prompt));
    }
    return prompts;
}
