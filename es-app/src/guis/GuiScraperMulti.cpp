//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiScraperMulti.cpp
//
//  Multiple game scraping user interface.
//  Shows the progress for the scraping as it's running.
//  This interface is triggered from GuiScraperMenu.
//  GuiScraperSearch is called from here.
//

#include "guis/GuiScraperMulti.h"

#include "CollectionSystemsManager.h"
#include "Gamelist.h"
#include "MameNames.h"
#include "SystemData.h"
#include "Window.h"
#include "components/ButtonComponent.h"
#include "components/MenuComponent.h"
#include "components/TextComponent.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiScraperSearch.h"
#include "views/ViewController.h"

GuiScraperMulti::GuiScraperMulti(Window* window,
                                 const std::queue<ScraperSearchParams>& searches,
                                 bool approveResults)
    : GuiComponent(window)
    , mBackground(window, ":/graphics/frame.svg")
    , mGrid(window, glm::ivec2{1, 5})
    , mSearchQueue(searches)
    , mApproveResults(approveResults)
{
    assert(mSearchQueue.size());

    addChild(&mBackground);
    addChild(&mGrid);

    mIsProcessing = true;

    mTotalGames = static_cast<int>(mSearchQueue.size());
    mCurrentGame = 0;
    mTotalSuccessful = 0;
    mTotalSkipped = 0;

    // Set up grid.
    mTitle = std::make_shared<TextComponent>(mWindow, "SCRAPING IN PROGRESS",
                                             Font::get(FONT_SIZE_LARGE), 0x555555FF, ALIGN_CENTER);
    mGrid.setEntry(mTitle, glm::ivec2{0, 0}, false, true);

    mSystem = std::make_shared<TextComponent>(mWindow, "SYSTEM", Font::get(FONT_SIZE_MEDIUM),
                                              0x777777FF, ALIGN_CENTER);
    mGrid.setEntry(mSystem, glm::ivec2{0, 1}, false, true);

    mSubtitle = std::make_shared<TextComponent>(
        mWindow, "subtitle text", Font::get(FONT_SIZE_SMALL), 0x888888FF, ALIGN_CENTER);
    mGrid.setEntry(mSubtitle, glm::ivec2{0, 2}, false, true);

    if (mApproveResults && !Settings::getInstance()->getBool("ScraperSemiautomatic"))
        mSearchComp = std::make_shared<GuiScraperSearch>(
            mWindow, GuiScraperSearch::NEVER_AUTO_ACCEPT, mTotalGames);
    else if (mApproveResults && Settings::getInstance()->getBool("ScraperSemiautomatic"))
        mSearchComp = std::make_shared<GuiScraperSearch>(
            mWindow, GuiScraperSearch::ACCEPT_SINGLE_MATCHES, mTotalGames);
    else if (!mApproveResults)
        mSearchComp = std::make_shared<GuiScraperSearch>(
            mWindow, GuiScraperSearch::ALWAYS_ACCEPT_FIRST_RESULT, mTotalGames);
    mSearchComp->setAcceptCallback(
        std::bind(&GuiScraperMulti::acceptResult, this, std::placeholders::_1));
    mSearchComp->setSkipCallback(std::bind(&GuiScraperMulti::skip, this));
    mSearchComp->setCancelCallback(std::bind(&GuiScraperMulti::finish, this));
    mGrid.setEntry(mSearchComp, glm::ivec2{0, 3},
                   mSearchComp->getSearchType() != GuiScraperSearch::ALWAYS_ACCEPT_FIRST_RESULT,
                   true);

    std::vector<std::shared_ptr<ButtonComponent>> buttons;

    if (mApproveResults) {
        buttons.push_back(
            std::make_shared<ButtonComponent>(mWindow, "REFINE SEARCH", "refine search", [&] {
                // Check whether we should allow a refine of the game name.
                if (!mSearchComp->getAcceptedResult()) {
                    bool allowRefine = false;

                    // Previously refined.
                    if (mSearchComp->getRefinedSearch())
                        allowRefine = true;
                    // Interactive mode and "Auto-accept single game matches" not enabled.
                    else if (mSearchComp->getSearchType() !=
                             GuiScraperSearch::ACCEPT_SINGLE_MATCHES)
                        allowRefine = true;
                    // Interactive mode with "Auto-accept single game matches" enabled and more
                    // than one result.
                    else if (mSearchComp->getSearchType() ==
                                 GuiScraperSearch::ACCEPT_SINGLE_MATCHES &&
                             mSearchComp->getScraperResultsSize() > 1)
                        allowRefine = true;
                    // Dito but there were no games found, or the search has not been completed.
                    else if (mSearchComp->getSearchType() ==
                                 GuiScraperSearch::ACCEPT_SINGLE_MATCHES &&
                             !mSearchComp->getFoundGame())
                        allowRefine = true;

                    if (allowRefine) {
                        // Copy any search refine that may have been previously entered by opening
                        // the input screen using the "Y" button shortcut.
                        mSearchQueue.front().nameOverride = mSearchComp->getNameOverride();
                        mSearchComp->openInputScreen(mSearchQueue.front());
                        mGrid.resetCursor();
                    }
                }
            }));

        buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "SKIP", "skip game", [&] {
            // Skip game, unless the result has already been accepted.
            if (!mSearchComp->getAcceptedResult()) {
                skip();
                mGrid.resetCursor();
            }
        }));
    }

    buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "STOP", "stop",
                                                        std::bind(&GuiScraperMulti::finish, this)));

    mButtonGrid = makeButtonGrid(mWindow, buttons);
    mGrid.setEntry(mButtonGrid, glm::ivec2{0, 4}, true, false);

    // Limit the width of the GUI on ultrawide monitors. The 1.778 aspect ratio value is
    // the 16:9 reference.
    float aspectValue = 1.778f / Renderer::getScreenAspectRatio();
    float width = glm::clamp(0.95f * aspectValue, 0.70f, 0.95f) * Renderer::getScreenWidth();

    setSize(width, Renderer::getScreenHeight() * 0.849f);
    setPosition((Renderer::getScreenWidth() - mSize.x) / 2.0f,
                (Renderer::getScreenHeight() - mSize.y) / 2.0f);

    doNextSearch();
}

