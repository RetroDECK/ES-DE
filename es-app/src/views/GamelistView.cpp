//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GamelistView.cpp
//
//  Main gamelist logic.
//

#include "views/GamelistView.h"
#include "views/GamelistLegacy.h"

#include "CollectionSystemsManager.h"
#include "UIModeController.h"
#include "animations/LambdaAnimation.h"

#define FADE_IN_START_OPACITY 0.5f
#define FADE_IN_TIME 325

GamelistView::GamelistView(FileData* root)
    : GamelistBase {root}
    , mLegacyMode {false}
    , mViewStyle {ViewController::BASIC}
{
    mViewStyle = ViewController::getInstance()->getState().viewstyle;

    if (mLegacyMode)
        return;

    const float padding {0.01f};

    mList.setPosition(mSize.x * (0.50f + padding), mList.getPosition().y);
    mList.setSize(mSize.x * (0.50f - padding), mList.getSize().y);
    mList.setAlignment(TextListComponent<FileData*>::ALIGN_LEFT);
    mList.setCursorChangedCallback([&](const CursorState& /*state*/) { updateInfoPanel(); });
}

GamelistView::~GamelistView()
{
    // Remove theme extras.
    for (auto extra : mThemeExtras) {
        removeChild(extra);
        delete extra;
    }
    mThemeExtras.clear();
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
    for (auto& animation : mLottieAnimComponents)
        animation->resetFileAnimation();

    // Reset any Lottie animations.
    if (mLegacyMode) {
        for (auto extra : mThemeExtras)
            extra->resetFileAnimation();
    }

    mLastUpdated = nullptr;
    GuiComponent::onShow();
    if (mLegacyMode)
        legacyUpdateInfoPanel();
    else
        updateInfoPanel();
}

void GamelistView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
    auto themeSets = ThemeData::getThemeSets();
    std::map<std::string, ThemeData::ThemeSet>::const_iterator selectedSet {
        themeSets.find(Settings::getInstance()->getString("ThemeSet"))};

    assert(selectedSet != themeSets.cend());
    mLegacyMode = selectedSet->second.capabilities.legacyTheme;

    if (mLegacyMode) {
        legacyOnThemeChanged(theme);
        return;
    }

    using namespace ThemeFlags;

    if (mTheme->hasView("gamelist")) {
        for (auto& element : mTheme->getViewElements("gamelist").elements) {
            if (element.second.type == "image") {
                mImageComponents.push_back(std::make_unique<ImageComponent>());
                mImageComponents.back()->setDefaultZIndex(30.0f);
                mImageComponents.back()->applyTheme(theme, "gamelist", element.first, ALL);
                if (mImageComponents.back()->getMetadataField() != "")
                    mImageComponents.back()->setScrollHide(true);
                addChild(mImageComponents.back().get());
            }
            else if (element.second.type == "video") {
                mVideoComponents.push_back(std::make_unique<VideoFFmpegComponent>());
                mVideoComponents.back()->setDefaultZIndex(30.0f);
                addChild(mVideoComponents.back().get());
                mVideoComponents.back()->applyTheme(theme, "gamelist", element.first, ALL);
                if (mVideoComponents.back()->getMetadataField() != "")
                    mVideoComponents.back()->setScrollHide(true);
            }
            else if (element.second.type == "animation") {
                mLottieAnimComponents.push_back(std::make_unique<LottieComponent>());
                mLottieAnimComponents.back()->setDefaultZIndex(35.0f);
                mLottieAnimComponents.back()->applyTheme(theme, "gamelist", element.first, ALL);
                addChild(mLottieAnimComponents.back().get());
            }
            else if (element.second.type == "badges") {
                mBadgeComponents.push_back(std::make_unique<BadgeComponent>());
                mBadgeComponents.back()->setDefaultZIndex(35.0f);
                mBadgeComponents.back()->applyTheme(theme, "gamelist", element.first, ALL);
                mBadgeComponents.back()->setScrollHide(true);
                addChild(mBadgeComponents.back().get());
            }
            else if (element.second.type == "text") {
                if (element.second.has("container") && element.second.get<bool>("container")) {
                    mContainerComponents.push_back(std::make_unique<ScrollableContainer>());
                    mContainerComponents.back()->setAutoScroll(true);
                    mContainerComponents.back()->setDefaultZIndex(40.0f);
                    addChild(mContainerComponents.back().get());
                    mContainerTextComponents.push_back(std::make_unique<TextComponent>());
                    mContainerTextComponents.back()->setDefaultZIndex(40.0f);
                    mContainerComponents.back()->addChild(mContainerTextComponents.back().get());
                    mContainerComponents.back()->applyTheme(theme, "gamelist", element.first,
                                                            POSITION | ThemeFlags::SIZE | Z_INDEX |
                                                                VISIBLE);
                    mContainerTextComponents.back()->setSize(
                        mContainerComponents.back()->getSize().x, 0.0f);
                    mContainerTextComponents.back()->applyTheme(
                        theme, "gamelist", element.first,
                        (ALL ^ POSITION ^ Z_INDEX ^ ThemeFlags::SIZE ^ VISIBLE ^ ROTATION) | COLOR);
                    mContainerComponents.back()->setScrollHide(true);
                }
                else {
                    mTextComponents.push_back(std::make_unique<TextComponent>());
                    mTextComponents.back()->setDefaultZIndex(40.0f);
                    mTextComponents.back()->applyTheme(theme, "gamelist", element.first, ALL);
                    if (mTextComponents.back()->getMetadataField() != "")
                        mTextComponents.back()->setScrollHide(true);
                    addChild(mTextComponents.back().get());
                }
            }
            else if (element.second.type == "datetime") {
                mDateTimeComponents.push_back(std::make_unique<DateTimeComponent>());
                mDateTimeComponents.back()->setDefaultZIndex(40.0f);
                mDateTimeComponents.back()->applyTheme(theme, "gamelist", element.first, ALL);
                if (mDateTimeComponents.back()->getMetadataField() != "")
                    mDateTimeComponents.back()->setScrollHide(true);
                addChild(mDateTimeComponents.back().get());
            }
            else if (element.second.type == "gamelistinfo") {
                mGamelistInfoComponents.push_back(std::make_unique<TextComponent>());
                mGamelistInfoComponents.back()->setDefaultZIndex(45.0f);
                mGamelistInfoComponents.back()->applyTheme(theme, "gamelist", element.first, ALL);
                addChild(mGamelistInfoComponents.back().get());
            }
            else if (element.second.type == "rating") {
                mRatingComponents.push_back(std::make_unique<RatingComponent>());
                mRatingComponents.back()->setDefaultZIndex(45.0f);
                mRatingComponents.back()->applyTheme(theme, "gamelist", element.first, ALL);
                mRatingComponents.back()->setScrollHide(true);
                addChild(mRatingComponents.back().get());
            }
        }
    }

    mList.setDefaultZIndex(50.0f);
    mList.applyTheme(theme, "gamelist", "textlist_gamelist", ALL);

    sortChildren();
}

