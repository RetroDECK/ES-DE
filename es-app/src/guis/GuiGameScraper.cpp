//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiGameScraper.cpp
//
//  Single game scraping user interface.
//  This interface is triggered from GuiMetaDataEd.
//  GuiScraperSearch is called from here.
//

#include "guis/GuiGameScraper.h"

#include "FileData.h"
#include "MameNames.h"
#include "SystemData.h"
#include "components/ButtonComponent.h"
#include "components/MenuComponent.h"
#include "components/TextComponent.h"
#include "views/ViewController.h"

GuiGameScraper::GuiGameScraper(Window* window,
                               ScraperSearchParams params,
                               std::function<void(const ScraperSearchResult&)> doneFunc)
    : GuiComponent(window)
    , mGrid(window, Vector2i(1, 7))
    , mBox(window, ":/graphics/frame.svg")
    , mSearchParams(params)
    , mClose(false)
{
    addChild(&mBox);
    addChild(&mGrid);

    // Row 0 is a spacer.

    std::string scrapeName;

    if (Settings::getInstance()->getBool("ScraperSearchMetadataName")) {
        scrapeName = mSearchParams.game->getName();
    }
    else {
        if (params.game->isArcadeGame() &&
            Settings::getInstance()->getString("Scraper") == "thegamesdb")
            scrapeName =
                Utils::FileSystem::getFileName(mSearchParams.game->getPath()) + " (" +
                MameNames::getInstance()->getCleanName(mSearchParams.game->getCleanName()) + ")";
        else
            scrapeName = Utils::FileSystem::getFileName(mSearchParams.game->getPath());
    }

    mGameName = std::make_shared<TextComponent>(
        mWindow,
        scrapeName +
            ((mSearchParams.game->getType() == FOLDER) ? "  " + ViewController::FOLDER_CHAR : ""),
        Font::get(FONT_SIZE_MEDIUM), 0x777777FF, ALIGN_CENTER);
    mGrid.setEntry(mGameName, Vector2i(0, 1), false, true);

    // Row 2 is a spacer.

    mSystemName = std::make_shared<TextComponent>(
        mWindow, Utils::String::toUpper(mSearchParams.system->getFullName()),
        Font::get(FONT_SIZE_SMALL), 0x888888FF, ALIGN_CENTER);
    mGrid.setEntry(mSystemName, Vector2i(0, 3), false, true);

    // Row 4 is a spacer.

    // GuiScraperSearch.
    mSearch = std::make_shared<GuiScraperSearch>(window, GuiScraperSearch::NEVER_AUTO_ACCEPT, 1);
    mGrid.setEntry(mSearch, Vector2i(0, 5), true);

    // Buttons
    std::vector<std::shared_ptr<ButtonComponent>> buttons;

    buttons.push_back(
        std::make_shared<ButtonComponent>(mWindow, "REFINE SEARCH", "refine search", [&] {
            // Refine the search, unless the result has already been accepted.
            if (!mSearch->getAcceptedResult()) {
                mSearch->openInputScreen(mSearchParams);
                mGrid.resetCursor();
            }
        }));
    buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "CANCEL", "cancel", [&] {
        if (mSearch->getSavedNewMedia()) {
            // If the user aborted the scraping but there was still some media downloaded,
            // then force an unload of the textures for the game image and marquee, and make
            // an update of the game entry. Otherwise the images would not get updated until
            // the user scrolls up and down the gamelist.
            TextureResource::manualUnload(mSearchParams.game->getImagePath(), false);
            TextureResource::manualUnload(mSearchParams.game->getMarqueePath(), false);
            ViewController::get()->onFileChanged(mSearchParams.game, true);
        }
        delete this;
    }));
    mButtonGrid = makeButtonGrid(mWindow, buttons);

    mGrid.setEntry(mButtonGrid, Vector2i(0, 6), true, false);

    mSearch->setAcceptCallback([this, doneFunc](const ScraperSearchResult& result) {
        doneFunc(result);
        close();
    });
    mSearch->setCancelCallback([&] { delete this; });

    // Limit the width of the GUI on ultrawide monitors. The 1.778 aspect ratio value is
    // the 16:9 reference.
    float aspectValue = 1.778f / Renderer::getScreenAspectRatio();
    float width = Math::clamp(0.95f * aspectValue, 0.70f, 0.95f) * Renderer::getScreenWidth();

    setSize(width, Renderer::getScreenHeight() * 0.747f);
    setPosition((Renderer::getScreenWidth() - mSize.x()) / 2.0f,
                (Renderer::getScreenHeight() - mSize.y()) / 2.0f);

    mGrid.resetCursor();
    mSearch->search(params); // Start the search.
}

void GuiGameScraper::onSizeChanged()
{
    mBox.fitTo(mSize, {}, Vector2f(-32.0f, -32.0f));

    mGrid.setRowHeightPerc(0, 0.04f, false);
    mGrid.setRowHeightPerc(1, mGameName->getFont()->getLetterHeight() / mSize.y(),
                           false); // Game name.
    mGrid.setRowHeightPerc(2, 0.04f, false);
    mGrid.setRowHeightPerc(3, mSystemName->getFont()->getLetterHeight() / mSize.y(),
                           false); // System name.
    mGrid.setRowHeightPerc(4, 0.04f, false);
    mGrid.setRowHeightPerc(6, mButtonGrid->getSize().y() / mSize.y(), false); // Buttons.
    mGrid.setSize(mSize);
}

bool GuiGameScraper::input(InputConfig* config, Input input)
{
    if (config->isMappedTo("b", input) && input.value) {
        if (mSearch->getSavedNewMedia()) {
            // If the user aborted the scraping but there was still some media downloaded,
            // then force an unload of the textures for the game image and marquee, and make
            // an update of the game entry. Otherwise the images would not get updated until
            // the user scrolls up and down the gamelist.
            TextureResource::manualUnload(mSearchParams.game->getImagePath(), false);
            TextureResource::manualUnload(mSearchParams.game->getMarqueePath(), false);
            ViewController::get()->onFileChanged(mSearchParams.game, true);
        }
        delete this;
        return true;
    }

    return GuiComponent::input(config, input);
}

void GuiGameScraper::update(int deltaTime)
{
    GuiComponent::update(deltaTime);

    if (mClose)
        delete this;
}

std::vector<HelpPrompt> GuiGameScraper::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts = mGrid.getHelpPrompts();
    prompts.push_back(HelpPrompt("b", "back (cancel)"));

    return prompts;
}

HelpStyle GuiGameScraper::getHelpStyle()
{
    HelpStyle style = HelpStyle();
    style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");
    return style;
}

void GuiGameScraper::close()
{
    // This will cause update() to close the GUI.
    mClose = true;
}
