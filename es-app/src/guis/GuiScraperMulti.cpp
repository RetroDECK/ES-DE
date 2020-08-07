//
//  GuiScraperMulti.cpp
//
//  Multiple game scraping user interface.
//  Shows the progress for the scraping as it's running.
//  This interface is triggered from GuiScraperMenu.
//  GuiScraperSearch is called from here.
//

#include "guis/GuiScraperMulti.h"

#include "components/ButtonComponent.h"
#include "components/MenuComponent.h"
#include "components/TextComponent.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiScraperSearch.h"
#include "views/ViewController.h"
#include "Gamelist.h"
#include "PowerSaver.h"
#include "SystemData.h"
#include "Window.h"

GuiScraperMulti::GuiScraperMulti(
        Window* window,
        const std::queue<ScraperSearchParams>& searches,
        bool approveResults)
        : GuiComponent(window),
        mBackground(window, ":/graphics/frame.png"),
        mGrid(window, Vector2i(1, 5)),
        mSearchQueue(searches)
{
    assert(mSearchQueue.size());

    addChild(&mBackground);
    addChild(&mGrid);

    PowerSaver::pause();
    mIsProcessing = true;

    mTotalGames = (int)mSearchQueue.size();
    mCurrentGame = 0;
    mTotalSuccessful = 0;
    mTotalSkipped = 0;

    // Set up grid.
    mTitle = std::make_shared<TextComponent>(mWindow, "SCRAPING IN PROGRESS",
            Font::get(FONT_SIZE_LARGE), 0x555555FF, ALIGN_CENTER);
    mGrid.setEntry(mTitle, Vector2i(0, 0), false, true);

    mSystem = std::make_shared<TextComponent>(mWindow, "SYSTEM",
            Font::get(FONT_SIZE_MEDIUM), 0x777777FF, ALIGN_CENTER);
    mGrid.setEntry(mSystem, Vector2i(0, 1), false, true);

    mSubtitle = std::make_shared<TextComponent>(mWindow, "subtitle text",
            Font::get(FONT_SIZE_SMALL), 0x888888FF, ALIGN_CENTER);
    mGrid.setEntry(mSubtitle, Vector2i(0, 2), false, true);

    if (approveResults && !Settings::getInstance()->getBool("ScraperSemiautomatic"))
        mSearchComp = std::make_shared<GuiScraperSearch>(mWindow,
                GuiScraperSearch::NEVER_AUTO_ACCEPT, mTotalGames);
    else if (approveResults && Settings::getInstance()->getBool("ScraperSemiautomatic"))
        mSearchComp = std::make_shared<GuiScraperSearch>(mWindow,
                GuiScraperSearch::ACCEPT_SINGLE_MATCHES, mTotalGames);
    else if (!approveResults)
        mSearchComp = std::make_shared<GuiScraperSearch>(mWindow,
                GuiScraperSearch::ALWAYS_ACCEPT_FIRST_RESULT, mTotalGames);
    mSearchComp->setAcceptCallback(std::bind(&GuiScraperMulti::acceptResult,
            this, std::placeholders::_1));
    mSearchComp->setSkipCallback(std::bind(&GuiScraperMulti::skip, this));
    mSearchComp->setCancelCallback(std::bind(&GuiScraperMulti::finish, this));
    mGrid.setEntry(mSearchComp, Vector2i(0, 3), mSearchComp->getSearchType() !=
            GuiScraperSearch::ALWAYS_ACCEPT_FIRST_RESULT, true);

    std::vector< std::shared_ptr<ButtonComponent> > buttons;

    if (approveResults) {
        buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "REFINE SEARCH",
                "refine search", [&] {
            mSearchComp->openInputScreen(mSearchQueue.front());
            mGrid.resetCursor();
        }));

        buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "SKIP", "skip game", [&] {
            skip();
            mGrid.resetCursor();
        }));
    }

    buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "STOP",
            "stop (progress saved)", std::bind(&GuiScraperMulti::finish, this)));

    mButtonGrid = makeButtonGrid(mWindow, buttons);
    mGrid.setEntry(mButtonGrid, Vector2i(0, 4), true, false);

    setSize(Renderer::getScreenWidth() * 0.95f, Renderer::getScreenHeight() * 0.849f);
    setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, (Renderer::getScreenHeight() -
            mSize.y()) / 2);

    doNextSearch();
}

GuiScraperMulti::~GuiScraperMulti()
{
    // View type probably changed (basic -> detailed).
    for (auto it = SystemData::sSystemVector.cbegin();
            it !=SystemData::sSystemVector.cend(); it++)
        ViewController::get()->reloadGameListView(*it, false);
}

void GuiScraperMulti::onSizeChanged()
{
    mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

    mGrid.setRowHeightPerc(0, mTitle->getFont()->getLetterHeight() * 1.9725f / mSize.y(), false);
    mGrid.setRowHeightPerc(1, (mSystem->getFont()->getLetterHeight() + 2) / mSize.y(), false);
    mGrid.setRowHeightPerc(2, mSubtitle->getFont()->getHeight() * 1.75f / mSize.y(), false);
    mGrid.setRowHeightPerc(4, mButtonGrid->getSize().y() / mSize.y(), false);
    mGrid.setSize(mSize);
}

void GuiScraperMulti::doNextSearch()
{
    if (mSearchQueue.empty()) {
        finish();
        return;
    }

    // Update title.
    std::stringstream ss;
    mSystem->setText(Utils::String::toUpper(mSearchQueue.front().system->getFullName()));

    // Update subtitle.
    ss.str(""); // Clear.
    ss << "GAME " << (mCurrentGame + 1) << " OF " << mTotalGames << " - " <<
            Utils::FileSystem::getFileName(mSearchQueue.front().game->getPath());
    mSubtitle->setText(ss.str());

    mSearchComp->search(mSearchQueue.front());
}

void GuiScraperMulti::acceptResult(const ScraperSearchResult& result)
{
    ScraperSearchParams& search = mSearchQueue.front();

    GuiScraperSearch::saveMetadata(result, search.game->metadata);

    updateGamelist(search.system);

    mSearchQueue.pop();
    mCurrentGame++;
    mTotalSuccessful++;
    doNextSearch();
}

void GuiScraperMulti::skip()
{
    mSearchQueue.pop();
    mCurrentGame++;
    mTotalSkipped++;
    doNextSearch();
}

void GuiScraperMulti::finish()
{
    std::stringstream ss;
    if (mTotalSuccessful == 0) {
        ss << "NO GAMES WERE SCRAPED";
    }
    else {
        ss << mTotalSuccessful << " GAME" <<
                ((mTotalSuccessful > 1) ? "S" : "") << " SUCCESSFULLY SCRAPED";

        if (mTotalSkipped > 0)
            ss << "\n" << mTotalSkipped << " GAME"
                    << ((mTotalSkipped > 1) ? "S" : "") << " SKIPPED";
    }

    mWindow->pushGui(new GuiMsgBox(mWindow, getHelpStyle(), ss.str(), "OK", [&] {
        mIsProcessing = false;
        delete this;
    }));

    PowerSaver::resume();
}

std::vector<HelpPrompt> GuiScraperMulti::getHelpPrompts()
{
    return mGrid.getHelpPrompts();
}

HelpStyle GuiScraperMulti::getHelpStyle()
{
    HelpStyle style = HelpStyle();
    style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");
    return style;
}
