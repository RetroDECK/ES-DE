//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiGamelistFilter.cpp
//
//  User interface for the gamelist filters.
//  Triggered from the GuiGamelistOptions menu.
//  Actual filter logic is covered by FileFilterIndex.
//

#include "guis/GuiGamelistFilter.h"

#include "SystemData.h"
#include "UIModeController.h"
#include "components/BadgeComponent.h"
#include "components/OptionListComponent.h"
#include "guis/GuiTextEditKeyboardPopup.h"
#include "guis/GuiTextEditPopup.h"
#include "utils/LocalizationUtil.h"
#include "utils/StringUtil.h"

GuiGamelistFilter::GuiGamelistFilter(SystemData* system,
                                     std::function<void(bool)> filterChangedCallback)
    : mMenu {_("FILTER GAMELIST")}
    , mSystem {system}
    , mFiltersChangedCallback {filterChangedCallback}
    , mFiltersChanged {false}
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
    row.addElement(std::make_shared<TextComponent>(_("RESET ALL FILTERS"),
                                                   Font::get(FONT_SIZE_MEDIUM), mMenuColorPrimary),
                   true);
    row.makeAcceptInputHandler(std::bind(&GuiGamelistFilter::resetAllFilters, this));
    mMenu.addRow(row);
    row.elements.clear();

    addFiltersToMenu();

    mMenu.addButton(_("BACK"), _("back"), std::bind(&GuiGamelistFilter::applyFilters, this));

    mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x) / 2.0f,
                      Renderer::getScreenHeight() * 0.13f);

    // Save the initial filter values to be able to check later if any changes were made.
    mInitialTextFilter = mTextFilterField->getValue();

    for (std::map<FilterIndexType,
                  std::shared_ptr<OptionListComponent<std::string>>>::const_iterator it =
             mFilterOptions.cbegin();
         it != mFilterOptions.cend(); ++it) {
        std::shared_ptr<OptionListComponent<std::string>> optionList {it->second};
        std::vector<std::string> filters {optionList->getSelectedObjects()};
        mInitialFilters.push_back(filters);
    }
}

void GuiGamelistFilter::resetAllFilters()
{
    mFilterIndex->resetFilters();
    for (std::map<FilterIndexType,
                  std::shared_ptr<OptionListComponent<std::string>>>::const_iterator it =
             mFilterOptions.cbegin();
         it != mFilterOptions.cend(); ++it) {
        std::shared_ptr<OptionListComponent<std::string>> optionList {it->second};
        optionList->selectNone();
    }

    mFilterIndex->setTextFilter("");
    mTextFilterField->setValue("");
    mFiltersChanged = true;
}

