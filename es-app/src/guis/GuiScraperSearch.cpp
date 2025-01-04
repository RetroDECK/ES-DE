//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiScraperSearch.cpp
//
//  User interface for the scraper where the user is able to see an overview
//  of the game being scraped and an option to override the game search string.
//  Used by both single-game scraping from the GuiMetaDataEd menu as well as
//  to resolve scraping conflicts when run from GuiScraperMenu.
//  The function to properly save scraped metadata is located here too.
//
//  This GUI is called from GuiScraperSingle for single-game scraping and
//  from GuiScraperMulti for multi-game scraping.
//

#include "guis/GuiScraperSearch.h"

#include "CollectionSystemsManager.h"
#include "FileData.h"
#include "Log.h"
#include "MameNames.h"
#include "PlatformId.h"
#include "SystemData.h"
#include "Window.h"
#include "components/ComponentList.h"
#include "components/DateTimeEditComponent.h"
#include "components/ImageComponent.h"
#include "components/RatingComponent.h"
#include "components/ScrollableContainer.h"
#include "components/TextComponent.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiTextEditKeyboardPopup.h"
#include "guis/GuiTextEditPopup.h"
#include "resources/Font.h"
#include "utils/LocalizationUtil.h"
#include "utils/StringUtil.h"

GuiScraperSearch::GuiScraperSearch(SearchType type, unsigned int scrapeCount, int rowCount)
    : mRenderer {Renderer::getInstance()}
    , mGrid {glm::ivec2 {5, 3}}
    , mSearchType {type}
    , mRowCount {rowCount}
    , mScrapeCount {scrapeCount}
    , mNextSearch {false}
    , mHashSearch {false}
    , mRefinedSearch {false}
    , mBlockAccept {false}
    , mAcceptedResult {false}
    , mFoundGame {false}
    , mScrapeRatings {false}
    , mRetrySearch {false}
    , mRetryCount {0}
    , mRetryTimer {glm::clamp(
          Settings::getInstance()->getInt("ScraperRetryOnErrorTimer") * 1000, 1000, 30000)}
    , mRetryAccumulator {0}
    , mAutomaticModeGameEntry {0}
{
    addChild(&mGrid);

    mWindow->setAllowTextScrolling(true);

    // Left spacer (empty component, needed for borders).
    mGrid.setEntry(std::make_shared<GuiComponent>(), glm::ivec2 {0, 0}, false, false,
                   glm::ivec2 {1, 3}, GridFlags::BORDER_TOP | GridFlags::BORDER_BOTTOM);

    // Selected result name.
    mResultName = std::make_shared<TextComponent>("Result name", Font::get(FONT_SIZE_MEDIUM),
                                                  mMenuColorPrimary);

    // Selected result thumbnail.
    mResultThumbnail = std::make_shared<ImageComponent>();
    mGrid.setEntry(mResultThumbnail, glm::ivec2 {1, 1}, false, false, glm::ivec2 {1, 1});

    // Selected result description and container.
    mDescContainer = std::make_shared<ScrollableContainer>();

    // Adjust the game description text scrolling parameters depending on the search type.
    if (mSearchType == MANUAL_MODE || mSearchType == SEMIAUTOMATIC_MODE)
        mDescContainer->setScrollParameters(3000.0f, 3000.0f, 0.8f);
    else
        mDescContainer->setScrollParameters(6000.0f, 3000.0f, 0.8f);

    mResultDesc = std::make_shared<TextComponent>("Result desc", Font::get(FONT_SIZE_SMALL),
                                                  mMenuColorPrimary, ALIGN_LEFT, ALIGN_CENTER,
                                                  glm::ivec2 {0, 1});
    mDescContainer->addChild(mResultDesc.get());
    mDescContainer->setAutoScroll(true);

    // Metadata.
    auto font = Font::get(FONT_SIZE_SMALL); // Placeholder, gets replaced in onSizeChanged().
    const unsigned int mdColor {mMenuColorPrimary};
    const unsigned int mdLblColor {mMenuColorTertiary};
    mMD_Rating = std::make_shared<RatingComponent>(false, true);
    mMD_ReleaseDate = std::make_shared<DateTimeEditComponent>();
    mMD_ReleaseDate->setColor(mdColor);
    mMD_ReleaseDate->setUppercase(true);
    mMD_Developer = std::make_shared<TextComponent>("", font, mdColor, ALIGN_LEFT);
    mMD_Publisher = std::make_shared<TextComponent>("", font, mdColor, ALIGN_LEFT);
    mMD_Genre = std::make_shared<TextComponent>("", font, mdColor, ALIGN_LEFT);
    mMD_Players = std::make_shared<TextComponent>("", font, mdColor, ALIGN_LEFT);
    mMD_Filler = std::make_shared<TextComponent>("", font, mdColor);

    if (Settings::getInstance()->getString("Scraper") != "thegamesdb")
        mScrapeRatings = true;

    if (mScrapeRatings)
        mMD_Pairs.push_back(MetaDataPair(
            std::make_shared<TextComponent>(_("RATING:"), font, mdLblColor), mMD_Rating, false));

    mMD_Pairs.push_back(MetaDataPair(
        std::make_shared<TextComponent>(_("RELEASED:"), font, mdLblColor), mMD_ReleaseDate));
    mMD_Pairs.push_back(MetaDataPair(
        std::make_shared<TextComponent>(_("DEVELOPER:"), font, mdLblColor), mMD_Developer));
    mMD_Pairs.push_back(MetaDataPair(
        std::make_shared<TextComponent>(_("PUBLISHER:"), font, mdLblColor), mMD_Publisher));
    mMD_Pairs.push_back(
        MetaDataPair(std::make_shared<TextComponent>(_("GENRE:"), font, mdLblColor), mMD_Genre));
    mMD_Pairs.push_back(MetaDataPair(
        std::make_shared<TextComponent>(_("PLAYERS:"), font, mdLblColor), mMD_Players));

    // If no rating is being scraped, add a filler to make sure that the fonts keep the same
    // size so the GUI looks consistent.
    if (!mScrapeRatings)
        mMD_Pairs.push_back(
            MetaDataPair(std::make_shared<TextComponent>("", font, mdLblColor), mMD_Filler));

    mMD_Grid =
        std::make_shared<ComponentGrid>(glm::ivec2 {2, static_cast<int>(mMD_Pairs.size() * 2 - 1)});
    unsigned int i {0};
    for (auto it = mMD_Pairs.cbegin(); it != mMD_Pairs.cend(); ++it) {
        mMD_Grid->setEntry(it->first, glm::ivec2 {0, i}, false, true);
        mMD_Grid->setEntry(it->second, glm::ivec2 {1, i}, false, it->resize);
        i += 2;
    }

    mGrid.setEntry(mMD_Grid, glm::ivec2 {2, 1}, false, false);

    // Result list.
    mResultList = std::make_shared<ComponentList>();
    mResultList->setCursorChangedCallback([this](CursorState state) {
        if (state == CursorState::CURSOR_STOPPED)
            updateInfoPane();
    });

    updateView();
}

