//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiAlternativeEmulators.cpp
//
//  User interface to select between alternative emulators per system
//  based on configuration entries in es_systems.xml.
//

#include "guis/GuiAlternativeEmulators.h"

#include "GamelistFileParser.h"
#include "SystemData.h"
#include "utils/LocalizationUtil.h"
#include "views/ViewController.h"

GuiAlternativeEmulators::GuiAlternativeEmulators()
    : mMenu {_("ALTERNATIVE EMULATORS")}
    , mHasSystems {false}
{
    addChild(&mMenu);
    mMenu.addButton(_("BACK"), _("back"), [this] { delete this; });

    // Horizontal sizes for the system and label entries.
    float systemSizeX {mMenu.getSize().x / 3.27f};

    std::vector<SystemData*> sortedSystems {SystemData::sSystemVector};

    // Sort systems by short name.
    std::sort(std::begin(sortedSystems), std::end(sortedSystems), [](SystemData* a, SystemData* b) {
        return Utils::String::toUpper(a->getName()) < Utils::String::toUpper(b->getName());
    });

    for (auto it = sortedSystems.cbegin(); it != sortedSystems.cend(); ++it) {
        // Only include systems that have at least two command entries, unless the system
        // has an invalid entry.
        if ((*it)->getAlternativeEmulator().substr(0, 9) != "<INVALID>" &&
            (*it)->getSystemEnvData()->mLaunchCommands.size() < 2)
            continue;

        ComponentListRow row;

        std::string name {(*it)->getName()};
        std::shared_ptr<TextComponent> systemText {
            std::make_shared<TextComponent>(name, Font::get(FONT_SIZE_MEDIUM), mMenuColorPrimary,
                                            ALIGN_LEFT, ALIGN_CENTER, glm::ivec2 {0, 0})};

        systemText->setSize(systemSizeX, systemText->getSize().y);
        row.addElement(systemText, false);

        std::string configuredLabel {(*it)->getAlternativeEmulator()};
        std::string label;

        if (configuredLabel == "") {
            label = (*it)->getSystemEnvData()->mLaunchCommands.front().second;
        }
        else {
            for (auto command : (*it)->getSystemEnvData()->mLaunchCommands) {
                if (command.second == configuredLabel) {
                    label = command.second;
                    break;
                }
            }
        }

        bool invalidEntry {false};

        if (label.empty()) {
            label = ViewController::EXCLAMATION_CHAR + " " + _("INVALID ENTRY");
            invalidEntry = true;
        }

        std::shared_ptr<TextComponent> labelText;

        if (label == (*it)->getSystemEnvData()->mLaunchCommands.front().second) {
            labelText = std::make_shared<TextComponent>(
                label, Font::get(FONT_SIZE_MEDIUM, FONT_PATH_LIGHT), mMenuColorPrimary, ALIGN_RIGHT,
                ALIGN_CENTER, glm::ivec2 {0, 0});
        }
        else {
            // Mark any non-default value with bold and add a gear symbol as well.
            labelText = std::make_shared<TextComponent>(
                label + (!invalidEntry ? " " + ViewController::GEAR_CHAR : ""),
                Font::get(FONT_SIZE_MEDIUM, FONT_PATH_BOLD), mMenuColorPrimary, ALIGN_RIGHT,
                ALIGN_CENTER, glm::ivec2 {0, 0});
        }

        // Mark invalid entries with red color.
        if (invalidEntry)
            labelText->setColor(mMenuColorRed);

        mCommandRows[name] = labelText;
        labelText->setSize(mMenu.getSize().x - systemSizeX -
                               20.0f * Renderer::getScreenHeightModifier(),
                           systemText->getSize().y);

        row.addElement(labelText, false);

        SystemData* systemEntry {
            *std::find(SystemData::sSystemVector.cbegin(), SystemData::sSystemVector.cend(), *it)};

        row.makeAcceptInputHandler([this, systemEntry, labelText] {
            if (labelText->getValue() ==
                ViewController::CROSSEDCIRCLE_CHAR + " " + _("CLEARED ENTRY"))
                return;
            selectorWindow(systemEntry);
        });

        mMenu.addRow(row);
        mHasSystems = true;
    }

    // Add a dummy row if no enabled systems have any alternative emulators defined in
    // es_systems.xml.
    if (!mHasSystems) {
        ComponentListRow row;
        std::shared_ptr<TextComponent> systemText {std::make_shared<TextComponent>(
            ViewController::EXCLAMATION_CHAR + " " + _("NO ALTERNATIVE EMULATORS DEFINED"),
            Font::get(FONT_SIZE_MEDIUM), mMenuColorPrimary, ALIGN_CENTER)};
        row.addElement(systemText, true);
        mMenu.addRow(row);
    }

    setSize(mMenu.getSize());
    setPosition((Renderer::getScreenWidth() - mSize.x) / 2.0f, Renderer::getScreenHeight() * 0.13f);
}

