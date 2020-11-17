//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiSettings.cpp
//
//  User interface template for a settings GUI.
//  The saving of es_settings.cfg and the reload of the gamelists are triggered from here
//  based on the flags set by the actual menu entries' lambda functions.
//

#include "guis/GuiSettings.h"

#include "guis/GuiTextEditPopup.h"
#include "views/gamelist/IGameListView.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "Settings.h"
#include "SystemData.h"
#include "Window.h"

GuiSettings::GuiSettings(
        Window* window,
        std::string title)
        : GuiComponent(window),
        mMenu(window, title),
        mNeedsSaving(false),
        mNeedsCollectionsUpdate(false),
        mNeedsReloading(false),
        mNeedsSorting(false),
        mNeedsSortingCollections(false),
        mGoToSystem(nullptr),
        mNeedsGoToStart(false),
        mNeedsGoToSystemView(false)
{
    addChild(&mMenu);
    mMenu.addButton("BACK", "back", [this] { delete this; });

    setSize(static_cast<float>(Renderer::getScreenWidth()),
            static_cast<float>(Renderer::getScreenHeight()));
    mMenu.setPosition((mSize.x() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.15f);
}

GuiSettings::~GuiSettings()
{
    save();
}

void GuiSettings::save()
{
    if (!mSaveFuncs.size())
        return;

    for (auto it = mSaveFuncs.cbegin(); it != mSaveFuncs.cend(); it++)
        (*it)();

    if (mNeedsSaving)
        Settings::getInstance()->saveFile();

    if (mNeedsCollectionsUpdate) {
        CollectionSystemManager::get()->loadEnabledListFromSettings();
        CollectionSystemManager::get()->updateSystemsList();
    }

    if (mNeedsReloading)
        ViewController::get()->reloadAll();

    if (mNeedsSorting) {
        for (auto it = SystemData::sSystemVector.cbegin(); it !=
                SystemData::sSystemVector.cend(); it++) {
            if (!(!mNeedsSortingCollections && (*it)->isCollection())) {
                (*it)->sortSystem(true);
            }
            // Jump to the first row of the gamelist.
            IGameListView* gameList = ViewController::get()->getGameListView((*it)).get();
            gameList->setCursor(gameList->getFirstEntry());
        }
    }

    if (mNeedsGoToStart)
        ViewController::get()->goToStart();

    if (mNeedsGoToSystemView)
        ViewController::get()->goToSystemView(mGoToSystem, false);

    if (mNeedsSaving || mNeedsCollectionsUpdate || mNeedsReloading || mNeedsSorting ||
            mNeedsGoToStart || mNeedsGoToSystemView)
        mWindow->invalidateCachedBackground();
}

void GuiSettings::addEditableTextComponent(
        const std::string label,
        std::shared_ptr<GuiComponent> ed,
        std::string value,
        std::string defaultValue,
        bool isPassword)
{
    ComponentListRow row;
    row.elements.clear();

    auto lbl = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(label),
            Font::get(FONT_SIZE_MEDIUM), 0x777777FF);

    row.addElement(lbl, true);
    row.addElement(ed, true);

    auto spacer = std::make_shared<GuiComponent>(mWindow);
    spacer->setSize(Renderer::getScreenWidth() * 0.005f, 0);
    row.addElement(spacer, false);

    auto bracket = std::make_shared<ImageComponent>(mWindow);
    bracket->setImage(":/graphics/arrow.svg");
    bracket->setResize(Vector2f(0, lbl->getFont()->getLetterHeight()));
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

    row.makeAcceptInputHandler([this, label, ed, updateVal, isPassword] {
        // Never display the value if it's a password, instead set it to blank.
        if (isPassword)
            mWindow->pushGui(new GuiTextEditPopup(mWindow, getHelpStyle(), label,
                    "", updateVal, false));
        else
            mWindow->pushGui(new GuiTextEditPopup(mWindow, getHelpStyle(), label,
                    ed->getValue(), updateVal, false));
    });
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

//  Keep code for potential future use.
//  if (config->isMappedTo("start", input) && input.value != 0) {
//      // Close everything.
//      Window* window = mWindow;
//      while (window->peekGui() && window->peekGui() != ViewController::get())
//          delete window->peekGui();
//      return true;
//  }

    return GuiComponent::input(config, input);
}

HelpStyle GuiSettings::getHelpStyle()
{
    HelpStyle style = HelpStyle();
    style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");
    return style;
}

std::vector<HelpPrompt> GuiSettings::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
    prompts.push_back(HelpPrompt("b", "back"));
    return prompts;
}