GuiScraperSearch::~GuiScraperSearch()
{
    // The following manual resets are required to avoid a race condition when the
    // STOP button is pressed in the multi-scraper. Without this code there will be
    // a memory leak as the curl easy handle is not cleaned up. For a normally completed
    // scraping however, the destructor will already have been called in HttpReq.
    if (mSearchHandle)
        mSearchHandle.reset();

    if (mMDRetrieveURLsHandle)
        mMDRetrieveURLsHandle.reset();

    if (mMDResolveHandle)
        mMDResolveHandle.reset();

    if (mThumbnailReqMap.size() > 0)
        mThumbnailReqMap.clear();

    HttpReq::cleanupCurlMulti();

    // This is required to properly refresh the gamelist view if the user aborted the
    // scraping when the miximage was getting generated.
    if (Settings::getInstance()->getBool("MiximageGenerate") &&
        mMiximageGeneratorThread.joinable()) {
        // We always let the miximage generator thread complete.
        mMiximageGeneratorThread.join();
        mMiximageGenerator.reset();
        mScrapeResult.savedNewMedia = true;
        TextureResource::manualUnload(mLastSearch.game->getMiximagePath(), false);
        ViewController::getInstance()->onFileChanged(mLastSearch.game, true);
    }

    if (mCalculateMD5HashThread.joinable())
        mCalculateMD5HashThread.join();

    mWindow->setAllowTextScrolling(false);
}

void GuiScraperSearch::onSizeChanged()
{
    mGrid.setSize(
        glm::vec2 {std::round(mSize.x), (mResultList->getRowHeight() * mRowCount) +
                                            std::round(mRenderer->getScreenHeightModifier())});

    if (mSize.x == 0 || mSize.y == 0)
        return;

    // Column widths.
    if (mSearchType == AUTOMATIC_MODE)
        mGrid.setColWidthPerc(0, 0.02f); // Looks better when this is higher in auto mode.
    else
        mGrid.setColWidthPerc(0, 0.01f);

    mGrid.setColWidthPerc(1, 0.25f);

    if (mSearchType == AUTOMATIC_MODE)
        mGrid.setColWidthPerc(2, 0.33f);
    else
        mGrid.setColWidthPerc(2, (mRenderer->getIsVerticalOrientation() ? 0.34f : 0.30f));

    // Row heights.
    if (mSearchType == AUTOMATIC_MODE) // Show name.
        mGrid.setRowHeightPerc(0, (mResultName->getFont()->getHeight() * 1.6f) /
                                      mGrid.getSize().y); // Result name.
    else
        mGrid.setRowHeightPerc(0, 0.0725f); // Hide name but do padding.

    if (mSearchType == AUTOMATIC_MODE)
        mGrid.setRowHeightPerc(2, 0.2f);
    else
        mGrid.setRowHeightPerc(1, 0.505f);

    const float thumbnailCellScale {0.93f};

    // Limit the thumbnail size using setMaxSize so the aspect ratio is maintained, and also
    // add some padding
    mResultThumbnail->setMaxSize(mGrid.getColWidth(1) * thumbnailCellScale, mGrid.getRowHeight(1));

    // Metadata.
    resizeMetadata();

    // Small vertical spacer between the metadata fields and the result list.
    mGrid.setColWidthPerc(3, 0.004f);

    if (mSearchType != AUTOMATIC_MODE)
        mDescContainer->setSize(mGrid.getColWidth(1) * thumbnailCellScale + mGrid.getColWidth(2),
                                mResultDesc->getFont()->getHeight() * 3.2f);
    else
        mDescContainer->setSize(mGrid.getColWidth(4) * thumbnailCellScale,
                                mResultDesc->getFont()->getHeight() * 8.0f);

    // Make description text wrap at edge of container.
    mResultDesc->setSize(mDescContainer->getSize().x, 0.0f);

    // Set the width of mResultName to the cell width so that text abbreviation will work correctly.
    mResultName->setSize(mGrid.getColWidth(1) + mGrid.getColWidth(2), mResultName->getSize().y);

    mGrid.onSizeChanged();
    mBusyAnim.setSize(mSize);
}

