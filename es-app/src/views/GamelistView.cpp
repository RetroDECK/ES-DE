//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GamelistView.cpp
//
//  Main gamelist logic.
//

#include "views/GamelistView.h"

#include "CollectionSystemsManager.h"
#include "UIModeController.h"
#include "animations/LambdaAnimation.h"

#define FADE_IN_START_OPACITY 0.5f
#define FADE_IN_TIME 650

GamelistView::GamelistView(FileData* root)
    : GamelistBase {root}
    , mViewStyle {ViewController::BASIC}
    , mVideo {nullptr}
{
    mViewStyle = ViewController::getInstance()->getState().viewstyle;

    mHeaderText.setText("Logo Text", false);
    mHeaderText.setSize(mSize.x, 0.0f);
    mHeaderText.setPosition(0.0f, 0.0f);
    mHeaderText.setHorizontalAlignment(ALIGN_CENTER);
    mHeaderText.setDefaultZIndex(50.0f);

    mHeaderText.setText(mRoot->getSystem()->getFullName());

    mHeaderImage.setResize(0.0f, mSize.y * 0.185f);
    mHeaderImage.setOrigin(0.5f, 0.0f);
    mHeaderImage.setPosition(mSize.x / 2.0f, 0.0f);
    mHeaderImage.setDefaultZIndex(50.0f);

    mBackground.setResize(mSize.x, mSize.y);
    mBackground.setDefaultZIndex(0.0f);

    addChild(&mHeaderText);
    addChild(&mBackground);

    const float padding = 0.01f;

    if (mViewStyle == ViewController::VIDEO) {
        // Create the video window.
        mVideo = new VideoFFmpegComponent;
    }

    mList.setPosition(mSize.x * (0.50f + padding), mList.getPosition().y);
    mList.setSize(mSize.x * (0.50f - padding), mList.getSize().y);
    mList.setAlignment(TextListComponent<FileData*>::ALIGN_LEFT);
    mList.setCursorChangedCallback([&](const CursorState& /*state*/) { updateInfoPanel(); });

    // Thumbnail.
    mThumbnail.setOrigin(0.5f, 0.5f);
    mThumbnail.setPosition(2.0f, 2.0f);
    mThumbnail.setVisible(false);
    mThumbnail.setMaxSize(mSize.x * (0.25f - 2.0f * padding), mSize.y * 0.10f);
    mThumbnail.setDefaultZIndex(25.0f);
    addChild(&mThumbnail);

    // Marquee.
    mMarquee.setOrigin(0.5f, 0.5f);
    // Default to off the screen.
    mMarquee.setPosition(2.0f, 2.0f);
    mMarquee.setVisible(false);
    mMarquee.setMaxSize(mSize.x * (0.5f - 2.0f * padding), mSize.y * 0.18f);
    mMarquee.setDefaultZIndex(35.0f);
    addChild(&mMarquee);

    // Image.
    mImage.setOrigin(0.5f, 0.5f);
    mImage.setPosition(mSize.x * 0.25f, mList.getPosition().y + mSize.y * 0.2125f);
    mImage.setMaxSize(mSize.x * (0.50f - 2.0f * padding), mSize.y * 0.4f);
    mImage.setDefaultZIndex(30.0f);
    addChild(&mImage);

    if (mViewStyle == ViewController::VIDEO) {
        // Video.
        mVideo->setOrigin(0.5f, 0.5f);
        mVideo->setPosition(mSize.x * 0.25f, mSize.y * 0.4f);
        mVideo->setSize(mSize.x * (0.5f - 2.0f * padding), mSize.y * 0.4f);
        mVideo->setDefaultZIndex(30.0f);
        addChild(mVideo);
    }

    // Metadata labels + values.
    mLblRating.setText("Rating: ", false);
    addChild(&mLblRating);
    addChild(&mRating);
    mLblReleaseDate.setText("Released: ", false);
    addChild(&mLblReleaseDate);
    addChild(&mReleaseDate);
    mLblDeveloper.setText("Developer: ", false);
    addChild(&mLblDeveloper);
    addChild(&mDeveloper);
    mLblPublisher.setText("Publisher: ", false);
    addChild(&mLblPublisher);
    addChild(&mPublisher);
    mLblGenre.setText("Genre: ", false);
    addChild(&mLblGenre);
    addChild(&mGenre);
    mLblPlayers.setText("Players: ", false);
    addChild(&mLblPlayers);
    addChild(&mPlayers);
    mLblLastPlayed.setText("Last played: ", false);
    addChild(&mLblLastPlayed);
    mLastPlayed.setDisplayRelative(true);
    addChild(&mLastPlayed);
    mLblPlayCount.setText("Times played: ", false);
    addChild(&mLblPlayCount);
    addChild(&mPlayCount);

    // Badges.
    addChild(&mBadges);
    mBadges.setOrigin(0.5f, 0.5f);
    mBadges.setPosition(mSize.x * 0.8f, mSize.y * 0.7f);
    mBadges.setSize(mSize.x * 0.15f, mSize.y * 0.2f);
    mBadges.setDefaultZIndex(50.0f);

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

    mGamelistInfo.setOrigin(0.5f, 0.5f);
    mGamelistInfo.setFont(Font::get(FONT_SIZE_SMALL));
    mGamelistInfo.setDefaultZIndex(50.0f);
    mGamelistInfo.setVisible(true);
    addChild(&mGamelistInfo);

    initMDLabels();
    initMDValues();
}

