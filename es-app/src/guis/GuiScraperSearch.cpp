//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiScraperSearch.cpp
//
//  User interface for the scraper where the user is able to see an overview
//  of the game being scraped and an option to override the game search string.
//  Used by both single-game scraping from the GuiMetaDataEd menu as well as
//  to resolve scraping conflicts when run from GuiScraperMenu.
//  The function to properly save scraped metadata is located here too.
//
//  This GUI is called from GuiGameScraper for single-game scraping and
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
#include "guis/GuiTextEditPopup.h"
#include "resources/Font.h"
#include "utils/StringUtil.h"
#include "views/ViewController.h"

#define FAILED_VERIFICATION_RETRIES 8

GuiScraperSearch::GuiScraperSearch(Window* window, SearchType type, unsigned int scrapeCount)
    : GuiComponent(window)
    , mGrid(window, glm::ivec2{4, 3})
    , mBusyAnim(window)
    , mSearchType(type)
    , mScrapeCount(scrapeCount)
    , mScrapeRatings(false)
    , mRefinedSearch(false)
    , mFoundGame(false)
{
    addChild(&mGrid);

    mBlockAccept = false;
    mAcceptedResult = false;
    mRetrySearch = false;
    mRetryCount = 0;

    // Left spacer (empty component, needed for borders).
    mGrid.setEntry(std::make_shared<GuiComponent>(mWindow), glm::ivec2{0, 0}, false, false,
                   glm::ivec2{1, 3}, GridFlags::BORDER_TOP | GridFlags::BORDER_BOTTOM);

    // Selected result name.
    mResultName = std::make_shared<TextComponent>(mWindow, "Result name",
                                                  Font::get(FONT_SIZE_MEDIUM), 0x777777FF);

    // Selected result thumbnail.
    mResultThumbnail = std::make_shared<ImageComponent>(mWindow);
    mGrid.setEntry(mResultThumbnail, glm::ivec2{1, 1}, false, false, glm::ivec2{1, 1});

    // Selected result description and container.
    mDescContainer = std::make_shared<ScrollableContainer>(mWindow);

    // Adjust the game description text scrolling parameters depending on the search type.
    if (mSearchType == NEVER_AUTO_ACCEPT)
        mDescContainer->setScrollParameters(3000, 3000, 85);
    else
        mDescContainer->setScrollParameters(6000, 3000, 85);

    mResultDesc = std::make_shared<TextComponent>(mWindow, "Result desc",
                                                  Font::get(FONT_SIZE_SMALL), 0x777777FF);
    mDescContainer->addChild(mResultDesc.get());
    mDescContainer->setAutoScroll(true);

    // Metadata.
    auto font = Font::get(FONT_SIZE_SMALL); // Placeholder, gets replaced in onSizeChanged().
    const unsigned int mdColor = 0x777777FF;
    const unsigned int mdLblColor = 0x666666FF;
    mMD_Rating = std::make_shared<RatingComponent>(mWindow);
    mMD_ReleaseDate = std::make_shared<DateTimeEditComponent>(mWindow);
    mMD_ReleaseDate->setColor(mdColor);
    mMD_ReleaseDate->setUppercase(true);
    mMD_Developer = std::make_shared<TextComponent>(mWindow, "", font, mdColor, ALIGN_LEFT,
                                                    glm::vec3{}, glm::vec2{}, 0x00000000, 0.02f);
    mMD_Publisher = std::make_shared<TextComponent>(mWindow, "", font, mdColor, ALIGN_LEFT,
                                                    glm::vec3{}, glm::vec2{}, 0x00000000, 0.02f);
    mMD_Genre = std::make_shared<TextComponent>(mWindow, "", font, mdColor, ALIGN_LEFT, glm::vec3{},
                                                glm::vec2{}, 0x00000000, 0.02f);
    mMD_Players = std::make_shared<TextComponent>(mWindow, "", font, mdColor, ALIGN_LEFT,
                                                  glm::vec3{}, glm::vec2{}, 0x00000000, 0.02f);
    mMD_Filler = std::make_shared<TextComponent>(mWindow, "", font, mdColor);

    if (Settings::getInstance()->getString("Scraper") != "thegamesdb")
        mScrapeRatings = true;

    if (mScrapeRatings)
        mMD_Pairs.push_back(
            MetaDataPair(std::make_shared<TextComponent>(mWindow, "RATING:", font, mdLblColor),
                         mMD_Rating, false));

    mMD_Pairs.push_back(MetaDataPair(
        std::make_shared<TextComponent>(mWindow, "RELEASED:", font, mdLblColor), mMD_ReleaseDate));
    mMD_Pairs.push_back(MetaDataPair(
        std::make_shared<TextComponent>(mWindow, "DEVELOPER:", font, mdLblColor), mMD_Developer));
    mMD_Pairs.push_back(MetaDataPair(
        std::make_shared<TextComponent>(mWindow, "PUBLISHER:", font, mdLblColor), mMD_Publisher));
    mMD_Pairs.push_back(MetaDataPair(
        std::make_shared<TextComponent>(mWindow, "GENRE:", font, mdLblColor), mMD_Genre));
    mMD_Pairs.push_back(MetaDataPair(
        std::make_shared<TextComponent>(mWindow, "PLAYERS:", font, mdLblColor), mMD_Players));

    // If no rating is being scraped, add a filler to make sure that the fonts keep the same
    // size so the GUI looks consistent.
    if (!mScrapeRatings)
        mMD_Pairs.push_back(MetaDataPair(
            std::make_shared<TextComponent>(mWindow, "", font, mdLblColor), mMD_Filler));

    mMD_Grid = std::make_shared<ComponentGrid>(
        mWindow, glm::ivec2{2, static_cast<int>(mMD_Pairs.size() * 2 - 1)});
    unsigned int i = 0;
    for (auto it = mMD_Pairs.cbegin(); it != mMD_Pairs.cend(); it++) {
        mMD_Grid->setEntry(it->first, glm::ivec2{0, i}, false, true);
        mMD_Grid->setEntry(it->second, glm::ivec2{1, i}, false, it->resize);
        i += 2;
    }

    mGrid.setEntry(mMD_Grid, glm::ivec2{2, 1}, false, false);

    // Result list.
    mResultList = std::make_shared<ComponentList>(mWindow);
    mResultList->setCursorChangedCallback([this](CursorState state) {
        if (state == CURSOR_STOPPED)
            updateInfoPane();
    });

    updateViewStyle();
}