void GuiScraperSearch::resizeMetadata()
{
    mMD_Grid->setSize(mGrid.getColWidth(2), mGrid.getRowHeight(1));
    if (mMD_Grid->getSize().y > mMD_Pairs.size()) {
        const float fontHeight {mMD_Grid->getSize().y / mMD_Pairs.size() * 0.8f};
        auto fontLbl = Font::get(fontHeight, FONT_PATH_REGULAR);
        auto fontComp = Font::get(fontHeight, FONT_PATH_LIGHT);

        // Update label fonts.
        float maxLblWidth {0.0f};
        for (auto it = mMD_Pairs.cbegin(); it != mMD_Pairs.cend(); ++it) {
            it->first->setFont(fontLbl);
            if (it->first->getTextCache() != nullptr &&
                it->first->getTextCache()->metrics.size.x > maxLblWidth)
                maxLblWidth = it->first->getTextCache()->metrics.size.x +
                              (16.0f * (mRenderer->getIsVerticalOrientation() ?
                                            mRenderer->getScreenHeightModifier() :
                                            mRenderer->getScreenWidthModifier()));
        }

        for (unsigned int i {0}; i < mMD_Pairs.size(); ++i)
            mMD_Grid->setRowHeightPerc(
                i * 2,
                (fontLbl->getLetterHeight() + (2.0f * (mRenderer->getIsVerticalOrientation() ?
                                                           mRenderer->getScreenWidthModifier() :
                                                           mRenderer->getScreenHeightModifier()))) /
                    mMD_Grid->getSize().y);

        // Update component fonts.
        mMD_ReleaseDate->setFont(fontComp);
        mMD_Developer->setFont(fontComp);
        mMD_Publisher->setFont(fontComp);
        mMD_Genre->setFont(fontComp);
        mMD_Players->setFont(fontComp);

        mMD_Grid->setColWidthPerc(0, maxLblWidth / mMD_Grid->getSize().x);

        if (mScrapeRatings) {
            // Make sure the rating component fits inside the column width regardless of screen
            // aspect ratio. Also move the component slightly to the left to compensate for the
            // padding baked into the actual SVG file.
            float ratingWidth {mMD_Grid->getRowHeight(4) * 5.0f * 1.23f};
            ratingWidth =
                std::round(glm::clamp(ratingWidth, 0.0f, mMD_Developer->getSize().x * 0.98f));
            mMD_Rating->setSize(0, std::round(ratingWidth / 5.0f));
            mMD_Grid->onSizeChanged();
            mMD_Rating->setPosition(
                std::round(maxLblWidth - std::round(mMD_Rating->getSize().y / 10.0f)),
                mMD_Rating->getPosition().y);
        }

        // Make result font follow label font.
        mResultDesc->setFont(Font::get(fontHeight, FONT_PATH_REGULAR));
    }
}

void GuiScraperSearch::updateView()
{
    // Unlink description, result list and result name.
    mGrid.removeEntry(mResultName);
    mGrid.removeEntry(mResultDesc);
    mGrid.removeEntry(mResultList);

    // Add them back depending on search type.
    if (mSearchType == AUTOMATIC_MODE) {
        // Show name.
        mGrid.setEntry(mResultName, glm::ivec2 {1, 0}, false, false, glm::ivec2 {3, 1},
                       GridFlags::BORDER_TOP);

        // Need a border on the bottom left.
        mGrid.setEntry(std::make_shared<GuiComponent>(), glm::ivec2 {0, 2}, false, false,
                       glm::ivec2 {4, 1}, GridFlags::BORDER_BOTTOM);

        // Show description on the right.
        mGrid.setEntry(mDescContainer, glm::ivec2 {4, 0}, false, false, glm::ivec2 {1, 3},
                       GridFlags::BORDER_TOP | GridFlags::BORDER_BOTTOM | GridFlags::BORDER_LEFT);
        // Make description text wrap at edge of container.
        mResultDesc->setSize(mDescContainer->getSize().x, 0.0f);
    }
    else {
        // Fake row where name would be.
        mGrid.setEntry(std::make_shared<GuiComponent>(), glm::ivec2 {1, 0}, false, true,
                       glm::ivec2 {3, 1}, GridFlags::BORDER_TOP);

        // Show result list on the right.
        mGrid.setEntry(mResultList, glm::ivec2 {4, 0}, true, true, glm::ivec2 {1, 3},
                       GridFlags::BORDER_LEFT | GridFlags::BORDER_TOP | GridFlags::BORDER_BOTTOM);

        // Show description under image/info.
        mGrid.setEntry(mDescContainer, glm::ivec2 {1, 2}, false, false, glm::ivec2 {3, 1},
                       GridFlags::BORDER_BOTTOM);
        // Make description text wrap at edge of container.
        mResultDesc->setSize(mDescContainer->getSize().x, 0);
    }
}

void GuiScraperSearch::search(ScraperSearchParams& params)
{
    mHashSearch = false;
    mBlockAccept = true;
    mAcceptedResult = false;
    mMiximageResult = false;
    mFoundGame = false;
    mScrapeResult = {};
    mAutomaticModeGameEntry = 0;

    mResultList->clear();
    mScraperResults.clear();
    mMDRetrieveURLsHandle.reset();
    mThumbnailReqMap.clear();
    mMDResolveHandle.reset();
    updateInfoPane();

    // For ScreenScraper we always want to use the jeuInfos (single-game) API call when in
    // automatic mode as this scraper service is not sorting the multi-search results based
    // on most relevant result (as TheGamesDB does). Using jeuInfos is also much faster than
    // using the jeuRecherche API call (multi-game search).
    if (mSearchType == AUTOMATIC_MODE)
        params.automaticMode = true;
    else
        params.automaticMode = false;

    mMD5Hash = "";
    params.md5Hash = "";
    if (!Utils::FileSystem::isDirectory(params.game->getPath()))
        params.fileSize = Utils::FileSystem::getFileSize(params.game->getPath());

    // Only use MD5 file hash searching when in automatic mode.
    if (mSearchType == AUTOMATIC_MODE &&
        Settings::getInstance()->getBool("ScraperSearchFileHash") &&
        Settings::getInstance()->getString("Scraper") == "screenscraper" && params.fileSize != 0 &&
        params.fileSize <=
            Settings::getInstance()->getInt("ScraperSearchFileHashMaxSize") * 1024 * 1024) {

        // Run the MD5 hash calculation in a separate thread as it may take a long time to
        // complete and we don't want to freeze the UI in the meanwhile.
        std::promise<bool>().swap(mMD5HashPromise);
        mMD5HashFuture = mMD5HashPromise.get_future();

        mHashSearch = true;
        mCalculateMD5HashThread =
            std::thread(&GuiScraperSearch::calculateMD5Hash, this, params.game->getPath());
    }

    mLastSearch = params;
    mSearchHandle = nullptr;
    mNextSearch = true;
}

void GuiScraperSearch::stop()
{
    mThumbnailReqMap.clear();
    mSearchHandle.reset();
    mMDResolveHandle.reset();
    mMDRetrieveURLsHandle.reset();
    mMiximageGenerator.reset();
    mBlockAccept = false;
    mAcceptedResult = false;
    mMiximageResult = false;
    mScrapeResult = {};
}

