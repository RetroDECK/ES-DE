//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
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
#include "utils/LocalizationUtil.h"

GuiScraperSingle::GuiScraperSingle(ScraperSearchParams& params,
                                   std::function<void(const ScraperSearchResult&)> doneFunc,
                                   bool& savedMediaAndAborted)
    : mClose {false}
    , mRenderer {Renderer::getInstance()}
    , mBackground {":/graphics/frame.svg"}
    , mGrid {glm::ivec2 {2, 6}}
    , mSearchParams {params}
    , mSavedMediaAndAborted {savedMediaAndAborted}
{
    addChild(&mBackground);
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
        scrapeName +
            ((mSearchParams.game->getType() == FOLDER) ? "  " + ViewController::FOLDER_CHAR : ""),
        Font::get(FONT_SIZE_LARGE * Utils::Localization::sMenuTitleScaleFactor), mMenuColorPrimary,
        ALIGN_CENTER);
    mGameName->setColor(mMenuColorTitle);
    mGrid.setEntry(mGameName, glm::ivec2 {0, 0}, false, true, glm::ivec2 {2, 2});

    mSystemName = std::make_shared<TextComponent>(
        Utils::String::toUpper(mSearchParams.system->getFullName()), Font::get(FONT_SIZE_SMALL),
        mMenuColorSecondary, ALIGN_CENTER);
    mGrid.setEntry(mSystemName, glm::ivec2 {0, 2}, false, true, glm::ivec2 {2, 1});

    // Row 3 is a spacer.

    // GuiScraperSearch.
    mSearch = std::make_shared<GuiScraperSearch>(GuiScraperSearch::MANUAL_MODE, 1, 8);
    mGrid.setEntry(mSearch, glm::ivec2 {0, 4}, true, true, glm::ivec2 {2, 1});

    mResultList = mSearch->getResultList();

    // Set up scroll indicators.
    mScrollUp = std::make_shared<ImageComponent>();
    mScrollDown = std::make_shared<ImageComponent>();

    mScrollUp->setResize(0.0f, mGameName->getFont()->getLetterHeight() / 2.0f);
    mScrollUp->setOrigin(0.0f, -0.35f);

    mScrollDown->setResize(0.0f, mGameName->getFont()->getLetterHeight() / 2.0f);
    mScrollDown->setOrigin(0.0f, 0.35f);

    mScrollIndicator =
        std::make_shared<ScrollIndicatorComponent>(mResultList, mScrollUp, mScrollDown);

    mGrid.setEntry(mScrollUp, glm::ivec2 {1, 0}, false, false, glm::ivec2 {1, 1});
    mGrid.setEntry(mScrollDown, glm::ivec2 {1, 1}, false, false, glm::ivec2 {1, 1});

    // Buttons
    std::vector<std::shared_ptr<ButtonComponent>> buttons;

    buttons.push_back(
        std::make_shared<ButtonComponent>(_("REFINE SEARCH"), _("refine search"), [&] {
            // Refine the search, unless the result has already been accepted.
            if (!mSearch->getAcceptedResult()) {
                // Copy any search refine that may have been previously entered by opening
                // the input screen using the "Y" button shortcut.
                mSearchParams.nameOverride = mSearch->getNameOverride();
                mSearch->openInputScreen(mSearchParams);
                mGrid.resetCursor();
            }
        }));
    buttons.push_back(std::make_shared<ButtonComponent>(_("CANCEL"), _("cancel"), [&] {
        if (mSearch->getSavedNewMedia()) {
            // If the user aborted the scraping but there was still some media downloaded,
            // then flag to GuiMetaDataEd that the image and marquee textures need to be
            // manually unloaded and that the gamelist needs to be reloaded. Otherwise the
            // images would not get updated until the user scrolls up and down the gamelist.
            mSavedMediaAndAborted = true;
        }
        delete this;
    }));
    mButtonGrid = MenuComponent::makeButtonGrid(buttons);

    mGrid.setEntry(mButtonGrid, glm::ivec2 {0, 5}, true, false, glm::ivec2 {2, 1});

    mSearch->setAcceptCallback([this, doneFunc](const ScraperSearchResult& result) {
        doneFunc(result);
        close();
    });
    mSearch->setCancelCallback([&] { delete this; });
    mSearch->setRefineCallback([&] {
        mScrollUp->setOpacity(0.0f);
        mScrollDown->setOpacity(0.0f);
        mResultList->resetScrollIndicatorStatus();
    });

    // Limit the width of the GUI on ultrawide monitors. The 1.778 aspect ratio value is
    // the 16:9 reference.
    const float aspectValue {1.778f / Renderer::getScreenAspectRatio()};
    const float width {glm::clamp(0.95f * aspectValue, 0.70f, 0.95f) * mRenderer->getScreenWidth()};
    const float screenSize {mRenderer->getIsVerticalOrientation() ? mRenderer->getScreenWidth() :
                                                                    mRenderer->getScreenHeight()};

    const float height {(mGameName->getFont()->getLetterHeight() + screenSize * 0.0637f) +
                        mSystemName->getFont()->getLetterHeight() + screenSize * 0.04f +
                        mButtonGrid->getSize().y + Font::get(FONT_SIZE_MEDIUM)->getHeight() * 8.0f};

    setSize(width, height);
    setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f,
                (mRenderer->getScreenHeight() - mSize.y) / 2.0f);

    mGrid.resetCursor();
    mSearch->search(params); // Start the search.
}

void GuiScraperSingle::onSizeChanged()
{
    const float gameNameHeight {mRenderer->getIsVerticalOrientation() ?
                                    mRenderer->getScreenWidth() * 0.0637f :
                                    mRenderer->getScreenHeight() * 0.0637f};

    mGrid.setRowHeightPerc(0, (mGameName->getFont()->getLetterHeight() + gameNameHeight) / mSize.y /
                                  2.0f);
    mGrid.setRowHeightPerc(1, (mGameName->getFont()->getLetterHeight() + gameNameHeight) / mSize.y /
                                  2.0f);
    mGrid.setRowHeightPerc(2, mSystemName->getFont()->getLetterHeight() / mSize.y, false);
    mGrid.setRowHeightPerc(3, 0.04f, false);
    mGrid.setRowHeightPerc(4, (Font::get(FONT_SIZE_MEDIUM)->getHeight() * 8.0f) / mSize.y, false);

    if (mRenderer->getIsVerticalOrientation())
        mGrid.setColWidthPerc(1, 0.05f);
    else
        mGrid.setColWidthPerc(1, 0.04f);

    mGrid.setSize(mSize);
    mBackground.fitTo(mSize);

    // Add some extra margins to the game name.
    const float newSizeX {mSize.x * 0.96f};
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
    std::vector<HelpPrompt> prompts {mGrid.getHelpPrompts()};
    prompts.push_back(HelpPrompt("b", _("back (cancel)")));

    return prompts;
}

void GuiScraperSingle::close()
{
    // This will cause update() to close the GUI.
    mClose = true;
}
