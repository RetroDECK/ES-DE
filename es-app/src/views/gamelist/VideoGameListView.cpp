//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  VideoGameListView.cpp
//
//  Interface that defines a GameListView of the type 'video'.
//

#include "views/gamelist/VideoGameListView.h"

#include "animations/LambdaAnimation.h"
#if defined(_RPI_)
#include "components/VideoOmxComponent.h"
#endif
#include "components/VideoVlcComponent.h"
#include "utils/FileSystemUtil.h"
#include "views/ViewController.h"
#if defined(_RPI_)
#include "Settings.h"
#endif
#include "CollectionSystemManager.h"
#include "SystemData.h"

#define FADE_IN_START_OPACITY 0.5f
#define FADE_IN_TIME 650

VideoGameListView::VideoGameListView(
        Window* window,
        FileData* root)
        : BasicGameListView(window, root),
        mDescContainer(window),
        mDescription(window),
        mGamelistInfo(window),

        mThumbnail(window),
        mMarquee(window),
        mImage(window),
        mVideo(nullptr),
        mVideoPlaying(false),

        mLblRating(window),
        mLblReleaseDate(window),
        mLblDeveloper(window),
        mLblPublisher(window),
        mLblGenre(window),
        mLblPlayers(window),
        mLblLastPlayed(window),
        mLblPlayCount(window),

        mRating(window),
        mReleaseDate(window),
        mDeveloper(window),
        mPublisher(window),
        mGenre(window),
        mPlayers(window),
        mLastPlayed(window),
        mPlayCount(window),
        mName(window),
        mLastUpdated(nullptr)
{
    const float padding = 0.01f;

    // Create the correct type of video window.
    #if defined(_RPI_)
    if (Settings::getInstance()->getBool("VideoOmxPlayer"))
        mVideo = new VideoOmxComponent(window);
    else
        mVideo = new VideoVlcComponent(window);
    #else
    mVideo = new VideoVlcComponent(window);
    #endif

    mList.setPosition(mSize.x() * (0.50f + padding), mList.getPosition().y());
    mList.setSize(mSize.x() * (0.50f - padding), mList.getSize().y());
    mList.setAlignment(TextListComponent<FileData*>::ALIGN_LEFT);
    mList.setCursorChangedCallback([&](const CursorState& /*state*/) { updateInfoPanel(); });

    // Thumbnail.
    mThumbnail.setOrigin(0.5f, 0.5f);
    mThumbnail.setPosition(2.0f, 2.0f);
    mThumbnail.setVisible(false);
    mThumbnail.setMaxSize(mSize.x() * (0.25f - 2 * padding), mSize.y() * 0.10f);
    mThumbnail.setDefaultZIndex(35);
    addChild(&mThumbnail);

    // Marquee.
    mMarquee.setOrigin(0.5f, 0.5f);
    mMarquee.setPosition(mSize.x() * 0.25f, mSize.y() * 0.10f);
    mMarquee.setMaxSize(mSize.x() * (0.5f - 2 * padding), mSize.y() * 0.18f);
    mMarquee.setDefaultZIndex(35);
    addChild(&mMarquee);

    // Video.
    mVideo->setOrigin(0.5f, 0.5f);
    mVideo->setPosition(mSize.x() * 0.25f, mSize.y() * 0.4f);
    mVideo->setSize(mSize.x() * (0.5f - 2 * padding), mSize.y() * 0.4f);
    mVideo->setDefaultZIndex(30);
    addChild(mVideo);

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

    mName.setPosition(mSize.x(), mSize.y());
    mName.setDefaultZIndex(40);
    mName.setColor(0xAAAAAAFF);
    mName.setFont(Font::get(FONT_SIZE_MEDIUM));
    mName.setHorizontalAlignment(ALIGN_CENTER);
    addChild(&mName);

    mDescContainer.setPosition(mSize.x() * padding, mSize.y() * 0.65f);
    mDescContainer.setSize(mSize.x() * (0.50f - 2 * padding), mSize.y() -
            mDescContainer.getPosition().y());
    mDescContainer.setAutoScroll(true);
    mDescContainer.setDefaultZIndex(40);
    addChild(&mDescContainer);

    mDescription.setFont(Font::get(FONT_SIZE_SMALL));
    mDescription.setSize(mDescContainer.getSize().x(), 0);
    mDescContainer.addChild(&mDescription);

    mGamelistInfo.setOrigin(0.5f, 0.5f);
    mGamelistInfo.setFont(Font::get(FONT_SIZE_SMALL));
    mGamelistInfo.setDefaultZIndex(50);
    mGamelistInfo.setVisible(true);
    addChild(&mGamelistInfo);

    initMDLabels();
    initMDValues();
}

VideoGameListView::~VideoGameListView()
{
    delete mVideo;
}

void VideoGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
    BasicGameListView::onThemeChanged(theme);

    using namespace ThemeFlags;
    mThumbnail.applyTheme(theme, getName(), "md_thumbnail",
            POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION | VISIBLE);
    mMarquee.applyTheme(theme, getName(), "md_marquee",
            POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION | VISIBLE);
    mImage.applyTheme(theme, getName(), "md_image",
            POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION | VISIBLE);
    mVideo->applyTheme(theme, getName(), "md_video",
            POSITION | ThemeFlags::SIZE | ThemeFlags::DELAY | Z_INDEX | ROTATION | VISIBLE);
    mName.applyTheme(theme, getName(), "md_name", ALL);

    initMDLabels();
    std::vector<TextComponent*> labels = getMDLabels();
    assert(labels.size() == 8);
    std::vector<std::string> lblElements = {
            "md_lbl_rating", "md_lbl_releasedate", "md_lbl_developer", "md_lbl_publisher",
            "md_lbl_genre", "md_lbl_players", "md_lbl_lastplayed", "md_lbl_playcount"
    };

    for (unsigned int i = 0; i < labels.size(); i++)
        labels[i]->applyTheme(theme, getName(), lblElements[i], ALL);

    initMDValues();
    std::vector<GuiComponent*> values = getMDValues();
    assert(values.size() == 8);
    std::vector<std::string> valElements = {
            "md_rating", "md_releasedate", "md_developer", "md_publisher",
            "md_genre", "md_players", "md_lastplayed", "md_playcount"
    };

    for (unsigned int i = 0; i < values.size(); i++)
        values[i]->applyTheme(theme, getName(), valElements[i], ALL ^ ThemeFlags::TEXT);

    mDescContainer.applyTheme(theme, getName(), "md_description",
            POSITION | ThemeFlags::SIZE | Z_INDEX | VISIBLE);
    mDescription.setSize(mDescContainer.getSize().x(), 0);
    mDescription.applyTheme(theme, getName(), "md_description",
            ALL ^ (POSITION | ThemeFlags::SIZE | ThemeFlags::ORIGIN | TEXT | ROTATION));

    mGamelistInfo.applyTheme(theme, getName(), "gamelistInfo", ALL ^ ThemeFlags::TEXT);

    sortChildren();
}

void VideoGameListView::initMDLabels()
{
    std::vector<TextComponent*> components = getMDLabels();

    const unsigned int colCount = 2;
    const unsigned int rowCount = static_cast<int>(components.size() / 2);

    Vector3f start(mSize.x() * 0.01f, mSize.y() * 0.625f, 0.0f);

    const float colSize = (mSize.x() * 0.48f) / colCount;
    const float rowPadding = 0.01f * mSize.y();

    for (unsigned int i = 0; i < components.size(); i++) {
        const unsigned int row = i % rowCount;
        Vector3f pos(0.0f, 0.0f, 0.0f);
        if (row == 0) {
            pos = start + Vector3f(colSize * (i / rowCount), 0, 0);
        }
        else {
            // Work from the last component.
            GuiComponent* lc = components[i-1];
            pos = lc->getPosition() + Vector3f(0, lc->getSize().y() + rowPadding, 0);
        }

        components[i]->setFont(Font::get(FONT_SIZE_SMALL));
        components[i]->setPosition(pos);
        components[i]->setDefaultZIndex(40);
    }
}

void VideoGameListView::initMDValues()
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

    const float colSize = (mSize.x() * 0.48f) / 2;
    for (unsigned int i = 0; i < labels.size(); i++) {
        const float heightDiff = (labels[i]->getSize().y() - values[i]->getSize().y()) / 2;
        values[i]->setPosition(labels[i]->getPosition() +
                Vector3f(labels[i]->getSize().x(),heightDiff, 0));
        values[i]->setSize(colSize - labels[i]->getSize().x(), values[i]->getSize().y());
        values[i]->setDefaultZIndex(40);

        float testBot = values[i]->getPosition().y() + values[i]->getSize().y();

        if (testBot > bottom)
            bottom = testBot;
    }

    mDescContainer.setPosition(mDescContainer.getPosition().x(), bottom + mSize.y() * 0.01f);
    mDescContainer.setSize(mDescContainer.getSize().x(), mSize.y() -
            mDescContainer.getPosition().y());
}