void GuiScraperSearch::onSearchDone(std::vector<ScraperSearchResult>& results)
{
    mResultList->clear();
    mScraperResults = results;

    auto font = Font::get(FONT_SIZE_MEDIUM);
    unsigned int color {mMenuColorPrimary};
    if (results.empty()) {
        // Check if the scraper used is still valid.
        if (!isValidConfiguredScraper()) {
            mWindow->pushGui(new GuiMsgBox(
                getHelpStyle(),
                Utils::String::toUpper("Configured scraper is no longer available.\n"
                                       "Please change the scraping source in the settings."),
                "FINISH", mSkipCallback));
        }
        else {
            LOG(LogDebug)
                << "GuiScraperSearch::onSearchDone(): Scraper service did not return any results";

            mFoundGame = false;
            ComponentListRow row;
            row.addElement(std::make_shared<TextComponent>(_("NO GAMES FOUND"), font, color), true);

            if (mSkipCallback)
                row.makeAcceptInputHandler(mSkipCallback);

            mResultList->addRow(row);
        }
    }
    else {
        mFoundGame = true;
        ComponentListRow row;

        for (size_t i {0}; i < results.size(); ++i) {
            // If the platform IDs returned by the scraper do not match the platform IDs of the
            // scraped game, then add the additional platform information to the end of the game
            // name (within square brackets).
            std::string gameName {results.at(i).mdl.get("name")};
            std::string otherPlatforms;

            if (mMD5Hash != "") {
                const std::string entryText {
                    results.size() > 1 ? "Result entry " + std::to_string(i) + ": " : ""};
                if (results[i].md5Hash == mMD5Hash) {
                    mAutomaticModeGameEntry = static_cast<int>(i);
                    LOG(LogDebug)
                        << "GuiScraperSearch::onSearchDone(): " << entryText
                        << "Perfect match, MD5 digest in server response identical to file hash";
                }
                else if (results[i].md5Hash != "") {
                    LOG(LogDebug) << "GuiScraperSearch::onSearchDone(): " << entryText
                                  << "Not a perfect match, MD5 digest in server response not "
                                     "identical to file hash";
                }
                else {
                    LOG(LogDebug) << "GuiScraperSearch::onSearchDone(): " << entryText
                                  << "Server did not return an MD5 digest, can't tell whether this "
                                     "is a perfect match";
                }
            }

            // As the platform names are found via reverse lookup there could be multiple entries.
            // So if any of the entries match the platforms of the last search, then just keep
            // this platform ID and remove the other ones.
            for (auto& platformID : mLastSearch.system->getSystemEnvData()->mPlatformIds) {
                if (!results.at(i).platformIDs.empty() &&
                    std::find(results.at(i).platformIDs.begin(), results.at(i).platformIDs.end(),
                              platformID) != results.at(i).platformIDs.end()) {
                    results.at(i).platformIDs.clear();
                    results.at(i).platformIDs.push_back(platformID);
                }
            }

            bool hasOtherPlatforms {false};

            for (auto& platformID : mLastSearch.system->getSystemEnvData()->mPlatformIds) {
                if (!results.at(i).platformIDs.empty() &&
                    std::find(results.at(i).platformIDs.cbegin(), results.at(i).platformIDs.cend(),
                              platformID) == results.at(i).platformIDs.cend())
                    hasOtherPlatforms = true;
            }

            if (hasOtherPlatforms) {
                if (std::find(results.at(i).platformIDs.cbegin(), results.at(i).platformIDs.cend(),
                              PlatformIds::PlatformId::PC) != results.at(i).platformIDs.cend()) {
                    // The PC platform is a bit special as it's widely used by a number of
                    // different systems. As such remove these other IDs and only display the
                    // main PC ID as the list of platforms would otherwise be quite long.
                    otherPlatforms = PlatformIds::getPlatformName(PlatformIds::PlatformId::PC);
                }
                else {
                    for (auto& platform : results.at(i).platformIDs)
                        otherPlatforms += PlatformIds::getPlatformName(platform) + "/";
                }
            }

            if (otherPlatforms != "" && otherPlatforms.back() == '/')
                otherPlatforms.pop_back();

            if (otherPlatforms != "")
                gameName.append(" [").append(otherPlatforms).append("]");

            row.elements.clear();
            auto gameEntry =
                std::make_shared<TextComponent>(Utils::String::toUpper(gameName), font, color);
            gameEntry->setHorizontalScrolling(true);
            row.addElement(gameEntry, true, true, glm::ivec2 {1, 0});
            row.makeAcceptInputHandler([this, i] { returnResult(mScraperResults.at(i)); });
            mResultList->addRow(row);
        }
    }

    mBlockAccept = false;
    updateInfoPane();
    updateHelpPrompts();

    // If there is a single result in semi-automatic mode or a single or more results in
    // fully automatic mode, then block the ability to manually accept the entry as it will
    // be selected as soon as the thumbnail has finished downloading. This also makes sure
    // the busy animation will play during this time window.
    if (!mRefinedSearch && ((mSearchType == SEMIAUTOMATIC_MODE && results.size() == 1) ||
                            (mSearchType == AUTOMATIC_MODE && mScraperResults.size() > 0)))
        mBlockAccept = true;

    // If there is no thumbnail to download and we're in semi-automatic mode, proceed to return
    // the results or we'll get stuck forever waiting for a thumbnail to be downloaded.
    if (mSearchType == SEMIAUTOMATIC_MODE && results.size() == 1 &&
        mScraperResults.front().thumbnailImageUrl == "")
        returnResult(mScraperResults.front());

    // For automatic mode, if there's no thumbnail to download or no matching games found,
    // proceed directly or we'll get stuck forever.
    if (mSearchType == AUTOMATIC_MODE) {
        if (mScraperResults.size() == 0 ||
            (mScraperResults.size() > 0 && mScraperResults.front().thumbnailImageUrl == "")) {
            if (mScraperResults.size() == 0)
                mSkipCallback();
            else
                returnResult(mScraperResults[mAutomaticModeGameEntry]);
        }
    }
}