void GamelistView::update(int deltaTime)
{
    if (mLegacyMode) {
        legacyUpdate(deltaTime);
        return;
    }

    if (ViewController::getInstance()->getGameLaunchTriggered()) {
        for (auto& image : mImageComponents) {
            if (image->isAnimationPlaying(0))
                image->finishAnimation(0);
        }
    }

    for (auto& video : mVideoComponents) {
        if (!mVideoPlaying) {
            if (!video->getScrollHide())
                video->onHide();
            else if (!video->hasStaticImage())
                video->onHide();
            else if (video->getOpacity() == 0)
                video->onHide();
        }
        else if (mVideoPlaying && !video->isVideoPaused() && !mWindow->isScreensaverActive()) {
            video->onShow();
        }

        if (ViewController::getInstance()->getGameLaunchTriggered() && video->isAnimationPlaying(0))
            video->finishAnimation(0);
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
    if (mLegacyMode)
        style.applyTheme(mTheme, getName());
    else
        style.applyTheme(mTheme, "gamelist");
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
    if (mLegacyMode) {
        legacyUpdateInfoPanel();
        return;
    }

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
    if (mList.isScrolling()) {
        if ((mLastUpdated && mLastUpdated->metadata.get("hidemetadata") == "true") ||
            (mLastUpdated->getSystem()->isCustomCollection() &&
             mLastUpdated->getPath() == mLastUpdated->getSystem()->getName()))
            hideMetaDataFields = true;
    }

    if (hideMetaDataFields) {
        for (auto& text : mTextComponents) {
            if (text->getMetadataField() != "")
                text->setVisible(false);
        }
        for (auto& date : mDateTimeComponents)
            date->setVisible(false);
        for (auto& badge : mBadgeComponents)
            badge->setVisible(false);
        for (auto& rating : mRatingComponents)
            rating->setVisible(false);
        for (auto& cText : mContainerTextComponents) {
            if (cText->getMetadataField() != "md_description")
                cText->setVisible(false);
        }
    }
    else {
        for (auto& text : mTextComponents) {
            if (text->getMetadataField() != "")
                text->setVisible(true);
        }
        for (auto& date : mDateTimeComponents)
            date->setVisible(true);
        for (auto& badge : mBadgeComponents)
            badge->setVisible(true);
        for (auto& rating : mRatingComponents)
            rating->setVisible(true);
        for (auto& cText : mContainerTextComponents) {
            if (cText->getMetadataField() != "md_description")
                cText->setVisible(true);
        }
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
                for (auto& image : mImageComponents) {
                    if (image->getMetadataField() == "md_image")
                        image->setImage(mRandomGame->getImagePath());
                    else if (image->getMetadataField() == "md_miximage")
                        image->setImage(mRandomGame->getMiximagePath());
                    else if (image->getMetadataField() == "md_marquee")
                        image->setImage(mRandomGame->getMarqueePath(), false, true);
                    else if (image->getMetadataField() == "md_screenshot")
                        image->setImage(mRandomGame->getScreenshotPath());
                    else if (image->getMetadataField() == "md_titlescreen")
                        image->setImage(mRandomGame->getTitleScreenPath());
                    else if (image->getMetadataField() == "md_cover")
                        image->setImage(mRandomGame->getCoverPath());
                    else if (image->getMetadataField() == "md_backcover")
                        image->setImage(mRandomGame->getBackCoverPath());
                    else if (image->getMetadataField() == "md_3dbox")
                        image->setImage(mRandomGame->get3DBoxPath());
                    else if (image->getMetadataField() == "md_fanart")
                        image->setImage(mRandomGame->getFanArtPath());
                    else if (image->getMetadataField() == "md_thumbnail")
                        image->setImage(mRandomGame->getThumbnailPath());
                }

                for (auto& video : mVideoComponents) {
                    if (video->getMetadataField() == "md_image")
                        video->setImage(mRandomGame->getImagePath());
                    else if (video->getMetadataField() == "md_miximage")
                        video->setImage(mRandomGame->getMiximagePath());
                    else if (video->getMetadataField() == "md_marquee")
                        video->setImage(mRandomGame->getMarqueePath(), false, true);
                    else if (video->getMetadataField() == "md_screenshot")
                        video->setImage(mRandomGame->getScreenshotPath());
                    else if (video->getMetadataField() == "md_titlescreen")
                        video->setImage(mRandomGame->getTitleScreenPath());
                    else if (video->getMetadataField() == "md_cover")
                        video->setImage(mRandomGame->getCoverPath());
                    else if (video->getMetadataField() == "md_backcover")
                        video->setImage(mRandomGame->getBackCoverPath());
                    else if (video->getMetadataField() == "md_3dbox")
                        video->setImage(mRandomGame->get3DBoxPath());
                    else if (video->getMetadataField() == "md_fanart")
                        video->setImage(mRandomGame->getFanArtPath());
                    else if (video->getMetadataField() == "md_thumbnail")
                        video->setImage(mRandomGame->getThumbnailPath());

                    // Always stop the video before setting a new video as it will otherwise
                    // continue to play if it has the same path (i.e. it is the same physical
                    // video file) as the previously set video. That may happen when entering a
                    // folder with the same name as the first game file inside, or as in this
                    // case, when entering a custom collection.
                    video->onHide();

                    if (video->hasStaticVideo())
                        video->setStaticVideo();
                    else if (!video->setVideo(mRandomGame->getVideoPath()))
                        video->setDefaultVideo();
                }
            }
            else {
                for (auto& image : mImageComponents) {
                    if (image->getMetadataField() != "")
                        image->setImage("");
                }

                for (auto& video : mVideoComponents) {
                    video->setImage("");
                    video->setVideo("");
                    if (video->hasStaticVideo()) {
                        video->onStopVideo();
                        video->setStaticVideo();
                    }
                    else {
                        video->setDefaultVideo();
                    }
                }
            }
        }
        else {
            for (auto& image : mImageComponents) {
                if (image->getMetadataField() == "md_image")
                    image->setImage(file->getImagePath());
                else if (image->getMetadataField() == "md_miximage")
                    image->setImage(file->getMiximagePath());
                else if (image->getMetadataField() == "md_marquee")
                    image->setImage(file->getMarqueePath(), false, true);
                else if (image->getMetadataField() == "md_screenshot")
                    image->setImage(file->getScreenshotPath());
                else if (image->getMetadataField() == "md_titlescreen")
                    image->setImage(file->getTitleScreenPath());
                else if (image->getMetadataField() == "md_cover")
                    image->setImage(file->getCoverPath());
                else if (image->getMetadataField() == "md_backcover")
                    image->setImage(file->getBackCoverPath());
                else if (image->getMetadataField() == "md_3dbox")
                    image->setImage(file->get3DBoxPath());
                else if (image->getMetadataField() == "md_fanart")
                    image->setImage(file->getFanArtPath());
                else if (image->getMetadataField() == "md_thumbnail")
                    image->setImage(file->getThumbnailPath());
            }

            for (auto& video : mVideoComponents) {
                if (video->getMetadataField() == "md_image")
                    video->setImage(file->getImagePath());
                else if (video->getMetadataField() == "md_miximage")
                    video->setImage(file->getMiximagePath());
                else if (video->getMetadataField() == "md_marquee")
                    video->setImage(file->getMarqueePath(), false, true);
                else if (video->getMetadataField() == "md_screenshot")
                    video->setImage(file->getScreenshotPath());
                else if (video->getMetadataField() == "md_titlescreen")
                    video->setImage(file->getTitleScreenPath());
                else if (video->getMetadataField() == "md_cover")
                    video->setImage(file->getCoverPath());
                else if (video->getMetadataField() == "md_backcover")
                    video->setImage(file->getBackCoverPath());
                else if (video->getMetadataField() == "md_3dbox")
                    video->setImage(file->get3DBoxPath());
                else if (video->getMetadataField() == "md_fanart")
                    video->setImage(file->getFanArtPath());
                else if (video->getMetadataField() == "md_thumbnail")
                    video->setImage(file->getThumbnailPath());

                video->onHide();

                if (video->hasStaticVideo())
                    video->setStaticVideo();
                else if (!video->setVideo(file->getVideoPath()))
                    video->setDefaultVideo();
            }
        }

        mVideoPlaying = true;

        // Populate the gamelistInfo field which shows an icon if a folder has been entered
        // as well as the game count for the entire system (total and favorites separately).
        // If a filter has been applied, then the number of filtered and total games replaces
        // the game counter.
        for (auto& gamelistInfo : mGamelistInfoComponents) {
            std::string gamelistInfoString;
            Alignment infoAlign = gamelistInfo->getHorizontalAlignment();

            if (mIsFolder && infoAlign == ALIGN_RIGHT)
                gamelistInfoString = ViewController::FOLDER_CHAR + "  ";

            if (mIsFiltered) {
                if (mFilteredGameCountAll == mFilteredGameCount)
                    gamelistInfoString += ViewController::FILTER_CHAR + " " +
                                          std::to_string(mFilteredGameCount) + " / " +
                                          std::to_string(mGameCount);
                else
                    gamelistInfoString +=
                        ViewController::FILTER_CHAR + " " + std::to_string(mFilteredGameCount) +
                        " + " + std::to_string(mFilteredGameCountAll - mFilteredGameCount) + " / " +
                        std::to_string(mGameCount);
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

            gamelistInfo->setValue(gamelistInfoString);
        }

        // Fade in the game image.
        for (auto& image : mImageComponents) {
            if (image->getScrollFadeIn()) {
                auto func = [&image](float t) {
                    image->setOpacity(static_cast<unsigned char>(
                        glm::mix(static_cast<float>(FADE_IN_START_OPACITY), 1.0f, t) * 255));
                };
                image->setAnimation(new LambdaAnimation(func, FADE_IN_TIME), 0, nullptr, false);
            }
        }

        // Fade in the static image.
        for (auto& video : mVideoComponents) {
            if (video->getScrollFadeIn()) {
                auto func = [&video](float t) {
                    video->setOpacity(static_cast<unsigned char>(
                        glm::mix(static_cast<float>(FADE_IN_START_OPACITY), 1.0f, t) * 255));
                };
                video->setAnimation(new LambdaAnimation(func, FADE_IN_TIME), 0, nullptr, false);
            }
        }

        for (auto& container : mContainerComponents)
            container->reset();

        for (auto& rating : mRatingComponents)
            rating->setValue(file->metadata.get("rating"));

        // Populate the badge slots based on game metadata.
        std::vector<BadgeComponent::BadgeInfo> badgeSlots;
        for (auto& badgeComponent : mBadgeComponents) {
            for (auto& badge : badgeComponent->getBadgeTypes()) {
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
            badgeComponent->setBadges(badgeSlots);
        }

        for (auto& text : mTextComponents) {
            if (text->getMetadataField() == "md_name")
                text->setText(file->metadata.get("name"));
        }

        if (file->getType() == GAME) {
            if (!hideMetaDataFields) {
                for (auto& date : mDateTimeComponents) {
                    if (date->getMetadataField() == "md_lastplayed")
                        date->setValue(file->metadata.get("lastplayed"));
                    else if (date->getMetadataField() == "md_playcount")
                        date->setValue(file->metadata.get("playcount"));
                }
            }
            else if (file->getType() == FOLDER) {
                if (!hideMetaDataFields) {
                    for (auto& date : mDateTimeComponents) {
                        if (date->getMetadataField() == "md_lastplayed") {
                            date->setValue(file->metadata.get("lastplayed"));
                            date->setVisible(false);
                            date->setVisible(false);
                        }
                    }
                }
            }
        }

        std::string metadata;

        auto getMetadataValue = [&file, &metadata]() -> std::string {
            if (metadata == "md_name")
                return file->metadata.get("name");
            else if (metadata == "md_description")
                return file->metadata.get("desc");
            else if (metadata == "md_developer")
                return file->metadata.get("developer");
            else if (metadata == "md_publisher")
                return file->metadata.get("publisher");
            else if (metadata == "md_genre")
                return file->metadata.get("genre");
            else if (metadata == "md_players")
                return file->metadata.get("players");
            else if (metadata == "md_favorite")
                return file->metadata.get("favorite") == "true" ? "Yes" : "No";
            else if (metadata == "md_completed")
                return file->metadata.get("completed") == "true" ? "Yes" : "No";
            else if (metadata == "md_kidgame")
                return file->metadata.get("kidgame") == "true" ? "Yes" : "No";
            else if (metadata == "md_broken")
                return file->metadata.get("broken") == "true" ? "Yes" : "No";
            else if (metadata == "md_playcount")
                return file->metadata.get("playcount");
            else if (metadata == "md_altemulator")
                return file->metadata.get("altemulator");
            else
                return metadata;
        };

        for (auto& text : mContainerTextComponents) {
            metadata = text->getMetadataField();
            if (metadata == "")
                continue;

            if (metadata == "md_rating") {
                text->setValue(mRatingComponents.front()->getRatingValue());
                continue;
            }
            else if (metadata == "md_controller") {
                std::string controller =
                    BadgeComponent::getDisplayName(file->metadata.get("controller"));
                text->setValue(controller == "unknown" ? "" : controller);
                continue;
            }

            text->setValue(getMetadataValue());
        }

        for (auto& text : mTextComponents) {
            metadata = text->getMetadataField();
            if (metadata == "")
                continue;

            if (metadata == "md_rating") {
                text->setValue(mRatingComponents.front()->getRatingValue());
                continue;
            }
            else if (metadata == "md_controller") {
                std::string controller =
                    BadgeComponent::getDisplayName(file->metadata.get("controller"));
                text->setValue(controller == "unknown" ? "" : controller);
                continue;
            }

            text->setValue(getMetadataValue());
        }

        for (auto& date : mDateTimeComponents) {
            std::string metadata = date->getMetadataField();
            if (metadata == "")
                continue;

            if (metadata == "md_releasedate") {
                date->setValue(file->metadata.get("releasedate"));
            }
            else if (metadata == "md_lastplayed") {
                date->setValue(file->metadata.get("lastplayed"));
                date->setDisplayRelative(true);
            }
            else {
                date->setValue("19700101T000000");
            }
        }

        fadingOut = false;
    }

    std::vector<GuiComponent*> comps;

    for (auto& text : mTextComponents) {
        if (text->getScrollHide())
            comps.emplace_back(text.get());
    }
    for (auto& date : mDateTimeComponents) {
        if (date->getScrollHide())
            comps.emplace_back(date.get());
    }
    for (auto& image : mImageComponents) {
        if (image->getScrollHide())
            comps.emplace_back(image.get());
    }
    for (auto& video : mVideoComponents) {
        if (video->getScrollHide())
            comps.emplace_back(video.get());
    }
    for (auto& badge : mBadgeComponents) {
        if (badge->getScrollHide())
            comps.emplace_back(badge.get());
    }
    for (auto& rating : mRatingComponents) {
        if (rating->getScrollHide())
            comps.emplace_back(rating.get());
    }
    for (auto& container : mContainerComponents) {
        if (container->getScrollHide())
            comps.emplace_back(container.get());
    }

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
