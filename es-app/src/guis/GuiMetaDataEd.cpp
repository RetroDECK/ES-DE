//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiMetaDataEd.cpp
//
//  Game metadata edit user interface.
//  This interface is triggered from the GuiGamelistOptions menu.
//  The scraping interface is handled by GuiGameScraper which calls GuiScraperSearch.
//

#include "guis/GuiMetaDataEd.h"

#include "CollectionSystemsManager.h"
#include "FileData.h"
#include "FileFilterIndex.h"
#include "Gamelist.h"
#include "SystemData.h"
#include "Window.h"
#include "components/ButtonComponent.h"
#include "components/ComponentList.h"
#include "components/DateTimeEditComponent.h"
#include "components/MenuComponent.h"
#include "components/RatingComponent.h"
#include "components/SwitchComponent.h"
#include "components/TextComponent.h"
#include "guis/GuiGameScraper.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiTextEditKeyboardPopup.h"
#include "guis/GuiTextEditPopup.h"
#include "resources/Font.h"
#include "utils/StringUtil.h"
#include "views/ViewController.h"

#define TITLE_HEIGHT (mTitle->getFont()->getLetterHeight() + Renderer::getScreenHeight() * 0.060f)

GuiMetaDataEd::GuiMetaDataEd(Window* window,
                             MetaDataList* md,
                             const std::vector<MetaDataDecl>& mdd,
                             ScraperSearchParams scraperParams,
                             const std::string& /*header*/,
                             std::function<void()> saveCallback,
                             std::function<void()> clearGameFunc,
                             std::function<void()> deleteGameFunc)
    : GuiComponent{window}
    , mBackground{window, ":/graphics/frame.svg"}
    , mGrid{window, glm::ivec2{3, 6}}
    , mScraperParams{scraperParams}
    , mMetaDataDecl{mdd}
    , mMetaData{md}
    , mSavedCallback{saveCallback}
    , mClearGameFunc{clearGameFunc}
    , mDeleteGameFunc{deleteGameFunc}
    , mMediaFilesUpdated{false}
    , mInvalidEmulatorEntry{false}
{
    addChild(&mBackground);
    addChild(&mGrid);

    mTitle = std::make_shared<TextComponent>(mWindow, "EDIT METADATA", Font::get(FONT_SIZE_LARGE),
                                             0x555555FF, ALIGN_CENTER);
    mGrid.setEntry(mTitle, glm::ivec2{0, 0}, false, true, glm::ivec2{3, 2});

    // Extract possible subfolders from the path.
    std::string folderPath =
        Utils::String::replace(Utils::FileSystem::getParent(scraperParams.game->getPath()),
                               scraperParams.system->getSystemEnvData()->mStartPath, "");

    if (folderPath.size() >= 2) {
        folderPath.erase(0, 1);
#if defined(_WIN64)
        folderPath.push_back('\\');
        folderPath = Utils::String::replace(folderPath, "/", "\\");
#else
        folderPath.push_back('/');
#endif
    }

    mSubtitle = std::make_shared<TextComponent>(
        mWindow,
        folderPath + Utils::FileSystem::getFileName(scraperParams.game->getPath()) + " [" +
            Utils::String::toUpper(scraperParams.system->getName()) + "]" +
            (scraperParams.game->getType() == FOLDER ? "  " + ViewController::FOLDER_CHAR : ""),
        Font::get(FONT_SIZE_SMALL), 0x777777FF, ALIGN_CENTER, glm::vec3{}, glm::vec2{}, 0x00000000,
        0.05f);

    mGrid.setEntry(mSubtitle, glm::ivec2{0, 2}, false, true, glm::ivec2{3, 1});

    mList = std::make_shared<ComponentList>(mWindow);
    mGrid.setEntry(mList, glm::ivec2{0, 4}, true, true, glm::ivec2{3, 1});

    // Set up scroll indicators.
    mScrollUp = std::make_shared<ImageComponent>(mWindow);
    mScrollDown = std::make_shared<ImageComponent>(mWindow);
    mScrollIndicator = std::make_shared<ScrollIndicatorComponent>(mList, mScrollUp, mScrollDown);

    mScrollUp->setResize(0.0f, mTitle->getFont()->getLetterHeight() / 2.0f);
    mScrollUp->setOrigin(0.0f, -0.35f);

    mScrollDown->setResize(0.0f, mTitle->getFont()->getLetterHeight() / 2.0f);
    mScrollDown->setOrigin(0.0f, 0.35f);

    mGrid.setEntry(mScrollUp, glm::ivec2{2, 0}, false, false, glm::ivec2{1, 1});
    mGrid.setEntry(mScrollDown, glm::ivec2{2, 1}, false, false, glm::ivec2{1, 1});

    // Populate list.
    for (auto iter = mdd.cbegin(); iter != mdd.cend(); iter++) {
        std::shared_ptr<GuiComponent> ed;
        std::string currentKey = iter->key;
        std::string originalValue = mMetaData->get(iter->key);
        std::string gamePath;

        // Don't add statistics.
        if (iter->isStatistic)
            continue;

        // Don't show the alternative emulator entry if the corresponding option has been disabled.
        if (!Settings::getInstance()->getBool("AlternativeEmulatorPerGame") &&
            iter->type == MD_ALT_EMULATOR) {
            ed = std::make_shared<TextComponent>(
                window, "", Font::get(FONT_SIZE_SMALL, FONT_PATH_LIGHT), 0x777777FF, ALIGN_RIGHT);
            assert(ed);
            ed->setValue(mMetaData->get(iter->key));
            mEditors.push_back(ed);
            continue;
        }

        // It's very important to put the element with the help prompt as the last row
        // entry instead of for instance the spacer. That is so because ComponentList
        // always looks for the help prompt at the back of the element stack.
        ComponentListRow row;
        auto lbl =
            std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(iter->displayName),
                                            Font::get(FONT_SIZE_SMALL), 0x777777FF);
        row.addElement(lbl, true); // Label.

        switch (iter->type) {
            case MD_BOOL: {
                ed = std::make_shared<SwitchComponent>(window);
                // Make the switches slightly smaller.
                glm::vec2 switchSize{ed->getSize() * 0.9f};
                ed->setResize(switchSize.x, switchSize.y);
                ed->setOrigin(-0.05f, -0.09f);

                ed->setChangedColor(ICONCOLOR_USERMARKED);
                row.addElement(ed, false, true);
                break;
            }
            case MD_RATING: {
                auto spacer = std::make_shared<GuiComponent>(mWindow);
                spacer->setSize(Renderer::getScreenWidth() * 0.0025f, 0.0f);
                row.addElement(spacer, false);

                ed = std::make_shared<RatingComponent>(window, true);
                ed->setChangedColor(ICONCOLOR_USERMARKED);
                const float height = lbl->getSize().y * 0.71f;
                ed->setSize(0.0f, height);
                row.addElement(ed, false, true);

                // Pass input to the actual RatingComponent instead of the spacer.
                row.input_handler = std::bind(&GuiComponent::input, ed.get(), std::placeholders::_1,
                                              std::placeholders::_2);
                break;
            }
            case MD_DATE: {
                auto spacer = std::make_shared<GuiComponent>(mWindow);
                spacer->setSize(Renderer::getScreenWidth() * 0.0025f, 0.0f);
                row.addElement(spacer, false);

                ed = std::make_shared<DateTimeEditComponent>(window, true);
                ed->setOriginalColor(DEFAULT_TEXTCOLOR);
                ed->setChangedColor(TEXTCOLOR_USERMARKED);
                row.addElement(ed, false);

                // Pass input to the actual DateTimeEditComponent instead of the spacer.
                row.input_handler = std::bind(&GuiComponent::input, ed.get(), std::placeholders::_1,
                                              std::placeholders::_2);
                break;
            }
            case MD_ALT_EMULATOR: {
                mInvalidEmulatorEntry = false;

                ed = std::make_shared<TextComponent>(window, "",
                                                     Font::get(FONT_SIZE_SMALL, FONT_PATH_LIGHT),
                                                     0x777777FF, ALIGN_RIGHT);
                row.addElement(ed, true);

                auto spacer = std::make_shared<GuiComponent>(mWindow);
                spacer->setSize(Renderer::getScreenWidth() * 0.005f, 0.0f);
                row.addElement(spacer, false);

                auto bracket = std::make_shared<ImageComponent>(mWindow);
                bracket->setImage(":/graphics/arrow.svg");
                bracket->setResize(glm::vec2{0.0f, lbl->getFont()->getLetterHeight()});
                row.addElement(bracket, false);

                const std::string title = iter->displayPrompt;

                // OK callback (apply new value to ed).
                auto updateVal = [this, ed, originalValue](const std::string& newVal) {
                    ed->setValue(newVal);
                    if (newVal == originalValue) {
                        ed->setColor(DEFAULT_TEXTCOLOR);
                    }
                    else {
                        ed->setColor(TEXTCOLOR_USERMARKED);
                        mInvalidEmulatorEntry = false;
                    }
                };

                if (originalValue != "" &&
                    scraperParams.system->getLaunchCommandFromLabel(originalValue) == "") {
                    LOG(LogWarning)
                        << "GuiMetaDataEd: Invalid alternative emulator \"" << originalValue
                        << "\" configured for game \"" << mScraperParams.game->getName() << "\"";
                    mInvalidEmulatorEntry = true;
                }

                if (scraperParams.system->getSystemEnvData()->mLaunchCommands.size() == 1) {
                    lbl->setOpacity(DISABLED_OPACITY);
                    bracket->setOpacity(DISABLED_OPACITY);
                }

                if (mInvalidEmulatorEntry ||
                    scraperParams.system->getSystemEnvData()->mLaunchCommands.size() > 1) {
                    row.makeAcceptInputHandler([this, title, scraperParams, ed, updateVal,
                                                originalValue] {
                        GuiSettings* s = nullptr;

                        bool singleEntry =
                            scraperParams.system->getSystemEnvData()->mLaunchCommands.size() == 1;

                        if (mInvalidEmulatorEntry && singleEntry)
                            s = new GuiSettings(mWindow, "CLEAR INVALID ENTRY");
                        else
                            s = new GuiSettings(mWindow, title);

                        if (!mInvalidEmulatorEntry && ed->getValue() == "" && singleEntry)
                            return;

                        std::vector<std::pair<std::string, std::string>> launchCommands =
                            scraperParams.system->getSystemEnvData()->mLaunchCommands;

                        if (ed->getValue() != "" && mInvalidEmulatorEntry && singleEntry)
                            launchCommands.push_back(std::make_pair(
                                "", ViewController::EXCLAMATION_CHAR + " " + originalValue));
                        else if (ed->getValue() != "")
                            launchCommands.push_back(std::make_pair(
                                "", ViewController::CROSSEDCIRCLE_CHAR + " CLEAR ENTRY"));

                        for (auto entry : launchCommands) {
                            if (mInvalidEmulatorEntry && singleEntry &&
                                entry.second !=
                                    ViewController::EXCLAMATION_CHAR + " " + originalValue)
                                continue;

                            std::string selectedLabel = ed->getValue();
                            std::string label;
                            ComponentListRow row;

                            if (entry.second == "")
                                continue;
                            else
                                label = entry.second;

                            std::shared_ptr<TextComponent> labelText =
                                std::make_shared<TextComponent>(
                                    mWindow, label, Font::get(FONT_SIZE_MEDIUM), 0x777777FF);

                            if (scraperParams.system->getAlternativeEmulator() == "" &&
                                scraperParams.system->getSystemEnvData()
                                        ->mLaunchCommands.front()
                                        .second == label)
                                labelText->setValue(labelText->getValue().append(" [SYSTEM-WIDE]"));

                            if (scraperParams.system->getAlternativeEmulator() == label)
                                labelText->setValue(labelText->getValue().append(" [SYSTEM-WIDE]"));

                            row.addElement(labelText, true);
                            row.makeAcceptInputHandler(
                                [this, s, updateVal, entry, selectedLabel, launchCommands] {
                                    if (entry.second == launchCommands.back().second &&
                                        launchCommands.back().first == "") {
                                        updateVal("");
                                    }
                                    else if (entry.second != selectedLabel) {
                                        updateVal(entry.second);
                                    }
                                    mInvalidEmulatorEntry = false;
                                    delete s;
                                });

                            // This transparent bracket is only added to generate the correct help
                            // prompts.
                            auto bracket = std::make_shared<ImageComponent>(mWindow);
                            bracket->setImage(":/graphics/arrow.svg");
                            bracket->setOpacity(0);
                            bracket->setSize(bracket->getSize() / 3.0f);
                            row.addElement(bracket, false);

                            // Select the row that corresponds to the selected label.
                            if (selectedLabel == label)
                                s->addRow(row, true);
                            else
                                s->addRow(row, false);
                        }

                        // Adjust the width depending on the aspect ratio of the screen, to make the
                        // screen look somewhat coherent regardless of screen type. The 1.778 aspect
                        // ratio value is the 16:9 reference.
                        float aspectValue = 1.778f / Renderer::getScreenAspectRatio();

                        float maxWidthModifier = glm::clamp(0.64f * aspectValue, 0.42f, 0.92f);
                        float maxWidth =
                            static_cast<float>(Renderer::getScreenWidth()) * maxWidthModifier;

                        s->setMenuSize(glm::vec2{maxWidth, s->getMenuSize().y});

                        auto menuSize = s->getMenuSize();
                        auto menuPos = s->getMenuPosition();

                        s->setMenuPosition(glm::vec3{(s->getSize().x - menuSize.x) / 2.0f,
                                                     (s->getSize().y - menuSize.y) / 3.0f,
                                                     menuPos.z});
                        mWindow->pushGui(s);
                    });
                }
                else {
                    lbl->setOpacity(DISABLED_OPACITY);
                    bracket->setOpacity(DISABLED_OPACITY);
                }
                break;
            }
            case MD_MULTILINE_STRING:
            default: {
                // MD_STRING.
                ed = std::make_shared<TextComponent>(window, "",
                                                     Font::get(FONT_SIZE_SMALL, FONT_PATH_LIGHT),
                                                     0x777777FF, ALIGN_RIGHT);
                row.addElement(ed, true);

                auto spacer = std::make_shared<GuiComponent>(mWindow);
                spacer->setSize(Renderer::getScreenWidth() * 0.005f, 0.0f);
                row.addElement(spacer, false);

                auto bracket = std::make_shared<ImageComponent>(mWindow);
                bracket->setImage(":/graphics/arrow.svg");
                bracket->setResize(glm::vec2{0.0f, lbl->getFont()->getLetterHeight()});
                row.addElement(bracket, false);

                bool multiLine = iter->type == MD_MULTILINE_STRING;
                const std::string title = iter->displayPrompt;

                gamePath = Utils::FileSystem::getStem(mScraperParams.game->getPath());

                // OK callback (apply new value to ed).
                auto updateVal = [ed, currentKey, originalValue,
                                  gamePath](const std::string& newVal) {
                    // If the user has entered a blank game name, then set the name to the ROM
                    // filename (minus the extension).
                    if (currentKey == "name" && newVal == "") {
                        ed->setValue(gamePath);
                        if (gamePath == originalValue)
                            ed->setColor(DEFAULT_TEXTCOLOR);
                        else
                            ed->setColor(TEXTCOLOR_USERMARKED);
                    }
                    else if (newVal == "" &&
                             (currentKey == "developer" || currentKey == "publisher" ||
                              currentKey == "genre" || currentKey == "players")) {
                        ed->setValue("unknown");
                        if (originalValue == "unknown")
                            ed->setColor(DEFAULT_TEXTCOLOR);
                        else
                            ed->setColor(TEXTCOLOR_USERMARKED);
                    }
                    else {
                        ed->setValue(newVal);
                        if (newVal == originalValue)
                            ed->setColor(DEFAULT_TEXTCOLOR);
                        else
                            ed->setColor(TEXTCOLOR_USERMARKED);
                    }
                };

                if (Settings::getInstance()->getBool("VirtualKeyboard")) {
                    row.makeAcceptInputHandler([this, title, ed, updateVal, multiLine] {
                        mWindow->pushGui(new GuiTextEditKeyboardPopup(
                            mWindow, getHelpStyle(), title, ed->getValue(), updateVal, multiLine,
                            "apply", "APPLY CHANGES?", "", ""));
                    });
                }
                else {
                    row.makeAcceptInputHandler([this, title, ed, updateVal, multiLine] {
                        mWindow->pushGui(new GuiTextEditPopup(mWindow, getHelpStyle(), title,
                                                              ed->getValue(), updateVal, multiLine,
                                                              "APPLY", "APPLY CHANGES?"));
                    });
                }
                break;
            }
        }

        assert(ed);
        mList->addRow(row);

        if (iter->type == MD_ALT_EMULATOR && mInvalidEmulatorEntry == true)
            ed->setValue(ViewController::EXCLAMATION_CHAR + " " + originalValue);
        else
            ed->setValue(mMetaData->get(iter->key));

        mEditors.push_back(ed);
    }

    std::vector<std::shared_ptr<ButtonComponent>> buttons;

    if (!scraperParams.system->hasPlatformId(PlatformIds::PLATFORM_IGNORE))
        buttons.push_back(std::make_shared<ButtonComponent>(
            mWindow, "SCRAPE", "scrape", std::bind(&GuiMetaDataEd::fetch, this)));

    buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "SAVE", "save metadata", [&] {
        save();
        ViewController::get()->onPauseVideo();
        delete this;
    }));
    buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "CANCEL", "cancel changes",
                                                        [&] { delete this; }));
    if (scraperParams.game->getType() == FOLDER) {
        if (mClearGameFunc) {
            auto clearSelf = [&] {
                mClearGameFunc();
                delete this;
            };
            auto clearSelfBtnFunc = [this, clearSelf] {
                mWindow->pushGui(new GuiMsgBox(mWindow, getHelpStyle(),
                                               "THIS WILL DELETE ANY MEDIA FILES AND\n"
                                               "THE GAMELIST.XML ENTRY FOR THIS FOLDER,\n"
                                               "BUT NEITHER THE FOLDER ITSELF OR ANY\n"
                                               "CONTENT INSIDE IT WILL BE REMOVED\n"
                                               "ARE YOU SURE?",
                                               "YES", clearSelf, "NO", nullptr));
            };
            buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "CLEAR", "clear folder",
                                                                clearSelfBtnFunc));
        }
    }
    else {
        if (mClearGameFunc) {
            auto clearSelf = [&] {
                mClearGameFunc();
                delete this;
            };
            auto clearSelfBtnFunc = [this, clearSelf] {
                mWindow->pushGui(new GuiMsgBox(mWindow, getHelpStyle(),
                                               "THIS WILL DELETE ANY MEDIA FILES\n"
                                               "AND THE GAMELIST.XML ENTRY FOR\n"
                                               "THIS GAME, BUT THE GAME FILE\n"
                                               "ITSELF WILL NOT BE REMOVED\n"
                                               "ARE YOU SURE?",
                                               "YES", clearSelf, "NO", nullptr));
            };
            buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "CLEAR", "clear file",
                                                                clearSelfBtnFunc));
        }

        if (mDeleteGameFunc) {
            auto deleteFilesAndSelf = [&] {
                mDeleteGameFunc();
                delete this;
            };
            auto deleteGameBtnFunc = [this, deleteFilesAndSelf] {
                mWindow->pushGui(new GuiMsgBox(mWindow, getHelpStyle(),
                                               "THIS WILL DELETE THE GAME\n"
                                               "FILE, ANY MEDIA FILES AND\n"
                                               "THE GAMELIST.XML ENTRY\n"
                                               "ARE YOU SURE?",
                                               "YES", deleteFilesAndSelf, "NO", nullptr));
            };
            buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "DELETE", "delete game",
                                                                deleteGameBtnFunc));
        }
    }

    mButtons = makeButtonGrid(mWindow, buttons);
    mGrid.setEntry(mButtons, glm::ivec2{0, 5}, true, false, glm::ivec2{3, 1});

    // Resize + center.
    float width =
        static_cast<float>(std::min(static_cast<int>(Renderer::getScreenHeight() * 1.05f),
                                    static_cast<int>(Renderer::getScreenWidth() * 0.90f)));

    // Set height explicitly to ten rows for the component list.
    float height = mList->getRowHeight(0) * 10.0f + mTitle->getSize().y + mSubtitle->getSize().y +
                   mButtons->getSize().y;

    setSize(width, height);
}