void GuiScraperSearch::onSearchError(const std::string& error,
                                     const bool retry,
                                     const bool fatalError,
                                     HttpReq::Status status)
{
    if (fatalError) {
        LOG(LogWarning) << "GuiScraperSearch: " << Utils::String::replace(error, "\n", "");
        mWindow->pushGui(new GuiMsgBox(getHelpStyle(), Utils::String::toUpper(error), _("OK"),
                                       mCancelCallback, "", nullptr, "", nullptr, nullptr, true));
        return;
    }

    const int retries {
        glm::clamp(Settings::getInstance()->getInt("ScraperRetryOnErrorCount"), 0, 10)};
    if (retry && mSearchType != MANUAL_MODE && retries > 0 && mRetryCount < retries) {
        LOG(LogError) << "GuiScraperSearch: " << Utils::String::replace(error, "\n", "");
        mRetrySearch = true;
        ++mRetryCount;
        LOG(LogInfo) << "GuiScraperSearch: Attempting automatic retry " << mRetryCount << " of "
                     << retries;
        return;
    }
    else {
        mRetryCount = 0;
    }

    if (mScrapeCount > 1) {
        LOG(LogError) << "GuiScraperSearch: " << Utils::String::replace(error, "\n", "");
        mWindow->pushGui(new GuiMsgBox(getHelpStyle(), Utils::String::toUpper(error), _("RETRY"),
                                       std::bind(&GuiScraperSearch::search, this, mLastSearch),
                                       _("SKIP"), mSkipCallback, _("CANCEL"), mCancelCallback,
                                       nullptr, true));
    }
    else {
        LOG(LogError) << "GuiScraperSearch: " << Utils::String::replace(error, "\n", "");
        mWindow->pushGui(new GuiMsgBox(getHelpStyle(), Utils::String::toUpper(error), _("RETRY"),
                                       std::bind(&GuiScraperSearch::search, this, mLastSearch),
                                       _("CANCEL"), mCancelCallback, "", nullptr, nullptr, true));
    }
}

int GuiScraperSearch::getSelectedIndex()
{
    if (!mScraperResults.size() || mGrid.getSelectedComponent() != mResultList)
        return -1;

    return mResultList->getCursorId();
}

void GuiScraperSearch::updateInfoPane()
{
    int i {getSelectedIndex()};
    if (mSearchType == AUTOMATIC_MODE && mScraperResults.size())
        i = 0;

    if (i != -1 && static_cast<int>(mScraperResults.size()) > i) {
        ScraperSearchResult& res {mScraperResults.at(i)};

        mResultName->setText(Utils::String::toUpper(res.mdl.get("name")));
        mResultDesc->setText(Utils::String::toUpper(res.mdl.get("desc")));
        mDescContainer->resetComponent();

        mResultThumbnail->setImage("");
        const std::string& thumb {res.screenshotUrl.empty() ? res.coverUrl : res.screenshotUrl};
        mScraperResults[i].thumbnailImageUrl = thumb;

        // Cache the thumbnail image in mScraperResults so that we don't need to download
        // it every time the list is scrolled back and forth.
        if (mScraperResults[i].thumbnailImageData.size() > 350) {
            std::string content {mScraperResults[i].thumbnailImageData};
            mResultThumbnail->setImage(content.data(), content.length());
            mGrid.onSizeChanged(); // A hack to fix the thumbnail position since its size changed.
        }
        // If it's not cached in mScraperResults it should mean that it's the first time
        // we access the entry, and therefore we need to download the image.
        else {
            if (!thumb.empty()) {
                // Make sure we don't attempt to download the same thumbnail twice.
                if (mScraperResults[i].thumbnailDownloadStatus != IN_PROGRESS) {
                    mScraperResults[i].thumbnailDownloadStatus = IN_PROGRESS;
                    // Add an entry into the thumbnail map, this way we can track and download
                    // each thumbnail separately even as they're downloading while scrolling
                    // through the result list. Add the row number as an index in case the same
                    // thumbnail is used by more than one game.
                    mThumbnailReqMap.insert(std::pair<std::string, std::unique_ptr<HttpReq>>(
                        mScraperResults[i].thumbnailImageUrl + "." + std::to_string(i),
                        std::unique_ptr<HttpReq>(new HttpReq(thumb, true))));
                }
            }
        }

        // Metadata.
        if (mScrapeRatings) {
            mMD_Rating->setValue(Utils::String::toUpper(res.mdl.get("rating")));
            mMD_Rating->setOpacity(1.0f);
        }
        mMD_ReleaseDate->setValue(Utils::String::toUpper(res.mdl.get("releasedate")));

        if (res.mdl.get("developer") == "unknown")
            mMD_Developer->setText(Utils::String::toUpper(_(res.mdl.get("developer").c_str())));
        else
            mMD_Developer->setText(Utils::String::toUpper(res.mdl.get("developer")));

        if (res.mdl.get("publisher") == "unknown")
            mMD_Publisher->setText(Utils::String::toUpper(_(res.mdl.get("publisher").c_str())));
        else
            mMD_Publisher->setText(Utils::String::toUpper(res.mdl.get("publisher")));

        if (res.mdl.get("genre") == "unknown")
            mMD_Genre->setText(Utils::String::toUpper(_(res.mdl.get("genre").c_str())));
        else
            mMD_Genre->setText(Utils::String::toUpper(res.mdl.get("genre")));

        if (res.mdl.get("players") == "unknown")
            mMD_Players->setText(Utils::String::toUpper(_(res.mdl.get("players").c_str())));
        else
            mMD_Players->setText(Utils::String::toUpper(res.mdl.get("players")));

        mGrid.onSizeChanged();
    }
    else {
        mResultName->setText("");
        mResultDesc->setText("");
        mResultThumbnail->setImage("");

        // Metadata.
        if (mScrapeRatings) {
            mMD_Rating->setValue("");
            mMD_Rating->setOpacity(0.0f);
        }
        // Set the release date to this value to force DateTimeEditComponent to put a
        // blank instead of the text 'unknown' prior to the scrape result being returned.
        mMD_ReleaseDate->setValue("19710101T010101");
        mMD_Developer->setText("");
        mMD_Publisher->setText("");
        mMD_Genre->setText("");
        mMD_Players->setText("");
    }
}

