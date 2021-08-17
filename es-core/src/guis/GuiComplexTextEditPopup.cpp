//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiComplexTextEditPopup.cpp
//
//  Text edit popup with a title, two text strings, a text input box and buttons
//  to load the second text string and to clear the input field.
//  Intended for updating settings for configuration files and similar.
//

#include "guis/GuiComplexTextEditPopup.h"

#include "Window.h"
#include "components/ButtonComponent.h"
#include "components/MenuComponent.h"
#include "components/TextEditComponent.h"
#include "guis/GuiMsgBox.h"

GuiComplexTextEditPopup::GuiComplexTextEditPopup(
    Window* window,
    const HelpStyle& helpstyle,
    const std::string& title,
    const std::string& infoString1,
    const std::string& infoString2,
    const std::string& initValue,
    const std::function<void(const std::string&)>& okCallback,
    bool multiLine,
    const std::string& acceptBtnText,
    const std::string& saveConfirmationText,
    const std::string& loadBtnText,
    const std::string& loadBtnHelpText,
    const std::string& clearBtnText,
    const std::string& clearBtnHelpText,
    bool hideCancelButton)
    : GuiComponent(window)
    , mHelpStyle(helpstyle)
    , mBackground(window, ":/graphics/frame.svg")
    , mGrid(window, glm::ivec2{1, 5})
    , mMultiLine(multiLine)
    , mInitValue(initValue)
    , mOkCallback(okCallback)
    , mSaveConfirmationText(saveConfirmationText)
    , mHideCancelButton(hideCancelButton)
{
    addChild(&mBackground);
    addChild(&mGrid);

    mTitle = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(title),
                                             Font::get(FONT_SIZE_MEDIUM), 0x555555FF, ALIGN_CENTER);
    mInfoString1 = std::make_shared<TextComponent>(mWindow, infoString1, Font::get(FONT_SIZE_SMALL),
                                                   0x555555FF, ALIGN_CENTER);
    mInfoString2 = std::make_shared<TextComponent>(mWindow, infoString2, Font::get(FONT_SIZE_SMALL),
                                                   0x555555FF, ALIGN_CENTER);

    mText = std::make_shared<TextEditComponent>(mWindow);
    mText->setValue(initValue);

    std::vector<std::shared_ptr<ButtonComponent>> buttons;
    buttons.push_back(std::make_shared<ButtonComponent>(mWindow, acceptBtnText, acceptBtnText,
                                                        [this, okCallback] {
                                                            okCallback(mText->getValue());
                                                            delete this;
                                                        }));
    buttons.push_back(std::make_shared<ButtonComponent>(mWindow, loadBtnText, loadBtnHelpText,
                                                        [this, infoString2] {
                                                            mText->setValue(infoString2);
                                                            mText->setCursor(0);
                                                            mText->setCursor(infoString2.size());
                                                        }));
    buttons.push_back(std::make_shared<ButtonComponent>(mWindow, clearBtnText, clearBtnHelpText,
                                                        [this] { mText->setValue(""); }));
    if (!mHideCancelButton)
        buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "CANCEL", "discard changes",
                                                            [this] { delete this; }));

    mButtonGrid = makeButtonGrid(mWindow, buttons);

    mGrid.setEntry(mTitle, glm::ivec2{0, 0}, false, true);
    mGrid.setEntry(mInfoString1, glm::ivec2{0, 1}, false, true);
    mGrid.setEntry(mInfoString2, glm::ivec2{0, 2}, false, false);
    mGrid.setEntry(mText, glm::ivec2{0, 3}, true, false, glm::ivec2{1, 1},
                   GridFlags::BORDER_TOP | GridFlags::BORDER_BOTTOM);
    mGrid.setEntry(mButtonGrid, glm::ivec2{0, 4}, true, false);
    mGrid.setRowHeightPerc(1, 0.15f, true);

    float textHeight = mText->getFont()->getHeight();

    if (multiLine)
        textHeight *= 6.0f;

    // Adjust the width relative to the aspect ratio of the screen to make the GUI look coherent
    // regardless of screen type. The 1.778 aspect ratio value is the 16:9 reference.
    float aspectValue = 1.778f / Renderer::getScreenAspectRatio();
    float infoWidth = Math::clamp(0.70f * aspectValue, 0.60f, 0.85f) * Renderer::getScreenWidth();
    float windowWidth = Math::clamp(0.75f * aspectValue, 0.65f, 0.90f) * Renderer::getScreenWidth();

    mText->setSize(0, textHeight);
    mInfoString2->setSize(infoWidth, mInfoString2->getFont()->getHeight());

    setSize(windowWidth, mTitle->getFont()->getHeight() + textHeight + mButtonGrid->getSize().y +
                             mButtonGrid->getSize().y * 1.85f);
    setPosition((Renderer::getScreenWidth() - mSize.x) / 2.0f,
                (Renderer::getScreenHeight() - mSize.y) / 2.0f);
    mText->startEditing();
}

void GuiComplexTextEditPopup::onSizeChanged()
{
    mBackground.fitTo(mSize, glm::vec3{}, glm::vec2{-32.0f, -32.0f});
    mText->setSize(mSize.x - 40.0f, mText->getSize().y);

    // Update grid.
    mGrid.setRowHeightPerc(0, mTitle->getFont()->getHeight() / mSize.y);
    mGrid.setRowHeightPerc(2, mButtonGrid->getSize().y / mSize.y);
    mGrid.setSize(mSize);
}

bool GuiComplexTextEditPopup::input(InputConfig* config, Input input)
{
    if (GuiComponent::input(config, input))
        return true;

    if (!mHideCancelButton) {
        // Pressing back when not text editing closes us.
        if (config->isMappedTo("b", input) && input.value) {
            if (mText->getValue() != mInitValue) {
                // Changes were made, ask if the user wants to save them.
                mWindow->pushGui(new GuiMsgBox(
                    mWindow, mHelpStyle, mSaveConfirmationText, "YES",
                    [this] {
                        this->mOkCallback(mText->getValue());
                        delete this;
                        return true;
                    },
                    "NO",
                    [this] {
                        delete this;
                        return false;
                    }));
            }
            else {
                delete this;
            }
        }
    }
    return false;
}

std::vector<HelpPrompt> GuiComplexTextEditPopup::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts = mGrid.getHelpPrompts();
    if (!mHideCancelButton)
        prompts.push_back(HelpPrompt("b", "back"));
    return prompts;
}
