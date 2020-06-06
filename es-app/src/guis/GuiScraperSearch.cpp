//
//	GuiScraperSearch.cpp
//
//	User interface for the scraper where the user is able to see an overview
//	of the game being scraped and an option to override the game search string.
//	Used by both single-game scraping from the GuiMetaDataEd menu as well as
//	to resolve scraping conflicts when run from GuiScraperMenu.
//	The function to properly save scraped metadata is located here too.
//
//	This component is called from GuiGameScraper for single-game scraping and
//	from GuiScraperMulti for multi-game scraping.
//

#include "guis/GuiScraperSearch.h"

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
#include "PlatformId.h"
#include "MameNames.h"
#include "SystemData.h"
#include "FileData.h"
#include "Log.h"
#include "Window.h"

GuiScraperSearch::GuiScraperSearch(
		Window* window,
		SearchType type)
		: GuiComponent(window),
		mGrid(window, Vector2i(4, 3)),
		mBusyAnim(window),
		mSearchType(type)
{
	addChild(&mGrid);

	mBlockAccept = false;

	// Left spacer (empty component, needed for borders).
	mGrid.setEntry(std::make_shared<GuiComponent>(mWindow), Vector2i(0, 0),
			false, false, Vector2i(1, 3), GridFlags::BORDER_TOP | GridFlags::BORDER_BOTTOM);

	// Selected result name.
	mResultName = std::make_shared<TextComponent>(mWindow, "Result name",
			Font::get(FONT_SIZE_MEDIUM), 0x777777FF);

	// Selected result thumbnail.
	mResultThumbnail = std::make_shared<ImageComponent>(mWindow);
	mGrid.setEntry(mResultThumbnail, Vector2i(1, 1), false, false, Vector2i(1, 1));

	// Selected result description and container.
	mDescContainer = std::make_shared<ScrollableContainer>(mWindow);
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
	mMD_Developer = std::make_shared<TextComponent>(mWindow, "", font, mdColor);
	mMD_Publisher = std::make_shared<TextComponent>(mWindow, "", font, mdColor);
	mMD_Genre = std::make_shared<TextComponent>(mWindow, "", font, mdColor);
	mMD_Players = std::make_shared<TextComponent>(mWindow, "", font, mdColor);

	if (Settings::getInstance()->getBool("ScrapeRatings") &&
			Settings::getInstance()->getString("Scraper") != "TheGamesDB")
		mMD_Pairs.push_back(MetaDataPair(std::make_shared<TextComponent>
				(mWindow, "RATING:", font, mdLblColor), mMD_Rating, false));
	mMD_Pairs.push_back(MetaDataPair(std::make_shared<TextComponent>
			(mWindow, "RELEASED:", font, mdLblColor), mMD_ReleaseDate));
	mMD_Pairs.push_back(MetaDataPair(std::make_shared<TextComponent>
			(mWindow, "DEVELOPER:", font, mdLblColor), mMD_Developer));
	mMD_Pairs.push_back(MetaDataPair(std::make_shared<TextComponent>
			(mWindow, "PUBLISHER:", font, mdLblColor), mMD_Publisher));
	mMD_Pairs.push_back(MetaDataPair(std::make_shared<TextComponent>
			(mWindow, "GENRE:", font, mdLblColor), mMD_Genre));
	mMD_Pairs.push_back(MetaDataPair(std::make_shared<TextComponent>
			(mWindow, "PLAYERS:", font, mdLblColor), mMD_Players));

	mMD_Grid = std::make_shared<ComponentGrid>(mWindow,
			Vector2i(2, (int)mMD_Pairs.size()*2 - 1));
	unsigned int i = 0;
	for (auto it = mMD_Pairs.cbegin(); it != mMD_Pairs.cend(); it++) {
		mMD_Grid->setEntry(it->first, Vector2i(0, i), false, true);
		mMD_Grid->setEntry(it->second, Vector2i(1, i), false, it->resize);
		i += 2;
	}

	mGrid.setEntry(mMD_Grid, Vector2i(2, 1), false, false);

	// Result list.
	mResultList = std::make_shared<ComponentList>(mWindow);
	mResultList->setCursorChangedCallback([this](CursorState state) {
			if (state == CURSOR_STOPPED) updateInfoPane(); });

	updateViewStyle();
}