bool GuiScraperSearch::input(InputConfig* config, Input input)
{
    if (config->isMappedTo("a", input) && input.value != 0) {
        if (mBlockAccept || mScraperResults.empty())
            return true;
        mResultList->setHorizontalScrolling(false);
    }

    // Check whether we should allow a refine of the game name.
    if (!mAcceptedResult && config->isMappedTo("y", input) && input.value != 0) {
        bool allowRefine {false};

        // Previously refined.
        if (mRefinedSearch)
            allowRefine = true;
        // Interactive mode and "Auto-accept single game matches" not enabled.
        else if (mSearchType != SEMIAUTOMATIC_MODE)
            allowRefine = true;
        // Interactive mode with "Auto-accept single game matches" enabled and more than one result.
        else if (mSearchType == SEMIAUTOMATIC_MODE && mScraperResults.size() > 1)
            allowRefine = true;
        // Dito but there were no games found, or the search has not been completed.
        else if (mSearchType == SEMIAUTOMATIC_MODE && !mFoundGame)
            allowRefine = true;

        if (allowRefine) {
            mResultList->resetSelectedRow();
            openInputScreen(mLastSearch);
        }
    }

    // If multi-scraping, skip game unless the result has already been accepted.
    if (mSkipCallback != nullptr && !mAcceptedResult && // Line break.
        config->isMappedTo("x", input) && input.value)
        mSkipCallback();

    return GuiComponent::input(config, input);
}

void GuiScraperSearch::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans {parentTrans * getTransform()};

    renderChildren(trans);
    mRenderer->drawRect(0.0f, 0.0f, mSize.x,
                        (mResultList->getRowHeight() * mRowCount) +
                            mRenderer->getScreenHeightModifier(),
                        mMenuColorPanelDimmed, mMenuColorPanelDimmed);

    // Slight adjustment upwards so the busy grid is not rendered precisely at the text edge.
    trans = glm::translate(
        trans,
        glm::vec3 {0.0f, std::round(-(mRenderer->getScreenResolutionModifier() * 10.0f)), 0.0f});

    if (mBlockAccept) {
        mRenderer->setMatrix(trans);
        mBusyAnim.render(trans);
    }
}

void GuiScraperSearch::returnResult(ScraperSearchResult result)
{
    mBlockAccept = true;
    mAcceptedResult = true;

    // Resolve metadata image before returning.
    if (result.mediaFilesDownloadStatus != COMPLETED) {
        result.mediaFilesDownloadStatus = IN_PROGRESS;
        LOG(LogDebug) << "GuiScraperSearch::returnResult(): Resolving metadata for \""
                      << result.mdl.get("name") << "\", game ID \"" << result.gameID << "\"";
        mMDResolveHandle = resolveMetaDataAssets(result, mLastSearch);
        return;
    }

    mScrapeCount -= 1;
    mAcceptCallback(result);
    mRefinedSearch = false;
    mRetryCount = 0;
}

