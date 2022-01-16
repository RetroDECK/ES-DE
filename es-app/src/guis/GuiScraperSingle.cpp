//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiScraperSingle.cpp
//
//  Single game scraping user interface.
//  This interface is triggered from GuiMetaDataEd.
//  GuiScraperSearch is called from here.
//

#include "guis/GuiScraperSingle.h"

#include "FileData.h"
#include "MameNames.h"
#include "SystemData.h"
#include "components/ButtonComponent.h"
#include "components/MenuComponent.h"
#include "components/TextComponent.h"
#include "views/ViewController.h"

GuiScraperSingle::GuiScraperSingle(Window* window,
                                   ScraperSearchParams& params,
                                   std::function<void(const ScraperSearchResult&)> doneFunc,
                                   bool& savedMediaAndAborted)
    : GuiComponent(window)
    , mClose(false)
    , mGrid(window, glm::ivec2 {2, 6})
    , mBox(window, ":/graphics/frame.svg")
    , mSearchParams(params)
    , mSavedMediaAndAborted(savedMediaAndAborted)
{
    addChild(&mBox);
    addChild(&mGrid);

    std::string scrapeName;

    if (Settings::getInstance()->getBool("ScraperSearchMetadataName")) {
        scrapeName = mSearchParams.game->getName();
    }
    else {
        if (params.game->isArcadeGame() &&
            Settings::getInstance()->getString("Scraper") == "thegamesdb")
            scrapeName = Utils::FileSystem::getFileName(mSearchParams.game->getPath()) + " (" +
                         MameNames::getInstance().getCleanName(mSearchParams.game->getCleanName()) +
                         ")";
        else
            scrapeName = Utils::FileSystem::getFileName(mSearchParams.game->getPath());
    }

    mGameName = std::make_shared<TextComponent>(
        mWindow,
        scrapeName +
            ((mSearchParams.game->getType() == FOLDER) ? "  " + ViewController::FOLDER_CHAR : ""),
        Font::get(FONT_SIZE_LARGE), 0x777777FF, ALIGN_CENTER);
    mGameName->setColor(0x555555FF);
    mGrid.setEntry(mGameName, glm::ivec2 {0, 0}, false, true, glm::ivec2 {2, 2});

    mSystemName = std::make_shared<TextComponent>(
        mWindow, Utils::String::toUpper(mSearchParams.system->getFullName()),
        Font::get(FONT_SIZE_SMALL), 0x888888FF, ALIGN_CENTER);
    mGrid.setEntry(mSystemName, glm::ivec2 {0, 2}, false, true, glm::ivec2 {2, 1});

    // Row 3 is a spacer.

    // GuiScraperSearch.
    mSearch = std::make_shared<GuiScraperSearch>(window, GuiScraperSearch::NEVER_AUTO_ACCEPT, 1);
    mGrid.setEntry(mSearch, glm::ivec2 {0, 4}, true, true, glm::ivec2 {2, 1});

    mResultList = mSearch->getResultList();

    // Set up scroll indicators.
    mScrollUp = std::make_shared<ImageComponent>(mWindow);
    mScrollDown = std::make_shared<ImageComponent>(mWindow);
    mScrollIndicator =
        std::make_shared<ScrollIndicatorComponent>(mResultList, mScrollUp, mScrollDown);

    mScrollUp->setResize(0.0f, mGameName->getFont()->getLetterHeight() / 2.0f);
    mScrollUp->setOrigin(0.0f, -0.35f);

    mScrollDown->setResize(0.0f, mGameName->getFont()->getLetterHeight() / 2.0f);
    mScrollDown->setOrigin(0.0f, 0.35f);

    mGrid.setEntry(mScrollUp, glm::ivec2 {1, 0}, false, false, glm::ivec2 {1, 1});
    mGrid.setEntry(mScrollDown, glm::ivec2 {1, 1}, false, false, glm::ivec2 {1, 1});

    // Buttons
    std::vector<std::shared_ptr<ButtonComponent>> buttons;

    buttons.push_back(
        std::make_shared<ButtonComponent>(mWindow, "REFINE SEARCH", "refine search", [&] {
            // Refine the search, unless the result has already been accepted.
            if (!mSearch->getAcceptedResult()) {
                // Copy any search refine that may have been previously entered by opening
                // the input screen using the "Y" button shortcut.
                mSearchParams.nameOverride = mSearch->getNameOverride();
                mSearch->openInputScreen(mSearchParams);
                mGrid.resetCursor();
            }
        }));
    buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "CANCEL", "cancel", [&] {
        if (mSearch->getSavedNewMedia()) {
            // If the user aborted the scraping but there was still some media downloaded,
            // then flag to GuiMetaDataEd that the image and marquee textures need to be
            // manually unloaded and that the gamelist needs to be reloaded. Otherwise the
            // images would not get updated until the user scrolls up and down the gamelist.
            mSavedMediaAndAborted = true;
        }
        delete this;
    }));
    mButtonGrid = makeButtonGrid(mWindow, buttons);

    mGrid.setEntry(mButtonGrid, glm::ivec2 {0, 5}, true, false, glm::ivec2 {2, 1});

    mSearch->setAcceptCallback([this, doneFunc](const ScraperSearchResult& result) {
        doneFunc(result);
        close();
    });
    mSearch->setCancelCallback([&] { delete this; });
    mSearch->setRefineCallback([&] {
        mScrollUp->setOpacity(0);
        mScrollDown->setOpacity(0);
        mResultList->resetScrollIndicatorStatus();
    });

    // Limit the width of the GUI on ultrawide monitors. The 1.778 aspect ratio value is
    // the 16:9 reference.
    float aspectValue = 1.778f / Renderer::getScreenAspectRatio();
    float width = glm::clamp(0.95f * aspectValue, 0.70f, 0.95f) * Renderer::getScreenWidth();

    float height = (mGameName->getFont()->getLetterHeight() +
                    static_cast<float>(Renderer::getScreenHeight()) * 0.0637f) +
                   mSystemName->getFont()->getLetterHeight() +
                   static_cast<float>(Renderer::getScreenHeight()) * 0.04f +
                   mButtonGrid->getSize().y + Font::get(FONT_SIZE_MEDIUM)->getHeight() * 8.0f;

    setSize(width, height);
    setPosition((Renderer::getScreenWidth() - mSize.x) / 2.0f,
                (Renderer::getScreenHeight() - mSize.y) / 2.0f);

    mGrid.resetCursor();
    mSearch->search(params); // Start the search.
}