void GuiGamelistFilter::addFiltersToMenu()
{
    ComponentListRow row;

    auto lbl = std::make_shared<TextComponent>(
        Utils::String::toUpper(ViewController::KEYBOARD_CHAR + " " + _("GAME NAME")),
        Font::get(FONT_SIZE_MEDIUM), mMenuColorPrimary);

    mTextFilterField = std::make_shared<TextComponent>("", Font::get(FONT_SIZE_MEDIUM),
                                                       mMenuColorPrimary, ALIGN_RIGHT);
    mTextFilterField->setSize(
        0.0f, mTextFilterField->getFont()->getHeight(mTextFilterField->getLineSpacing()));

    // Don't show the free text filter entry unless there are any games in the system.
    if (mSystem->getRootFolder()->getChildren().size() > 0) {
        row.addElement(lbl, true);
        row.addElement(mTextFilterField, true);

        auto spacer = std::make_shared<GuiComponent>();
        spacer->setSize(Renderer::getScreenWidth() * 0.005f, 0.0f);
        row.addElement(spacer, false);

        auto bracket = std::make_shared<ImageComponent>();
        bracket->setResize(glm::vec2 {0.0f, lbl->getFont()->getLetterHeight()});
        bracket->setImage(":/graphics/arrow.svg");
        bracket->setColorShift(mMenuColorPrimary);
        row.addElement(bracket, false);

        mTextFilterField->setValue(mFilterIndex->getTextFilter());
    }

    // Callback function.
    auto updateVal = [this](const std::string& newVal) {
        mTextFilterField->setValue(Utils::String::trim(newVal));
        mFilterIndex->setTextFilter(Utils::String::trim(newVal));
    };

    if (Settings::getInstance()->getBool("VirtualKeyboard")) {
        row.makeAcceptInputHandler([this, updateVal] {
            const float verticalPosition {
                Renderer::getIsVerticalOrientation() ? mMenu.getPosition().y : 0.0f};
            mWindow->pushGui(new GuiTextEditKeyboardPopup(
                getHelpStyle(), verticalPosition, _("GAME NAME"), mTextFilterField->getValue(),
                updateVal, false, _("OK"), _("APPLY CHANGES?")));
        });
    }
    else {
        row.makeAcceptInputHandler([this, updateVal] {
            mWindow->pushGui(new GuiTextEditPopup(getHelpStyle(), _("GAME NAME"),
                                                  mTextFilterField->getValue(), updateVal, false,
                                                  _("OK"), _("APPLY CHANGES?")));
        });
    }

    mMenu.addRow(row);

    std::vector<FilterDataDecl> decls {mFilterIndex->getFilterDataDecls()};

    for (std::vector<FilterDataDecl>::const_iterator it = decls.cbegin(); // Line break.
         it != decls.cend(); ++it) {
        FilterIndexType type {(*it).type}; // Type of filter.

        // Don't include the alternative emulators if the corresponding setting has been disabled.
        if (type == ALTEMULATOR_FILTER &&
            !Settings::getInstance()->getBool("AlternativeEmulatorPerGame"))
            continue;

        std::map<std::string, int>* allKeys {(*it).allIndexKeys};

        bool exclusiveSelect {false};

        if (type == FAVORITES_FILTER || type == KIDGAME_FILTER || type == COMPLETED_FILTER ||
            type == BROKEN_FILTER)
            exclusiveSelect = true;

        // Don't display the hidden games filter if we're actually hiding these games.
        if (type == HIDDEN_FILTER) {
            if (Settings::getInstance()->getBool("ShowHiddenGames"))
                exclusiveSelect = true;
            else
                continue;
        }

        std::string menuLabel {(*it).menuLabel}; // Text to show in menu.
        std::shared_ptr<OptionListComponent<std::string>> optionList;

        // For bool values, make the selection exclusive so that both True and False can't be
        // selected at the same time. This should be changed to a SwitchComponent at some point.
        if (exclusiveSelect)
            optionList = std::make_shared<OptionListComponent<std::string>>(getHelpStyle(),
                                                                            menuLabel, true, true);
        else
            optionList = std::make_shared<OptionListComponent<std::string>>(getHelpStyle(),
                                                                            menuLabel, true, false);

        // Still display fields that can't be filtered in the menu, but notify the user and set
        // the OptionListComponent as disabled.
        if (allKeys->size() == 1 || allKeys->empty()) {
            optionList->setEnabled(false);
            optionList->setOpacity(DISABLED_OPACITY);
            optionList->setOverrideMultiText(_("NOTHING TO FILTER"));
        }

        if (type == CONTROLLER_FILTER) {
            for (auto it : *allKeys) {
                std::string displayName {
                    BadgeComponent::getDisplayName(Utils::String::toLower(it.first))};
                if (displayName == "unknown")
                    displayName = it.first;
                optionList->add(Utils::String::toUpper(displayName), it.first,
                                mFilterIndex->isKeyBeingFilteredBy(it.first, type));
            }
        }
        else {
            if (type == FAVORITES_FILTER || type == COMPLETED_FILTER || type == KIDGAME_FILTER ||
                type == HIDDEN_FILTER || type == BROKEN_FILTER) {
                for (auto it : *allKeys)
                    optionList->add(_(it.first.c_str()), it.first,
                                    mFilterIndex->isKeyBeingFilteredBy(it.first, type));
            }
            else {
                for (auto it : *allKeys)
                    optionList->add(it.first, it.first,
                                    mFilterIndex->isKeyBeingFilteredBy(it.first, type));
            }
        }

        if (allKeys->size() == 0)
            optionList->add("", "", false);

        mMenu.addWithLabel(menuLabel, optionList);

        mFilterOptions[type] = optionList;
    }
}

void GuiGamelistFilter::applyFilters()
{
    if (mInitialTextFilter != mTextFilterField->getValue())
        mFiltersChanged = true;

    std::vector<FilterDataDecl> decls {mFilterIndex->getFilterDataDecls()};
    for (std::map<FilterIndexType,
                  std::shared_ptr<OptionListComponent<std::string>>>::const_iterator it =
             mFilterOptions.cbegin();
         it != mFilterOptions.cend(); ++it) {
        std::shared_ptr<OptionListComponent<std::string>> optionList {it->second};
        std::vector<std::string> filters {optionList->getSelectedObjects()};
        auto iteratorDistance = std::distance(mFilterOptions.cbegin(), it);
        if (mInitialFilters[iteratorDistance] != filters)
            mFiltersChanged = true;
        mFilterIndex->setFilter(it->first, &filters);
    }

    mFiltersChangedCallback(mFiltersChanged);
    delete this;
}

bool GuiGamelistFilter::input(InputConfig* config, Input input)
{
    bool consumed {GuiComponent::input(config, input)};
    if (consumed)
        return true;

    if (config->isMappedTo("b", input) && input.value != 0)
        applyFilters();

    return false;
}

std::vector<HelpPrompt> GuiGamelistFilter::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts {mMenu.getHelpPrompts()};
    prompts.push_back(HelpPrompt("b", _("back")));
    prompts.push_back(HelpPrompt("a", _("select")));
    return prompts;
}
