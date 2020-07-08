//
//  GuiMetaDataEd.cpp
//
//  Game metadata edit user interface.
//  This interface is triggered from the GuiGamelistOptions menu.
//  The scraping interface is handled by GuiGameScraper which calls
//  GuiScraperSearch.
//

#include "guis/GuiMetaDataEd.h"

#include "components/ButtonComponent.h"
#include "components/ComponentList.h"
#include "components/DateTimeEditComponent.h"
#include "components/MenuComponent.h"
#include "components/RatingComponent.h"
#include "components/SwitchComponent.h"
#include "components/TextComponent.h"
#include "guis/GuiGameScraper.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiComplexTextEditPopup.h"
#include "resources/Font.h"
#include "utils/StringUtil.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "FileData.h"
#include "FileFilterIndex.h"
#include "SystemData.h"
#include "Window.h"

GuiMetaDataEd::GuiMetaDataEd(
        Window* window,
        MetaDataList* md,
        const std::vector<MetaDataDecl>& mdd,
        ScraperSearchParams scraperParams,
        const std::string& /*header*/,
        std::function<void()> saveCallback,
        std::function<void()> deleteFunc)
        : GuiComponent(window),
        mScraperParams(scraperParams),

        mBackground(window, ":/graphics/frame.png"),
        mGrid(window, Vector2i(1, 3)),

        mMetaDataDecl(mdd),
        mMetaData(md),
        mSavedCallback(saveCallback),
        mDeleteFunc(deleteFunc),
        mMetadataUpdated(false)
{
    addChild(&mBackground);
    addChild(&mGrid);

    mHeaderGrid = std::make_shared<ComponentGrid>(mWindow, Vector2i(1, 5));

    mTitle = std::make_shared<TextComponent>(mWindow, "EDIT METADATA",
            Font::get(FONT_SIZE_LARGE), 0x555555FF, ALIGN_CENTER);
    mSubtitle = std::make_shared<TextComponent>(mWindow,
            Utils::String::toUpper(Utils::FileSystem::getFileName(scraperParams.game->
            getPath())), Font::get(FONT_SIZE_SMALL), 0x777777FF, ALIGN_CENTER);
    mHeaderGrid->setEntry(mTitle, Vector2i(0, 1), false, true);
    mHeaderGrid->setEntry(mSubtitle, Vector2i(0, 3), false, true);

    mGrid.setEntry(mHeaderGrid, Vector2i(0, 0), false, true);

    mList = std::make_shared<ComponentList>(mWindow);
    mGrid.setEntry(mList, Vector2i(0, 1), true, true);

    // Populate list.
    for (auto iter = mdd.cbegin(); iter != mdd.cend(); iter++) {
        std::shared_ptr<GuiComponent> ed;

        // Don't add statistics.
        if (iter->isStatistic)
            continue;

        // Don't show the launch string override entry if this option has been disabled.
        if (!Settings::getInstance()->getBool("LaunchCommandOverride") &&
                iter->type == MD_LAUNCHCOMMAND) {
            ed = std::make_shared<TextComponent>(window, "", Font::get(FONT_SIZE_SMALL,
                    FONT_PATH_LIGHT), 0x777777FF, ALIGN_RIGHT);
            assert(ed);
            ed->setValue(mMetaData->get(iter->key));
            mEditors.push_back(ed);
            continue;
        }

        // Create ed and add it (and any related components) to mMenu.
        // ed's value will be set below.
        // It's very important to put the element with the help prompt as the last row
        // entry instead of for instance the spacer. That is so because ComponentList
        // always looks for the help prompt at the back of the element stack.
        ComponentListRow row;
        auto lbl = std::make_shared<TextComponent>(mWindow,
                Utils::String::toUpper(iter->displayName), Font::get(FONT_SIZE_SMALL), 0x777777FF);
        row.addElement(lbl, true); // Label.

        switch (iter->type) {
        case MD_BOOL: {
                ed = std::make_shared<SwitchComponent>(window);
                row.addElement(ed, false, true);
                break;
            }
        case MD_RATING: {
                auto spacer = std::make_shared<GuiComponent>(mWindow);
                spacer->setSize(Renderer::getScreenWidth() * 0.0025f, 0);
                row.addElement(spacer, false);

                ed = std::make_shared<RatingComponent>(window);
                const float height = lbl->getSize().y() * 0.71f;
                ed->setSize(0, height);
                row.addElement(ed, false, true);

                // Pass input to the actual RatingComponent instead of the spacer.
                row.input_handler = std::bind(&GuiComponent::input,
                        ed.get(), std::placeholders::_1, std::placeholders::_2);
                break;
            }
        case MD_DATE: {
                auto spacer = std::make_shared<GuiComponent>(mWindow);
                spacer->setSize(Renderer::getScreenWidth() * 0.0025f, 0);
                row.addElement(spacer, false);

                ed = std::make_shared<DateTimeEditComponent>(window);
                row.addElement(ed, false);

                // Pass input to the actual DateTimeEditComponent instead of the spacer.
                row.input_handler = std::bind(&GuiComponent::input, ed.get(),
                        std::placeholders::_1, std::placeholders::_2);
                break;
            }
        case MD_TIME: {
                ed = std::make_shared<DateTimeEditComponent>(window,
                        DateTimeEditComponent::DISP_RELATIVE_TO_NOW);
                row.addElement(ed, false);
                break;
            }
        case MD_LAUNCHCOMMAND: {
                ed = std::make_shared<TextComponent>(window, "",
                        Font::get(FONT_SIZE_SMALL, FONT_PATH_LIGHT), 0x777777FF, ALIGN_RIGHT);
                row.addElement(ed, true);

                auto spacer = std::make_shared<GuiComponent>(mWindow);
                spacer->setSize(Renderer::getScreenWidth() * 0.005f, 0);
                row.addElement(spacer, false);

                auto bracket = std::make_shared<ImageComponent>(mWindow);
                bracket->setImage(":/graphics/arrow.svg");
                bracket->setResize(Vector2f(0, lbl->getFont()->getLetterHeight()));
                row.addElement(bracket, false);

                bool multiLine = false;
                const std::string title = iter->displayPrompt;
                auto updateVal = [ed](const std::string& newVal) {
                        ed->setValue(newVal); }; // OK callback (apply new value to ed).

                std::string staticTextString = "Default value from es_systems.cfg:";
                std::string defaultLaunchCommand = scraperParams.system->
                        getSystemEnvData()->mLaunchCommand;

                row.makeAcceptInputHandler([this, title, staticTextString,
                        defaultLaunchCommand, ed, updateVal, multiLine] {
                            mWindow->pushGui(new GuiComplexTextEditPopup(mWindow, getHelpStyle(),
                            title, staticTextString, defaultLaunchCommand, ed->getValue(),
                            updateVal, multiLine, "APPLY", "APPLY CHANGES?"));
                });
                break;
            }
        case MD_MULTILINE_STRING:
        default: {
                // MD_STRING.
                ed = std::make_shared<TextComponent>(window, "", Font::get(FONT_SIZE_SMALL,
                        FONT_PATH_LIGHT), 0x777777FF, ALIGN_RIGHT);
                row.addElement(ed, true);

                auto spacer = std::make_shared<GuiComponent>(mWindow);
                spacer->setSize(Renderer::getScreenWidth() * 0.005f, 0);
                row.addElement(spacer, false);

                auto bracket = std::make_shared<ImageComponent>(mWindow);
                bracket->setImage(":/graphics/arrow.svg");
                bracket->setResize(Vector2f(0, lbl->getFont()->getLetterHeight()));
                row.addElement(bracket, false);

                bool multiLine = iter->type == MD_MULTILINE_STRING;
                const std::string title = iter->displayPrompt;

                 // OK callback (apply new value to ed).
                auto updateVal = [ed](const std::string& newVal) { ed->setValue(newVal); };
                row.makeAcceptInputHandler([this, title, ed, updateVal, multiLine] {
                    mWindow->pushGui(new GuiTextEditPopup(mWindow, getHelpStyle(), title,
                            ed->getValue(), updateVal, multiLine, "APPLY", "APPLY CHANGES?"));
                });
                break;
            }
        }

        assert(ed);
        mList->addRow(row);
        ed->setValue(mMetaData->get(iter->key));
        mEditors.push_back(ed);
    }

    std::vector< std::shared_ptr<ButtonComponent> > buttons;

    if (mScraperParams.game->getType() != FOLDER) {
        if (!scraperParams.system->hasPlatformId(PlatformIds::PLATFORM_IGNORE))
            buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "SCRAPE", "scrape",
                    std::bind(&GuiMetaDataEd::fetch, this)));
    }

    buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "SAVE", "save metadata",
            [&] { save(); delete this; }));
    buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "CANCEL", "cancel changes",
            [&] { delete this; }));

    if (mDeleteFunc) {
        auto deleteFileAndSelf = [&] { mDeleteFunc(); delete this; };
        auto deleteBtnFunc = [this, deleteFileAndSelf] { mWindow->pushGui(
                new GuiMsgBox(mWindow, getHelpStyle(),
                        "THIS WILL DELETE THE ACTUAL GAME FILE(S)!\nARE YOU SURE?",
                        "YES", deleteFileAndSelf, "NO", nullptr)); };
        buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "DELETE",
                "delete game", deleteBtnFunc));
    }

    mButtons = makeButtonGrid(mWindow, buttons);
    mGrid.setEntry(mButtons, Vector2i(0, 2), true, false);

    // Resize + center.
    float width = (float)Math::min(Renderer::getScreenHeight(),
            (int)(Renderer::getScreenWidth() * 0.90f));
    setSize(width, Renderer::getScreenHeight() * 0.82f);
    setPosition((Renderer::getScreenWidth() - mSize.x()) / 2,
            (Renderer::getScreenHeight() - mSize.y()) / 2);
}

