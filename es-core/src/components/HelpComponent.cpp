//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  HelpComponent.cpp
//
//  Help information in icon and text pairs.
//

#include "components/HelpComponent.h"

#include "Log.h"
#include "Settings.h"
#include "Window.h"
#include "components/ComponentGrid.h"
#include "components/ImageComponent.h"
#include "components/TextComponent.h"
#include "resources/TextureResource.h"
#include "utils/StringUtil.h"

static std::map<std::string, std::string> sIconPathMap{};

HelpComponent::HelpComponent(Window* window)
    : GuiComponent(window)
{
    assignIcons();
}

void HelpComponent::assignIcons()
{
    std::string controllerType = Settings::getInstance()->getString("InputControllerType");

    sIconPathMap.clear();

    // These graphics files are common between all controller types.
    sIconPathMap["up/down"] = ":/help/dpad_updown.svg";
    sIconPathMap["left/right"] = ":/help/dpad_leftright.svg";
    sIconPathMap["up/down/left/right"] = ":/help/dpad_all.svg";
    sIconPathMap["thumbstickclick"] = ":/help/thumbstick_click.svg";
    sIconPathMap["l"] = ":/help/button_l.svg";
    sIconPathMap["r"] = ":/help/button_r.svg";
    sIconPathMap["lr"] = ":/help/button_lr.svg";

    // These graphics files are custom per controller type.
    if (controllerType == "snes") {
        sIconPathMap["a"] = ":/help/button_a_SNES.svg";
        sIconPathMap["b"] = ":/help/button_b_SNES.svg";
        sIconPathMap["x"] = ":/help/button_x_SNES.svg";
        sIconPathMap["y"] = ":/help/button_y_SNES.svg";
        sIconPathMap["start"] = ":/help/button_start_SNES.svg";
        sIconPathMap["back"] = ":/help/button_back_SNES.svg";
    }
    else if (controllerType == "ps4") {
        sIconPathMap["a"] = ":/help/button_a_PS.svg";
        sIconPathMap["b"] = ":/help/button_b_PS.svg";
        sIconPathMap["x"] = ":/help/button_x_PS.svg";
        sIconPathMap["y"] = ":/help/button_y_PS.svg";
        sIconPathMap["start"] = ":/help/button_start_PS4.svg";
        sIconPathMap["back"] = ":/help/button_back_PS4.svg";
    }
    else if (controllerType == "ps5") {
        sIconPathMap["a"] = ":/help/button_a_PS.svg";
        sIconPathMap["b"] = ":/help/button_b_PS.svg";
        sIconPathMap["x"] = ":/help/button_x_PS.svg";
        sIconPathMap["y"] = ":/help/button_y_PS.svg";
        sIconPathMap["start"] = ":/help/button_start_PS5.svg";
        sIconPathMap["back"] = ":/help/button_back_PS5.svg";
    }
    else if (controllerType == "xbox360") {
        sIconPathMap["a"] = ":/help/button_a_XBOX.svg";
        sIconPathMap["b"] = ":/help/button_b_XBOX.svg";
        sIconPathMap["x"] = ":/help/button_x_XBOX.svg";
        sIconPathMap["y"] = ":/help/button_y_XBOX.svg";
        sIconPathMap["start"] = ":/help/button_start_XBOX360.svg";
        sIconPathMap["back"] = ":/help/button_back_XBOX360.svg";
    }
    else {
        // Xbox One and later.
        sIconPathMap["a"] = ":/help/button_a_XBOX.svg";
        sIconPathMap["b"] = ":/help/button_b_XBOX.svg";
        sIconPathMap["x"] = ":/help/button_x_XBOX.svg";
        sIconPathMap["y"] = ":/help/button_y_XBOX.svg";
        sIconPathMap["start"] = ":/help/button_start_XBOX.svg";
        sIconPathMap["back"] = ":/help/button_back_XBOX.svg";
    }
}

void HelpComponent::clearPrompts()
{
    mPrompts.clear();
    updateGrid();
}

void HelpComponent::setPrompts(const std::vector<HelpPrompt>& prompts)
{
    mPrompts = prompts;
    updateGrid();
}

