//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GridGameListView.cpp
//
//  Interface that defines a GameListView of the type 'grid'.
//

#include "views/gamelist/GridGameListView.h"

#include "CollectionSystemsManager.h"
#include "Settings.h"
#include "Sound.h"
#include "SystemData.h"
#include "animations/LambdaAnimation.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"

#define FADE_IN_START_OPACITY 0.5f
#define FADE_IN_TIME 650

GridGameListView::GridGameListView(Window* window, FileData* root)
    : ISimpleGameListView(window, root)
    , mGrid(window)
    , mMarquee(window)
    , mImage(window)
    , mLblRating(window)
    , mLblReleaseDate(window)
    , mLblDeveloper(window)
    , mLblPublisher(window)
    , mLblGenre(window)
    , mLblPlayers(window)
    , mLblLastPlayed(window)
    , mLblPlayCount(window)
    , mRating(window)
    , mReleaseDate(window)
    , mDeveloper(window)
    , mPublisher(window)
    , mGenre(window)
    , mPlayers(window)
    , mLastPlayed(window)
    , mPlayCount(window)
    , mName(window)
    , mDescContainer(window)
    , mDescription(window)
    , mGamelistInfo(window)
{
    const float padding = 0.01f;

    mGrid.setPosition(mSize.x * 0.1f, mSize.y * 0.1f);
    mGrid.setDefaultZIndex(20.0f);
    mGrid.setCursorChangedCallback([&](const CursorState& /*state*/) { updateInfoPanel(); });
    addChild(&mGrid);

    populateList(root->getChildrenListToDisplay(), root);

    // Metadata labels + values.
    mLblRating.setText("Rating: ");
    addChild(&mLblRating);
    addChild(&mRating);
    mLblReleaseDate.setText("Released: ");
    addChild(&mLblReleaseDate);
    addChild(&mReleaseDate);
    mLblDeveloper.setText("Developer: ");
    addChild(&mLblDeveloper);
    addChild(&mDeveloper);
    mLblPublisher.setText("Publisher: ");
    addChild(&mLblPublisher);
    addChild(&mPublisher);
    mLblGenre.setText("Genre: ");
    addChild(&mLblGenre);
    addChild(&mGenre);
    mLblPlayers.setText("Players: ");
    addChild(&mLblPlayers);
    addChild(&mPlayers);
    mLblLastPlayed.setText("Last played: ");
    addChild(&mLblLastPlayed);
    mLastPlayed.setDisplayRelative(true);
    addChild(&mLastPlayed);
    mLblPlayCount.setText("Times played: ");
    addChild(&mLblPlayCount);
    addChild(&mPlayCount);

    mName.setPosition(mSize.x, mSize.y);
    mName.setDefaultZIndex(40.0f);
    mName.setColor(0xAAAAAAFF);
    mName.setFont(Font::get(FONT_SIZE_MEDIUM));
    mName.setHorizontalAlignment(ALIGN_CENTER);
    addChild(&mName);

    mDescContainer.setPosition(mSize.x * padding, mSize.y * 0.65f);
    mDescContainer.setSize(mSize.x * (0.50f - 2.0f * padding),
                           mSize.y - mDescContainer.getPosition().y);
    mDescContainer.setAutoScroll(true);
    mDescContainer.setDefaultZIndex(40.0f);
    addChild(&mDescContainer);

    mDescription.setFont(Font::get(FONT_SIZE_SMALL));
    mDescription.setSize(mDescContainer.getSize().x, 0.0f);
    mDescContainer.addChild(&mDescription);

    mMarquee.setOrigin(0.5f, 0.5f);
    mMarquee.setPosition(mSize.x * 0.25f, mSize.y * 0.10f);
    mMarquee.setMaxSize(mSize.x * (0.5f - 2.0f * padding), mSize.y * 0.18f);
    mMarquee.setDefaultZIndex(35.0f);
    mMarquee.setVisible(false);
    addChild(&mMarquee);

    mImage.setOrigin(0.5f, 0.5f);
    mImage.setPosition(2.0f, 2.0f);
    mImage.setMaxSize(mSize.x * (0.50f - 2.0f * padding), mSize.y * 0.4f);
    mImage.setDefaultZIndex(10.0f);
    mImage.setVisible(false);
    addChild(&mImage);

    mGamelistInfo.setOrigin(0.5f, 0.5f);
    mGamelistInfo.setFont(Font::get(FONT_SIZE_SMALL));
    mGamelistInfo.setDefaultZIndex(50.0f);
    mGamelistInfo.setVisible(true);
    addChild(&mGamelistInfo);

    initMDLabels();
    initMDValues();
    updateInfoPanel();
}