void GuiMetaDataEd::onSizeChanged()
{
    const float titleSubtitleSpacing = mSize.y * 0.03f;

    mGrid.setRowHeightPerc(0, TITLE_HEIGHT / mSize.y / 2.0f);
    mGrid.setRowHeightPerc(1, TITLE_HEIGHT / mSize.y / 2.0f);
    mGrid.setRowHeightPerc(2, titleSubtitleSpacing / mSize.y);
    mGrid.setRowHeightPerc(3, (titleSubtitleSpacing * 1.2f) / mSize.y);
    mGrid.setRowHeightPerc(4, ((mList->getRowHeight(0) * 10.0f) + 2.0f) / mSize.y);

    mGrid.setColWidthPerc(0, 0.07f);
    mGrid.setColWidthPerc(2, 0.07f);

    mGrid.setSize(mSize);
    mBackground.fitTo(mSize, glm::vec3{}, glm::vec2{-32.0f, -32.0f});

    setPosition((Renderer::getScreenWidth() - mSize.x) / 2.0f,
                (Renderer::getScreenHeight() - mSize.y) / 2.0f);
}

void GuiMetaDataEd::save()
{
    // Remove game from index.
    mScraperParams.system->getIndex()->removeFromIndex(mScraperParams.game);

    // We need this to handle the special situation where the user sets a game to hidden while
    // ShowHiddenGames is set to false, meaning it will immediately disappear from the gamelist.
    bool showHiddenGames = Settings::getInstance()->getBool("ShowHiddenGames");
    bool hideGameWhileHidden = false;
    bool setGameAsCounted = false;

    for (unsigned int i = 0; i < mEditors.size(); i++) {
        if (mMetaDataDecl.at(i).isStatistic)
            continue;

        if (mMetaDataDecl.at(i).key == "altemulator" && mInvalidEmulatorEntry == true)
            continue;

        if (!showHiddenGames && mMetaDataDecl.at(i).key == "hidden" &&
            mEditors.at(i)->getValue() != mMetaData->get("hidden"))
            hideGameWhileHidden = true;

        // Check whether the flag to count the entry as a game was set to enabled.
        if (mMetaDataDecl.at(i).key == "nogamecount" &&
            mEditors.at(i)->getValue() != mMetaData->get("nogamecount") &&
            mMetaData->get("nogamecount") == "true") {
            setGameAsCounted = true;
        }

        mMetaData->set(mMetaDataDecl.at(i).key, mEditors.at(i)->getValue());
    }

    // If hidden games are not shown and the hide flag was set for the entry, then write the
    // metadata immediately regardless of the SaveGamelistsMode setting. Otherwise the file
    // will never be written as the game will be filtered from the gamelist. This solution is
    // not really good as the gamelist will be written twice, but it's a very special and
    // hopefully rare situation.
    if (hideGameWhileHidden)
        updateGamelist(mScraperParams.system);

    // Enter game in index.
    mScraperParams.system->getIndex()->addToIndex(mScraperParams.game);

    // If it's a folder that has been updated, we need to manually sort the gamelist
    // as CollectionSystemsManager ignores folders.
    if (mScraperParams.game->getType() == FOLDER)
        mScraperParams.system->sortSystem(false);

    if (mSavedCallback)
        mSavedCallback();

    if (hideGameWhileHidden) {
        std::vector<FileData*> hideGames;
        // If a folder was hidden there may be children inside that we also need to hide.
        if (mScraperParams.game->getType() == FOLDER) {
            for (FileData* child : mScraperParams.game->getChildrenRecursive()) {
                if (!child->getHidden())
                    child->metadata.set("hidden", "true");
                hideGames.push_back(child);
            }
        }
        else {
            hideGames.push_back(mScraperParams.game);
        }
        for (FileData* hideGame : hideGames) {
            if (hideGame->getType() == GAME) {
                // Update disabled auto collections when hiding a game, as otherwise these could
                // get invalid gamelist cursor positions. A cursor pointing to a removed game
                // would crash the application upon enabling the collections.
                CollectionSystemsManager::get()->refreshCollectionSystems(hideGame, true);
                // Remove the game from the index of all systems.
                for (SystemData* sys : SystemData::sSystemVector) {
                    std::vector<FileData*> children;
                    for (FileData* child : sys->getRootFolder()->getChildrenRecursive())
                        children.push_back(child->getSourceFileData());
                    if (std::find(children.begin(), children.end(), hideGame) != children.end()) {
                        sys->getIndex()->removeFromIndex(hideGame);
                        // Reload the gamelist as well as the view style may need to change.
                        ViewController::get()->reloadGameListView(sys);
                    }
                }
            }
        }
    }
    else {
        // Update all collections where the game is present.
        CollectionSystemsManager::get()->refreshCollectionSystems(mScraperParams.game);
    }

    // If game counting was re-enabled for the game, then reactivate it in any custom collections
    // where it may exist.
    if (setGameAsCounted)
        CollectionSystemsManager::get()->reactivateCustomCollectionEntry(mScraperParams.game);

    mScraperParams.system->onMetaDataSavePoint();

    // If hidden games are not shown and the hide flag was set for the entry, we also need
    // to re-sort the gamelist as we may need to remove the parent folder if all the entries
    // within this folder are now hidden.
    if (hideGameWhileHidden)
        mScraperParams.system->sortSystem(true);

    // Make sure that the cached background is updated to reflect any possible visible
    // changes to the gamelist (e.g. the game name).
    mWindow->invalidateCachedBackground();
}