GamelistView::~GamelistView()
{
    // Remove theme extras.
    for (auto extra : mThemeExtras) {
        removeChild(extra);
        delete extra;
    }
    mThemeExtras.clear();

    if (mViewStyle == ViewController::VIDEO && mVideo != nullptr)
        delete mVideo;
}

void GamelistView::onFileChanged(FileData* file, bool reloadGamelist)
{
    if (reloadGamelist) {
        // Might switch to a detailed view.
        ViewController::getInstance()->reloadGamelistView(this);
        return;
    }

    // We could be tricky here to be efficient;
    // but this shouldn't happen very often so we'll just always repopulate.
    FileData* cursor {getCursor()};
    if (!cursor->isPlaceHolder()) {
        populateList(cursor->getParent()->getChildrenListToDisplay(), cursor->getParent());
        setCursor(cursor);
    }
    else {
        populateList(mRoot->getChildrenListToDisplay(), mRoot);
        setCursor(cursor);
    }
}

void GamelistView::onShow()
{
    // Reset any Lottie animations.
    for (auto extra : mThemeExtras)
        extra->resetFileAnimation();

    mLastUpdated = nullptr;
    GuiComponent::onShow();
    updateInfoPanel();
}

void GamelistView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
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
    mThemeExtras = ThemeData::makeExtras(theme, getName());
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

    mList.applyTheme(theme, getName(), "gamelist", ALL);

    mThumbnail.applyTheme(theme, getName(), "md_thumbnail",
                          POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION | VISIBLE);
    mMarquee.applyTheme(theme, getName(), "md_marquee",
                        POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION | VISIBLE);
    mImage.applyTheme(theme, getName(), "md_image",
                      POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION | VISIBLE);

    if (mViewStyle == ViewController::VIDEO) {
        mVideo->applyTheme(theme, getName(), "md_video",
                           POSITION | ThemeFlags::SIZE | ThemeFlags::DELAY | Z_INDEX | ROTATION |
                               VISIBLE);
    }
    mName.applyTheme(theme, getName(), "md_name", ALL);
    mBadges.applyTheme(theme, getName(), "md_badges", ALL);

    initMDLabels();
    std::vector<TextComponent*> labels {getMDLabels()};
    assert(labels.size() == 8);
    std::vector<std::string> lblElements = {
        "md_lbl_rating", "md_lbl_releasedate", "md_lbl_developer",  "md_lbl_publisher",
        "md_lbl_genre",  "md_lbl_players",     "md_lbl_lastplayed", "md_lbl_playcount"};

    for (unsigned int i = 0; i < labels.size(); ++i)
        labels[i]->applyTheme(theme, getName(), lblElements[i], ALL);

    initMDValues();
    std::vector<GuiComponent*> values {getMDValues()};
    assert(values.size() == 8);
    std::vector<std::string> valElements = {"md_rating",     "md_releasedate", "md_developer",
                                            "md_publisher",  "md_genre",       "md_players",
                                            "md_lastplayed", "md_playcount"};

    for (unsigned int i = 0; i < values.size(); ++i)
        values[i]->applyTheme(theme, getName(), valElements[i], ALL ^ ThemeFlags::TEXT);

    mDescContainer.applyTheme(theme, getName(), "md_description",
                              POSITION | ThemeFlags::SIZE | Z_INDEX | VISIBLE);
    mDescription.setSize(mDescContainer.getSize().x, 0.0f);
    mDescription.applyTheme(
        theme, getName(), "md_description",
        ALL ^ (POSITION | ThemeFlags::SIZE | ThemeFlags::ORIGIN | TEXT | ROTATION));

    mGamelistInfo.applyTheme(theme, getName(), "gamelistInfo", ALL ^ ThemeFlags::TEXT);
    // If there is no position defined in the theme for gamelistInfo, then hide it.
    if (mGamelistInfo.getPosition() == glm::vec3 {})
        mGamelistInfo.setVisible(false);
    else
        mGamelistInfo.setVisible(true);

    sortChildren();
}