void GridGameListView::onFileChanged(FileData* file, bool reloadGameList)
{
    if (reloadGameList) {
        // Might switch to a detailed view.
        ViewController::get()->reloadGameListView(this);
        return;
    }

    ISimpleGameListView::onFileChanged(file, reloadGameList);
}

void GridGameListView::setCursor(FileData* cursor)
{
    if (!mGrid.setCursor(cursor) && (!cursor->isPlaceHolder())) {
        populateList(cursor->getParent()->getChildrenListToDisplay(), cursor->getParent());
        mGrid.setCursor(cursor);

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

bool GridGameListView::input(InputConfig* config, Input input)
{
    if (input.value == 0 &&
        (config->isMappedLike("left", input) || config->isMappedLike("right", input) ||
         (config->isMappedLike("up", input)) || (config->isMappedLike("down", input))))
        NavigationSounds::getInstance()->playThemeNavigationSound(SCROLLSOUND);

    if (input.value != 0 && config->isMappedLike("righttrigger", input)) {
        NavigationSounds::getInstance()->playThemeNavigationSound(SCROLLSOUND);
        mGrid.setCursor(mGrid.getLast());
    }

    if (input.value != 0 && config->isMappedLike("lefttrigger", input)) {
        NavigationSounds::getInstance()->playThemeNavigationSound(SCROLLSOUND);
        mGrid.setCursor(mGrid.getFirst());
    }

    if (config->isMappedLike("left", input) || config->isMappedLike("right", input))
        return GuiComponent::input(config, input);

    return ISimpleGameListView::input(config, input);
}

const std::string GridGameListView::getImagePath(FileData* file)
{
    ImageSource src = mGrid.getImageSource();

    if (src == ImageSource::IMAGE)
        return file->getImagePath();
    else if (src == ImageSource::MIXIMAGE)
        return file->getMiximagePath();
    else if (src == ImageSource::SCREENSHOT)
        return file->getScreenshotPath();
    else if (src == ImageSource::COVER)
        return file->getCoverPath();
    else if (src == ImageSource::MARQUEE)
        return file->getMarqueePath();
    else if (src == ImageSource::BOX3D)
        return file->get3DBoxPath();

    // If no thumbnail was found, then use the image media type.
    if (file->getThumbnailPath() == "")
        return file->getImagePath();

    return file->getThumbnailPath();
}

void GridGameListView::populateList(const std::vector<FileData*>& files, FileData* firstEntry)
{
    firstGameEntry = nullptr;

    mGrid.clear();
    mHeaderText.setText(mRoot->getSystem()->getFullName());
    if (files.size() > 0) {
        for (auto it = files.cbegin(); it != files.cend(); it++) {
            if (!firstGameEntry && (*it)->getType() == GAME)
                firstGameEntry = (*it);
            mGrid.add((*it)->getName(), getImagePath(*it), *it);
        }
    }
    else {
        addPlaceholder(firstEntry);
    }

    generateGamelistInfo(getCursor(), firstEntry);
    generateFirstLetterIndex(files);
}

void GridGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
    ISimpleGameListView::onThemeChanged(theme);

    using namespace ThemeFlags;
    mGrid.applyTheme(theme, getName(), "gamegrid", ALL);
    mName.applyTheme(theme, getName(), "md_name", ALL);
    mMarquee.applyTheme(theme, getName(), "md_marquee",
                        POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION | VISIBLE);
    mImage.applyTheme(theme, getName(), "md_image",
                      POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION | VISIBLE);

    initMDLabels();
    std::vector<TextComponent*> labels = getMDLabels();
    assert(labels.size() == 8);
    std::vector<std::string> lblElements = {
        "md_lbl_rating", "md_lbl_releasedate", "md_lbl_developer",  "md_lbl_publisher",
        "md_lbl_genre",  "md_lbl_players",     "md_lbl_lastplayed", "md_lbl_playcount"};

    for (unsigned int i = 0; i < labels.size(); i++)
        labels[i]->applyTheme(theme, getName(), lblElements[i], ALL);

    initMDValues();
    std::vector<GuiComponent*> values = getMDValues();
    assert(values.size() == 8);
    std::vector<std::string> valElements = {"md_rating",     "md_releasedate", "md_developer",
                                            "md_publisher",  "md_genre",       "md_players",
                                            "md_lastplayed", "md_playcount"};

    for (unsigned int i = 0; i < values.size(); i++)
        values[i]->applyTheme(theme, getName(), valElements[i], ALL ^ ThemeFlags::TEXT);

    mDescContainer.applyTheme(theme, getName(), "md_description",
                              POSITION | ThemeFlags::SIZE | Z_INDEX | VISIBLE);
    mDescription.setSize(mDescContainer.getSize().x, 0.0f);
    mDescription.applyTheme(
        theme, getName(), "md_description",
        ALL ^ (POSITION | ThemeFlags::SIZE | ThemeFlags::ORIGIN | TEXT | ROTATION));

    // Repopulate list in case a new theme is displaying a different image.
    // Preserve selection.
    FileData* file = mGrid.getSelected();
    populateList(mRoot->getChildrenListToDisplay(), mRoot);
    mGrid.setCursor(file);

    mGamelistInfo.applyTheme(theme, getName(), "gamelistInfo", ALL ^ ThemeFlags::TEXT);
    // If there is no position defined in the theme for gamelistInfo, then hide it.
    if (mGamelistInfo.getPosition() == glm::vec3{})
        mGamelistInfo.setVisible(false);
    else
        mGamelistInfo.setVisible(true);

    sortChildren();
}

void GridGameListView::onShow()
{
    GuiComponent::onShow();
    updateInfoPanel();
}

void GridGameListView::initMDLabels()
{
    std::vector<TextComponent*> components = getMDLabels();

    const unsigned int colCount = 2;
    const unsigned int rowCount = static_cast<int>(components.size() / 2);

    glm::vec3 start{mSize.x * 0.01f, mSize.y * 0.625f, 0.0f};

    const float colSize = (mSize.x * 0.48f) / colCount;
    const float rowPadding = 0.01f * mSize.y;

    for (unsigned int i = 0; i < components.size(); i++) {
        const unsigned int row = i % rowCount;
        glm::vec3 pos{};
        if (row == 0) {
            pos = start + glm::vec3{colSize * (i / rowCount), 0.0f, 0.0f};
        }
        else {
            // Work from the last component.
            GuiComponent* lc = components[i - 1];
            pos = lc->getPosition() + glm::vec3{0.0f, lc->getSize().y + rowPadding, 0.0f};
        }

        components[i]->setFont(Font::get(FONT_SIZE_SMALL));
        components[i]->setPosition(pos);
        components[i]->setDefaultZIndex(40.0f);
    }
}

void GridGameListView::initMDValues()
{
    std::vector<TextComponent*> labels = getMDLabels();
    std::vector<GuiComponent*> values = getMDValues();

    std::shared_ptr<Font> defaultFont = Font::get(FONT_SIZE_SMALL);
    mRating.setSize(defaultFont->getHeight() * 5.0f, static_cast<float>(defaultFont->getHeight()));
    mReleaseDate.setFont(defaultFont);
    mDeveloper.setFont(defaultFont);
    mPublisher.setFont(defaultFont);
    mGenre.setFont(defaultFont);
    mPlayers.setFont(defaultFont);
    mLastPlayed.setFont(defaultFont);
    mPlayCount.setFont(defaultFont);

    float bottom = 0.0f;

    const float colSize = (mSize.x * 0.48f) / 2.0f;
    for (unsigned int i = 0; i < labels.size(); i++) {
        const float heightDiff = (labels[i]->getSize().y - values[i]->getSize().y) / 2.0f;
        values[i]->setPosition(labels[i]->getPosition() +
                               glm::vec3{labels[i]->getSize().x, heightDiff, 0.0f});
        values[i]->setSize(colSize - labels[i]->getSize().x, values[i]->getSize().y);
        values[i]->setDefaultZIndex(40.0f);

        float testBot = values[i]->getPosition().y + values[i]->getSize().y;
        if (testBot > bottom)
            bottom = testBot;
    }

    mDescContainer.setPosition(mDescContainer.getPosition().x, bottom + mSize.y * 0.01f);
    mDescContainer.setSize(mDescContainer.getSize().x, mSize.y - mDescContainer.getPosition().y);
}

void GridGameListView::updateInfoPanel()
{
    FileData* file = (mGrid.size() == 0 || mGrid.isScrolling()) ? nullptr : mGrid.getSelected();
    bool hideMetaDataFields = false;

    if (file)
        hideMetaDataFields = (file->metadata.get("hidemetadata") == "true");

    if (hideMetaDataFields) {
        mLblRating.setVisible(false);
        mRating.setVisible(false);
        mLblReleaseDate.setVisible(false);
        mReleaseDate.setVisible(false);
        mLblDeveloper.setVisible(false);
        mDeveloper.setVisible(false);
        mLblPublisher.setVisible(false);
        mPublisher.setVisible(false);
        mLblGenre.setVisible(false);
        mGenre.setVisible(false);
        mLblPlayers.setVisible(false);
        mPlayers.setVisible(false);
        mLblLastPlayed.setVisible(false);
        mLastPlayed.setVisible(false);
        mLblPlayCount.setVisible(false);
        mPlayCount.setVisible(false);
    }
    else {
        mLblRating.setVisible(true);
        mRating.setVisible(true);
        mLblReleaseDate.setVisible(true);
        mReleaseDate.setVisible(true);
        mLblDeveloper.setVisible(true);
        mDeveloper.setVisible(true);
        mLblPublisher.setVisible(true);
        mPublisher.setVisible(true);
        mLblGenre.setVisible(true);
        mGenre.setVisible(true);
        mLblPlayers.setVisible(true);
        mPlayers.setVisible(true);
        mLblLastPlayed.setVisible(true);
        mLastPlayed.setVisible(true);
        mLblPlayCount.setVisible(true);
        mPlayCount.setVisible(true);
    }

    bool fadingOut = false;
    if (file == nullptr) {
        fadingOut = true;
    }
    else {
        mMarquee.setImage(file->getMarqueePath(), false, true);

        // Populate the gamelistInfo field which shows an icon if a folder has been entered
        // as well as the game count for the entire system (total and favorites separately).
        // If a filter has been applied, then the number of filtered and total games replaces
        // the game counter.
        std::string gamelistInfoString;
        Alignment infoAlign = mGamelistInfo.getHorizontalAlignment();

        if (mIsFolder && infoAlign == ALIGN_RIGHT)
            gamelistInfoString = ViewController::FOLDER_CHAR + "  ";

        if (mIsFiltered) {
            if (mFilteredGameCountAll == mFilteredGameCount)
                gamelistInfoString += ViewController::FILTER_CHAR + " " +
                                      std::to_string(mFilteredGameCount) + " / " +
                                      std::to_string(mGameCount);
            else
                gamelistInfoString += ViewController::FILTER_CHAR + " " +
                                      std::to_string(mFilteredGameCount) + " + " +
                                      std::to_string(mFilteredGameCountAll - mFilteredGameCount) +
                                      " / " + std::to_string(mGameCount);
        }
        else {
            gamelistInfoString +=
                ViewController::CONTROLLER_CHAR + " " + std::to_string(mGameCount);
            if (!(file->getSystem()->isCollection() &&
                  file->getSystem()->getFullName() == "favorites"))
                gamelistInfoString += "  " + ViewController::FAVORITE_CHAR + " " +
                                      std::to_string(mFavoritesGameCount);
        }

        if (mIsFolder && infoAlign != ALIGN_RIGHT)
            gamelistInfoString += "  " + ViewController::FOLDER_CHAR;

        mGamelistInfo.setValue(gamelistInfoString);

        // Fade in the game image.
        auto func = [this](float t) {
            mImage.setOpacity(static_cast<unsigned char>(
                glm::mix(static_cast<float>(FADE_IN_START_OPACITY), 1.0f, t) * 255));
        };
        mImage.setAnimation(new LambdaAnimation(func, FADE_IN_TIME), 0, nullptr, false);

        mDescription.setText(file->metadata.get("desc"));
        mDescContainer.reset();

        mRating.setValue(file->metadata.get("rating"));
        mReleaseDate.setValue(file->metadata.get("releasedate"));
        mDeveloper.setValue(file->metadata.get("developer"));
        mPublisher.setValue(file->metadata.get("publisher"));
        mGenre.setValue(file->metadata.get("genre"));
        mPlayers.setValue(file->metadata.get("players"));
        mName.setValue(file->metadata.get("name"));

        if (file->getType() == GAME) {
            if (!hideMetaDataFields) {
                mLastPlayed.setValue(file->metadata.get("lastplayed"));
                mPlayCount.setValue(file->metadata.get("playcount"));
            }
        }
        else if (file->getType() == FOLDER) {
            if (!hideMetaDataFields) {
                mLastPlayed.setValue(file->metadata.get("lastplayed"));
                mLblPlayCount.setVisible(false);
                mPlayCount.setVisible(false);
            }
        }

        fadingOut = false;
    }

    std::vector<GuiComponent*> comps = getMDValues();
    comps.push_back(&mDescription);
    comps.push_back(&mName);
    comps.push_back(&mMarquee);
    comps.push_back(&mImage);
    std::vector<TextComponent*> labels = getMDLabels();
    comps.insert(comps.cend(), labels.cbegin(), labels.cend());

    for (auto it = comps.cbegin(); it != comps.cend(); it++) {
        GuiComponent* comp = *it;
        // An animation is playing, then animate if reverse != fadingOut.
        // An animation is not playing, then animate if opacity != our target opacity.
        if ((comp->isAnimationPlaying(0) && comp->isAnimationReversed(0) != fadingOut) ||
            (!comp->isAnimationPlaying(0) && comp->getOpacity() != (fadingOut ? 0 : 255))) {

            // TEMPORARY - This does not seem to work, needs to be reviewed later.
            // auto func = [comp](float t) {
            auto func = [](float t) {
                // comp->setOpacity(static_cast<unsigned char>(glm::mix(0.0f, 1.0f, t) * 255));
            };
            comp->setAnimation(new LambdaAnimation(func, 150), 0, nullptr, fadingOut);
        }
    }
}

void GridGameListView::addPlaceholder(FileData* firstEntry)
{
    // Empty list, add a placeholder.
    FileData* placeholder;

    if (firstEntry && firstEntry->getSystem()->isGroupedCustomCollection())
        placeholder = firstEntry->getSystem()->getPlaceholder();
    else
        placeholder = this->mRoot->getSystem()->getPlaceholder();

    mGrid.add(placeholder->getName(), "", placeholder);
}

void GridGameListView::launch(FileData* game)
{
    // This triggers ViewController to launch the game.
    ViewController::get()->triggerGameLaunch(game);
}

void GridGameListView::remove(FileData* game, bool deleteFile)
{
    // Delete the game file on the filesystem.
    if (deleteFile)
        Utils::FileSystem::removeFile(game->getPath());

    FileData* parent = game->getParent();
    // Select next element in list, or previous if none.
    if (getCursor() == game) {
        std::vector<FileData*> siblings = parent->getChildrenListToDisplay();
        auto gameIter = std::find(siblings.cbegin(), siblings.cend(), game);
        int gamePos = static_cast<int>(std::distance(siblings.cbegin(), gameIter));
        if (gameIter != siblings.cend()) {
            if ((gamePos + 1) < static_cast<int>(siblings.size()))
                setCursor(siblings.at(gamePos + 1));
            else if ((gamePos - 1) > 0)
                setCursor(siblings.at(gamePos - 1));
        }
    }
    mGrid.remove(game);

    if (mGrid.size() == 0)
        addPlaceholder();

    // If a game has been deleted, immediately remove the entry from gamelist.xml
    // regardless of the value of the setting SaveGamelistsMode.
    game->setDeletionFlag(true);
    parent->getSystem()->writeMetaData();

    // Remove before repopulating (removes from parent), then update the view.
    delete game;
    onFileChanged(parent, false);
}

void GridGameListView::removeMedia(FileData* game)
{
    std::string systemMediaDir = FileData::getMediaDirectory() + game->getSystem()->getName();
    std::string mediaType;
    std::string path;

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
    if (Utils::FileSystem::exists(game->getVideoPath())) {
        mediaType = "videos";
        path = game->getVideoPath();
        Utils::FileSystem::removeFile(path);
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    if (Utils::FileSystem::exists(game->getMiximagePath())) {
        mediaType = "miximages";
        path = game->getMiximagePath();
        Utils::FileSystem::removeFile(path);
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    if (Utils::FileSystem::exists(game->getScreenshotPath())) {
        mediaType = "screenshots";
        path = game->getScreenshotPath();
        Utils::FileSystem::removeFile(path);
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    if (Utils::FileSystem::exists(game->getCoverPath())) {
        mediaType = "covers";
        path = game->getCoverPath();
        Utils::FileSystem::removeFile(path);
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    if (Utils::FileSystem::exists(game->getMarqueePath())) {
        mediaType = "marquees";
        path = game->getMarqueePath();
        Utils::FileSystem::removeFile(path);
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    if (Utils::FileSystem::exists(game->get3DBoxPath())) {
        mediaType = "3dboxes";
        path = game->get3DBoxPath();
        Utils::FileSystem::removeFile(path);
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }

    if (Utils::FileSystem::exists(game->getThumbnailPath())) {
        mediaType = "thumbnails";
        path = game->getThumbnailPath();
        Utils::FileSystem::removeFile(path);
        removeEmptyDirFunc(systemMediaDir, mediaType, path);
    }
}

std::vector<TextComponent*> GridGameListView::getMDLabels()
{
    std::vector<TextComponent*> ret;
    ret.push_back(&mLblRating);
    ret.push_back(&mLblReleaseDate);
    ret.push_back(&mLblDeveloper);
    ret.push_back(&mLblPublisher);
    ret.push_back(&mLblGenre);
    ret.push_back(&mLblPlayers);
    ret.push_back(&mLblLastPlayed);
    ret.push_back(&mLblPlayCount);
    return ret;
}

std::vector<GuiComponent*> GridGameListView::getMDValues()
{
    std::vector<GuiComponent*> ret;
    ret.push_back(&mRating);
    ret.push_back(&mReleaseDate);
    ret.push_back(&mDeveloper);
    ret.push_back(&mPublisher);
    ret.push_back(&mGenre);
    ret.push_back(&mPlayers);
    ret.push_back(&mLastPlayed);
    ret.push_back(&mPlayCount);
    return ret;
}

std::vector<HelpPrompt> GridGameListView::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;

    if (Settings::getInstance()->getBool("QuickSystemSelect"))
        prompts.push_back(HelpPrompt("lr", "system"));
    prompts.push_back(HelpPrompt("up/down/left/right", "choose"));

    if (mRoot->getSystem()->getThemeFolder() == "custom-collections" && mCursorStack.empty() &&
        ViewController::get()->getState().viewing == ViewController::GAME_LIST)
        prompts.push_back(HelpPrompt("a", "enter"));
    else
        prompts.push_back(HelpPrompt("a", "launch"));

    prompts.push_back(HelpPrompt("b", "back"));

    if (mRoot->getSystem()->isGameSystem() &&
        mRoot->getSystem()->getThemeFolder() != "custom-collections")
        prompts.push_back(HelpPrompt("x", "view media"));

    if (mRoot->getSystem()->isGameSystem() && !mCursorStack.empty() &&
        mRoot->getSystem()->getThemeFolder() == "custom-collections")
        prompts.push_back(HelpPrompt("x", "view media"));

    if (!UIModeController::getInstance()->isUIModeKid())
        prompts.push_back(HelpPrompt("back", "options"));
    if (mRoot->getSystem()->isGameSystem() && Settings::getInstance()->getBool("RandomAddButton"))
        prompts.push_back(HelpPrompt("thumbstickclick", "random"));

    if (mRoot->getSystem()->isGameSystem() &&
        (mRoot->getSystem()->getThemeFolder() != "custom-collections" || !mCursorStack.empty()) &&
        !UIModeController::getInstance()->isUIModeKid() &&
        !UIModeController::getInstance()->isUIModeKiosk() &&
        (Settings::getInstance()->getBool("FavoritesAddButton") ||
         CollectionSystemsManager::get()->isEditing())) {
        std::string prompt = CollectionSystemsManager::get()->getEditingCollection();
        prompts.push_back(HelpPrompt("y", prompt));
    }
    else if (mRoot->getSystem()->isGameSystem() &&
             mRoot->getSystem()->getThemeFolder() == "custom-collections" &&
             CollectionSystemsManager::get()->isEditing()) {
        std::string prompt = CollectionSystemsManager::get()->getEditingCollection();
        prompts.push_back(HelpPrompt("y", prompt));
    }
    return prompts;
}

void GridGameListView::update(int deltaTime)
{
    // Update.
    ISimpleGameListView::update(deltaTime);
}
