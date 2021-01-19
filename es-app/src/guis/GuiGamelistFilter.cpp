//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiGamelistFilter.cpp
//
//  User interface for the gamelist filters.
//  Triggered from the GuiGamelistOptions menu.
//  Actual filter logic is covered by FileFilterIndex.
//

#include "guis/GuiGamelistFilter.h"

#include "components/OptionListComponent.h"
#include "guis/GuiTextEditPopup.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "SystemData.h"

GuiGamelistFilter::GuiGamelistFilter(
        Window* window,
        SystemData* system)
        : GuiComponent(window),
        mMenu(window, "FILTER GAMELIST BY"),
        mSystem(system)
{
    initializeMenu();
}

void GuiGamelistFilter::initializeMenu()
{
    addChild(&mMenu);

    // Get filters from system.
    mFilterIndex = mSystem->getIndex();

    ComponentListRow row;

    // Show filtered menu.
    row.elements.clear();
    row.addElement(std::make_shared<TextComponent>(mWindow, "RESET ALL FILTERS",
            Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
    row.makeAcceptInputHandler(std::bind(&GuiGamelistFilter::resetAllFilters, this));
    mMenu.addRow(row);
    row.elements.clear();

    addFiltersToMenu();

    mMenu.addButton("BACK", "back", std::bind(&GuiGamelistFilter::applyFilters, this));

    mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2,
            Renderer::getScreenHeight() * 0.15f);
}

void GuiGamelistFilter::resetAllFilters()
{
    mFilterIndex->resetFilters();
    for (std::map<FilterIndexType, std::shared_ptr< OptionListComponent<std::string>>>::
            const_iterator it = mFilterOptions.cbegin(); it != mFilterOptions.cend(); it++) {
        std::shared_ptr<OptionListComponent<std::string>> optionList = it->second;
        optionList->selectNone();
    }

    mFilterIndex->setTextFilter("");
    mTextFilterField->setValue("");
}

GuiGamelistFilter::~GuiGamelistFilter()
{
    mFilterOptions.clear();
}

void GuiGamelistFilter::addFiltersToMenu()
{
    ComponentListRow row;

    auto lbl = std::make_shared<TextComponent>(mWindow,
            Utils::String::toUpper("TEXT FILTER (GAME NAME)"),
            Font::get(FONT_SIZE_MEDIUM), 0x777777FF);

    mTextFilterField = std::make_shared<TextComponent>(mWindow, "",
            Font::get(FONT_SIZE_MEDIUM), 0x777777FF, ALIGN_RIGHT);

    row.addElement(lbl, true);
    row.addElement(mTextFilterField, true);

    auto spacer = std::make_shared<GuiComponent>(mWindow);
    spacer->setSize(Renderer::getScreenWidth() * 0.005f, 0);
    row.addElement(spacer, false);

    auto bracket = std::make_shared<ImageComponent>(mWindow);
    bracket->setImage(":/graphics/arrow.svg");
    bracket->setResize(Vector2f(0, lbl->getFont()->getLetterHeight()));
    row.addElement(bracket, false);

    mTextFilterField->setValue(mFilterIndex->getTextFilter());

    // Callback function.
    auto updateVal = [this](const std::string& newVal) {
       mTextFilterField->setValue(Utils::String::toUpper(newVal));
       mFilterIndex->setTextFilter(Utils::String::toUpper(newVal));
    };

    row.makeAcceptInputHandler([this, updateVal] {
            mWindow->pushGui(new GuiTextEditPopup(mWindow, getHelpStyle(),
                    "TEXT FILTER (GAME NAME)", mTextFilterField->getValue(),
                    updateVal, false, "OK", "APPLY CHANGES?"));
    });

    mMenu.addRow(row);

    std::vector<FilterDataDecl> decls = mFilterIndex->getFilterDataDecls();

    for (std::vector<FilterDataDecl>::const_iterator it =
            decls.cbegin(); it != decls.cend(); it++) {

        FilterIndexType type = (*it).type; // Type of filter.
         // All possible filters for this type.
        std::map<std::string, int>* allKeys = (*it).allIndexKeys;
        std::string menuLabel = (*it).menuLabel; // Text to show in menu.
        std::shared_ptr< OptionListComponent<std::string> > optionList;

        // Add filters (with first one selected).
        ComponentListRow row;

        // Add genres.
        optionList = std::make_shared<OptionListComponent<std::string>>
                (mWindow, getHelpStyle(), menuLabel, true);
        for (auto it: *allKeys)
            optionList->add(it.first, it.first, mFilterIndex->isKeyBeingFilteredBy(it.first, type));
        if (allKeys->size() > 0)
            mMenu.addWithLabel(menuLabel, optionList);

        mFilterOptions[type] = optionList;
    }
}

void GuiGamelistFilter::applyFilters()
{
    std::vector<FilterDataDecl> decls = mFilterIndex->getFilterDataDecls();
    for (std::map<FilterIndexType, std::shared_ptr<OptionListComponent<std::string>>>::
            const_iterator it = mFilterOptions.cbegin(); it != mFilterOptions.cend(); it++) {
        std::shared_ptr<OptionListComponent<std::string>> optionList = it->second;
        std::vector<std::string> filters = optionList->getSelectedObjects();
        mFilterIndex->setFilter(it->first, &filters);
    }

    delete this;
}

bool GuiGamelistFilter::input(InputConfig* config, Input input)
{
    bool consumed = GuiComponent::input(config, input);
    if (consumed)
        return true;

    if (config->isMappedTo("b", input) && input.value != 0)
        applyFilters();

    return false;
}

std::vector<HelpPrompt> GuiGamelistFilter::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
    prompts.push_back(HelpPrompt("b", "back"));
    prompts.push_back(HelpPrompt("a", "select"));
    return prompts;
}

HelpStyle GuiGamelistFilter::getHelpStyle()
{
    HelpStyle style = HelpStyle();
    style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");
    return style;
}