void GuiMetaDataEd::onSizeChanged()
{
    mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

    mGrid.setSize(mSize);

    const float titleHeight = mTitle->getFont()->getLetterHeight();
    const float subtitleHeight = mSubtitle->getFont()->getLetterHeight();
    const float titleSubtitleSpacing = mSize.y() * 0.03f;

    mGrid.setRowHeightPerc(0, (titleHeight + titleSubtitleSpacing + subtitleHeight +
            TITLE_VERT_PADDING) / mSize.y());
    mGrid.setRowHeightPerc(2, mButtons->getSize().y() / mSize.y());

    mHeaderGrid->setRowHeightPerc(1, titleHeight / mHeaderGrid->getSize().y());
    mHeaderGrid->setRowHeightPerc(2, titleSubtitleSpacing / mHeaderGrid->getSize().y());
    mHeaderGrid->setRowHeightPerc(3, subtitleHeight / mHeaderGrid->getSize().y());
}

void GuiMetaDataEd::save()
{
    // Remove game from index.
    mScraperParams.system->getIndex()->removeFromIndex(mScraperParams.game);

    for (unsigned int i = 0; i < mEditors.size(); i++) {
        if (mMetaDataDecl.at(i).isStatistic)
            continue;

        mMetaData->set(mMetaDataDecl.at(i).key, mEditors.at(i)->getValue());
    }

    // Enter game in index.
    mScraperParams.system->getIndex()->addToIndex(mScraperParams.game);

    if (mSavedCallback)
        mSavedCallback();

    // Update respective Collection Entries.
    CollectionSystemManager::get()->refreshCollectionSystems(mScraperParams.game);

    mScraperParams.system->onMetaDataSavePoint();
}