void GuiScraperSearch::update(int deltaTime)
{
    // The only purpose of calling startScraperSearch() here instead of in search() is because
    // the optional MD5 hash calculation needs to run in a separate thread to not lock the UI.
    if (mNextSearch && mHashSearch) {
        if (mMD5HashFuture.valid()) {
            // Only wait one millisecond as this update() function runs very frequently.
            if (mMD5HashFuture.wait_for(std::chrono::milliseconds(1)) ==
                std::future_status::ready) {
                if (mCalculateMD5HashThread.joinable())
                    mCalculateMD5HashThread.join();
                mLastSearch.md5Hash = mMD5Hash;
                mSearchHandle = startScraperSearch(mLastSearch);
                mNextSearch = false;
            }
        }
    }
    else if (mNextSearch) {
        mSearchHandle = startScraperSearch(mLastSearch);
        mNextSearch = false;
    }

    GuiComponent::update(deltaTime);

    if (mBlockAccept)
        mBusyAnim.update(deltaTime);

    if (mRetrySearch) {
        // There was an error and we're attempting an automatic retry.
        mRetryAccumulator += deltaTime;
        if (mRetryAccumulator < mRetryTimer)
            return;
        mRetrySearch = false;
        mRetryAccumulator = 0;
        stop();
        search(mLastSearch);
        return;
    }

    // Check if the thumbnail for the currently selected game has finished downloading.
    if (mScraperResults.size() > 0) {
        auto it =
            mThumbnailReqMap.find(mScraperResults[mResultList->getCursorId()].thumbnailImageUrl +
                                  "." + std::to_string(mResultList->getCursorId()));
        if (it != mThumbnailReqMap.end() && it->second->status() != HttpReq::REQ_IN_PROGRESS)
            updateThumbnail();
    }

    if (mSearchHandle && mSearchHandle->status() != ASYNC_IN_PROGRESS) {
        auto status = mSearchHandle->status();
        mScraperResults = mSearchHandle->getResults();
        const std::string statusString {mSearchHandle->getStatusString()};
        const bool retryFlag {mSearchHandle->getRetry()};
        const bool fatalErrorFlag {mSearchHandle->getFatalError()};

        // We reset here because onSearchDone in auto mode can call mSkipCallback() which
        // can call another search() which will set our mSearchHandle to something important.
        mSearchHandle.reset();

        if (status == ASYNC_DONE && mScraperResults.size() == 0)
            onSearchDone(mScraperResults);

        if (status == ASYNC_DONE && mScraperResults.size() > 0) {
            if (mScraperResults.front().mediaURLFetch == COMPLETED) {
                onSearchDone(mScraperResults);
            }
            else {
                std::string gameIDs;
                for (auto it = mScraperResults.cbegin(); it != mScraperResults.cend(); ++it)
                    gameIDs += it->gameID + ',';

                // Remove the last comma
                gameIDs.pop_back();
                mMDRetrieveURLsHandle = startMediaURLsFetch(gameIDs);
            }
        }
        else if (status == ASYNC_ERROR) {
            onSearchError(statusString, retryFlag, fatalErrorFlag);
        }
    }

    if (mMDRetrieveURLsHandle && mMDRetrieveURLsHandle->status() != ASYNC_IN_PROGRESS) {
        if (mMDRetrieveURLsHandle->status() == ASYNC_DONE) {
            auto results_media = mMDRetrieveURLsHandle->getResults();
            auto statusString_media = mMDRetrieveURLsHandle->getStatusString();
            auto results_scrape = mScraperResults;
            mMDRetrieveURLsHandle.reset();
            mScraperResults.clear();

            // Combine the intial scrape results with the media URL results.
            for (auto it = results_media.cbegin(); it != results_media.cend(); ++it) {
                for (unsigned int i = 0; i < results_scrape.size(); ++i) {
                    if (results_scrape[i].gameID == it->gameID) {
                        results_scrape[i].box3DUrl = it->box3DUrl;
                        results_scrape[i].backcoverUrl = it->backcoverUrl;
                        results_scrape[i].coverUrl = it->coverUrl;
                        results_scrape[i].fanartUrl = it->fanartUrl;
                        results_scrape[i].marqueeUrl = it->marqueeUrl;
                        results_scrape[i].screenshotUrl = it->screenshotUrl;
                        results_scrape[i].titlescreenUrl = it->titlescreenUrl;
                        results_scrape[i].physicalmediaUrl = it->physicalmediaUrl;
                        results_scrape[i].videoUrl = it->videoUrl;
                        results_scrape[i].scraperRequestAllowance = it->scraperRequestAllowance;
                        results_scrape[i].mediaURLFetch = COMPLETED;
                    }
                }
            }
            onSearchDone(results_scrape);
        }
        else if (mMDRetrieveURLsHandle->status() == ASYNC_ERROR) {
            onSearchError(mMDRetrieveURLsHandle->getStatusString(),
                          mMDRetrieveURLsHandle->getRetry(),
                          (mSearchHandle != nullptr ? mSearchHandle->getFatalError() : false));
            mMDRetrieveURLsHandle.reset();
        }
    }

    // Check if a miximage generator thread was started, and if the processing has been completed.
    if (mMiximageGenerator && mGeneratorFuture.valid()) {
        // Only wait one millisecond as this update() function runs very frequently.
        if (mGeneratorFuture.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready) {
            mMDResolveHandle.reset();
            // We always let the miximage generator thread complete.
            if (mMiximageGeneratorThread.joinable())
                mMiximageGeneratorThread.join();
            if (!mGeneratorFuture.get())
                mScrapeResult.savedNewMedia = true;
            returnResult(mScrapeResult);
            mMiximageGenerator.reset();
        }
    }

    if (mMDResolveHandle && mMDResolveHandle->status() != ASYNC_IN_PROGRESS) {
        if (mMDResolveHandle->status() == ASYNC_DONE) {
            mScrapeResult = mMDResolveHandle->getResult();
            mScrapeResult.mediaFilesDownloadStatus = COMPLETED;
            mMDResolveHandle.reset();

            if (mScrapeResult.mediaFilesDownloadStatus == COMPLETED &&
                Settings::getInstance()->getBool("MiximageGenerate")) {
                std::string currentMiximage = mLastSearch.game->getMiximagePath();
                if (currentMiximage == "" ||
                    (currentMiximage != "" &&
                     Settings::getInstance()->getBool("MiximageOverwrite"))) {

                    mMiximageGenerator =
                        std::make_unique<MiximageGenerator>(mLastSearch.game, mResultMessage);

                    // The promise/future mechanism is used as signaling for the thread to
                    // indicate that processing has been completed. The reason to run a separate
                    // thread is that the busy animation will then be played and that the user
                    // interface does not become completely unresponsive during the miximage
                    // generation.
                    std::promise<bool>().swap(mGeneratorPromise);
                    mGeneratorFuture = mGeneratorPromise.get_future();

                    mMiximageGeneratorThread =
                        std::thread(&MiximageGenerator::startThread, mMiximageGenerator.get(),
                                    &mGeneratorPromise);
                }
                else {
                    returnResult(mScrapeResult);
                }
            }
            else {
                returnResult(mScrapeResult);
            }
        }
        else if (mMDResolveHandle->status() == ASYNC_ERROR) {
            onSearchError(mMDResolveHandle->getStatusString(), mMDResolveHandle->getRetry(),
                          (mSearchHandle != nullptr ? mSearchHandle->getFatalError() : false));
            mMDResolveHandle.reset();
        }
    }
}

void GuiScraperSearch::updateThumbnail()
{
    auto it = mThumbnailReqMap.find(mScraperResults[mResultList->getCursorId()].thumbnailImageUrl +
                                    "." + std::to_string(mResultList->getCursorId()));

    if (it != mThumbnailReqMap.end() && it->second->status() == HttpReq::REQ_SUCCESS) {
        // Save thumbnail to mScraperResults cache and set the flag that the
        // thumbnail download has been completed for this game.
        if (mScraperResults[mResultList->getCursorId()].thumbnailDownloadStatus == IN_PROGRESS) {
            mScraperResults[mResultList->getCursorId()].thumbnailImageData =
                it->second->getContent();
            mScraperResults[mResultList->getCursorId()].thumbnailDownloadStatus = COMPLETED;
        }
        // Activate the thumbnail in the GUI.
        std::string content {mScraperResults[mResultList->getCursorId()].thumbnailImageData};
        if (content.size() > 350) {
            mResultThumbnail->setImage(content.data(), content.length());
            mGrid.onSizeChanged(); // A hack to fix the thumbnail position since its size changed.
        }
    }
    else {
        mResultThumbnail->setImage("");
        onSearchError(_("Error downloading thumbnail:") + " \n" + it->second->getErrorMsg(), true,
                      (mSearchHandle != nullptr ? mSearchHandle->getFatalError() : false),
                      it->second->status());
    }

    mThumbnailReqMap.erase(it);

    // When the thumbnail has been downloaded and we are in automatic mode, or if
    // we are in semi-automatic mode with a single matching game result, we proceed
    // to immediately download the rest of the media files.
    if ((mSearchType == AUTOMATIC_MODE ||
         (mSearchType == SEMIAUTOMATIC_MODE && mScraperResults.size() == 1 &&
          mRefinedSearch == false)) &&
        mScraperResults.front().thumbnailDownloadStatus == COMPLETED) {
        mRefinedSearch = false;
        if (mScraperResults.size() == 0) {
            mSkipCallback();
        }
        else {
            if (mSearchType == AUTOMATIC_MODE)
                returnResult(mScraperResults[mAutomaticModeGameEntry]);
            else
                returnResult(mScraperResults.front());
        }
    }
}