void VideoGameListView::updateInfoPanel()
{
    FileData* file = (mList.size() == 0 || mList.isScrolling()) ? nullptr : mList.getSelected();

    // If the game data has already been rendered to the info panel, then skip it this time.
    if (file == mLastUpdated)
        return;

    if (!mList.isScrolling())
        mLastUpdated = file;

    bool hideMetaDataFields = false;

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

    bool fadingOut;
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
            FileData* randomGame = CollectionSystemManager::get()->
                    updateCollectionFolderMetadata(file->getSystem());
            if (randomGame) {
                mThumbnail.setImage(randomGame->getThumbnailPath());
                mMarquee.setImage(randomGame->getMarqueePath());
                mVideo->setImage(randomGame->getImagePath());
                // Always stop the video before setting a new video as it will otherwise continue
                // to play if it has the same path (i.e. it is the same physical video file) as
                // the previously set video.
                // That may happen when entering a folder with the same name as the first game
                // file inside, or as in this case, when entering a custom collection.
                mVideo->onHide();

                if (!mVideo->setVideo(randomGame->getVideoPath()))
                    mVideo->setDefaultVideo();
            }
            else {
                mThumbnail.setImage("");
                mMarquee.setImage("");
                mVideo->setImage("");
                mVideo->setVideo("");
                mVideo->setDefaultVideo();
            }
        }
        else {
            mThumbnail.setImage(file->getThumbnailPath());
            mMarquee.setImage(file->getMarqueePath());
            mVideo->setImage(file->getImagePath());
            mVideo->onHide();

            if (!mVideo->setVideo(file->getVideoPath()))
                mVideo->setDefaultVideo();
        }

        mVideoPlaying = true;

        // Populate the gamelistInfo field which shows an icon if a folder has been entered
        // as well as the game count for the entire system (total and favorites separately).
        // If a filter has been applied, then the number of filtered and total games replaces
        // the game counter.
        std::string gamelistInfoString;

        if (mIsFolder)
            gamelistInfoString = "\uF07C  ";

        if (mIsFiltered) {
            if (mFilteredGameCountAll == mFilteredGameCount)
                gamelistInfoString += "\uF0b0 " + std::to_string(mFilteredGameCount) + " / " +
                        std::to_string(mGameCount);
            else
                gamelistInfoString += "\uF0b0 " + std::to_string(mFilteredGameCount) + " + " +
                        std::to_string(mFilteredGameCountAll - mFilteredGameCount) + " / " +
                        std::to_string(mGameCount);
        }
        else {
            gamelistInfoString += "\uF11b " + std::to_string(mGameCount);
            if (!(file->getSystem()->isCollection() &&
                    file->getSystem()->getFullName() == "favorites"))
                gamelistInfoString += "  \uF005 " + std::to_string(mFavoritesGameCount);
        }

        mGamelistInfo.setValue(gamelistInfoString);

        // Fade in the game image.
        auto func = [this](float t) {
            mVideo->setOpacity(static_cast<unsigned char>(Math::lerp(
                    static_cast<float>(FADE_IN_START_OPACITY), 1.0f, t) * 255));
            };
        mVideo->setAnimation(new LambdaAnimation(func, FADE_IN_TIME), 0, nullptr, false);

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
    comps.push_back(&mThumbnail);
    comps.push_back(&mMarquee);
    comps.push_back(mVideo);
    comps.push_back(&mDescription);
    comps.push_back(&mName);
    std::vector<TextComponent*> labels = getMDLabels();
    comps.insert(comps.cend(), labels.cbegin(), labels.cend());

    for (auto it = comps.cbegin(); it != comps.cend(); it++) {
        GuiComponent* comp = *it;
        // An animation is playing, then animate if reverse != fadingOut.
        // An animation is not playing, then animate if opacity != our target opacity
        if ((comp->isAnimationPlaying(0) && comp->isAnimationReversed(0) != fadingOut) ||
            (!comp->isAnimationPlaying(0) && comp->getOpacity() != (fadingOut ? 0 : 255))) {
            auto func = [comp](float t) {
                comp->setOpacity(static_cast<unsigned char>(Math::lerp(0.0f, 1.0f, t) * 255));
            };
            comp->setAnimation(new LambdaAnimation(func, 200), 0, nullptr, fadingOut);
        }
    }
}

void VideoGameListView::launch(FileData* game)
{
    ViewController::get()->triggerGameLaunch(game);
}

std::vector<TextComponent*> VideoGameListView::getMDLabels()
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

std::vector<GuiComponent*> VideoGameListView::getMDValues()
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

void VideoGameListView::update(int deltaTime)
{
    if (!mVideoPlaying)
        mVideo->onHide();
    else if (mVideoPlaying && !mVideo->isVideoPaused() && !mWindow->isScreensaverActive())
        mVideo->onShow();

    BasicGameListView::update(deltaTime);
    mVideo->update(deltaTime);

    if (ViewController::get()->getGameLaunchTriggered() && mVideo->isAnimationPlaying(0))
        mVideo->finishAnimation(0);
}

void VideoGameListView::onShow()
{
    mLastUpdated = nullptr;
    GuiComponent::onShow();
    updateInfoPanel();
}