GuiScraperMulti::~GuiScraperMulti()
{
    if (mTotalSuccessful > 0) {
        // Sort all systems to possibly update their view style from Basic to Detailed or Video.
        for (auto it = SystemData::sSystemVector.cbegin(); // Line break.
             it != SystemData::sSystemVector.cend(); it++) {
            (*it)->sortSystem();
        }
    }
    ViewController::get()->onPauseVideo();
}

void GuiScraperMulti::onSizeChanged()
{
    mBackground.fitTo(mSize, glm::vec3{}, glm::vec2{-32.0f, -32.0f});

    mGrid.setRowHeightPerc(0, mTitle->getFont()->getLetterHeight() * 1.9725f / mSize.y, false);
    mGrid.setRowHeightPerc(1, (mSystem->getFont()->getLetterHeight() + 2.0f) / mSize.y, false);
    mGrid.setRowHeightPerc(2, mSubtitle->getFont()->getHeight() * 1.75f / mSize.y, false);
    mGrid.setRowHeightPerc(4, mButtonGrid->getSize().y / mSize.y, false);
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

    std::string scrapeName;

    if (Settings::getInstance()->getBool("ScraperSearchMetadataName")) {
        scrapeName = mSearchQueue.front().game->getName();
    }
    else {
        if (mSearchQueue.front().game->isArcadeGame() &&
            Settings::getInstance()->getString("Scraper") == "thegamesdb")
            scrapeName =
                Utils::FileSystem::getFileName(mSearchQueue.front().game->getPath()) + " (" +
                MameNames::getInstance()->getCleanName(mSearchQueue.front().game->getCleanName()) +
                ")";
        else
            scrapeName = Utils::FileSystem::getFileName(mSearchQueue.front().game->getPath());
    }

    // Extract possible subfolders from the path.
    std::string folderPath =
        Utils::String::replace(Utils::FileSystem::getParent(mSearchQueue.front().game->getPath()),
                               mSearchQueue.front().system->getSystemEnvData()->mStartPath, "");

    if (folderPath.size() >= 2) {
        folderPath.erase(0, 1);
#if defined(_WIN64)
        folderPath.push_back('\\');
        folderPath = Utils::String::replace(folderPath, "/", "\\");
#else
        folderPath.push_back('/');
#endif
    }

    // Update subtitle.
    ss.str("");
    ss << "GAME " << (mCurrentGame + 1) << " OF " << mTotalGames << " - " << folderPath
       << scrapeName
       << ((mSearchQueue.front().game->getType() == FOLDER) ? "  " + ViewController::FOLDER_CHAR :
                                                              "");
    mSubtitle->setText(ss.str());

    mSearchComp->search(mSearchQueue.front());
}

void GuiScraperMulti::acceptResult(const ScraperSearchResult& result)
{
    ScraperSearchParams& search = mSearchQueue.front();

    GuiScraperSearch::saveMetadata(result, search.game->metadata, search.game);

    updateGamelist(search.system);

    mSearchQueue.pop();
    mCurrentGame++;
    mTotalSuccessful++;
    CollectionSystemsManager::get()->refreshCollectionSystems(search.game);
    doNextSearch();
}

void GuiScraperMulti::skip()
{
    mSearchQueue.pop();
    mCurrentGame++;
    mTotalSkipped++;
    mSearchComp->decreaseScrapeCount();
    mSearchComp->unsetRefinedSearch();
    doNextSearch();
}

void GuiScraperMulti::finish()
{
    std::stringstream ss;
    if (mTotalSuccessful == 0) {
        ss << "NO GAMES WERE SCRAPED";
    }
    else {
        ss << mTotalSuccessful << " GAME" << ((mTotalSuccessful > 1) ? "S" : "")
           << " SUCCESSFULLY SCRAPED";

        if (mTotalSkipped > 0)
            ss << "\n"
               << mTotalSkipped << " GAME" << ((mTotalSkipped > 1) ? "S" : "") << " SKIPPED";
    }

    mWindow->pushGui(new GuiMsgBox(mWindow, getHelpStyle(), ss.str(), "OK", [&] {
        mIsProcessing = false;
        delete this;
    }));
}

std::vector<HelpPrompt> GuiScraperMulti::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts = mGrid.getHelpPrompts();
    // Remove the 'Choose' entry if in fully automatic mode.
    if (!mApproveResults)
        prompts.pop_back();
    return prompts;
}

HelpStyle GuiScraperMulti::getHelpStyle()
{
    HelpStyle style = HelpStyle();
    style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");
    return style;
}
