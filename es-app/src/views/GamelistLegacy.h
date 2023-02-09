//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GamelistLegacy.h
//
//  Specific gamelist functions for backward compatibility (legacy mode).
//

#ifndef ES_APP_VIEWS_GAMELIST_LEGACY_H
#define ES_APP_VIEWS_GAMELIST_LEGACY_H

#include "CollectionSystemsManager.h"
#include "animations/LambdaAnimation.h"

#define FADE_IN_START_OPACITY 0.5f
#define FADE_IN_TIME 325

void GamelistView::legacyPopulateFields()
{
    const float padding {0.01f};

    // Logo text (fallback if no logo image exists).
    mTextComponents.push_back(std::make_unique<TextComponent>());
    mTextComponents.back()->setText(mRoot->getSystem()->getFullName(), false);
    mTextComponents.back()->setSize(mSize.x, 0.0f);
    mTextComponents.back()->setPosition(0.0f, 0.0f);
    mTextComponents.back()->setHorizontalAlignment(ALIGN_CENTER);
    mTextComponents.back()->setColor(0x000000FF);
    mTextComponents.back()->setDefaultZIndex(50.0f);
    mTextComponents.back()->setZIndex(10.0f);
    addChild(mTextComponents.back().get());

    // Logo.
    mImageComponents.push_back(std::make_unique<ImageComponent>());
    mImageComponents.back()->setResize(0.0f, mSize.y * 0.185f);
    mImageComponents.back()->setOrigin(0.5f, 0.0f);
    mImageComponents.back()->setPosition(mSize.x / 2.0f, 0.0f);
    mImageComponents.back()->setDefaultZIndex(50.0f);
    mImageComponents.back()->setVisible(false);
    addChild(mImageComponents.back().get());

    // Background.
    mImageComponents.push_back(std::make_unique<ImageComponent>());
    mImageComponents.back()->setResize(mSize.x, mSize.y);
    mImageComponents.back()->setDefaultZIndex(0.0f);
    addChild(mImageComponents.back().get());

    // Thumbnails.
    mImageComponents.push_back(std::make_unique<ImageComponent>());
    mImageComponents.back()->setThemeMetadata("image_md_thumbnail");
    mImageComponents.back()->setOrigin(0.5f, 0.5f);
    mImageComponents.back()->setMaxSize(mSize.x * (0.25f - 2.0f * padding), mSize.y * 0.10f);
    mImageComponents.back()->setDefaultZIndex(25.0f);
    mImageComponents.back()->setScrollFadeIn(true);
    mImageComponents.back()->setVisible(false);
    addChild(mImageComponents.back().get());

    // Marquee.
    mImageComponents.push_back(std::make_unique<ImageComponent>());
    mImageComponents.back()->setThemeMetadata("image_md_marquee");
    mImageComponents.back()->setLinearInterpolation(true);
    mImageComponents.back()->setOrigin(0.5f, 0.5f);
    mImageComponents.back()->setMaxSize(mSize.x * (0.5f - 2.0f * padding), mSize.y * 0.18f);
    mImageComponents.back()->setDefaultZIndex(35.0f);
    mImageComponents.back()->setVisible(false);
    addChild(mImageComponents.back().get());

    // Image.
    mImageComponents.push_back(std::make_unique<ImageComponent>());
    mImageComponents.back()->setThemeMetadata("image_md_image");
    mImageComponents.back()->setOrigin(0.5f, 0.5f);
    mImageComponents.back()->setPosition(mSize.x * 0.25f,
                                         mPrimary->getPosition().y + mSize.y * 0.2125f);
    mImageComponents.back()->setMaxSize(mSize.x * (0.50f - 2.0f * padding), mSize.y * 0.4f);
    mImageComponents.back()->setDefaultZIndex(30.0f);
    mImageComponents.back()->setScrollFadeIn(true);
    mImageComponents.back()->setVisible(false);
    addChild(mImageComponents.back().get());

    if (mViewStyle == ViewController::VIDEO) {
        // Video.
        mVideoComponents.push_back(std::make_unique<VideoFFmpegComponent>());
        mVideoComponents.back()->setThemeMetadata("video_md_video");
        mVideoComponents.back()->setOrigin(0.5f, 0.5f);
        mVideoComponents.back()->setPosition(mSize.x * 0.25f,
                                             mPrimary->getPosition().y + mSize.y * 0.2125f);
        mVideoComponents.back()->setSize(mSize.x * (0.5f - 2.0f * padding), mSize.y * 0.4f);
        mVideoComponents.back()->setDefaultZIndex(30.0f);
        mVideoComponents.back()->setScrollFadeIn(true);
        mVideoComponents.back()->setVisible(false);
        addChild(mVideoComponents.back().get());
    }

    mPrimary->setPosition(mSize.x * (0.50f + padding), mPrimary->getPosition().y);
    mPrimary->setSize(mSize.x * (0.50f - padding), mPrimary->getSize().y);
    mPrimary->setAlignment(TextListComponent<FileData*>::PrimaryAlignment::ALIGN_LEFT);
    mPrimary->setCursorChangedCallback([&](const CursorState& state) { legacyUpdateView(state); });

    // Metadata labels + values.
    mTextComponents.push_back(std::make_unique<TextComponent>());
    mTextComponents.back()->setText("Rating: ", false);
    mTextComponents.back()->setThemeMetadata("text_md_lbl_rating");
    addChild(mTextComponents.back().get());

    mTextComponents.push_back(std::make_unique<TextComponent>());
    mTextComponents.back()->setText("Released: ", false);
    mTextComponents.back()->setThemeMetadata("text_md_lbl_releasedate");
    addChild(mTextComponents.back().get());

    mTextComponents.push_back(std::make_unique<TextComponent>());
    mTextComponents.back()->setText("Developer: ", false);
    mTextComponents.back()->setThemeMetadata("text_md_lbl_developer");
    addChild(mTextComponents.back().get());

    mTextComponents.push_back(std::make_unique<TextComponent>());
    mTextComponents.back()->setText("Publisher: ", false);
    mTextComponents.back()->setThemeMetadata("text_md_lbl_publisher");
    addChild(mTextComponents.back().get());

    mTextComponents.push_back(std::make_unique<TextComponent>());
    mTextComponents.back()->setText("Genre: ", false);
    mTextComponents.back()->setThemeMetadata("text_md_lbl_genre");
    addChild(mTextComponents.back().get());

    mTextComponents.push_back(std::make_unique<TextComponent>());
    mTextComponents.back()->setText("Players: ", false);
    mTextComponents.back()->setThemeMetadata("text_md_lbl_players");
    addChild(mTextComponents.back().get());

    mTextComponents.push_back(std::make_unique<TextComponent>());
    mTextComponents.back()->setText("Last played: ", false);
    mTextComponents.back()->setThemeMetadata("text_md_lbl_lastplayed");
    addChild(mTextComponents.back().get());

    mTextComponents.push_back(std::make_unique<TextComponent>());
    mTextComponents.back()->setText("Times played: ", false);
    mTextComponents.back()->setThemeMetadata("text_md_lbl_playcount");
    addChild(mTextComponents.back().get());

    mRatingComponents.push_back(std::make_unique<RatingComponent>());
    mRatingComponents.back()->setThemeMetadata("rating_md_rating");
    mRatingComponents.back()->setDefaultZIndex(40.0f);
    addChild(mRatingComponents.back().get());

    mDateTimeComponents.push_back(std::make_unique<DateTimeComponent>());
    mDateTimeComponents.back()->setThemeMetadata("datetime_md_releasedate");
    addChild(mDateTimeComponents.back().get());

    mTextComponents.push_back(std::make_unique<TextComponent>());
    mTextComponents.back()->setThemeMetadata("text_md_developer");
    addChild(mTextComponents.back().get());

    mTextComponents.push_back(std::make_unique<TextComponent>());
    mTextComponents.back()->setThemeMetadata("text_md_publisher");
    addChild(mTextComponents.back().get());

    mTextComponents.push_back(std::make_unique<TextComponent>());
    mTextComponents.back()->setThemeMetadata("text_md_genre");
    addChild(mTextComponents.back().get());

    mTextComponents.push_back(std::make_unique<TextComponent>());
    mTextComponents.back()->setThemeMetadata("text_md_players");
    addChild(mTextComponents.back().get());

    mDateTimeComponents.push_back(std::make_unique<DateTimeComponent>());
    mDateTimeComponents.back()->setThemeMetadata("datetime_md_lastplayed");
    mDateTimeComponents.back()->setDisplayRelative(true);
    addChild(mDateTimeComponents.back().get());

    mTextComponents.push_back(std::make_unique<TextComponent>());
    mTextComponents.back()->setThemeMetadata("text_md_playcount");
    addChild(mTextComponents.back().get());

    mTextComponents.push_back(std::make_unique<TextComponent>());
    mTextComponents.back()->setThemeMetadata("text_md_name");
    mTextComponents.back()->setPosition(mSize.x, mSize.y);
    mTextComponents.back()->setFont(Font::get(FONT_SIZE_MEDIUM_FIXED));
    mTextComponents.back()->setHorizontalAlignment(ALIGN_CENTER);
    mTextComponents.back()->setColor(0xAAAAAAFF);
    mTextComponents.back()->setDefaultZIndex(40.0f);
    addChild(mTextComponents.back().get());

    // Badges.
    mBadgeComponents.push_back(std::make_unique<BadgeComponent>());
    mBadgeComponents.back()->setThemeMetadata("badges_md_badges");
    mBadgeComponents.back()->setOrigin(0.5f, 0.5f);
    mBadgeComponents.back()->setPosition(mSize.x * 0.8f, mSize.y * 0.7f);
    mBadgeComponents.back()->setSize(mSize.x * 0.15f, mSize.y * 0.2f);
    mBadgeComponents.back()->setDefaultZIndex(50.0f);
    addChild(mBadgeComponents.back().get());

    // Scrollable container (game description).
    mContainerComponents.push_back(std::make_unique<ScrollableContainer>());
    mContainerComponents.back()->setThemeMetadata("text_md_description");
    mContainerComponents.back()->setAutoScroll(true);
    mContainerComponents.back()->setVisible(false);
    mContainerComponents.back()->setDefaultZIndex(40.0f);
    addChild(mContainerComponents.back().get());

    mTextComponents.push_back(std::make_unique<TextComponent>());
    mTextComponents.back()->setFont(Font::get(FONT_SIZE_SMALL));
    mTextComponents.back()->setSize(mContainerComponents.back()->getSize().x, 0.0f);
    mContainerComponents.back()->addChild(mTextComponents.back().get());

    mGamelistInfoComponents.push_back(std::make_unique<TextComponent>());
    mGamelistInfoComponents.back()->setThemeMetadata("text_gamelistInfo");
    mGamelistInfoComponents.back()->setOrigin(0.5f, 0.5f);
    mGamelistInfoComponents.back()->setFont(Font::get(FONT_SIZE_SMALL));
    mGamelistInfoComponents.back()->setDefaultZIndex(50.0f);
    mGamelistInfoComponents.back()->setVisible(true);
    addChild(mGamelistInfoComponents.back().get());
}