void GuiMetaDataEd::fetch()
{
    GuiGameScraper* scr = new GuiGameScraper(mWindow, mScraperParams,
            std::bind(&GuiMetaDataEd::fetchDone, this, std::placeholders::_1));
    mWindow->pushGui(scr);
}

void GuiMetaDataEd::fetchDone(const ScraperSearchResult& result)
{
    // Clone the mMetaData object.
    MetaDataList* metadata = nullptr;
    metadata = new MetaDataList(*mMetaData);

    // Check if any values were manually changed before starting the scraping.
    // If so, it's these values we should compare against when scraping, not
    // the values previously saved for the game.
    for (unsigned int i = 0; i < mEditors.size(); i++) {
        const std::string& key = mMetaDataDecl.at(i).key;
        if (metadata->get(key) != mEditors[i]->getValue())
            metadata->set(key, mEditors[i]->getValue());
    }

    mMetadataUpdated = GuiScraperSearch::saveMetadata(result, *metadata);

    // Update the list with the scraped metadata values.
    for (unsigned int i = 0; i < mEditors.size(); i++) {
        const std::string& key = mMetaDataDecl.at(i).key;
        if (mEditors.at(i)->getValue() != metadata->get(key)) {
            if (key == "rating") {
                mEditors.at(i)->setColorShift(0xDD2222FF);
            }
            else {
                mEditors.at(i)->setColor(0x994444FF);
            }
        }
        mEditors.at(i)->setValue(metadata->get(key));
    }

    delete metadata;
}

void GuiMetaDataEd::close()
{
    // Find out if the user made any changes.
    bool dirty = mMetadataUpdated;
    for (unsigned int i = 0; i < mEditors.size(); i++) {
        const std::string& key = mMetaDataDecl.at(i).key;
        std::string mMetaDataValue = mMetaData->get(key);
        std::string mEditorsValue = mEditors.at(i)->getValue();

        // Incredibly ugly workaround to avoid the "SAVE CHANGES?" window for games
        // with missing release date metadata.
        if (key == "releasedate" && (mMetaDataValue == "" || mMetaDataValue == "not-a-date-time"))
            mMetaDataValue = "19700101T010000";

        if (mMetaDataValue != mEditorsValue) {
            dirty = true;
            break;
        }
    }

//	Keep code for potential future use.
//	std::function<void()> closeFunc;
//	if (!closeAllWindows) {
//		closeFunc = [this] { delete this; };
//	}
//	else {
//		Window* window = mWindow;
//		closeFunc = [window, this] {
//			while (window->peekGui() != ViewController::get())
//				delete window->peekGui();
//		};
//	}

    std::function<void()> closeFunc;
        closeFunc = [this] { delete this; };

    if (dirty)
    {
        // Changes were made, ask if the user wants to save them.
        mWindow->pushGui(new GuiMsgBox(mWindow, getHelpStyle(),
            "SAVE CHANGES?",
            "YES", [this, closeFunc] { save(); closeFunc(); },
            "NO", closeFunc
        ));
    }
    else {
        closeFunc();
    }
}

bool GuiMetaDataEd::input(InputConfig* config, Input input)
{
    if (GuiComponent::input(config, input))
        return true;

    if (input.value != 0 && (config->isMappedTo("b", input))) {
        close();
        return true;
    }

    return false;
}

std::vector<HelpPrompt> GuiMetaDataEd::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts = mGrid.getHelpPrompts();
    prompts.push_back(HelpPrompt("b", "back"));
    return prompts;
}

HelpStyle GuiMetaDataEd::getHelpStyle()
{
    HelpStyle style = HelpStyle();
    style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");
    return style;
}