void GuiScraperSearch::onSizeChanged()
{
	mGrid.setSize(mSize);

	if (mSize.x() == 0 || mSize.y() == 0)
		return;

	// Column widths.
	if (mSearchType == ALWAYS_ACCEPT_FIRST_RESULT)
		mGrid.setColWidthPerc(0, 0.02f); // Looks better when this is higher in auto mode.
	else
		mGrid.setColWidthPerc(0, 0.01f);

	mGrid.setColWidthPerc(1, 0.25f);
	mGrid.setColWidthPerc(2, 0.25f);

	// Row heights.
	if (mSearchType == ALWAYS_ACCEPT_FIRST_RESULT) // Show name.
		mGrid.setRowHeightPerc(0, (mResultName->getFont()->getHeight() * 1.6f) /
				mGrid.getSize().y()); // Result name.
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
		mDescContainer->setSize(mGrid.getColWidth(1)*boxartCellScale +
			mGrid.getColWidth(2), mResultDesc->getFont()->getHeight() * 3);
	else
		mDescContainer->setSize(mGrid.getColWidth(3)*boxartCellScale,
				mResultDesc->getFont()->getHeight() * 7);

	// Make description text wrap at edge of container.
	mResultDesc->setSize(mDescContainer->getSize().x(), 0);

	mGrid.onSizeChanged();

	mBusyAnim.setSize(mSize);
}