GuiScraperSearch::~GuiScraperSearch()
{
    // The following manual resets are required to avoid a race condition when the
    // STOP button is pressed in the multi-scraper. Without this code there will be
    // a memory leak as the cURL easy handle is not cleaned up. For a normally completed
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
        mScrapeResult.savedNewMedia = true;
        // We always let the miximage generator thread complete.
        mMiximageGeneratorThread.join();
        mMiximageGenerator.reset();
        TextureResource::manualUnload(mLastSearch.game->getMiximagePath(), false);
        ViewController::get()->onFileChanged(mLastSearch.game, true);
    }
}

void GuiScraperSearch::onSizeChanged()
{
    mGrid.setSize(mSize);

    if (mSize.x == 0 || mSize.y == 0)
        return;

    // Column widths.
    if (mSearchType == ALWAYS_ACCEPT_FIRST_RESULT)
        mGrid.setColWidthPerc(0, 0.02f); // Looks better when this is higher in auto mode.
    else
        mGrid.setColWidthPerc(0, 0.01f);

    mGrid.setColWidthPerc(1, 0.25f);

    if (mSearchType == ALWAYS_ACCEPT_FIRST_RESULT)
        mGrid.setColWidthPerc(2, 0.25f);
    else
        mGrid.setColWidthPerc(2, 0.28f);

    // Row heights.
    if (mSearchType == ALWAYS_ACCEPT_FIRST_RESULT) // Show name.
        mGrid.setRowHeightPerc(0, (mResultName->getFont()->getHeight() * 1.6f) /
                                      mGrid.getSize().y); // Result name.
    else
        mGrid.setRowHeightPerc(0, 0.0825f); // Hide name but do padding.

    if (mSearchType == ALWAYS_ACCEPT_FIRST_RESULT)
        mGrid.setRowHeightPerc(2, 0.2f);
    else
        mGrid.setRowHeightPerc(1, 0.505f);

    const float boxartCellScale = 0.9f;

    // Limit thumbnail size using setMaxHeight - we do this instead of letting mGrid
    // call setSize because it maintains the aspect ratio.
    // We also pad a little so it doesn't rub up against the metadata labels.
    mResultThumbnail->setMaxSize(mGrid.getColWidth(1) * boxartCellScale, mGrid.getRowHeight(1));

    // Metadata.
    resizeMetadata();

    if (mSearchType != ALWAYS_ACCEPT_FIRST_RESULT)
        mDescContainer->setSize(mGrid.getColWidth(1) * boxartCellScale + mGrid.getColWidth(2),
                                mResultDesc->getFont()->getHeight() * 3.0f);
    else
        mDescContainer->setSize(mGrid.getColWidth(3) * boxartCellScale,
                                mResultDesc->getFont()->getHeight() * 6.0f);

    // Make description text wrap at edge of container.
    mResultDesc->setSize(mDescContainer->getSize().x, 0.0f);

    // Set the width of mResultName to the cell width so that text abbreviation will work correctly.
    glm::vec2 resultNameSize{mResultName->getSize()};
    mResultName->setSize(mGrid.getColWidth(3), resultNameSize.y);

    mGrid.onSizeChanged();
    mBusyAnim.setSize(mSize);
}