void GuiAlternativeEmulators::updateMenu(const std::string& systemName,
                                         const std::string& label,
                                         bool defaultEmulator)
{
    // If another label was selected, then update the menu accordingly.
    if (defaultEmulator) {
        mCommandRows[systemName].get()->setFont(Font::get(FONT_SIZE_MEDIUM, FONT_PATH_LIGHT));
        mCommandRows[systemName].get()->setValue(label);
    }
    else {
        // Mark any non-default value with bold and add a gear symbol as well.
        mCommandRows[systemName].get()->setFont(Font::get(FONT_SIZE_MEDIUM, FONT_PATH_BOLD));
        mCommandRows[systemName].get()->setValue(label + " " + ViewController::GEAR_CHAR);
    }

    mCommandRows[systemName].get()->setColor(mMenuColorPrimary);
}

void GuiAlternativeEmulators::selectorWindow(SystemData* system)
{
    auto s = new GuiSettings(Utils::String::toUpper(system->getFullName()));

    std::string selectedLabel {system->getAlternativeEmulator()};
    std::string label;

    for (auto entry : system->getSystemEnvData()->mLaunchCommands) {
        ComponentListRow row;

        if (entry.second == "")
            label = ViewController::CROSSEDCIRCLE_CHAR + " " + _("CLEAR INVALID ENTRY");
        else
            label = entry.second;

        std::shared_ptr<TextComponent> labelText = std::make_shared<TextComponent>(
            label, Font::get(FONT_SIZE_MEDIUM), mMenuColorPrimary, ALIGN_LEFT);
        labelText->setSelectable(true);

        if (system->getSystemEnvData()->mLaunchCommands.front().second == label)
            labelText->setValue(
                labelText->getValue().append(" [").append(_("DEFAULT")).append("]"));

        row.addElement(labelText, true);
        row.makeAcceptInputHandler([this, s, system, labelText, entry, selectedLabel] {
            if (entry.second != selectedLabel) {
                if (entry.second == system->getSystemEnvData()->mLaunchCommands.front().second)
                    system->setAlternativeEmulator("");
                else
                    system->setAlternativeEmulator(entry.second);
                GamelistFileParser::updateGamelist(system, true);

                if (entry.second == system->getSystemEnvData()->mLaunchCommands.front().second) {
                    if (system->getSystemEnvData()->mLaunchCommands.front().second == "") {
                        updateMenu(system->getName(),
                                   ViewController::CROSSEDCIRCLE_CHAR + " " + _("CLEARED ENTRY"),
                                   (entry.second ==
                                    system->getSystemEnvData()->mLaunchCommands.front().second));
                    }
                    else {
                        updateMenu(system->getName(),
                                   system->getSystemEnvData()->mLaunchCommands.front().second,
                                   (entry.second ==
                                    system->getSystemEnvData()->mLaunchCommands.front().second));
                    }
                }
                else {
                    updateMenu(system->getName(), labelText->getValue(),
                               (entry.second ==
                                system->getSystemEnvData()->mLaunchCommands.front().second));
                }
            }
            delete s;
        });

        // Select the row that corresponds to the selected label.
        if (selectedLabel == label)
            s->addRow(row, true);
        else
            s->addRow(row, false);
    }

    // Set a maximum width depending on the aspect ratio of the screen, to make the screen look
    // somewhat coherent regardless of screen type. The 1.778 aspect ratio value is the 16:9
    // reference.
    float aspectValue {1.778f / Renderer::getScreenAspectRatio()};
    float maxWidthModifier {glm::clamp(0.77f * aspectValue, 0.50f,
                                       (Renderer::getIsVerticalOrientation() ? 0.94f : 0.92f))};
    float maxWidth {Renderer::getScreenWidth() * maxWidthModifier};

    // Set the width of the selector window to the menu width, unless the system full name is
    // too large to fit. If so, allow the size to be exceeded up to the maximum size calculated
    // above.
    float systemTextWidth {
        Font::get(FONT_SIZE_LARGE)->sizeText(Utils::String::toUpper(system->getFullName())).x *
        1.15f};

    float width {0.0f};
    float menuWidth {mMenu.getSize().x};

    if (systemTextWidth <= menuWidth)
        width = menuWidth;
    else if (systemTextWidth > maxWidth)
        width = maxWidth;
    else
        width = systemTextWidth;

    s->setMenuSize(glm::vec2 {width, s->getMenuSize().y});

    auto menuSize = s->getMenuSize();
    auto menuPos = s->getMenuPosition();

    s->setMenuPosition(glm::vec3 {(s->getSize().x - menuSize.x) / 2.0f, menuPos.y, menuPos.z});

    // Hack to properly update the window and set the scroll indicators. Why this is required
    // is currently a mystery.
    auto list = s->getMenu().getList();
    list->update(1);
    int cursor {list->getCursorId()};
    list->setCursor(list->getFirst());
    list->moveCursor(cursor);

    mWindow->pushGui(s);
}

bool GuiAlternativeEmulators::input(InputConfig* config, Input input)
{
    if (input.value != 0 && (config->isMappedTo("b", input) || config->isMappedTo("back", input))) {
        delete this;
        return true;
    }

    return mMenu.input(config, input);
}

std::vector<HelpPrompt> GuiAlternativeEmulators::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts {mMenu.getHelpPrompts()};
    prompts.push_back(HelpPrompt("b", _("back")));
    if (mHasSystems)
        prompts.push_back(HelpPrompt("a", _("select")));
    return prompts;
}