void GamelistView::legacyOnThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
    if (mTextList == nullptr) {
        mTextList = std::make_unique<TextListComponent<FileData*>>();
        mPrimary = mTextList.get();
    }

    mSystemNameSuffix = true;
    legacyPopulateFields();

    using namespace ThemeFlags;

    mTextComponents[LOGOTEXT]->applyTheme(theme, getName(), "text_logoText", ALL);
    mImageComponents[LOGO]->applyTheme(theme, getName(), "image_logo", ALL);
    mImageComponents[BACKGROUND]->applyTheme(theme, getName(), "image_background", ALL);

    auto themeView = theme->getViewElements(getName());
    if (themeView.elements.find("text_logoText") == themeView.elements.end())
        mTextComponents[LOGOTEXT]->setVisible(false);

    if (getName() == "basic")
        mPrimary->setAlignment(TextListComponent<FileData*>::PrimaryAlignment::ALIGN_CENTER);

    // Make sure we don't display both the logo image and logo text.
    if (mImageComponents[LOGO]->hasImage())
        mTextComponents[LOGOTEXT]->setVisible(false);

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

    mPrimary->setPosition(0.0f, mSize.y * 0.1f);
    mPrimary->setSize(mSize.x, mSize.y * 0.8f);
    mPrimary->setDefaultZIndex(50.0f);
    mPrimary->applyTheme(theme, getName(), "textlist_gamelist", ALL);
    addChild(mPrimary);

    mImageComponents[LegacyImage::MD_THUMBNAIL]->applyTheme(
        theme, getName(), mImageComponents[LegacyImage::MD_THUMBNAIL]->getThemeMetadata(), ALL);
    mImageComponents[LegacyImage::MD_MARQUEE]->applyTheme(theme, getName(), "image_md_marquee",
                                                          POSITION | ThemeFlags::SIZE | Z_INDEX |
                                                              ROTATION | VISIBLE);

    if (mViewStyle == ViewController::DETAILED) {
        mImageComponents[LegacyImage::MD_IMAGE]->applyTheme(
            theme, getName(), mImageComponents[LegacyImage::MD_IMAGE]->getThemeMetadata(),
            POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION | VISIBLE);
    }
    else if (mViewStyle == ViewController::VIDEO) {
        mVideoComponents.front()->applyTheme(
            theme, getName(), mVideoComponents.front()->getThemeMetadata(),
            POSITION | ThemeFlags::SIZE | ThemeFlags::DELAY | Z_INDEX | ROTATION | VISIBLE);
        mImageComponents[LegacyImage::MD_IMAGE]->setVisible(false);
        mImageComponents[LegacyImage::MD_IMAGE]->applyTheme(
            theme, getName(), mImageComponents[LegacyImage::MD_IMAGE]->getThemeMetadata(),
            POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION | VISIBLE);
    }

    legacyInitMDLabels();
    legacyInitMDValues();

    mTextComponents[LegacyText::MD_NAME]->applyTheme(
        theme, getName(), mTextComponents[LegacyText::MD_NAME]->getThemeMetadata(), ALL);

    for (size_t i = 0; i < mBadgeComponents.size(); ++i)
        mBadgeComponents[i]->applyTheme(theme, getName(), mBadgeComponents[i]->getThemeMetadata(),
                                        ALL);

    for (size_t i = 0; i < mRatingComponents.size(); ++i)
        mRatingComponents[i]->applyTheme(theme, getName(), mRatingComponents[i]->getThemeMetadata(),
                                         ALL);

    mDateTimeComponents[LegacyDateTime::MD_RELEASEDATE]->applyTheme(
        theme, getName(), mDateTimeComponents[LegacyDateTime::MD_RELEASEDATE]->getThemeMetadata(),
        ALL);

    mDateTimeComponents[LegacyDateTime::MD_LASTPLAYED]->applyTheme(
        theme, getName(), mDateTimeComponents[LegacyDateTime::MD_LASTPLAYED]->getThemeMetadata(),
        ALL);

    for (size_t i = LegacyText::MD_LBL_RATING; i < LegacyText::MD_NAME; ++i) {
        mTextComponents[i]->applyTheme(theme, getName(), mTextComponents[i]->getThemeMetadata(),
                                       ALL ^ ThemeFlags::TEXT);
    }

    for (auto& container : mContainerComponents) {
        container->applyTheme(theme, getName(), container->getThemeMetadata(),
                              POSITION | ThemeFlags::SIZE | Z_INDEX | VISIBLE);
    }

    mTextComponents[LegacyText::MD_DESCRIPTION]->setSize(mContainerComponents.front()->getSize().x,
                                                         0.0f);
    mTextComponents[LegacyText::MD_DESCRIPTION]->applyTheme(
        theme, getName(), "text_md_description",
        ALL ^ (POSITION | ThemeFlags::SIZE | ThemeFlags::ORIGIN | TEXT | ROTATION));

    for (auto& gamelistInfo : mGamelistInfoComponents)
        gamelistInfo->applyTheme(theme, getName(), gamelistInfo->getThemeMetadata(),
                                 ALL ^ ThemeFlags::TEXT);

    // If there is no position defined in the theme for gamelistInfo, then hide it.
    if (mGamelistInfoComponents.front()->getPosition() == glm::vec3 {})
        mGamelistInfoComponents.front()->setVisible(false);
    else
        mGamelistInfoComponents.front()->setVisible(true);

    // Hide some components if we're in Basic mode.
    if (mViewStyle == ViewController::BASIC) {
        mImageComponents[LegacyImage::MD_IMAGE]->setVisible(false);
        for (auto& container : mContainerComponents)
            container->setVisible(false);
    }

    populateList(mRoot->getChildrenListToDisplay(), mRoot);
    sortChildren();
    mHelpStyle.applyTheme(mTheme, getName());
}