void GuiScraperSearch::resizeMetadata()
{
    mMD_Grid->setSize(mGrid.getColWidth(2), mGrid.getRowHeight(1));
    if (mMD_Grid->getSize().y > mMD_Pairs.size()) {
        const int fontHeight = static_cast<int>(mMD_Grid->getSize().y / mMD_Pairs.size() * 0.8f);
        auto fontLbl = Font::get(fontHeight, FONT_PATH_REGULAR);
        auto fontComp = Font::get(fontHeight, FONT_PATH_LIGHT);

        // Update label fonts.
        float maxLblWidth = 0;
        for (auto it = mMD_Pairs.cbegin(); it != mMD_Pairs.cend(); it++) {
            it->first->setFont(fontLbl);
            it->first->setSize(0, 0);
            if (it->first->getSize().x > maxLblWidth)
                maxLblWidth = it->first->getSize().x + (16.0f * Renderer::getScreenWidthModifier());
        }

        for (unsigned int i = 0; i < mMD_Pairs.size(); i++)
            mMD_Grid->setRowHeightPerc(
                i * 2, (fontLbl->getLetterHeight() + (2.0f * Renderer::getScreenHeightModifier())) /
                           mMD_Grid->getSize().y);

        // Update component fonts.
        mMD_ReleaseDate->setFont(fontComp);
        mMD_Developer->setFont(fontComp);
        mMD_Publisher->setFont(fontComp);
        mMD_Genre->setFont(fontComp);
        mMD_Players->setFont(fontComp);

        mMD_Grid->setColWidthPerc(0, maxLblWidth / mMD_Grid->getSize().x);

        if (mScrapeRatings) {
            // Rating is manually sized.
            mMD_Rating->setSize(mMD_Grid->getColWidth(1), fontLbl->getHeight() * 0.65f);
            mMD_Grid->onSizeChanged();
        }

        // Make result font follow label font.
        mResultDesc->setFont(Font::get(fontHeight, FONT_PATH_REGULAR));
    }
}

void GuiScraperSearch::updateViewStyle()
{
    // Unlink description, result list and result name.
    mGrid.removeEntry(mResultName);
    mGrid.removeEntry(mResultDesc);
    mGrid.removeEntry(mResultList);

    // Add them back depending on search type.
    if (mSearchType == ALWAYS_ACCEPT_FIRST_RESULT) {
        // Show name.
        mGrid.setEntry(mResultName, glm::ivec2{1, 0}, false, false, glm::ivec2{2, 1},
                       GridFlags::BORDER_TOP);

        // Need a border on the bottom left.
        mGrid.setEntry(std::make_shared<GuiComponent>(mWindow), glm::ivec2{0, 2}, false, false,
                       glm::ivec2{3, 1}, GridFlags::BORDER_BOTTOM);

        // Show description on the right.
        mGrid.setEntry(mDescContainer, glm::ivec2{3, 0}, false, false, glm::ivec2{1, 3},
                       GridFlags::BORDER_TOP | GridFlags::BORDER_BOTTOM);
        // Make description text wrap at edge of container.
        mResultDesc->setSize(mDescContainer->getSize().x, 0.0f);
    }
    else {
        // Fake row where name would be.
        mGrid.setEntry(std::make_shared<GuiComponent>(mWindow), glm::ivec2{1, 0}, false, true,
                       glm::ivec2{2, 1}, GridFlags::BORDER_TOP);

        // Show result list on the right.
        mGrid.setEntry(mResultList, glm::ivec2{3, 0}, true, true, glm::ivec2{1, 3},
                       GridFlags::BORDER_LEFT | GridFlags::BORDER_TOP | GridFlags::BORDER_BOTTOM);

        // Show description under image/info.
        mGrid.setEntry(mDescContainer, glm::ivec2{1, 2}, false, false, glm::ivec2{2, 1},
                       GridFlags::BORDER_BOTTOM);
        // Make description text wrap at edge of container.
        mResultDesc->setSize(mDescContainer->getSize().x, 0);
    }
}