void GamelistView::update(int deltaTime)
{
    if (ViewController::getInstance()->getGameLaunchTriggered() && mImage.isAnimationPlaying(0))
        mImage.finishAnimation(0);

    if (mViewStyle == ViewController::VIDEO) {
        if (!mVideoPlaying)
            mVideo->onHide();
        else if (mVideoPlaying && !mVideo->isVideoPaused() && !mWindow->isScreensaverActive())
            mVideo->onShow();

        if (ViewController::getInstance()->getGameLaunchTriggered() &&
            mVideo->isAnimationPlaying(0))
            mVideo->finishAnimation(0);
    }

    updateChildren(deltaTime);
}

void GamelistView::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans {parentTrans * getTransform()};

    float scaleX {trans[0].x};
    float scaleY {trans[1].y};

    glm::ivec2 pos {static_cast<int>(std::round(trans[3].x)),
                    static_cast<int>(std::round(trans[3].y))};
    glm::ivec2 size {static_cast<int>(std::round(mSize.x * scaleX)),
                     static_cast<int>(std::round(mSize.y * scaleY))};

    Renderer::pushClipRect(pos, size);
    renderChildren(trans);
    Renderer::popClipRect();
}

HelpStyle GamelistView::getHelpStyle()
{
    HelpStyle style;
    style.applyTheme(mTheme, getName());
    return style;
}

std::vector<HelpPrompt> GamelistView::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;

    if (Settings::getInstance()->getBool("QuickSystemSelect") &&
        SystemData::sSystemVector.size() > 1)
        prompts.push_back(HelpPrompt("left/right", "system"));

    if (mRoot->getSystem()->getThemeFolder() == "custom-collections" && mCursorStack.empty() &&
        ViewController::getInstance()->getState().viewing == ViewController::GAMELIST)
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
        ViewController::getInstance()->getState().viewing == ViewController::GAMELIST &&
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

