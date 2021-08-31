//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  DetailedGameListView.cpp
//
//  Interface that defines a GameListView of the type 'detailed'.
//

#include "views/gamelist/DetailedGameListView.h"

#include "CollectionSystemsManager.h"
#include "SystemData.h"
#include "animations/LambdaAnimation.h"
#include "views/ViewController.h"

#define FADE_IN_START_OPACITY 0.5f
#define FADE_IN_TIME 650

DetailedGameListView::DetailedGameListView(Window* window, FileData* root)
    : BasicGameListView(window, root)
    , mDescContainer(window)
    , mDescription(window)
    , mGamelistInfo(window)
    , mThumbnail(window)
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
    , mLastUpdated(nullptr)
{
    const float padding = 0.01f;

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

    mGamelistInfo.setOrigin(0.5f, 0.5f);
    mGamelistInfo.setFont(Font::get(FONT_SIZE_SMALL));
    mGamelistInfo.setDefaultZIndex(50.0f);
    mGamelistInfo.setVisible(true);
    addChild(&mGamelistInfo);

    initMDLabels();
    initMDValues();
}

void DetailedGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
    BasicGameListView::onThemeChanged(theme);

    using namespace ThemeFlags;
    mThumbnail.applyTheme(theme, getName(), "md_thumbnail",
                          POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION | VISIBLE);
    mMarquee.applyTheme(theme, getName(), "md_marquee",
                        POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION | VISIBLE);
    mImage.applyTheme(theme, getName(), "md_image",
                      POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION | VISIBLE);
    mName.applyTheme(theme, getName(), "md_name", ALL);

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

    mGamelistInfo.applyTheme(theme, getName(), "gamelistInfo", ALL ^ ThemeFlags::TEXT);
    // If there is no position defined in the theme for gamelistInfo, then hide it.
    if (mGamelistInfo.getPosition() == glm::vec3{})
        mGamelistInfo.setVisible(false);
    else
        mGamelistInfo.setVisible(true);

    sortChildren();
}

void DetailedGameListView::initMDLabels()
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

void DetailedGameListView::initMDValues()
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

void DetailedGameListView::updateInfoPanel()
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

    bool fadingOut = false;
    if (file == nullptr) {
        fadingOut = true;
    }
    else {
        // If we're browsing a grouped custom collection, then update the folder metadata
        // which will generate a description of three random games and return a pointer to
        // the first of these so that we can display its game media.
        if (file->getSystem()->isCustomCollection() &&
            file->getPath() == file->getSystem()->getName()) {
            mRandomGame =
                CollectionSystemsManager::get()->updateCollectionFolderMetadata(file->getSystem());
            if (mRandomGame) {
                mThumbnail.setImage(mRandomGame->getThumbnailPath());
                mMarquee.setImage(mRandomGame->getMarqueePath(), false, true);
                mImage.setImage(mRandomGame->getImagePath());
            }
            else {
                mThumbnail.setImage("");
                mMarquee.setImage("");
                mImage.setImage("");
            }
        }
        else {
            mThumbnail.setImage(file->getThumbnailPath());
            mMarquee.setImage(file->getMarqueePath(), false, true);
            mImage.setImage(file->getImagePath());
        }

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
    comps.push_back(&mThumbnail);
    comps.push_back(&mMarquee);
    comps.push_back(&mImage);
    comps.push_back(&mDescription);
    comps.push_back(&mName);
    std::vector<TextComponent*> labels = getMDLabels();
    comps.insert(comps.cend(), labels.cbegin(), labels.cend());

    for (auto it = comps.cbegin(); it != comps.cend(); it++) {
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

void DetailedGameListView::launch(FileData* game)
{
    ViewController::get()->triggerGameLaunch(game);
}

std::vector<TextComponent*> DetailedGameListView::getMDLabels()
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

std::vector<GuiComponent*> DetailedGameListView::getMDValues()
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

void DetailedGameListView::update(int deltaTime)
{
    BasicGameListView::update(deltaTime);
    mImage.update(deltaTime);

    if (ViewController::get()->getGameLaunchTriggered() && mImage.isAnimationPlaying(0))
        mImage.finishAnimation(0);
}

void DetailedGameListView::onShow()
{
    mLastUpdated = nullptr;
    GuiComponent::onShow();
    updateInfoPanel();
}