void GuiScraperSearch::resizeMetadata()
{
	mMD_Grid->setSize(mGrid.getColWidth(2), mGrid.getRowHeight(1));
	if (mMD_Grid->getSize().y() > mMD_Pairs.size()) {
		const int fontHeight = (int)(mMD_Grid->getSize().y() / mMD_Pairs.size() * 0.8f);
		auto fontLbl = Font::get(fontHeight, FONT_PATH_REGULAR);
		auto fontComp = Font::get(fontHeight, FONT_PATH_LIGHT);

		// Update label fonts.
		float maxLblWidth = 0;
		for (auto it = mMD_Pairs.cbegin(); it != mMD_Pairs.cend(); it++) {
			it->first->setFont(fontLbl);
			it->first->setSize(0, 0);
			if (it->first->getSize().x() > maxLblWidth)
				maxLblWidth = it->first->getSize().x() + 6;
		}

		for (unsigned int i = 0; i < mMD_Pairs.size(); i++)
			mMD_Grid->setRowHeightPerc(i*2, (fontLbl->getLetterHeight() + 2) /
					mMD_Grid->getSize().y());

		// Update component fonts.
		mMD_ReleaseDate->setFont(fontComp);
		mMD_Developer->setFont(fontComp);
		mMD_Publisher->setFont(fontComp);
		mMD_Genre->setFont(fontComp);
		mMD_Players->setFont(fontComp);

		mMD_Grid->setColWidthPerc(0, maxLblWidth / mMD_Grid->getSize().x());

		if (Settings::getInstance()->getBool("ScrapeRatings") &&
			Settings::getInstance()->getString("Scraper") != "TheGamesDB") {
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
		mGrid.setEntry(mResultName, Vector2i(1, 0), false, true, Vector2i(2, 1),
				GridFlags::BORDER_TOP);

		// Need a border on the bottom left.
		mGrid.setEntry(std::make_shared<GuiComponent>(mWindow), Vector2i(0, 2),
				false, false, Vector2i(3, 1), GridFlags::BORDER_BOTTOM);

		// Show description on the right.
		mGrid.setEntry(mDescContainer, Vector2i(3, 0), false, false, Vector2i(1, 3),
				GridFlags::BORDER_TOP | GridFlags::BORDER_BOTTOM);
		// Make description text wrap at edge of container.
		mResultDesc->setSize(mDescContainer->getSize().x(), 0);
	}
	else {
		// Fake row where name would be.
		mGrid.setEntry(std::make_shared<GuiComponent>(mWindow), Vector2i(1, 0),
				false, true, Vector2i(2, 1), GridFlags::BORDER_TOP);

		// Show result list on the right.
		mGrid.setEntry(mResultList, Vector2i(3, 0), true, true, Vector2i(1, 3),
				GridFlags::BORDER_LEFT | GridFlags::BORDER_TOP | GridFlags::BORDER_BOTTOM);

		// Show description under image/info.
		mGrid.setEntry(mDescContainer, Vector2i(1, 2), false, false, Vector2i(2, 1),
				GridFlags::BORDER_BOTTOM);
		// Make description text wrap at edge of container.
		mResultDesc->setSize(mDescContainer->getSize().x(), 0);
	}
}

void GuiScraperSearch::search(const ScraperSearchParams& params)
{
	mBlockAccept = true;

	mResultList->clear();
	mScraperResults.clear();
	mMDRetrieveURLsHandle.reset();
	mThumbnailReq.reset();
	mMDResolveHandle.reset();
	updateInfoPane();

	mLastSearch = params;
	mSearchHandle = startScraperSearch(params);
}

void GuiScraperSearch::stop()
{
	mThumbnailReq.reset();
	mSearchHandle.reset();
	mMDResolveHandle.reset();
	mMDRetrieveURLsHandle.reset();
	mBlockAccept = false;
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
			mWindow->pushGui(new GuiMsgBox(mWindow, Utils::String::toUpper("Configured scraper "
					"is no longer available.\nPlease change the scraping source in the settings."),
				"FINISH", mSkipCallback));
		}
		else {
			ComponentListRow row;
			row.addElement(std::make_shared<TextComponent>(mWindow, "NO GAMES FOUND - SKIP",
					font, color), true);

			if (mSkipCallback)
				row.makeAcceptInputHandler(mSkipCallback);

			mResultList->addRow(row);
			mGrid.resetCursor();
		}
	}
	else {
		ComponentListRow row;
		for (size_t i = 0; i < results.size(); i++) {
			row.elements.clear();
			row.addElement(std::make_shared<TextComponent>(mWindow,
					Utils::String::toUpper(results.at(i).mdl.get("name")), font, color), true);
			row.makeAcceptInputHandler([this, i] { returnResult(mScraperResults.at(i)); });
			mResultList->addRow(row);
		}
		mGrid.resetCursor();
	}

	mBlockAccept = false;
	updateInfoPane();

	// If there is no scraping result or if there is no game media to download
	// as a thumbnail, then proceed directly.
	if (mSearchType == ALWAYS_ACCEPT_FIRST_RESULT) {
		if (mScraperResults.size() == 0 || (mScraperResults.size() > 0 &&
				mScraperResults.front().ThumbnailImageUrl == "")) {
			if (mScraperResults.size() == 0)
				mSkipCallback();
			else
				returnResult(mScraperResults.front());
		}
	}

}