void GuiScraperSearch::search(const ScraperSearchParams& params)
{
    mBlockAccept = true;
    mAcceptedResult = false;
    mMiximageResult = false;
    mScrapeResult = {};

    mResultList->clear();
    mScraperResults.clear();
    mMDRetrieveURLsHandle.reset();
    mThumbnailReqMap.clear();
    mMDResolveHandle.reset();
    updateInfoPane();

    mLastSearch = params;
    mSearchHandle = startScraperSearch(params);
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

void GuiScraperSearch::onSearchDone(const std::vector<ScraperSearchResult>& results)
{
    mResultList->clear();

    mScraperResults = results;

    auto font = Font::get(FONT_SIZE_MEDIUM);
    unsigned int color = 0x777777FF;
    if (results.empty()) {
        // Check if the scraper used is still valid.
        if (!isValidConfiguredScraper()) {
            mWindow->pushGui(new GuiMsgBox(
                mWindow, getHelpStyle(),
                Utils::String::toUpper("Configured scraper is no longer available.\n"
                                       "Please change the scraping source in the settings."),
                "FINISH", mSkipCallback));
        }
        else {
            mFoundGame = false;
            ComponentListRow row;
            row.addElement(std::make_shared<TextComponent>(mWindow, "NO GAMES FOUND", font, color),
                           true);

            if (mSkipCallback)
                row.makeAcceptInputHandler(mSkipCallback);

            mResultList->addRow(row);
            mGrid.resetCursor();
        }
    }
    else {
        mFoundGame = true;
        ComponentListRow row;

        for (size_t i = 0; i < results.size(); i++) {
            row.elements.clear();
            row.addElement(
                std::make_shared<TextComponent>(
                    mWindow, Utils::String::toUpper(results.at(i).mdl.get("name")), font, color),
                true);
            row.makeAcceptInputHandler([this, i] { returnResult(mScraperResults.at(i)); });
            mResultList->addRow(row);
        }
        mGrid.resetCursor();
    }

    mBlockAccept = false;
    updateInfoPane();

    // If there is no thumbnail to download and we're in semi-automatic mode, proceed to return
    // the results or we'll get stuck forever waiting for a thumbnail to be downloaded.
    if (mSearchType == ACCEPT_SINGLE_MATCHES && results.size() == 1 &&
        mScraperResults.front().thumbnailImageUrl == "")
        returnResult(mScraperResults.front());

    // For automatic mode, if there's no thumbnail to download or no matching games found,
    // proceed directly or we'll get stuck forever.
    if (mSearchType == ALWAYS_ACCEPT_FIRST_RESULT) {
        if (mScraperResults.size() == 0 ||
            (mScraperResults.size() > 0 && mScraperResults.front().thumbnailImageUrl == "")) {
            if (mScraperResults.size() == 0)
                mSkipCallback();
            else
                returnResult(mScraperResults.front());
        }
    }
}

void GuiScraperSearch::onSearchError(const std::string& error, HttpReq::Status status)
{
    // This is a workaround for a somehow frequently recurring issue with screenscraper.fr
    // where requests to download the thumbnails are randomly met with TLS verification errors.
    // It's unclear why it only happens to the thumbnail requests, but it usually goes away
    // after a few days or so. If this issue occurs and the corresponding setting has been
    // enabled, we'll retry the search automatically up to FAILED_VERIFICATION_RETRIES number
    // of times. Usually a few retries is enough to get the thumbnail to download. If not,
    // the error dialog will be presented to the user, and if the "Retry" button is pressed,
    // a new round of retries will take place.
    if (status == HttpReq::REQ_FAILED_VERIFICATION && mRetryCount < FAILED_VERIFICATION_RETRIES &&
        Settings::getInstance()->getBool("ScraperRetryPeerVerification")) {
        LOG(LogError) << "GuiScraperSearch: " << Utils::String::replace(error, "\n", "");
        mRetrySearch = true;
        mRetryCount++;
        LOG(LogError) << "GuiScraperSearch: Attempting automatic retry " << mRetryCount << " of "
                      << FAILED_VERIFICATION_RETRIES;
        return;
    }
    else {
        mRetryCount = 0;
    }

    if (mScrapeCount > 1) {
        LOG(LogError) << "GuiScraperSearch: " << Utils::String::replace(error, "\n", "");
        mWindow->pushGui(new GuiMsgBox(mWindow, getHelpStyle(), Utils::String::toUpper(error),
                                       "RETRY",
                                       std::bind(&GuiScraperSearch::search, this, mLastSearch),
                                       "SKIP", mSkipCallback, "CANCEL", mCancelCallback, true));
    }
    else {
        LOG(LogError) << "GuiScraperSearch: " << Utils::String::replace(error, "\n", "");
        mWindow->pushGui(new GuiMsgBox(mWindow, getHelpStyle(), Utils::String::toUpper(error),
                                       "RETRY",
                                       std::bind(&GuiScraperSearch::search, this, mLastSearch),
                                       "CANCEL", mCancelCallback, "", nullptr, true));
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
    int i = getSelectedIndex();
    if (mSearchType == ALWAYS_ACCEPT_FIRST_RESULT && mScraperResults.size())
        i = 0;

    if (i != -1 && static_cast<int>(mScraperResults.size() > i)) {
        ScraperSearchResult& res = mScraperResults.at(i);

        mResultName->setText(Utils::String::toUpper(res.mdl.get("name")));
        mResultDesc->setText(Utils::String::toUpper(res.mdl.get("desc")));
        mDescContainer->reset();

        mResultThumbnail->setImage("");
        const std::string& thumb = res.screenshotUrl.empty() ? res.coverUrl : res.screenshotUrl;
        mScraperResults[i].thumbnailImageUrl = thumb;

        // Cache the thumbnail image in mScraperResults so that we don't need to download
        // it every time the list is scrolled back and forth.
        if (mScraperResults[i].thumbnailImageData.size() > 0) {
            std::string content = mScraperResults[i].thumbnailImageData;
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
                    // through the result list.
                    mThumbnailReqMap.insert(std::pair<std::string, std::unique_ptr<HttpReq>>(
                        mScraperResults[i].thumbnailImageUrl,
                        std::unique_ptr<HttpReq>(new HttpReq(thumb))));
                }
            }
        }

        // Metadata.
        if (mScrapeRatings) {
            mMD_Rating->setValue(Utils::String::toUpper(res.mdl.get("rating")));
            mMD_Rating->setOpacity(255);
        }
        mMD_ReleaseDate->setValue(Utils::String::toUpper(res.mdl.get("releasedate")));
        mMD_Developer->setText(Utils::String::toUpper(res.mdl.get("developer")));
        mMD_Publisher->setText(Utils::String::toUpper(res.mdl.get("publisher")));
        mMD_Genre->setText(Utils::String::toUpper(res.mdl.get("genre")));
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
            mMD_Rating->setOpacity(0);
        }
        // Set the release date to this value to force DateTimeEditComponent to put a
        // blank instead of the text 'unknown' prior to the scrape result being returned.
        mMD_ReleaseDate->setValue("19700101T010101");
        mMD_Developer->setText("");
        mMD_Publisher->setText("");
        mMD_Genre->setText("");
        mMD_Players->setText("");
    }
}