void HelpComponent::setStyle(const HelpStyle& style)
{
    mStyle = style;
    updateGrid();
}

void HelpComponent::updateGrid()
{
    if (!Settings::getInstance()->getBool("ShowHelpPrompts") || mPrompts.empty()) {
        mGrid.reset();
        return;
    }

    std::shared_ptr<Font>& font = mStyle.font;

    mGrid = std::make_shared<ComponentGrid>(mWindow,
                                            glm::ivec2{static_cast<int>(mPrompts.size()) * 4, 1});

    // [icon] [spacer1] [text] [spacer2]

    std::vector<std::shared_ptr<ImageComponent>> icons;
    std::vector<std::shared_ptr<TextComponent>> labels;

    float width = 0;
    const float height = std::round(font->getLetterHeight() * 1.25f);

    // State variable indicating whether gui is dimmed.
    bool isDimmed = mWindow->isBackgroundDimmed();

    for (auto it = mPrompts.cbegin(); it != mPrompts.cend(); it++) {
        auto icon = std::make_shared<ImageComponent>(mWindow);
        icon->setImage(getIconTexture(it->first.c_str()));
        icon->setColorShift(isDimmed ? mStyle.iconColorDimmed : mStyle.iconColor);
        icon->setResize(0, height);
        icons.push_back(icon);

        // Format label according to theme style.
        std::string lblInput = it->second;
        if (mStyle.textStyle == "lowercase") {
            lblInput = Utils::String::toLower(lblInput);
        }
        else if (mStyle.textStyle == "camelcase") {
            lblInput = Utils::String::toCamelCase(lblInput);
        }
        else {
            lblInput = Utils::String::toUpper(lblInput);
        }

        auto lbl = std::make_shared<TextComponent>(
            mWindow, lblInput, font, isDimmed ? mStyle.textColorDimmed : mStyle.textColor);
        labels.push_back(lbl);

        width +=
            icon->getSize().x + lbl->getSize().x +
            ((mStyle.iconTextSpacing + mStyle.entrySpacing) * Renderer::getScreenWidthModifier());
    }

    mGrid->setSize(width, height);

    for (unsigned int i = 0; i < icons.size(); i++) {
        const int col = i * 4;
        mGrid->setColWidthPerc(col, icons.at(i)->getSize().x / width);
        mGrid->setColWidthPerc(
            col + 1, (mStyle.iconTextSpacing * Renderer::getScreenWidthModifier()) / width);
        mGrid->setColWidthPerc(col + 2, labels.at(i)->getSize().x / width);

        mGrid->setEntry(icons.at(i), glm::ivec2{col, 0}, false, false);
        mGrid->setEntry(labels.at(i), glm::ivec2{col + 2, 0}, false, false);
    }

    mGrid->setPosition({mStyle.position.x, mStyle.position.y, 0.0f});
    mGrid->setOrigin(mStyle.origin);
}

std::shared_ptr<TextureResource> HelpComponent::getIconTexture(const char* name)
{
    auto it = mIconCache.find(name);
    if (it != mIconCache.cend())
        return it->second;

    auto pathLookup = sIconPathMap.find(name);
    if (pathLookup == sIconPathMap.cend()) {
        LOG(LogError) << "Unknown help icon \"" << name << "\"";
        return nullptr;
    }
    if (!ResourceManager::getInstance()->fileExists(pathLookup->second)) {
        LOG(LogError) << "Couldn't load help icon \"" << name << "\" as the file \""
                      << pathLookup->second << "\" is missing";
        return nullptr;
    }

    std::shared_ptr<TextureResource> tex =
        TextureResource::get(pathLookup->second, false, false, false);
    mIconCache[std::string(name)] = tex;
    return tex;
}

void HelpComponent::setOpacity(unsigned char opacity)
{
    GuiComponent::setOpacity(opacity);

    for (unsigned int i = 0; i < mGrid->getChildCount(); i++)
        mGrid->getChild(i)->setOpacity(opacity);
}

void HelpComponent::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans{parentTrans * getTransform()};

    if (mGrid)
        mGrid->render(trans);
}