void GamelistView::updateInfoPanel()
{
    FileData* file {(mList.size() == 0 || mList.isScrolling()) ? nullptr : mList.getSelected()};

    // If the game data has already been rendered to the info panel, then skip it this time.
    if (file == mLastUpdated)
        return;

    if (!mList.isScrolling())
        mLastUpdated = file;

    bool hideMetaDataFields {false};

    if (file) {
        // Always hide the metadata fields if browsing grouped custom collections.
        if (file->getSystem()->isCustomCollection() &&
            file->getPath() == file->getSystem()->getName())
            hideMetaDataFields = true;
        else
            hideMetaDataFields = (file->metadata.get("hidemetadata") == "true");

        // Always hide the metadata fields for placeholders as well.
        if (file->getType() == PLACEHOLDER) {
            hideMetaDataFields = true;
            mLastUpdated = nullptr;
        }
    }

    // If we're scrolling, hide the metadata fields if the last game had this options set,
    // or if we're in the grouped custom collection view.
    if (mList.isScrolling())
        if ((mLastUpdated && mLastUpdated->metadata.get("hidemetadata") == "true") ||
            (mLastUpdated->getSystem()->isCustomCollection() &&
             mLastUpdated->getPath() == mLastUpdated->getSystem()->getName()))
            hideMetaDataFields = true;

    if (hideMetaDataFields || mViewStyle == ViewController::BASIC) {
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
        mBadges.setVisible(false);
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
        mBadges.setVisible(true);
    }

    bool fadingOut = false;
    if (file == nullptr) {
        mVideoPlaying = false;
        fadingOut = true;
    }
    else {
        // If we're browsing a grouped custom collection, then update the folder metadata
        // which will generate a description of three random games and return a pointer to
        // the first of these so that we can display its game media.
        if (file->getSystem()->isCustomCollection() &&
            file->getPath() == file->getSystem()->getName()) {
            mRandomGame = CollectionSystemsManager::getInstance()->updateCollectionFolderMetadata(
                file->getSystem());
            if (mRandomGame) {
                mThumbnail.setImage(mRandomGame->getThumbnailPath());
                mMarquee.setImage(mRandomGame->getMarqueePath(), false, true);
                mImage.setImage(mRandomGame->getImagePath());
                if (mViewStyle == ViewController::VIDEO) {
                    mVideo->setImage(mRandomGame->getImagePath());
                    // Always stop the video before setting a new video as it will otherwise
                    // continue to play if it has the same path (i.e. it is the same physical video
                    // file) as the previously set video. That may happen when entering a folder
                    // with the same name as the first game file inside, or as in this case, when
                    // entering a custom collection.
                    mVideo->onHide();

                    if (!mVideo->setVideo(mRandomGame->getVideoPath()))
                        mVideo->setDefaultVideo();
                }
            }
            else {
                mThumbnail.setImage("");
                mMarquee.setImage("");
                mImage.setImage("");
                if (mViewStyle == ViewController::VIDEO) {
                    mVideo->setImage("");
                    mVideo->setVideo("");
                    mVideo->setDefaultVideo();
                }
            }
        }
        else {
            mThumbnail.setImage(file->getThumbnailPath());
            mMarquee.setImage(file->getMarqueePath(), false, true);
            mImage.setImage(file->getImagePath());
            if (mViewStyle == ViewController::VIDEO) {
                mVideo->setImage(file->getImagePath());
                mVideo->onHide();

                if (!mVideo->setVideo(file->getVideoPath()))
                    mVideo->setDefaultVideo();
            }
        }

        mVideoPlaying = true;

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

        if (mViewStyle == ViewController::DETAILED) {
            // Fade in the game image.
            auto func = [this](float t) {
                mImage.setOpacity(static_cast<unsigned char>(
                    glm::mix(static_cast<float>(FADE_IN_START_OPACITY), 1.0f, t) * 255));
            };
            mImage.setAnimation(new LambdaAnimation(func, FADE_IN_TIME), 0, nullptr, false);
        }
        else if (mViewStyle == ViewController::VIDEO) {
            // Fade in the static image.
            auto func = [this](float t) {
                mVideo->setOpacity(static_cast<unsigned char>(
                    glm::mix(static_cast<float>(FADE_IN_START_OPACITY), 1.0f, t) * 255));
            };
            mVideo->setAnimation(new LambdaAnimation(func, FADE_IN_TIME), 0, nullptr, false);
        }

        mDescription.setText(file->metadata.get("desc"));
        mDescContainer.reset();

        mRating.setValue(file->metadata.get("rating"));
        mReleaseDate.setValue(file->metadata.get("releasedate"));
        mDeveloper.setValue(file->metadata.get("developer"));
        mPublisher.setValue(file->metadata.get("publisher"));
        mGenre.setValue(file->metadata.get("genre"));
        mPlayers.setValue(file->metadata.get("players"));

        // Populate the badge slots based on game metadata.
        std::vector<BadgeComponent::BadgeInfo> badgeSlots;
        for (auto badge : mBadges.getBadgeTypes()) {
            BadgeComponent::BadgeInfo badgeInfo;
            badgeInfo.badgeType = badge;
            if (badge == "controller") {
                if (file->metadata.get("controller").compare("") != 0) {
                    badgeInfo.gameController = file->metadata.get("controller");
                    badgeSlots.push_back(badgeInfo);
                }
            }
            else if (badge == "altemulator") {
                if (file->metadata.get(badge).compare("") != 0)
                    badgeSlots.push_back(badgeInfo);
            }
            else {
                if (file->metadata.get(badge).compare("true") == 0)
                    badgeSlots.push_back(badgeInfo);
            }
        }
        mBadges.setBadges(badgeSlots);

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
    comps.push_back(&mThumbnail);
    comps.push_back(&mMarquee);
    comps.push_back(&mImage);
    comps.push_back(&mDescription);
    comps.push_back(&mName);
    comps.push_back(&mBadges);
    std::vector<TextComponent*> labels = getMDLabels();
    comps.insert(comps.cend(), labels.cbegin(), labels.cend());

    for (auto it = comps.cbegin(); it != comps.cend(); ++it) {
        GuiComponent* comp = *it;
        // An animation is playing, then animate if reverse != fadingOut.
        // An animation is not playing, then animate if opacity != our target opacity.
        if ((comp->isAnimationPlaying(0) && comp->isAnimationReversed(0) != fadingOut) ||
            (!comp->isAnimationPlaying(0) && comp->getOpacity() != (fadingOut ? 0 : 255))) {
            auto func = [comp](float t) {
                comp->setOpacity(static_cast<unsigned char>(glm::mix(0.0f, 1.0f, t) * 255));
            };
            comp->setAnimation(new LambdaAnimation(func, 150), 0, nullptr, fadingOut);
        }
    }
}

void GamelistView::initMDLabels()
{
    std::vector<TextComponent*> components {getMDLabels()};

    const unsigned int colCount {2};
    const unsigned int rowCount {static_cast<unsigned int>(components.size() / 2)};

    glm::vec3 start {mSize.x * 0.01f, mSize.y * 0.625f, 0.0f};

    const float colSize {(mSize.x * 0.48f) / colCount};
    const float rowPadding {0.01f * mSize.y};

    for (unsigned int i = 0; i < components.size(); ++i) {
        const unsigned int row = i % rowCount;
        glm::vec3 pos {};
        if (row == 0) {
            pos = start + glm::vec3 {colSize * (i / rowCount), 0.0f, 0.0f};
        }
        else {
            // Work from the last component.
            GuiComponent* lc {components[i - 1]};
            pos = lc->getPosition() + glm::vec3 {0.0f, lc->getSize().y + rowPadding, 0.0f};
        }

        components[i]->setFont(Font::get(FONT_SIZE_SMALL));
        components[i]->setPosition(pos);
        components[i]->setDefaultZIndex(40.0f);
    }
}

void GamelistView::initMDValues()
{
    std::vector<TextComponent*> labels {getMDLabels()};
    std::vector<GuiComponent*> values {getMDValues()};

    std::shared_ptr<Font> defaultFont {Font::get(FONT_SIZE_SMALL)};
    mRating.setSize(defaultFont->getHeight() * 5.0f, static_cast<float>(defaultFont->getHeight()));
    mReleaseDate.setFont(defaultFont);
    mDeveloper.setFont(defaultFont);
    mPublisher.setFont(defaultFont);
    mGenre.setFont(defaultFont);
    mPlayers.setFont(defaultFont);
    mLastPlayed.setFont(defaultFont);
    mPlayCount.setFont(defaultFont);

    float bottom {0.0f};

    const float colSize {(mSize.x * 0.48f) / 2.0f};
    for (unsigned int i = 0; i < labels.size(); ++i) {
        const float heightDiff = (labels[i]->getSize().y - values[i]->getSize().y) / 2.0f;
        values[i]->setPosition(labels[i]->getPosition() +
                               glm::vec3 {labels[i]->getSize().x, heightDiff, 0.0f});
        values[i]->setSize(colSize - labels[i]->getSize().x, values[i]->getSize().y);
        values[i]->setDefaultZIndex(40.0f);

        float testBot = values[i]->getPosition().y + values[i]->getSize().y;

        if (testBot > bottom)
            bottom = testBot;
    }

    mDescContainer.setPosition(mDescContainer.getPosition().x, bottom + mSize.y * 0.01f);
    mDescContainer.setSize(mDescContainer.getSize().x, mSize.y - mDescContainer.getPosition().y);
}

std::vector<TextComponent*> GamelistView::getMDLabels()
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

std::vector<GuiComponent*> GamelistView::getMDValues()
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