void GuiScraperSingle::onSizeChanged()
{
    mGrid.setRowHeightPerc(
        0, (mGameName->getFont()->getLetterHeight() + Renderer::getScreenHeight() * 0.0637f) /
               mSize.y / 2.0f);
    mGrid.setRowHeightPerc(
        1, (mGameName->getFont()->getLetterHeight() + Renderer::getScreenHeight() * 0.0637f) /
               mSize.y / 2.0f);
    mGrid.setRowHeightPerc(2, mSystemName->getFont()->getLetterHeight() / mSize.y, false);
    mGrid.setRowHeightPerc(3, 0.04f, false);
    mGrid.setRowHeightPerc(4, (Font::get(FONT_SIZE_MEDIUM)->getHeight() * 8.0f) / mSize.y, false);

    mGrid.setColWidthPerc(1, 0.04f);

    mGrid.setSize(glm::round(mSize));
    mBox.fitTo(mSize, glm::vec3 {}, glm::vec2 {-32.0f, -32.0f});

    // Add some extra margins to the game name.
    const float newSizeX = mSize.x * 0.96f;
    mGameName->setSize(newSizeX, mGameName->getSize().y);
    mGameName->setPosition((mSize.x - newSizeX) / 2.0f, 0.0f);
}

bool GuiScraperSingle::input(InputConfig* config, Input input)
{
    if (config->isMappedTo("b", input) && input.value) {
        if (mSearch->getSavedNewMedia()) {
            // If the user aborted the scraping but there was still some media downloaded,
            // then flag to GuiMetaDataEd that the image and marquee textures need to be
            // manually unloaded and that the gamelist needs to be reloaded. Otherwise the
            // images would not get updated until the user scrolls up and down the gamelist.
            mSavedMediaAndAborted = true;
        }
        delete this;
        return true;
    }

    return GuiComponent::input(config, input);
}

void GuiScraperSingle::update(int deltaTime)
{
    GuiComponent::update(deltaTime);

    if (mClose)
        delete this;
}

std::vector<HelpPrompt> GuiScraperSingle::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts = mGrid.getHelpPrompts();
    prompts.push_back(HelpPrompt("b", "back (cancel)"));

    return prompts;
}

HelpStyle GuiScraperSingle::getHelpStyle()
{
    HelpStyle style = HelpStyle();
    style.applyTheme(ViewController::getInstance()->getState().getSystem()->getTheme(), "system");
    return style;
}

void GuiScraperSingle::close()
{
    // This will cause update() to close the GUI.
    mClose = true;
}