bool GuiScraperSearch::input(InputConfig* config, Input input)
{
    if (config->isMappedTo("a", input) && input.value != 0) {
        if (mBlockAccept)
            return true;
    }

    // Refine the search, unless the result has already been accepted or we're in semi-automatic
    // mode and there are less than 2 search results.
    if (!mAcceptedResult && config->isMappedTo("y", input) && input.value != 0) {
        if (mSearchType != ACCEPT_SINGLE_MATCHES ||
            (mSearchType == ACCEPT_SINGLE_MATCHES && mScraperResults.size() > 1)) {
            openInputScreen(mLastSearch);
        }
    }

    // Skip game, unless the result has already been accepted.
    if (!mAcceptedResult && mScrapeCount > 1 && config->isMappedTo("x", input) && input.value != 0)
        mSkipCallback();

    return GuiComponent::input(config, input);
}

void GuiScraperSearch::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans{parentTrans * getTransform()};

    renderChildren(trans);
    Renderer::drawRect(0.0f, 0.0f, mSize.x, mSize.y, 0x00000009, 0x00000009);

    if (mBlockAccept) {
        Renderer::setMatrix(trans);
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
    GuiComponent::update(deltaTime);

    // There was a failure and we're attempting an automatic retry.
    if (mRetrySearch) {
        mRetrySearch = false;
        stop();
        search(mLastSearch);
        return;
    }

    if (mBlockAccept)
        mBusyAnim.update(deltaTime);

    // Check if the thumbnail for the currently selected game has finished downloading.
    if (mScraperResults.size() > 0) {
        auto it =
            mThumbnailReqMap.find(mScraperResults[mResultList->getCursorId()].thumbnailImageUrl);
        if (it != mThumbnailReqMap.end() && it->second->status() != HttpReq::REQ_IN_PROGRESS)
            updateThumbnail();
    }

    if (mSearchHandle && mSearchHandle->status() != ASYNC_IN_PROGRESS) {
        auto status = mSearchHandle->status();
        mScraperResults = mSearchHandle->getResults();
        auto statusString = mSearchHandle->getStatusString();

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
                for (auto it = mScraperResults.cbegin(); it != mScraperResults.cend(); it++)
                    gameIDs += it->gameID + ',';

                // Remove the last comma
                gameIDs.pop_back();
                mMDRetrieveURLsHandle = startMediaURLsFetch(gameIDs);
            }
        }
        else if (status == ASYNC_ERROR) {
            onSearchError(statusString);
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
            for (auto it = results_media.cbegin(); it != results_media.cend(); it++) {
                for (unsigned int i = 0; i < results_scrape.size(); i++) {
                    if (results_scrape[i].gameID == it->gameID) {
                        results_scrape[i].box3DUrl = it->box3DUrl;
                        results_scrape[i].coverUrl = it->coverUrl;
                        results_scrape[i].marqueeUrl = it->marqueeUrl;
                        results_scrape[i].screenshotUrl = it->screenshotUrl;
                        results_scrape[i].videoUrl = it->videoUrl;
                        results_scrape[i].scraperRequestAllowance = it->scraperRequestAllowance;
                        results_scrape[i].mediaURLFetch = COMPLETED;
                    }
                }
            }
            onSearchDone(results_scrape);
        }
        else if (mMDRetrieveURLsHandle->status() == ASYNC_ERROR) {
            onSearchError(mMDRetrieveURLsHandle->getStatusString());
            mMDRetrieveURLsHandle.reset();
        }
    }

    // Check if a miximage generator thread was started, and if the processing has been completed.
    if (mMiximageGenerator && mGeneratorFuture.valid()) {
        // Only wait one millisecond as this update() function runs very frequently.
        if (mGeneratorFuture.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready) {
            mMDResolveHandle.reset();
            // We always let the miximage generator thread complete.
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
            onSearchError(mMDResolveHandle->getStatusString());
            mMDResolveHandle.reset();
        }
    }
}

