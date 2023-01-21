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

static std::map<std::string, std::string> sIconPathMap {};

HelpComponent::HelpComponent()
    : mRenderer {Renderer::getInstance()}
{
    // Assign icons.
    assignIcons();
}

void HelpComponent::assignIcons()
{
    std::string controllerType {Settings::getInstance()->getString("InputControllerType")};

    std::map<std::string, std::string> sIconPathMapOld {sIconPathMap};
    sIconPathMap.clear();

    // These graphics files are common between all controller types.
    sIconPathMap["up/down"] = mStyle.mCustomButtons.dpad_updown.empty() ?
                                  ":/graphics/help/dpad_updown.svg" :
                                  mStyle.mCustomButtons.dpad_updown;
    sIconPathMap["left/right"] = mStyle.mCustomButtons.dpad_leftright.empty() ?
                                     ":/graphics/help/dpad_leftright.svg" :
                                     mStyle.mCustomButtons.dpad_leftright;
    sIconPathMap["up/down/left/right"] = mStyle.mCustomButtons.dpad_all.empty() ?
                                             ":/graphics/help/dpad_all.svg" :
                                             mStyle.mCustomButtons.dpad_all;
    sIconPathMap["thumbstickclick"] = mStyle.mCustomButtons.thumbstick_click.empty() ?
                                          ":/graphics/help/thumbstick_click.svg" :
                                          mStyle.mCustomButtons.thumbstick_click;
    sIconPathMap["l"] = mStyle.mCustomButtons.button_l.empty() ? ":/graphics/help/button_l.svg" :
                                                                 mStyle.mCustomButtons.button_l;
    sIconPathMap["r"] = mStyle.mCustomButtons.button_r.empty() ? ":/graphics/help/button_r.svg" :
                                                                 mStyle.mCustomButtons.button_r;
    sIconPathMap["lr"] = mStyle.mCustomButtons.button_lr.empty() ? ":/graphics/help/button_lr.svg" :
                                                                   mStyle.mCustomButtons.button_lr;
    sIconPathMap["lt"] = mStyle.mCustomButtons.button_lt.empty() ? ":/graphics/help/button_lt.svg" :
                                                                   mStyle.mCustomButtons.button_lt;
    sIconPathMap["rt"] = mStyle.mCustomButtons.button_rt.empty() ? ":/graphics/help/button_rt.svg" :
                                                                   mStyle.mCustomButtons.button_rt;
    sIconPathMap["ltrt"] = mStyle.mCustomButtons.button_ltrt.empty() ?
                               ":/graphics/help/button_ltrt.svg" :
                               mStyle.mCustomButtons.button_ltrt;

    // These graphics files are custom per controller type.
    if (controllerType == "snes") {
        sIconPathMap["a"] = mStyle.mCustomButtons.button_a_SNES.empty() ?
                                ":/graphics/help/button_a_SNES.svg" :
                                mStyle.mCustomButtons.button_a_SNES;
        sIconPathMap["b"] = mStyle.mCustomButtons.button_b_SNES.empty() ?
                                ":/graphics/help/button_b_SNES.svg" :
                                mStyle.mCustomButtons.button_b_SNES;
        sIconPathMap["x"] = mStyle.mCustomButtons.button_x_SNES.empty() ?
                                ":/graphics/help/button_x_SNES.svg" :
                                mStyle.mCustomButtons.button_x_SNES;
        sIconPathMap["y"] = mStyle.mCustomButtons.button_y_SNES.empty() ?
                                ":/graphics/help/button_y_SNES.svg" :
                                mStyle.mCustomButtons.button_y_SNES;
        sIconPathMap["back"] = mStyle.mCustomButtons.button_back_SNES.empty() ?
                                   ":/graphics/help/button_back_SNES.svg" :
                                   mStyle.mCustomButtons.button_back_SNES;
        sIconPathMap["start"] = mStyle.mCustomButtons.button_start_SNES.empty() ?
                                    ":/graphics/help/button_start_SNES.svg" :
                                    mStyle.mCustomButtons.button_start_SNES;
    }
    else if (controllerType == "switchpro") {
        sIconPathMap["a"] = mStyle.mCustomButtons.button_a_switch.empty() ?
                                ":/graphics/help/button_a_switch.svg" :
                                mStyle.mCustomButtons.button_a_switch;
        sIconPathMap["b"] = mStyle.mCustomButtons.button_b_switch.empty() ?
                                ":/graphics/help/button_b_switch.svg" :
                                mStyle.mCustomButtons.button_b_switch;
        sIconPathMap["x"] = mStyle.mCustomButtons.button_x_switch.empty() ?
                                ":/graphics/help/button_x_switch.svg" :
                                mStyle.mCustomButtons.button_x_switch;
        sIconPathMap["y"] = mStyle.mCustomButtons.button_y_switch.empty() ?
                                ":/graphics/help/button_y_switch.svg" :
                                mStyle.mCustomButtons.button_y_switch;
        sIconPathMap["back"] = mStyle.mCustomButtons.button_back_switch.empty() ?
                                   ":/graphics/help/button_back_switch.svg" :
                                   mStyle.mCustomButtons.button_back_switch;
        sIconPathMap["start"] = mStyle.mCustomButtons.button_start_switch.empty() ?
                                    ":/graphics/help/button_start_switch.svg" :
                                    mStyle.mCustomButtons.button_start_switch;
    }
    else if (controllerType == "ps123") {
        sIconPathMap["a"] = mStyle.mCustomButtons.button_a_PS.empty() ?
                                ":/graphics/help/button_a_PS.svg" :
                                mStyle.mCustomButtons.button_a_PS;
        sIconPathMap["b"] = mStyle.mCustomButtons.button_b_PS.empty() ?
                                ":/graphics/help/button_b_PS.svg" :
                                mStyle.mCustomButtons.button_b_PS;
        sIconPathMap["x"] = mStyle.mCustomButtons.button_x_PS.empty() ?
                                ":/graphics/help/button_x_PS.svg" :
                                mStyle.mCustomButtons.button_x_PS;
        sIconPathMap["y"] = mStyle.mCustomButtons.button_y_PS.empty() ?
                                ":/graphics/help/button_y_PS.svg" :
                                mStyle.mCustomButtons.button_y_PS;
        sIconPathMap["back"] = mStyle.mCustomButtons.button_back_PS123.empty() ?
                                   ":/graphics/help/button_back_PS123.svg" :
                                   mStyle.mCustomButtons.button_back_PS123;
        sIconPathMap["start"] = mStyle.mCustomButtons.button_start_PS123.empty() ?
                                    ":/graphics/help/button_start_PS123.svg" :
                                    mStyle.mCustomButtons.button_start_PS123;
    }
    else if (controllerType == "ps4") {
        sIconPathMap["a"] = mStyle.mCustomButtons.button_a_PS.empty() ?
                                ":/graphics/help/button_a_PS.svg" :
                                mStyle.mCustomButtons.button_a_PS;
        sIconPathMap["b"] = mStyle.mCustomButtons.button_b_PS.empty() ?
                                ":/graphics/help/button_b_PS.svg" :
                                mStyle.mCustomButtons.button_b_PS;
        sIconPathMap["x"] = mStyle.mCustomButtons.button_x_PS.empty() ?
                                ":/graphics/help/button_x_PS.svg" :
                                mStyle.mCustomButtons.button_x_PS;
        sIconPathMap["y"] = mStyle.mCustomButtons.button_y_PS.empty() ?
                                ":/graphics/help/button_y_PS.svg" :
                                mStyle.mCustomButtons.button_y_PS;
        sIconPathMap["back"] = mStyle.mCustomButtons.button_back_PS4.empty() ?
                                   ":/graphics/help/button_back_PS4.svg" :
                                   mStyle.mCustomButtons.button_back_PS4;
        sIconPathMap["start"] = mStyle.mCustomButtons.button_start_PS4.empty() ?
                                    ":/graphics/help/button_start_PS4.svg" :
                                    mStyle.mCustomButtons.button_start_PS4;
    }
    else if (controllerType == "ps5") {
        sIconPathMap["a"] = mStyle.mCustomButtons.button_a_PS.empty() ?
                                ":/graphics/help/button_a_PS.svg" :
                                mStyle.mCustomButtons.button_a_PS;
        sIconPathMap["b"] = mStyle.mCustomButtons.button_b_PS.empty() ?
                                ":/graphics/help/button_b_PS.svg" :
                                mStyle.mCustomButtons.button_b_PS;
        sIconPathMap["x"] = mStyle.mCustomButtons.button_x_PS.empty() ?
                                ":/graphics/help/button_x_PS.svg" :
                                mStyle.mCustomButtons.button_x_PS;
        sIconPathMap["y"] = mStyle.mCustomButtons.button_y_PS.empty() ?
                                ":/graphics/help/button_y_PS.svg" :
                                mStyle.mCustomButtons.button_y_PS;
        sIconPathMap["back"] = mStyle.mCustomButtons.button_back_PS5.empty() ?
                                   ":/graphics/help/button_back_PS5.svg" :
                                   mStyle.mCustomButtons.button_back_PS5;
        sIconPathMap["start"] = mStyle.mCustomButtons.button_start_PS5.empty() ?
                                    ":/graphics/help/button_start_PS5.svg" :
                                    mStyle.mCustomButtons.button_start_PS5;
    }
    else if (controllerType == "xbox360") {

        sIconPathMap["a"] = mStyle.mCustomButtons.button_a_XBOX.empty() ?
                                ":/graphics/help/button_a_XBOX.svg" :
                                mStyle.mCustomButtons.button_a_XBOX;
        sIconPathMap["b"] = mStyle.mCustomButtons.button_b_XBOX.empty() ?
                                ":/graphics/help/button_b_XBOX.svg" :
                                mStyle.mCustomButtons.button_b_XBOX;
        sIconPathMap["x"] = mStyle.mCustomButtons.button_x_XBOX.empty() ?
                                ":/graphics/help/button_x_XBOX.svg" :
                                mStyle.mCustomButtons.button_x_XBOX;
        sIconPathMap["y"] = mStyle.mCustomButtons.button_y_XBOX.empty() ?
                                ":/graphics/help/button_y_XBOX.svg" :
                                mStyle.mCustomButtons.button_y_XBOX;
        sIconPathMap["back"] = mStyle.mCustomButtons.button_back_XBOX360.empty() ?
                                   ":/graphics/help/button_back_XBOX360.svg" :
                                   mStyle.mCustomButtons.button_back_XBOX360;
        sIconPathMap["start"] = mStyle.mCustomButtons.button_start_XBOX360.empty() ?
                                    ":/graphics/help/button_start_XBOX360.svg" :
                                    mStyle.mCustomButtons.button_start_XBOX360;
    }
    else {
        // Xbox One and later.
        sIconPathMap["a"] = mStyle.mCustomButtons.button_a_XBOX.empty() ?
                                ":/graphics/help/button_a_XBOX.svg" :
                                mStyle.mCustomButtons.button_a_XBOX;
        sIconPathMap["b"] = mStyle.mCustomButtons.button_b_XBOX.empty() ?
                                ":/graphics/help/button_b_XBOX.svg" :
                                mStyle.mCustomButtons.button_b_XBOX;
        sIconPathMap["x"] = mStyle.mCustomButtons.button_x_XBOX.empty() ?
                                ":/graphics/help/button_x_XBOX.svg" :
                                mStyle.mCustomButtons.button_x_XBOX;
        sIconPathMap["y"] = mStyle.mCustomButtons.button_y_XBOX.empty() ?
                                ":/graphics/help/button_y_XBOX.svg" :
                                mStyle.mCustomButtons.button_y_XBOX;
        sIconPathMap["back"] = mStyle.mCustomButtons.button_back_XBOX.empty() ?
                                   ":/graphics/help/button_back_XBOX.svg" :
                                   mStyle.mCustomButtons.button_back_XBOX;
        sIconPathMap["start"] = mStyle.mCustomButtons.button_start_XBOX.empty() ?
                                    ":/graphics/help/button_start_XBOX.svg" :
                                    mStyle.mCustomButtons.button_start_XBOX;
    }

    // Invalidate cache for icons that have changed.
    auto it = sIconPathMap.begin();
    while (it != sIconPathMap.end()) {
        if (sIconPathMapOld.find(it->first) != sIconPathMapOld.end()) {
            if (sIconPathMapOld[it->first] != sIconPathMap[it->first]) {
                if (mIconCache.find(it->first) != mIconCache.end())
                    mIconCache.erase(mIconCache.find(it->first));
            }
        }
        ++it;
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
    assignIcons();
}

void HelpComponent::updateGrid()
{
    if (!Settings::getInstance()->getBool("ShowHelpPrompts") || mPrompts.empty()) {
        mGrid.reset();
        return;
    }

    std::shared_ptr<Font>& font {mStyle.font};
    mGrid = std::make_shared<ComponentGrid>(glm::ivec2 {static_cast<int>(mPrompts.size()) * 5, 1});

    std::vector<std::shared_ptr<ImageComponent>> icons;
    std::vector<std::shared_ptr<TextComponent>> labels;

    float width {0.0f};
    float height {font->getLetterHeight() * 1.25f};
    bool isDimmed {mWindow->isBackgroundDimmed()};

    for (auto it = mPrompts.cbegin(); it != mPrompts.cend(); ++it) {
        auto icon = std::make_shared<ImageComponent>();
        icon->setImage(getIconTexture(it->first.c_str()), false);
        icon->setColorShift(isDimmed ? mStyle.iconColorDimmed : mStyle.iconColor);
        icon->setResize(0, height);
        icon->setOpacity(mStyle.opacity);
        icons.push_back(icon);

        // Apply text style and color from the theme to the label and add it to the label list.
        std::string lblInput {it->second};
        if (mStyle.letterCase == "lowercase")
            lblInput = Utils::String::toLower(lblInput);
        else if (mStyle.letterCase == "capitalize")
            lblInput = Utils::String::toCapitalized(lblInput);
        else
            lblInput = Utils::String::toUpper(lblInput);
        auto lbl = std::make_shared<TextComponent>(
            lblInput, font, isDimmed ? mStyle.textColorDimmed : mStyle.textColor);
        lbl->setOpacity(mStyle.opacity);
        labels.push_back(lbl);

        width += icon->getSize().x + lbl->getSize().x +
                 ((mStyle.iconTextSpacing * mRenderer->getScreenWidth() +
                   mStyle.entrySpacing * mRenderer->getScreenWidth()));
    }

    mGrid->setSize(width, height);

    for (int i {0}; i < static_cast<int>(icons.size()); ++i) {
        const int col {i * 5};
        mGrid->setColWidthPerc(col, icons.at(i)->getSize().x / width);
        mGrid->setColWidthPerc(col + 1,
                               (mStyle.iconTextSpacing * mRenderer->getScreenWidth()) / width);
        mGrid->setColWidthPerc(col + 2, labels.at(i)->getSize().x / width);
        mGrid->setColWidthPerc(col + 3,
                               (mStyle.entrySpacing * mRenderer->getScreenWidth()) / width);

        mGrid->setEntry(icons.at(i), glm::ivec2 {col, 0}, false, false);
        mGrid->setEntry(labels.at(i), glm::ivec2 {col + 2, 0}, false, false);
    }

    // There is a bug for legacy themes where the entrySpacing width is added to the right of
    // the grid when aligning to the right using an X origin value of 1. This issue is retained
    // for legacy themes for backward compatibility reasons.
    if (mStyle.legacyTheme) {
        mGrid->setPosition({mStyle.position.x, mStyle.position.y, 0.0f});
    }
    else {
        mGrid->setPosition(
            {mStyle.position.x +
                 ((mStyle.entrySpacing * mRenderer->getScreenWidth()) * mStyle.origin.x),
             mStyle.position.y, 0.0f});
    }

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

    if (!ResourceManager::getInstance().fileExists(pathLookup->second)) {
        LOG(LogError) << "Couldn't load help icon \"" << name << "\" as the file \""
                      << pathLookup->second << "\" is missing";
        return nullptr;
    }

    std::shared_ptr<TextureResource> tex {
        TextureResource::get(pathLookup->second, false, false, false)};
    mIconCache[std::string(name)] = tex;
    return tex;
}

void HelpComponent::setOpacity(float opacity)
{
    GuiComponent::setOpacity(opacity * mStyle.opacity);

    for (unsigned int i = 0; i < mGrid->getChildCount(); ++i)
        mGrid->getChild(i)->setOpacity(opacity * mStyle.opacity);
}

void HelpComponent::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans {parentTrans * getTransform()};

    if (mGrid)
        mGrid->render(trans);
}
