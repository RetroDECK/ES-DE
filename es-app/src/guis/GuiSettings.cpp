//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiSettings.cpp
//
//  User interface template for a settings GUI.
//  The saving of es_settings.xml, the reload of gamelists and some other actions are
//  also triggered to be executed here via flags set by the menu entries' lambda functions.
//

#include "guis/GuiSettings.h"

#include "CollectionSystemsManager.h"
#include "FileFilterIndex.h"
#include "Settings.h"
#include "SystemData.h"
#include "Window.h"
#include "components/HelpComponent.h"
#include "guis/GuiTextEditKeyboardPopup.h"
#include "guis/GuiTextEditPopup.h"
#include "views/GamelistView.h"

#include <SDL2/SDL.h>

GuiSettings::GuiSettings(std::string title)
    : mRenderer {Renderer::getInstance()}
    , mMenu {title}
    , mGoToSystem {nullptr}
    , mNeedsSaving {false}
    , mNeedsCollectionsUpdate {false}
    , mNeedsSorting {false}
    , mNeedsSortingCollections {false}
    , mNeedsResetFilters {false}
    , mNeedsRescanROMDirectory {false}
    , mNeedsReloading {false}
    , mNeedsGoToStart {false}
    , mNeedsGoToSystem {false}
    , mNeedsGoToGroupedCollections {false}
    , mInvalidateCachedBackground {false}
{
    addChild(&mMenu);
    mMenu.addButton(_("BACK"), _("back"), [this] { delete this; });

    setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
    mMenu.setPosition((mSize.x - mMenu.getSize().x) / 2.0f, Renderer::getScreenHeight() * 0.13f);
}

GuiSettings::~GuiSettings()
{
    // Save on exit.
    save();
}

void GuiSettings::save()
{
    if (!mSaveFuncs.size())
        return;

    for (auto it = mSaveFuncs.cbegin(); it != mSaveFuncs.cend(); ++it)
        (*it)();

    if (mNeedsSaving)
        Settings::getInstance()->saveFile();

    if (mNeedsRescanROMDirectory) {
        if (CollectionSystemsManager::getInstance()->isEditing())
            CollectionSystemsManager::getInstance()->exitEditMode();
        mWindow->stopInfoPopup();
        //  Write any gamelist.xml changes before proceeding with the rescan.
        if (Settings::getInstance()->getString("SaveGamelistsMode") == "on exit") {
            for (auto system : SystemData::sSystemVector)
                system->writeMetaData();
        }
        // If a close menu function was passed to us, then run it.
        if (mCloseMenuFunction) {
            mCloseMenuFunction();
            mCloseMenuFunction = nullptr;
        }
        ViewController::getInstance()->rescanROMDirectory();
    }

    if (mNeedsCollectionsUpdate) {
        CollectionSystemsManager::getInstance()->loadEnabledListFromSettings();
        CollectionSystemsManager::getInstance()->updateSystemsList();
    }

    if (mNeedsSorting) {
        for (auto it = SystemData::sSystemVector.cbegin(); it != SystemData::sSystemVector.cend();
             ++it) {
            if (!(!mNeedsSortingCollections && (*it)->isCollection()))
                (*it)->sortSystem(true);

            // Jump to the first row of the gamelist.
            GamelistView* gameList {ViewController::getInstance()->getGamelistView((*it)).get()};
            gameList->setCursor(gameList->getFirstEntry());
        }
    }

    if (mNeedsResetFilters) {
        for (auto it = SystemData::sSystemVector.cbegin(); // Line break.
             it != SystemData::sSystemVector.cend(); ++it) {
            if ((*it)->getThemeFolder() == "custom-collections") {
                for (FileData* customSystem : (*it)->getRootFolder()->getChildrenListToDisplay())
                    customSystem->getSystem()->getIndex()->resetFilters();
            }
            (*it)->getIndex()->resetFilters();
        }
    }

    if (mNeedsReloading)
        ViewController::getInstance()->reloadAll();

    if (mNeedsGoToStart)
        ViewController::getInstance()->goToStart(false);

    // Special case from GuiCollectionSystemsOptions where we didn't yet know whether a matching
    // theme existed when creating a new custom collection.
    if (mNeedsGoToSystem && mNeedsGoToGroupedCollections) {
        mNeedsGoToSystem = false;
        for (SystemData* system : SystemData::sSystemVector) {
            if (system->getThemeFolder() == mGoToSystem->getThemeFolder()) {
                mNeedsGoToSystem = true;
                mNeedsGoToGroupedCollections = false;
                break;
            }
        }
    }

    if (mNeedsGoToSystem)
        ViewController::getInstance()->goToSystem(mGoToSystem, false);

    if (mNeedsGoToGroupedCollections) {
        bool groupedSystemExists {false};
        for (SystemData* system : SystemData::sSystemVector) {
            if (system->getThemeFolder() == "custom-collections") {
                ViewController::getInstance()->goToSystem(system, false);
                groupedSystemExists = true;
                break;
            }
        }
        if (!groupedSystemExists)
            // No grouped custom collection system exists, so go to the first system instead.
            ViewController::getInstance()->goToSystem(SystemData::sSystemVector.front(), false);
    }

    if (mNeedsCollectionsUpdate) {
        auto state = ViewController::getInstance()->getState();
        // If we're in any view other than the grouped custom collections, always jump to the
        // system view in case of any collection updates. This is overkill in some instances but
        // these views can behave a bit strange during collection changes so it's better to be on
        // the safe side.
        if (state.getSystem()->isCollection() &&
            state.getSystem()->getThemeFolder() != "custom-collections") {
            ViewController::getInstance()->goToStart(false);
            ViewController::getInstance()->resetCamera();
            SDL_Delay(100);
            mWindow->invalidateCachedBackground();
            ViewController::getInstance()->goToSystem(SystemData::sSystemVector.front(), false);
            // We don't want to invalidate the cached background when there has been a collection
            // systen change as that may show a black screen in some circumstances.
            return;
        }
        // If the last displayed custom collection was just disabled, then go to start (to the
        // system view).
        if (std::find(SystemData::sSystemVector.begin(), SystemData::sSystemVector.end(),
                      state.getSystem()) == SystemData::sSystemVector.end()) {
            ViewController::getInstance()->goToStart(false);
            ViewController::getInstance()->resetCamera();
            SDL_Delay(100);
            mWindow->invalidateCachedBackground();
            return;
        }
        ViewController::getInstance()->resetCamera();
    }

    // If a close menu function was passed to us, then run it.
    if (mCloseMenuFunction)
        mCloseMenuFunction();

    if (mInvalidateCachedBackground) {
        // This delay reduces the likelyhood that the SVG rasterizer which is running in a
        // separate thread is not done until the cached background is invalidated. Without
        // this delay there's a high chance that some theme elements are not rendered in
        // time and thus not getting included in the regenerated cached background.
        // This is just a hack though and a better mechanism is needed to handle this.
        SDL_Delay(100);
        mWindow->invalidateCachedBackground();
    }
}