void GuiScraperSearch::updateThumbnail()
{
    auto it = mThumbnailReqMap.find(mScraperResults[mResultList->getCursorId()].thumbnailImageUrl);

    if (it != mThumbnailReqMap.end() && it->second->status() == HttpReq::REQ_SUCCESS) {
        // Save thumbnail to mScraperResults cache and set the flag that the
        // thumbnail download has been completed for this game.
        if (mScraperResults[mResultList->getCursorId()].thumbnailDownloadStatus == IN_PROGRESS) {
            mScraperResults[mResultList->getCursorId()].thumbnailImageData =
                it->second->getContent();
            mScraperResults[mResultList->getCursorId()].thumbnailDownloadStatus = COMPLETED;
        }
        // Activate the thumbnail in the GUI.
        std::string content = mScraperResults[mResultList->getCursorId()].thumbnailImageData;
        if (content.size() > 0) {
            mResultThumbnail->setImage(content.data(), content.length());
            mGrid.onSizeChanged(); // A hack to fix the thumbnail position since its size changed.
        }
    }
    else {
        mResultThumbnail->setImage("");
        onSearchError("Error downloading thumbnail:\n " + it->second->getErrorMsg(),
                      it->second->status());
    }

    mThumbnailReqMap.erase(it);

    // When the thumbnail has been downloaded and we are in automatic mode, or if
    // we are in semi-automatic mode with a single matching game result, we proceed
    // to immediately download the rest of the media files.
    if ((mSearchType == ALWAYS_ACCEPT_FIRST_RESULT ||
         (mSearchType == ACCEPT_SINGLE_MATCHES && mScraperResults.size() == 1 &&
          mRefinedSearch == false)) &&
        mScraperResults.front().thumbnailDownloadStatus == COMPLETED) {
        mRefinedSearch = false;
        if (mScraperResults.size() == 0)
            mSkipCallback();
        else
            returnResult(mScraperResults.front());
    }
}

