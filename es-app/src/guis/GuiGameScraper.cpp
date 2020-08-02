//
//  GuiGameScraper.cpp
//
//  Single game scraping user interface.
//  This interface is triggered from GuiMetaDataEd.
//  GuiScraperSearch is called from here.
//

#include "guis/GuiGameScraper.h"

#include "components/ButtonComponent.h"
#include "components/MenuComponent.h"
#include "components/TextComponent.h"
#include "views/ViewController.h"
#include "FileData.h"
#include "PowerSaver.h"
#include "SystemData.h"

GuiGameScraper::GuiGameScraper(
        Window* window,
        ScraperSearchParams params,
        std::function<void(const ScraperSearchResult&)> doneFunc)
        : GuiComponent(window),
        mGrid(window, Vector2i(1, 7)),
        mBox(window, ":/graphics/frame.png"),
        mSearchParams(params),
        mClose(false)
{
    PowerSaver::pause();
    addChild(&mBox);
    addChild(&mGrid);

    // Row 0 is a spacer.

    mGameName = std::make_shared<TextComponent>(mWindow,
            Utils::FileSystem::getFileName(mSearchParams.game->getPath()),
            Font::get(FONT_SIZE_MEDIUM), 0x777777FF, ALIGN_CENTER);
    mGrid.setEntry(mGameName, Vector2i(0, 1), false, true);

    // Row 2 is a spacer.

    mSystemName = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(
            mSearchParams.system->getFullName()), Font::get(FONT_SIZE_SMALL),
            0x888888FF, ALIGN_CENTER);
    mGrid.setEntry(mSystemName, Vector2i(0, 3), false, true);

    // Row 4 is a spacer.

    // GuiScraperSearch.
    mSearch = std::make_shared<GuiScraperSearch>(window,
            GuiScraperSearch::NEVER_AUTO_ACCEPT, 1);
    mGrid.setEntry(mSearch, Vector2i(0, 5), true);

    // Buttons
    std::vector< std::shared_ptr<ButtonComponent> > buttons;

    buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "REFINE SEARCH",
            "refine search", [&] {
        mSearch->openInputScreen(mSearchParams);
        mGrid.resetCursor();
    }));
    buttons.push_back(std::make_shared<ButtonComponent>(
            mWindow, "CANCEL", "cancel", [&] { delete this; }));
    mButtonGrid = makeButtonGrid(mWindow, buttons);

    mGrid.setEntry(mButtonGrid, Vector2i(0, 6), true, false);

    // We call this->close() instead of just 'delete this' in the accept callback.
    // This is because of how GuiComponent::update works. If it was just 'delete this',
    // the following would happen when the metadata resolver is done:
    //     GuiGameScraper::update()
    //       GuiComponent::update()
    //         it = mChildren.cbegin();
    //         mBox::update()
    //         it++;
    //         mSearchComponent::update()
    //           acceptCallback -> delete this
    //         it++; // Error, mChildren has been deleted because it was part of 'this'.

    // So instead we do this:
    //     GuiGameScraper::update()
    //       GuiComponent::update()
    //         it = mChildren.cbegin();
    //         mBox::update()
    //         it++;
    //         mSearchComponent::update()
    //           acceptCallback -> close() -> mClose = true
    //         it++; // OK.
    //       if (mClose)
    //         delete this;
    mSearch->setAcceptCallback([this, doneFunc](const ScraperSearchResult& result) {
            doneFunc(result); close(); });
    mSearch->setCancelCallback([&] { delete this; });

    setSize(Renderer::getScreenWidth() * 0.95f, Renderer::getScreenHeight() * 0.747f);
    setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, (Renderer::getScreenHeight() -
            mSize.y()) / 2);

    mGrid.resetCursor();
    mSearch->search(params); // Start the search.
}

void GuiGameScraper::onSizeChanged()
{
    mBox.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

    mGrid.setRowHeightPerc(0, 0.04f, false);
    mGrid.setRowHeightPerc(1, mGameName->getFont()->getLetterHeight() /
            mSize.y(), false); // Game name.
    mGrid.setRowHeightPerc(2, 0.04f, false);
    mGrid.setRowHeightPerc(3, mSystemName->getFont()->getLetterHeight() /
            mSize.y(), false); // System name.
    mGrid.setRowHeightPerc(4, 0.04f, false);
    mGrid.setRowHeightPerc(6, mButtonGrid->getSize().y() / mSize.y(), false); // Buttons.
    mGrid.setSize(mSize);
}

bool GuiGameScraper::input(InputConfig* config, Input input)
{
    if (config->isMappedTo("b", input) && input.value) {
        PowerSaver::resume();
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
    return mGrid.getHelpPrompts();
}

HelpStyle GuiGameScraper::getHelpStyle()
{
    HelpStyle style = HelpStyle();
    style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");
    return style;
}

void GuiGameScraper::close()
{
    mClose = true;
}