void GuiMetaDataEd::fetch()
{
    GuiGameScraper* scr = new GuiGameScraper(
        mWindow, mScraperParams, std::bind(&GuiMetaDataEd::fetchDone, this, std::placeholders::_1));
    mWindow->pushGui(scr);
}

void GuiMetaDataEd::fetchDone(const ScraperSearchResult& result)
{
    // Clone the mMetaData object.
    MetaDataList* metadata = nullptr;
    metadata = new MetaDataList(*mMetaData);

    mMediaFilesUpdated = result.savedNewMedia;

    // Check if any values were manually changed before starting the scraping.
    // If so, it's these values we should compare against when scraping, not
    // the values previously saved for the game.
    for (unsigned int i = 0; i < mEditors.size(); i++) {
        const std::string& key = mMetaDataDecl.at(i).key;
        if (metadata->get(key) != mEditors[i]->getValue())
            metadata->set(key, mEditors[i]->getValue());
    }

    GuiScraperSearch::saveMetadata(result, *metadata, mScraperParams.game);

    // Update the list with the scraped metadata values.
    for (unsigned int i = 0; i < mEditors.size(); i++) {
        const std::string& key = mMetaDataDecl.at(i).key;
        if (mEditors.at(i)->getValue() != metadata->get(key)) {
            if (key == "rating")
                mEditors.at(i)->setOriginalColor(ICONCOLOR_SCRAPERMARKED);
            else
                mEditors.at(i)->setColor(TEXTCOLOR_SCRAPERMARKED);
        }
        // Save all the keys that should be scraped.
        if (mMetaDataDecl.at(i).shouldScrape)
            mEditors.at(i)->setValue(metadata->get(key));
    }

    delete metadata;
}