void GamelistView::legacyUpdateView(const CursorState& state)
{
    FileData* file {(mPrimary->size() > 0 && state == CursorState::CURSOR_STOPPED) ?
                        mPrimary->getSelected() :
                        nullptr};

    // If the game data has already been rendered to the info panel, then skip it this time.
    if (file == mLastUpdated)
        return;

    if (state == CursorState::CURSOR_STOPPED)
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
    if (mPrimary->isScrolling()) {
        if ((mLastUpdated && mLastUpdated->metadata.get("hidemetadata") == "true") ||
            (mLastUpdated->getSystem()->isCustomCollection() &&
             mLastUpdated->getPath() == mLastUpdated->getSystem()->getName()))
            hideMetaDataFields = true;
    }

    if (hideMetaDataFields || mViewStyle == ViewController::BASIC) {
        for (size_t i = LegacyText::MD_LBL_RATING; i < LegacyText::MD_DESCRIPTION; ++i)
            mTextComponents[i]->setVisible(false);
        for (auto& date : mDateTimeComponents)
            date->setVisible(false);
        for (auto& badge : mBadgeComponents)
            badge->setVisible(false);
        for (auto& rating : mRatingComponents)
            rating->setVisible(false);
    }
    else {
        for (size_t i = LegacyText::MD_LBL_RATING; i < LegacyText::MD_DESCRIPTION; ++i)
            mTextComponents[i]->setVisible(true);
        for (auto& date : mDateTimeComponents)
            date->setVisible(true);
        for (auto& badge : mBadgeComponents)
            badge->setVisible(true);
        for (auto& rating : mRatingComponents)
            rating->setVisible(true);
    }

    bool fadingOut {false};
    if (file == nullptr) {
        if (mViewStyle == ViewController::VIDEO) {
            mVideoComponents.front()->stopVideoPlayer();
            mVideoComponents.front()->setVideo("");
            if (!mVideoComponents.front()->hasStartDelay())
                mVideoComponents.front()->setImage("");
        }
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
                mImageComponents[LegacyImage::MD_THUMBNAIL]->setImage(mRandomGame->getImagePath());
                mImageComponents[LegacyImage::MD_MARQUEE]->setImage(mRandomGame->getMarqueePath());
                if (mViewStyle == ViewController::VIDEO) {
                    mVideoComponents.front()->setImage(mRandomGame->getImagePath());
                    mVideoComponents.front()->stopVideoPlayer();

                    if (!mVideoComponents.front()->setVideo(mRandomGame->getVideoPath()))
                        mVideoComponents.front()->setDefaultVideo();

                    mVideoComponents.front()->startVideoPlayer();
                    mImageComponents[LegacyImage::MD_IMAGE]->setImage(mRandomGame->getImagePath());
                }
                else {
                    mImageComponents[LegacyImage::MD_IMAGE]->setImage(mRandomGame->getImagePath());
                }
            }
            else {
                mImageComponents[LegacyImage::MD_THUMBNAIL]->setImage("");
                mImageComponents[LegacyImage::MD_MARQUEE]->setImage("");
                mImageComponents[LegacyImage::MD_IMAGE]->setImage("");
                if (mViewStyle == ViewController::VIDEO) {
                    mVideoComponents.front()->setImage("");
                    mVideoComponents.front()->setVideo("");
                    mVideoComponents.front()->setDefaultVideo();
                }
            }
        }
        else {
            mImageComponents[LegacyImage::MD_THUMBNAIL]->setImage(file->getImagePath());
            mImageComponents[LegacyImage::MD_MARQUEE]->setImage(file->getMarqueePath());
            if (mViewStyle == ViewController::VIDEO) {
                mVideoComponents.front()->setImage(file->getImagePath());
                mVideoComponents.front()->stopVideoPlayer();

                if (!mVideoComponents.front()->setVideo(file->getVideoPath()))
                    mVideoComponents.front()->setDefaultVideo();

                mVideoComponents.front()->startVideoPlayer();
                mImageComponents[LegacyImage::MD_IMAGE]->setImage(file->getImagePath());
            }
            else {
                mImageComponents[LegacyImage::MD_IMAGE]->setImage(file->getImagePath());
            }
        }

        mVideoPlaying = true;

        // Populate the gamelistInfo field which shows an icon if a folder has been entered
        // as well as the game count for the entire system (total and favorites separately).
        // If a filter has been applied, then the number of filtered and total games replaces
        // the game counter.
        for (auto& gamelistInfo : mGamelistInfoComponents) {
            std::string gamelistInfoString;
            Alignment infoAlign {gamelistInfo->getHorizontalAlignment()};

            if (mIsFolder && infoAlign == ALIGN_RIGHT)
                gamelistInfoString = ViewController::FOLDER_CHAR + "  ";

            if (mIsFiltered) {
                if (mFilteredGameCountAll == mFilteredGameCount)
                    gamelistInfoString.append(ViewController::FILTER_CHAR)
                        .append(" ")
                        .append(std::to_string(mFilteredGameCount))
                        .append(" / ")
                        .append(std::to_string(mGameCount));
                else
                    gamelistInfoString.append(ViewController::FILTER_CHAR)
                        .append(" ")
                        .append(std::to_string(mFilteredGameCount))
                        .append(" + ")
                        .append(std::to_string(mFilteredGameCountAll - mFilteredGameCount))
                        .append(" / ")
                        .append(std::to_string(mGameCount));
            }
            else {
                gamelistInfoString.append(ViewController::CONTROLLER_CHAR)
                    .append(" ")
                    .append(std::to_string(mGameCount));
                if (!(file->getSystem()->isCollection() &&
                      file->getSystem()->getFullName() == "favorites"))
                    gamelistInfoString.append("  ")
                        .append(ViewController::FAVORITE_CHAR)
                        .append(" ")
                        .append(std::to_string(mFavoritesGameCount));
            }

            if (mIsFolder && infoAlign != ALIGN_RIGHT)
                gamelistInfoString.append("  ").append(ViewController::FOLDER_CHAR);

            gamelistInfo->setValue(gamelistInfoString);
        }

        if (mViewStyle == ViewController::DETAILED) {
            // Fade in the game image.
            auto funcImage = [this](float t) {
                mImageComponents[LegacyImage::MD_IMAGE]->setOpacity(
                    glm::mix(FADE_IN_START_OPACITY, 1.0f, t));
            };
            mImageComponents[LegacyImage::MD_IMAGE]->setAnimation(
                new LambdaAnimation(funcImage, FADE_IN_TIME), 0, nullptr, false);

            // Fade in the thumbnail.
            auto funcThumbnail = [this](float t) {
                mImageComponents[LegacyImage::MD_THUMBNAIL]->setOpacity(
                    glm::mix(FADE_IN_START_OPACITY, 1.0f, t));
            };
            mImageComponents[LegacyImage::MD_THUMBNAIL]->setAnimation(
                new LambdaAnimation(funcThumbnail, FADE_IN_TIME), 0, nullptr, false);
        }
        else if (mViewStyle == ViewController::VIDEO) {
            // Fade in the static image.
            auto funcVideo = [this](float t) {
                mVideoComponents.front()->setOpacity(glm::mix(FADE_IN_START_OPACITY, 1.0f, t));
            };
            mVideoComponents.front()->setAnimation(new LambdaAnimation(funcVideo, FADE_IN_TIME), 0,
                                                   nullptr, false);
            // Fade in the game image.
            auto funcImage = [this](float t) {
                mImageComponents[LegacyImage::MD_IMAGE]->setOpacity(
                    glm::mix(FADE_IN_START_OPACITY, 1.0f, t));
            };
            mImageComponents[LegacyImage::MD_IMAGE]->setAnimation(
                new LambdaAnimation(funcImage, FADE_IN_TIME), 0, nullptr, false);
        }

        mTextComponents[LegacyText::MD_DESCRIPTION]->setText(file->metadata.get("desc"));
        for (auto& container : mContainerComponents)
            container->reset();

        for (auto& rating : mRatingComponents)
            rating->setValue(file->metadata.get("rating"));

        mDateTimeComponents[LegacyDateTime::MD_RELEASEDATE]->setValue(
            file->metadata.get("releasedate"));
        mTextComponents[LegacyText::MD_DEVELOPER]->setValue(file->metadata.get("developer"));
        mTextComponents[LegacyText::MD_PUBLISHER]->setValue(file->metadata.get("publisher"));
        mTextComponents[LegacyText::MD_GENRE]->setValue(file->metadata.get("genre"));
        mTextComponents[LegacyText::MD_PLAYERS]->setValue(file->metadata.get("players"));

        // Populate the badge slots based on game metadata.
        std::vector<BadgeComponent::BadgeInfo> badgeSlots;
        for (auto& badgeComponent : mBadgeComponents) {
            for (auto& badge : badgeComponent->getBadgeTypes()) {
                BadgeComponent::BadgeInfo badgeInfo;
                badgeInfo.badgeType = badge;
                if (badge == "collection" && CollectionSystemsManager::getInstance()->isEditing()) {
                    if (CollectionSystemsManager::getInstance()->inCustomCollection(
                            CollectionSystemsManager::getInstance()->getEditingCollection(),
                            file)) {
                        badgeSlots.emplace_back(badgeInfo);
                    }
                }
                else if (badge == "folder") {
                    if (file->getType() == FOLDER) {
                        if (file->metadata.get("folderlink") != "")
                            badgeInfo.folderLink = true;
                        badgeSlots.emplace_back(badgeInfo);
                    }
                }
                else if (badge == "controller") {
                    if (file->metadata.get("controller") != "") {
                        badgeInfo.gameController = file->metadata.get("controller");
                        badgeSlots.emplace_back(badgeInfo);
                    }
                }
                else if (badge == "altemulator") {
                    if (file->metadata.get(badge) != "")
                        badgeSlots.emplace_back(badgeInfo);
                }
                else {
                    if (file->metadata.get(badge) == "true")
                        badgeSlots.emplace_back(badgeInfo);
                }
            }
            badgeComponent->setBadges(badgeSlots);
        }

        mTextComponents[LegacyText::MD_NAME]->setValue(file->metadata.get("name"));

        if (file->getType() == GAME) {
            if (!hideMetaDataFields) {
                mDateTimeComponents[LegacyDateTime::MD_LASTPLAYED]->setValue(
                    file->metadata.get("lastplayed"));
                mTextComponents[LegacyText::MD_PLAYCOUNT]->setValue(
                    file->metadata.get("playcount"));
            }
        }
        else if (file->getType() == FOLDER) {
            if (!hideMetaDataFields) {
                mDateTimeComponents[LegacyDateTime::MD_LASTPLAYED]->setValue(
                    file->metadata.get("lastplayed"));
                mTextComponents[LegacyText::MD_LBL_PLAYCOUNT]->setVisible(false);
                mTextComponents[LegacyText::MD_PLAYCOUNT]->setVisible(false);
            }
        }

        fadingOut = false;
    }

    std::vector<GuiComponent*> comps;

    for (size_t i = LegacyText::MD_LBL_RATING; i < LegacyText::END; ++i)
        comps.emplace_back(mTextComponents[i].get());
    comps.emplace_back(mDateTimeComponents[LegacyDateTime::MD_RELEASEDATE].get());
    comps.emplace_back(mDateTimeComponents[LegacyDateTime::MD_LASTPLAYED].get());
    comps.emplace_back(mTextComponents[LegacyText::MD_NAME].get());
    comps.emplace_back(mImageComponents[LegacyImage::MD_THUMBNAIL].get());
    comps.emplace_back(mImageComponents[LegacyImage::MD_MARQUEE].get());
    comps.emplace_back(mImageComponents[LegacyImage::MD_IMAGE].get());
    if (mVideoComponents.size() > 0)
        comps.emplace_back(mVideoComponents.front().get());
    comps.push_back(mBadgeComponents.front().get());
    comps.push_back(mRatingComponents.front().get());

    for (auto it = comps.cbegin(); it != comps.cend(); ++it) {
        GuiComponent* comp {*it};
        if ((comp->isAnimationPlaying(0) && comp->isAnimationReversed(0) != fadingOut) ||
            (!comp->isAnimationPlaying(0) &&
             comp->getOpacity() != (fadingOut ? 0.0f : comp->getColorOpacity()))) {
            auto func = [comp](float t) { comp->setOpacity(glm::mix(0.0f, 1.0f, t)); };
            comp->setAnimation(new LambdaAnimation(func, 150), 0, nullptr, fadingOut);
        }
    }

    if (state == CursorState::CURSOR_SCROLLING)
        mLastUpdated = nullptr;
}