void GuiScraperSearch::onSearchError(const std::string& error)
{
	LOG(LogInfo) << "GuiScraperSearch search error: " << error;
	mWindow->pushGui(new GuiMsgBox(mWindow, Utils::String::toUpper(error),
		"RETRY", std::bind(&GuiScraperSearch::search, this, mLastSearch),
		"SKIP", mSkipCallback,
		"CANCEL", mCancelCallback));
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

	if (i != -1 && (int)mScraperResults.size() > i) {
		ScraperSearchResult& res = mScraperResults.at(i);
		mResultName->setText(Utils::String::toUpper(res.mdl.get("name")));
		mResultDesc->setText(Utils::String::toUpper(res.mdl.get("desc")));
		mDescContainer->reset();

		mResultThumbnail->setImage("");
		const std::string& thumb = res.screenshotUrl.empty() ? res.coverUrl : res.screenshotUrl;
		mScraperResults[i].ThumbnailImageUrl = thumb;

		// Cache the thumbnail image in mScraperResults so that we don't need to download
		// it every time the list is scrolled back and forth.
		if (mScraperResults[i].ThumbnailImageData.size() > 0) {
			std::string content = mScraperResults[i].ThumbnailImageData;
			mResultThumbnail->setImage(content.data(), content.length());
			mGrid.onSizeChanged(); // A hack to fix the thumbnail position since its size changed.
		}
		// If it's not cached in mScraperResults it should mean that it's the first time
		// we access the entry, and therefore we need to download the image.
		else {
			if (!thumb.empty()) {
				// Make sure we don't attempt to download the same thumbnail twice.
				if (!mThumbnailReq && mScraperResults[i].thumbnailDownloadStatus != IN_PROGRESS) {
					mScraperResults[i].thumbnailDownloadStatus = IN_PROGRESS;
					mThumbnailReq = std::unique_ptr<HttpReq>(new HttpReq(thumb));
				}
			}
			else {
				mThumbnailReq.reset();
			}
		}

		// Metadata.
		if (Settings::getInstance()->getBool("ScrapeRatings") &&
				Settings::getInstance()->getString("Scraper") != "TheGamesDB") {
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
		if (Settings::getInstance()->getBool("ScrapeRatings") &&
				Settings::getInstance()->getString("Scraper") != "TheGamesDB") {
			mMD_Rating->setValue("");
			mMD_Rating->setOpacity(0);
		}
		mMD_ReleaseDate->setValue("99990101T000000");
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

	return GuiComponent::input(config, input);
}

void GuiScraperSearch::render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = parentTrans * getTransform();

	renderChildren(trans);

	if (mBlockAccept) {
		Renderer::setMatrix(trans);
		Renderer::drawRect(0.0f, 0.0f, mSize.x(), mSize.y(), 0x00000011, 0x00000011);

		mBusyAnim.render(trans);
	}
}

void GuiScraperSearch::returnResult(ScraperSearchResult result)
{
	mBlockAccept = true;

	// Resolve metadata image before returning.
	if (result.mediaFilesDownloadStatus != COMPLETED) {
		result.mediaFilesDownloadStatus = IN_PROGRESS;
		mMDResolveHandle = resolveMetaDataAssets(result, mLastSearch);
		return;
	}

	mAcceptCallback(result);
}

void GuiScraperSearch::update(int deltaTime)
{
	GuiComponent::update(deltaTime);

	if (mBlockAccept)
		mBusyAnim.update(deltaTime);

	if (mThumbnailReq && mThumbnailReq->status() != HttpReq::REQ_IN_PROGRESS)
		updateThumbnail();

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
			auto status_media = mMDRetrieveURLsHandle->status();
			auto results_media = mMDRetrieveURLsHandle->getResults();
			auto statusString_media = mMDRetrieveURLsHandle->getStatusString();
			auto results_scrape = mScraperResults;
			mMDRetrieveURLsHandle.reset();
			mScraperResults.clear();

			// Combine the intial scrape results with the media URL results.
			for (auto it = results_media.cbegin(); it != results_media.cend(); it++) {
				for (unsigned int i = 0; i < results_scrape.size(); i++) {
					if (results_scrape[i].gameID == it->gameID) {
						results_scrape[i].box3dUrl = it->box3dUrl;
						results_scrape[i].coverUrl = it->coverUrl;
						results_scrape[i].marqueeUrl = it->marqueeUrl;
						results_scrape[i].screenshotUrl = it->screenshotUrl;
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

	if (mMDResolveHandle && mMDResolveHandle->status() != ASYNC_IN_PROGRESS) {
		if (mMDResolveHandle->status() == ASYNC_DONE) {
			ScraperSearchResult result = mMDResolveHandle->getResult();
			result.mediaFilesDownloadStatus = COMPLETED;
			mMDResolveHandle.reset();
			// This might end in us being deleted, depending on mAcceptCallback -
			// so make sure this is the last thing we do in update().
			returnResult(result);
		}
		else if (mMDResolveHandle->status() == ASYNC_ERROR) {
			onSearchError(mMDResolveHandle->getStatusString());
			mMDResolveHandle.reset();
		}
	}
}

void GuiScraperSearch::updateThumbnail()
{
	if (mThumbnailReq && mThumbnailReq->status() == HttpReq::REQ_SUCCESS) {
		// Save thumbnail to mScraperResults cache and set the flag that the
		// thumbnail download has been completed for this game.
		for (auto i = 0; i < mScraperResults.size(); i++) {
			if (mScraperResults[i].thumbnailDownloadStatus == IN_PROGRESS) {
				mScraperResults[i].ThumbnailImageData = mThumbnailReq->getContent();
				mScraperResults[i].thumbnailDownloadStatus = COMPLETED;
			}
		}
		// Activate the thumbnail in the GUI.
		std::string content = mThumbnailReq->getContent();
		mResultThumbnail->setImage(content.data(), content.length());
		mGrid.onSizeChanged(); // A hack to fix the thumbnail position since its size changed.
	}
	else {
		LOG(LogWarning) << "thumbnail req failed: " << mThumbnailReq->getErrorMsg();
		mResultThumbnail->setImage("");
	}

	mThumbnailReq.reset();

	// When the thumbnail has been downloaded and we are in non-interactive
	// mode, we proceed to automatically download the rest of the media files.
	// The reason to always complete the thumbnail download first is that it looks
	// a lot more consistent in the GUI. And since the thumbnail is being cached
	// anyway, this hardly takes any more time. Maybe rather the opposite as the
	// image used for the thumbnail (cover or screenshot) would have had to be
	// requested from the server again.
	if (mSearchType == ALWAYS_ACCEPT_FIRST_RESULT &&
			mScraperResults.front().thumbnailDownloadStatus == COMPLETED) {
		if (mScraperResults.size() == 0)
			mSkipCallback();
		else
			returnResult(mScraperResults.front());
	}
}

void GuiScraperSearch::openInputScreen(ScraperSearchParams& params)
{
	auto searchForFunc = [&](const std::string& name) {
		params.nameOverride = name;
		search(params);
	};

	stop();

	if (params.system->hasPlatformId(PlatformIds::ARCADE) ||
			params.system->hasPlatformId(PlatformIds::NEOGEO)) {
		mWindow->pushGui(new GuiTextEditPopup(mWindow, "SEARCH FOR",
			// Initial value is last search if there was one, otherwise the clean path name.
			// If it's a MAME or Neo Geo game, expand the game name accordingly.
			params.nameOverride.empty() ?
					MameNames::getInstance()->getCleanName(params.game->getCleanName()) :
					params.nameOverride,
			searchForFunc, false, "SEARCH"));
	}
	else {
		mWindow->pushGui(new GuiTextEditPopup(mWindow, "SEARCH FOR",
			// Initial value is last search if there was one, otherwise the clean path name.
			params.nameOverride.empty() ? params.game->getCleanName() : params.nameOverride,
			searchForFunc, false, "SEARCH"));
	}
}

bool GuiScraperSearch::saveMetadata(
		const ScraperSearchResult& result, MetaDataList& metadata)
{
	bool mMetadataUpdated = false;
	std::vector<MetaDataDecl> mMetaDataDecl = metadata.getMDD();

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

		// Overwrite all the other values if the flag to overwrite data has been set.
		if (Settings::getInstance()->getBool("ScraperOverwriteData")) {
			metadata.set(key, result.mdl.get(key));
			mMetadataUpdated = true;
		}
		// Else only update the value if it is set to the default metadata value.
		else if (metadata.get(key) == mMetaDataDecl.at(i).defaultValue) {
			metadata.set(key, result.mdl.get(key));
			mMetadataUpdated = true;
		}
	}

	return mMetadataUpdated;
}

std::vector<HelpPrompt> GuiScraperSearch::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mGrid.getHelpPrompts();
	if (getSelectedIndex() != -1)
		prompts.push_back(HelpPrompt("a", "accept result"));

	return prompts;
}

void GuiScraperSearch::onFocusGained()
{
	mGrid.onFocusGained();
}

void GuiScraperSearch::onFocusLost()
{
	mGrid.onFocusLost();
}