void GuiSettings::addEditableTextComponent(const std::string label,
                                           std::shared_ptr<GuiComponent> ed,
                                           std::string value,
                                           std::string defaultValue,
                                           bool isPassword)
{
    ComponentListRow row;
    row.elements.clear();

    auto lbl = std::make_shared<TextComponent>(Utils::String::toUpper(label),
                                               Font::get(FONT_SIZE_MEDIUM), mMenuColorPrimary,
                                               ALIGN_LEFT, ALIGN_CENTER, glm::ivec2 {0, 0});
    row.addElement(lbl, false);
    row.addElement(ed, true);

    const float maxLabelWidth {mMenu.getSize().x * 0.65f};
    lbl->setSize(glm::clamp(lbl->getTextCache()->metrics.size.x, 0.0f, maxLabelWidth),
                 lbl->getSize().y);

    auto spacer = std::make_shared<GuiComponent>();
    spacer->setSize(Renderer::getScreenWidth() * 0.005f, 0.0f);
    row.addElement(spacer, false);

    auto bracket = std::make_shared<ImageComponent>();
    bracket->setResize(glm::vec2 {0.0f, lbl->getFont()->getLetterHeight()});
    bracket->setImage(":/graphics/arrow.svg");
    bracket->setColorShift(mMenuColorPrimary);
    row.addElement(bracket, false);

    // OK callback (apply new value to ed).
    auto updateVal = [ed, defaultValue, isPassword](const std::string& newVal) {
        // If the field is blank, apply the default value if it's been passes as an argument.
        if (defaultValue != "" && newVal == "") {
            ed->setValue(defaultValue);
        }
        // If it's a password and actually set to something, then show a star mask.
        else if (isPassword && newVal == "") {
            ed->setValue("");
            ed->setHiddenValue("");
        }
        else if (isPassword) {
            ed->setValue("********");
            ed->setHiddenValue(newVal);
        }
        else {
            ed->setValue(newVal);
        }
    };

    if (Settings::getInstance()->getBool("VirtualKeyboard")) {
        row.makeAcceptInputHandler([this, label, ed, updateVal, isPassword] {
            // Never display the value if it's a password, instead set it to blank.
            if (isPassword)
                mWindow->pushGui(new GuiTextEditKeyboardPopup(
                    getHelpStyle(), getMenu().getPosition().y, label, "", updateVal, false,
                    _("SAVE"), _("SAVE CHANGES?")));
            else
                mWindow->pushGui(new GuiTextEditKeyboardPopup(
                    getHelpStyle(), getMenu().getPosition().y, label, ed->getValue(), updateVal,
                    false, _("SAVE"), _("SAVE CHANGES?")));
        });
    }
    else {
        row.makeAcceptInputHandler([this, label, ed, updateVal, isPassword] {
            if (isPassword)
                mWindow->pushGui(new GuiTextEditPopup(getHelpStyle(), label, "", updateVal, false,
                                                      _("SAVE"), _("SAVE CHANGES?")));
            else
                mWindow->pushGui(new GuiTextEditPopup(getHelpStyle(), label, ed->getValue(),
                                                      updateVal, false, _("SAVE"),
                                                      _("SAVE CHANGES?")));
        });
    }

    assert(ed);
    addRow(row);

    ed->setValue(value);
}

bool GuiSettings::input(InputConfig* config, Input input)
{
    if (config->isMappedTo("b", input) && input.value != 0) {
        delete this;
        return true;
    }

    return GuiComponent::input(config, input);
}

std::vector<HelpPrompt> GuiSettings::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts {mMenu.getHelpPrompts()};
    prompts.push_back(HelpPrompt("b", _("back")));
    return prompts;
}