void GuiScraperSearch::openInputScreen(ScraperSearchParams& params)
{
    auto searchForFunc = [&](const std::string& name) {
        stop();
        mRefinedSearch = true;
        params.nameOverride = name;
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
                Settings::getInstance()->getString("Scraper") == "thegamesdb")
                searchString = MameNames::getInstance()->getCleanName(params.game->getCleanName());
            else
                searchString = params.game->getCleanName();
        }
    }
    else {
        searchString = params.nameOverride;
    }

    mWindow->pushGui(new GuiTextEditPopup(mWindow, getHelpStyle(), "REFINE SEARCH", searchString,
                                          searchForFunc, false, "SEARCH", "APPLY CHANGES?"));
}

bool GuiScraperSearch::saveMetadata(const ScraperSearchResult& result,
                                    MetaDataList& metadata,
                                    FileData* scrapedGame)
{
    bool metadataUpdated = false;
    bool hasDefaultName = false;
    std::vector<MetaDataDecl> mMetaDataDecl = metadata.getMDD();
    std::string defaultName;

    // Get the default name, which is either the MAME name or the name of the physical file
    // or directory.
    if (scrapedGame->isArcadeGame())
        defaultName = MameNames::getInstance()->getCleanName(scrapedGame->getCleanName());
    else
        defaultName = Utils::FileSystem::getStem(scrapedGame->getFileName());

    // We want the comparison to be case sensitive.
    if (defaultName == metadata.get("name"))
        hasDefaultName = true;

    for (unsigned int i = 0; i < mMetaDataDecl.size(); i++) {

        // Skip elements that are tagged not to be scraped.
        if (!mMetaDataDecl.at(i).shouldScrape)
            continue;

        const std::string& key = mMetaDataDecl.at(i).key;

        // Skip element if the setting to not scrape metadata has been set,
        // unless its type is rating or name.
        if (!Settings::getInstance()->getBool("ScrapeMetadata") &&
            (key != "rating" && key != "name"))
            continue;

        // Skip saving of rating if the corresponding option has been set to false.
        if (key == "rating" && !Settings::getInstance()->getBool("ScrapeRatings"))
            continue;

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
        if (key == "releasedate" && metadata.get(key) == "19700101T010000")
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
        // Else only update the value if it is set to the default metadata value.
        else if (metadata.get(key) == mMetaDataDecl.at(i).defaultValue) {
            metadata.set(key, result.mdl.get(key));
            metadataUpdated = true;
        }

        // For the description, expand any escaped HTML quotation marks to literal
        // quotation marks.
        if (key == "desc" && metadataUpdated)
            metadata.set(key, Utils::String::replace(metadata.get(key), "&quot;", "\""));
    }

    return metadataUpdated;
}

std::vector<HelpPrompt> GuiScraperSearch::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;

    prompts.push_back(HelpPrompt("y", "refine search"));
    if (mScrapeCount > 1)
        prompts.push_back(HelpPrompt("x", "skip"));
    if (mFoundGame && (mRefinedSearch || mSearchType != ACCEPT_SINGLE_MATCHES ||
                       (mSearchType == ACCEPT_SINGLE_MATCHES && mScraperResults.size() > 1)))
        prompts.push_back(HelpPrompt("a", "accept result"));

    return prompts;
}

HelpStyle GuiScraperSearch::getHelpStyle()
{
    HelpStyle style = HelpStyle();
    style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");
    return style;
}