void GamelistView::legacyUpdate(int deltaTime)
{
    if (ViewController::getInstance()->getGameLaunchTriggered() &&
        mImageComponents[LegacyImage::MD_IMAGE]->isAnimationPlaying(0))
        mImageComponents[LegacyImage::MD_IMAGE]->finishAnimation(0);

    if (mViewStyle == ViewController::VIDEO) {
        if (ViewController::getInstance()->getGameLaunchTriggered() &&
            mVideoComponents.front()->isAnimationPlaying(0))
            mVideoComponents.front()->finishAnimation(0);
    }

    updateChildren(deltaTime);
}

void GamelistView::legacyInitMDLabels()
{
    std::vector<TextComponent*> components;

    for (size_t i = LegacyText::MD_LBL_RATING; i < LegacyText::MD_DEVELOPER; ++i)
        components.emplace_back(&*mTextComponents[i]);

    const unsigned int colCount {2};
    const unsigned int rowCount {static_cast<unsigned int>(components.size() / 2)};

    glm::vec3 start {mSize.x * 0.01f, mSize.y * 0.625f, 0.0f};

    const float colSize {(mSize.x * 0.48f) / colCount};
    const float rowPadding {0.01f * mSize.y};

    for (unsigned int i = 0; i < components.size(); ++i) {
        const unsigned int row {i % rowCount};
        glm::vec3 pos {0.0f, 0.0f, 0.0f};
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

void GamelistView::legacyInitMDValues()
{
    std::vector<TextComponent*> labels;
    std::vector<GuiComponent*> values;

    std::shared_ptr<Font> defaultFont {Font::get(FONT_SIZE_SMALL)};

    for (size_t i = LegacyText::MD_LBL_RATING; i < LegacyText::MD_DEVELOPER; ++i) {
        labels.emplace_back(&*mTextComponents[i]);
        mTextComponents[i]->setFont(defaultFont);
    }

    for (size_t i = LegacyText::MD_DEVELOPER; i < LegacyText::MD_DESCRIPTION; ++i)
        mTextComponents[i]->setFont(defaultFont);

    mDateTimeComponents[LegacyDateTime::MD_RELEASEDATE]->setFont(defaultFont);
    mDateTimeComponents[LegacyDateTime::MD_LASTPLAYED]->setFont(defaultFont);

    values.emplace_back(mRatingComponents.front().get());
    values.emplace_back(mDateTimeComponents[LegacyDateTime::MD_RELEASEDATE].get());
    values.emplace_back(mTextComponents[LegacyText::MD_DEVELOPER].get());
    values.emplace_back(mTextComponents[LegacyText::MD_PUBLISHER].get());
    values.emplace_back(mTextComponents[LegacyText::MD_GENRE].get());
    values.emplace_back(mTextComponents[LegacyText::MD_PLAYERS].get());
    values.emplace_back(mDateTimeComponents[LegacyDateTime::MD_LASTPLAYED].get());
    values.emplace_back(mTextComponents[LegacyText::MD_PLAYCOUNT].get());

    float bottom {0.0f};
    const float colSize {(mSize.x * 0.48f) / 2.0f};
    for (unsigned int i = 0; i < labels.size(); ++i) {
        const float heightDiff {(labels[i]->getSize().y - values[i]->getSize().y) / 2.0f};
        values[i]->setPosition(mSize.x + glm::vec3 {labels[i]->getSize().x, heightDiff, 0.0f});
        values[i]->setSize(colSize - labels[i]->getSize().x, values[i]->getSize().y);
        values[i]->setDefaultZIndex(40.0f);

        float testBot {values[i]->getPosition().y + values[i]->getSize().y};

        if (testBot > bottom)
            bottom = testBot;
    }

    // Default to off the screen for the following components.
    mRatingComponents.front()->setPosition(Renderer::getScreenWidth() * 2.0f,
                                           Renderer::getScreenHeight() * 2.0f);

    mContainerComponents.front()->setPosition(Renderer::getScreenWidth() * 2.0f,
                                              Renderer::getScreenHeight() * 2.0f);

    for (auto& container : mContainerComponents) {
        container->setPosition(container->getPosition().x, bottom + mSize.y * 0.01f);
        container->setSize(mSize.x * 0.2f, mSize.y * 0.2f);
    }
}

#endif // ES_APP_VIEWS_GAMELIST_LEGACY_H