void GuiScraperSearch::openInputScreen(ScraperSearchParams& params)
{
    auto searchForFunc = [&](std::string name) {
        // Trim leading and trailing whitespaces.
        name = Utils::String::trim(name);
        stop();
        mRefinedSearch = true;
        params.nameOverride = name;
        if (mRefineCallback != nullptr)
            mRefineCallback();
        search(params);
    };

    mRetryCount = 0;

    std::string searchString;

    if (params.nameOverride.empty()) {
        // If the setting to search based on metadata name has been set, then show this string
        // regardless of whether the entry is an arcade game and TheGamesDB is used.
        if (Settings::getInstance()->getBool("ScraperSearchMetadataName")) {
            searchString = Utils::String::removeParenthesis(params.game->metadata.get("name"));
        }
        else {
            // If searching based on the actual file name, then expand to the full game name
            // in case the scraper is set to TheGamesDB and it's an arcade game. This is
            // required as TheGamesDB does not support searches using the short MAME names.
            if (params.game->isArcadeGame() &&
                Settings::getInstance()->getString("Scraper") == "thegamesdb") {
                searchString = MameNames::getInstance().getCleanName(params.game->getCleanName());
            }
            else {
                if (params.game->getType() == GAME &&
                    Utils::FileSystem::isDirectory(params.game->getFullPath())) {
                    // For the special case where a directory has a supported file extension and is
                    // therefore interpreted as a file, exclude the extension from the search.
                    searchString = Utils::FileSystem::getStem(params.game->getCleanName());
                }
                else {
                    searchString = params.game->getCleanName();
                }
            }
        }
    }
    else {
        searchString = params.nameOverride;
    }

    if (Settings::getInstance()->getBool("ScraperConvertUnderscores"))
        searchString = Utils::String::replace(searchString, "_", " ");

    if (Settings::getInstance()->getBool("VirtualKeyboard")) {
        mWindow->pushGui(new GuiTextEditKeyboardPopup(
            getHelpStyle(), 0.0f, _("REFINE SEARCH"), searchString, searchForFunc, false,
            _("SEARCH"), _("SEARCH USING REFINED NAME?")));
    }
    else {
        mWindow->pushGui(new GuiTextEditPopup(getHelpStyle(), _("REFINE SEARCH"), searchString,
                                              searchForFunc, false, _("SEARCH"),
                                              _("SEARCH USING REFINED NAME?")));
    }
}

bool GuiScraperSearch::saveMetadata(const ScraperSearchResult& result,
                                    MetaDataList& metadata,
                                    FileData* scrapedGame)
{
    bool metadataUpdated {false};
    bool hasDefaultName {false};
    std::vector<MetaDataDecl> mMetaDataDecl {metadata.getMDD()};
    std::string defaultName;

    // Get the default name, which is either the MAME name or the name of the physical file
    // or directory.
    if (scrapedGame->isArcadeGame())
        defaultName = MameNames::getInstance().getCleanName(scrapedGame->getCleanName());
    else
        defaultName = Utils::FileSystem::getStem(scrapedGame->getFileName());

    // We want the comparison to be case sensitive.
    if (defaultName == metadata.get("name"))
        hasDefaultName = true;

    for (unsigned int i {0}; i < mMetaDataDecl.size(); ++i) {

        // Skip elements that are tagged not to be scraped.
        if (!mMetaDataDecl.at(i).shouldScrape)
            continue;

        const std::string& key {mMetaDataDecl.at(i).key};

        // Skip element if the setting to not scrape metadata has been set,
        // unless its type is rating, controller or name.
        if (!Settings::getInstance()->getBool("ScrapeMetadata") &&
            (key != "rating" && key != "controller" && key != "name"))
            continue;

        // Skip saving of rating metadata if the corresponding option has been set to false.
        if (key == "rating" && !Settings::getInstance()->getBool("ScrapeRatings"))
            continue;

        // ScreenScraper controller scraping is currently broken, it's unclear if they will fix it.
        // // Skip saving of controller metadata if the corresponding option has been set to false.
        // if (key == "controller" && !Settings::getInstance()->getBool("ScrapeControllers"))
        //    continue;

        // Skip saving of game name if the corresponding option has been set to false.
        if (key == "name" && !Settings::getInstance()->getBool("ScrapeGameNames"))
            continue;

        // Skip elements that are empty.
        if (result.mdl.get(key) == "")
            continue;

        // Skip elements that are the same as the default metadata value.
        if (result.mdl.get(key) == mMetaDataDecl.at(i).defaultValue)
            continue;

        // Skip elements that are identical to the existing value.
        if (result.mdl.get(key) == metadata.get(key))
            continue;

        // Make sure to set releasedate to the proper default value.
        if (key == "releasedate" && metadata.get(key) == "19700101T000000")
            metadata.set(key, mMetaDataDecl.at(i).defaultValue);

        // Overwrite all the other values if the flag to overwrite data has been set.
        if (Settings::getInstance()->getBool("ScraperOverwriteData")) {
            metadata.set(key, result.mdl.get(key));
            metadataUpdated = true;
        }
        // If the key is the game name and it's set to its default value, then update.
        else if (key == "name" && hasDefaultName) {
            metadata.set(key, result.mdl.get(key));
            metadataUpdated = true;
        }
        // We need to check against a translated "unknown" for keys that have this default value.
        else if ((key == "developer" || key == "publisher" || key == "genre" || key == "players") &&
                 metadata.get(key) == _("unknown")) {
            metadata.set(key, result.mdl.get(key));
            metadataUpdated = true;
        }
        // Else only update the value if it is set to the default metadata value.
        else if (metadata.get(key) == mMetaDataDecl.at(i).defaultValue) {
            metadata.set(key, result.mdl.get(key));
            metadataUpdated = true;
        }
    }

    return metadataUpdated;
}

std::vector<HelpPrompt> GuiScraperSearch::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;

    prompts.push_back(HelpPrompt("y", _("refine search")));

    // Only show the skip prompt during multi-scraping.
    if (mSkipCallback != nullptr)
        prompts.push_back(HelpPrompt("x", _("skip")));

    if (mFoundGame && (mRefinedSearch || mSearchType != SEMIAUTOMATIC_MODE ||
                       (mSearchType == SEMIAUTOMATIC_MODE && mScraperResults.size() > 1)))
        prompts.push_back(HelpPrompt("a", _("accept result")));

    return prompts;
}
