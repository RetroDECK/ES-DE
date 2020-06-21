//
//  BasicGameListView.cpp
//
//  Interface that defines a GameListView of the type 'Basic'.
//

#include "views/gamelist/BasicGameListView.h"

#include "utils/FileSystemUtil.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "Settings.h"
#include "SystemData.h"

BasicGameListView::BasicGameListView(
        Window* window,
        FileData* root)
        : ISimpleGameListView(window, root),
        mList(window)
{
    mList.setSize(mSize.x(), mSize.y() * 0.8f);
    mList.setPosition(0, mSize.y() * 0.2f);
    mList.setDefaultZIndex(20);
    addChild(&mList);

    populateList(root->getChildrenListToDisplay());
}

void BasicGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
    ISimpleGameListView::onThemeChanged(theme);
    using namespace ThemeFlags;
    mList.applyTheme(theme, getName(), "gamelist", ALL);

    sortChildren();
}

void BasicGameListView::onFileChanged(FileData* file, FileChangeType change)
{
    if (change == FILE_METADATA_CHANGED)
    {
        // Might switch to a detailed view.
        ViewController::get()->reloadGameListView(this);
        return;
    }

    ISimpleGameListView::onFileChanged(file, change);
}

void BasicGameListView::populateList(const std::vector<FileData*>& files)
{
    mList.clear();
    mHeaderText.setText(mRoot->getSystem()->getFullName());
    if (files.size() > 0) {
        for (auto it = files.cbegin(); it != files.cend(); it++) {
            if ((*it)->getFavorite() &&
                    mRoot->getSystem()->getName() != "favorites") {
                mList.add(FAVORITE_GAME_CHAR + "  " + (*it)->getName(),
                    *it, ((*it)->getType() == FOLDER));
            }
            else if ((*it)->getType() == FOLDER &&
                    mRoot->getSystem()->getName() != "collections") {
                mList.add(FAVORITE_FOLDER_CHAR + "  " + (*it)->getName(), *it, true);
            }
            else {
                mList.add((*it)->getName(), *it, ((*it)->getType() == FOLDER));
            }
        }
    }
    else {
        addPlaceholder();
    }
}

FileData* BasicGameListView::getCursor()
{
    return mList.getSelected();
}

void BasicGameListView::setCursor(FileData* cursor)
{
    if (!mList.setCursor(cursor) && (!cursor->isPlaceHolder())) {
        populateList(cursor->getParent()->getChildrenListToDisplay());
        mList.setCursor(cursor);

        // Update our cursor stack in case our cursor just
        // got set to some folder we weren't in before.
        if (mCursorStack.empty() || mCursorStack.top() != cursor->getParent())
        {
            std::stack<FileData*> tmp;
            FileData* ptr = cursor->getParent();
            while (ptr && ptr != mRoot)
            {
                tmp.push(ptr);
                ptr = ptr->getParent();
            }

            // Flip the stack and put it in mCursorStack.
            mCursorStack = std::stack<FileData*>();
            while (!tmp.empty())
            {
                mCursorStack.push(tmp.top());
                tmp.pop();
            }
        }
    }
}

FileData* BasicGameListView::getFirstEntry()
{
    return mList.getFirst();
}

FileData* BasicGameListView::getLastEntry()
{
    return mList.getLast();
}

void BasicGameListView::addPlaceholder()
{
    // Empty list - add a placeholder.
    FileData* placeholder = new FileData(PLACEHOLDER, "<No Entries Found>",
            this->mRoot->getSystem()->getSystemEnvData(), this->mRoot->getSystem());
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
    ViewController::get()->launch(game);
}

void BasicGameListView::remove(FileData *game, bool deleteFile)
{
    // Actually delete the file on the filesystem.
    if (deleteFile)
        Utils::FileSystem::removeFile(game->getPath());

    FileData* parent = game->getParent();
    // Select next element in list, or previous if none.
    if (getCursor() == game) {
        std::vector<FileData*> siblings = parent->getChildrenListToDisplay();
        auto gameIter = std::find(siblings.cbegin(), siblings.cend(), game);
        unsigned int gamePos = (int)std::distance(siblings.cbegin(), gameIter);
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

    // Remove before repopulating (removes from parent), then update the view.
    delete game;
    onFileChanged(parent, FILE_REMOVED);
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
    if (mRoot->getSystem()->isGameSystem() && !UIModeController::getInstance()->isUIModeKid())
    {
        std::string prompt = CollectionSystemManager::get()->getEditingCollection();
        prompts.push_back(HelpPrompt("y", prompt));
    }
    return prompts;
}
