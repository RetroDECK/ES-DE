//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiScraperSearch.h
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

#ifndef ES_APP_GUIS_GUI_SCRAPER_SEARCH_H
#define ES_APP_GUIS_GUI_SCRAPER_SEARCH_H

#include "components/BusyComponent.h"
#include "components/ComponentGrid.h"
#include "scrapers/Scraper.h"
#include "GuiComponent.h"

class ComponentList;
class DateTimeEditComponent;
class ImageComponent;
class RatingComponent;
class ScrollableContainer;
class TextComponent;

class GuiScraperSearch : public GuiComponent
{
public:
    enum SearchType {
        ALWAYS_ACCEPT_FIRST_RESULT,
        ACCEPT_SINGLE_MATCHES,
        NEVER_AUTO_ACCEPT
    };

    GuiScraperSearch(Window* window, SearchType searchType, unsigned int scrapeCount = 1);
    ~GuiScraperSearch();

    void search(const ScraperSearchParams& params);
    void openInputScreen(ScraperSearchParams& from);
    void stop();
    inline SearchType getSearchType() const { return mSearchType; }
    static bool saveMetadata(const ScraperSearchResult& result, MetaDataList& metadata);

    // Metadata assets will be resolved before calling the accept callback
    // (e.g. result.mdl's "image" is automatically downloaded and properly set).
    inline void setAcceptCallback(const std::function<void(const ScraperSearchResult&)>&
            acceptCallback) { mAcceptCallback = acceptCallback; }
    inline void setSkipCallback(const std::function<void()>&
            skipCallback) { mSkipCallback = skipCallback; };
    inline void setCancelCallback(const std::function<void()>&
            cancelCallback) { mScrapeCount -= 1; mCancelCallback = cancelCallback; }

    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const Transform4x4f& parentTrans) override;
    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override;
    void onSizeChanged() override;
    void onFocusGained() override;
    void onFocusLost() override;

private:
    void updateViewStyle();
    void updateThumbnail();
    void updateInfoPane();

    void resizeMetadata();

    void onSearchError(const std::string& error);
    void onSearchDone(const std::vector<ScraperSearchResult>& results);

    int getSelectedIndex();

    // For TheGamesDB, retrieve URLs for the additional metadata assets
    // that need to be downloaded.
    void retrieveMediaURLs(ScraperSearchResult result);

    // Resolve any metadata assets that need to be downloaded and return.
    void returnResult(ScraperSearchResult result);

    ComponentGrid mGrid;

    std::shared_ptr<TextComponent> mResultName;
    std::shared_ptr<ScrollableContainer> mDescContainer;
    std::shared_ptr<TextComponent> mResultDesc;
    std::shared_ptr<ImageComponent> mResultThumbnail;
    std::shared_ptr<ComponentList> mResultList;

    std::shared_ptr<ComponentGrid> mMD_Grid;
    std::shared_ptr<RatingComponent> mMD_Rating;
    std::shared_ptr<DateTimeEditComponent> mMD_ReleaseDate;
    std::shared_ptr<TextComponent> mMD_Developer;
    std::shared_ptr<TextComponent> mMD_Publisher;
    std::shared_ptr<TextComponent> mMD_Genre;
    std::shared_ptr<TextComponent> mMD_Players;
    std::shared_ptr<TextComponent> mMD_Filler;

    // Label-component pair.
    struct MetaDataPair {
        std::shared_ptr<TextComponent> first;
        std::shared_ptr<GuiComponent> second;
        bool resize;

        MetaDataPair(const std::shared_ptr<TextComponent>& f,
                const std::shared_ptr<GuiComponent>& s, bool r = true)
                : first(f), second(s), resize(r) {};
    };

    std::vector<MetaDataPair> mMD_Pairs;

    SearchType mSearchType;
    ScraperSearchParams mLastSearch;
    std::function<void(const ScraperSearchResult&)> mAcceptCallback;
    std::function<void()> mSkipCallback;
    std::function<void()> mCancelCallback;
    unsigned int mScrapeCount;
    bool mRefinedSearch;
    bool mBlockAccept;
    bool mFoundGame;
    bool mScrapeRatings;

    std::unique_ptr<ScraperSearchHandle> mSearchHandle;
    std::unique_ptr<ScraperSearchHandle> mMDRetrieveURLsHandle;
    std::unique_ptr<MDResolveHandle> mMDResolveHandle;
    std::vector<ScraperSearchResult> mScraperResults;
    std::map<std::string, std::unique_ptr<HttpReq>> mThumbnailReqMap;

    BusyComponent mBusyAnim;
};

#endif // ES_APP_GUIS_GUI_SCRAPER_SEARCH_H