void GuiMetaDataEd::close()
{
    // Find out if the user made any changes.
    bool metadataUpdated = false;
    for (unsigned int i = 0; i < mEditors.size(); i++) {
        const std::string& key = mMetaDataDecl.at(i).key;
        std::string mMetaDataValue = mMetaData->get(key);
        std::string mEditorsValue = mEditors.at(i)->getValue();

        if (key == "altemulator" && mInvalidEmulatorEntry == true)
            continue;

        if (mMetaDataValue != mEditorsValue) {
            metadataUpdated = true;
            break;
        }
    }

    std::function<void()> closeFunc;
    closeFunc = [this] {
        if (mMediaFilesUpdated) {
            // Always reload the gamelist if media files were updated, even if the user
            // chose to not save any metadata changes. Also manually unload the game image
            // and marquee, as otherwise they would not get updated until the user scrolls up
            // and down the gamelist.
            TextureResource::manualUnload(mScraperParams.game->getImagePath(), false);
            TextureResource::manualUnload(mScraperParams.game->getMarqueePath(), false);
            ViewController::get()->reloadGameListView(mScraperParams.system);
            mWindow->invalidateCachedBackground();
        }
        ViewController::get()->onPauseVideo();
        delete this;
    };

    if (metadataUpdated) {
        // Changes were made, ask if the user wants to save them.
        mWindow->pushGui(new GuiMsgBox(
            mWindow, getHelpStyle(), "SAVE CHANGES?", "YES",
            [this, closeFunc] {
                save();
                closeFunc();
            },
            "NO", closeFunc));
    }
    else {
        // Always save if the media files have been changed (i.e. newly scraped images).
        if (mMediaFilesUpdated)
            save();
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

    if (input.value != 0 && (config->isMappedTo("y", input))) {
        fetch();
        return true;
    }

    return false;
}

std::vector<HelpPrompt> GuiMetaDataEd::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts = mGrid.getHelpPrompts();
    prompts.push_back(HelpPrompt("y", "scrape"));
    prompts.push_back(HelpPrompt("b", "back"));
    return prompts;
}

HelpStyle GuiMetaDataEd::getHelpStyle()
{
    HelpStyle style = HelpStyle();
    style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");
    return style;
}
