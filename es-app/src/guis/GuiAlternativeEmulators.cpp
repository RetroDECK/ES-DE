//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiAlternativeEmulators.cpp
//
//  User interface to select between alternative emulators per system
//  based on configuration entries in es_systems.xml.
//

#include "guis/GuiAlternativeEmulators.h"

#include "Gamelist.h"
#include "SystemData.h"
#include "views/ViewController.h"

GuiAlternativeEmulators::GuiAlternativeEmulators(Window* window)
    : GuiComponent{window}
    , mMenu{window, "ALTERNATIVE EMULATORS"}
    , mHasSystems(false)
{
    addChild(&mMenu);
    mMenu.addButton("BACK", "back", [this] { delete this; });

    // Horizontal sizes for the system and label entries.
    float systemSizeX = mMenu.getSize().x / 3.27f;
    float labelSizeX = mMenu.getSize().x / 1.53f;

    for (auto it = SystemData::sSystemVector.cbegin(); // Line break.
         it != SystemData::sSystemVector.cend(); it++) {

        // Only include systems that have at least two command entries, unless the system
        // has an invalid entry.
        if ((*it)->getAlternativeEmulator().substr(0, 9) != "<INVALID>" &&
            (*it)->getSystemEnvData()->mLaunchCommands.size() < 2)
            continue;

        ComponentListRow row;

        // This transparent bracket is only added to generate a left margin.
        auto bracket = std::make_shared<ImageComponent>(mWindow);
        bracket->setImage(":/graphics/arrow.svg");
        bracket->setOpacity(0);
        bracket->setSize(bracket->getSize() / 3.0f);
        row.addElement(bracket, false);

        std::string name = (*it)->getName();
        std::shared_ptr<TextComponent> systemText =
            std::make_shared<TextComponent>(mWindow, name, Font::get(FONT_SIZE_MEDIUM), 0x777777FF);

        systemText->setSize(systemSizeX, systemText->getSize().y);
        row.addElement(systemText, false);

        std::string configuredLabel = (*it)->getAlternativeEmulator();
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

        bool invalidEntry = false;

        if (label.empty()) {
            label = ViewController::EXCLAMATION_CHAR + " INVALID ENTRY";
            invalidEntry = true;
        }

        std::shared_ptr<TextComponent> labelText;

        if (label == (*it)->getSystemEnvData()->mLaunchCommands.front().second) {
            labelText = std::make_shared<TextComponent>(
                mWindow, label, Font::get(FONT_SIZE_MEDIUM, FONT_PATH_LIGHT), 0x777777FF,
                ALIGN_RIGHT);
        }
        else {
            // Mark any non-default value with bold and add a gear symbol as well.
            labelText = std::make_shared<TextComponent>(
                mWindow, label + (!invalidEntry ? " " + ViewController::GEAR_CHAR : ""),
                Font::get(FONT_SIZE_MEDIUM, FONT_PATH_BOLD), 0x777777FF, ALIGN_RIGHT);
        }

        // Mark invalid entries with red color.
        if (invalidEntry)
            labelText->setColor(TEXTCOLOR_SCRAPERMARKED);

        mCommandRows[name] = labelText;
        labelText->setSize(labelSizeX, labelText->getSize().y);

        row.addElement(labelText, false);
        row.makeAcceptInputHandler([this, it, labelText] {
            if (labelText->getValue() == ViewController::CROSSEDCIRCLE_CHAR + " CLEARED ENTRY")
                return;
            selectorWindow(*it);
        });

        mMenu.addRow(row);
        mHasSystems = true;
    }

    // Add a dummy row if no enabled systems have any alternative emulators defined in
    // es_systems.xml.
    if (!mHasSystems) {
        ComponentListRow row;
        std::shared_ptr<TextComponent> systemText = std::make_shared<TextComponent>(
            mWindow, ViewController::EXCLAMATION_CHAR + " NO ALTERNATIVE EMULATORS DEFINED",
            Font::get(FONT_SIZE_MEDIUM), 0x777777FF, ALIGN_CENTER);
        row.addElement(systemText, true);
        mMenu.addRow(row);
    }

    float width =
        static_cast<float>(std::min(static_cast<int>(Renderer::getScreenHeight() * 1.05f),
                                    static_cast<int>(Renderer::getScreenWidth() * 0.90f)));

    setSize(mMenu.getSize());
    setPosition((Renderer::getScreenWidth() - mSize.x) / 2.0f, Renderer::getScreenHeight() * 0.13f);

    mMenu.setSize(width, Renderer::getScreenHeight() * 0.76f);
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

    mCommandRows[systemName].get()->setColor(DEFAULT_TEXTCOLOR);
}

void GuiAlternativeEmulators::selectorWindow(SystemData* system)
{
    auto s = new GuiSettings(mWindow, system->getFullName());

    std::string selectedLabel = system->getAlternativeEmulator();
    std::string label;

    for (auto entry : system->getSystemEnvData()->mLaunchCommands) {
        ComponentListRow row;

        if (entry.second == "")
            label = ViewController::CROSSEDCIRCLE_CHAR + " CLEAR INVALID ENTRY";
        else
            label = entry.second;

        std::shared_ptr<TextComponent> labelText = std::make_shared<TextComponent>(
            mWindow, label, Font::get(FONT_SIZE_MEDIUM), 0x777777FF, ALIGN_CENTER);

        if (system->getSystemEnvData()->mLaunchCommands.front().second == label)
            labelText->setValue(labelText->getValue().append(" [DEFAULT]"));

        row.addElement(labelText, true);
        row.makeAcceptInputHandler([this, s, system, labelText, entry, selectedLabel] {
            if (entry.second != selectedLabel) {
                if (entry.second == system->getSystemEnvData()->mLaunchCommands.front().second)
                    system->setAlternativeEmulator("");
                else
                    system->setAlternativeEmulator(entry.second);
                updateGamelist(system, true);

                if (entry.second == system->getSystemEnvData()->mLaunchCommands.front().second) {
                    if (system->getSystemEnvData()->mLaunchCommands.front().second == "") {
                        updateMenu(system->getName(),
                                   ViewController::CROSSEDCIRCLE_CHAR + " CLEARED ENTRY",
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

        // This transparent bracket is only added to generate the correct help prompts.
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

    // Adjust the width depending on the aspect ratio of the screen, to make the screen look
    // somewhat coherent regardless of screen type. The 1.778 aspect ratio value is the 16:9
    // reference.
    float aspectValue = 1.778f / Renderer::getScreenAspectRatio();

    float maxWidthModifier = glm::clamp(0.70f * aspectValue, 0.50f, 0.92f);
    float maxWidth = static_cast<float>(Renderer::getScreenWidth()) * maxWidthModifier;

    s->setMenuSize(glm::vec2{maxWidth, s->getMenuSize().y});

    auto menuSize = s->getMenuSize();
    auto menuPos = s->getMenuPosition();

    s->setMenuPosition(glm::vec3{(s->getSize().x - menuSize.x) / 2.0f,
                                 (s->getSize().y - menuSize.y) / 3.0f, menuPos.z});
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
    std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
    prompts.push_back(HelpPrompt("b", "back"));
    if (mHasSystems)
        prompts.push_back(HelpPrompt("a", "select"));
    return prompts;
}

HelpStyle GuiAlternativeEmulators::getHelpStyle()
{
    HelpStyle style = HelpStyle();
    style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");
    return style;
}
