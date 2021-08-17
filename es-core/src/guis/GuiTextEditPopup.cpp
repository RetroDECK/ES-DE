//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiTextEditPopup.cpp
//
//  Simple text edit popup with a title, a text input box and OK and Cancel buttons.
//

#include "guis/GuiTextEditPopup.h"

#include "Window.h"
#include "components/ButtonComponent.h"
#include "components/MenuComponent.h"
#include "components/TextEditComponent.h"
#include "guis/GuiMsgBox.h"

GuiTextEditPopup::GuiTextEditPopup(Window* window,
                                   const HelpStyle& helpstyle,
                                   const std::string& title,
                                   const std::string& initValue,
                                   const std::function<void(const std::string&)>& okCallback,
                                   bool multiLine,
                                   const std::string& acceptBtnText,
                                   const std::string& saveConfirmationText)
    : GuiComponent(window)
    , mHelpStyle(helpstyle)
    , mBackground(window, ":/graphics/frame.svg")
    , mGrid(window, glm::ivec2{1, 3})
    , mMultiLine(multiLine)
    , mInitValue(initValue)
    , mOkCallback(okCallback)
    , mSaveConfirmationText(saveConfirmationText)
{
    addChild(&mBackground);
    addChild(&mGrid);

    mTitle = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(title),
                                             Font::get(FONT_SIZE_MEDIUM), 0x555555FF, ALIGN_CENTER);

    mText = std::make_shared<TextEditComponent>(mWindow);
    mText->setValue(initValue);

    std::vector<std::shared_ptr<ButtonComponent>> buttons;
    buttons.push_back(std::make_shared<ButtonComponent>(mWindow, acceptBtnText, acceptBtnText,
                                                        [this, okCallback] {
                                                            okCallback(mText->getValue());
                                                            delete this;
                                                        }));
    buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "CLEAR", "clear",
                                                        [this] { mText->setValue(""); }));
    buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "CANCEL", "discard changes",
                                                        [this] { delete this; }));

    mButtonGrid = makeButtonGrid(mWindow, buttons);

    mGrid.setEntry(mTitle, glm::ivec2{0, 0}, false, true);
    mGrid.setEntry(mText, glm::ivec2{0, 1}, true, false, glm::ivec2{1, 1},
                   GridFlags::BORDER_TOP | GridFlags::BORDER_BOTTOM);
    mGrid.setEntry(mButtonGrid, glm::ivec2{0, 2}, true, false);

    float textHeight = mText->getFont()->getHeight();

    if (multiLine)
        textHeight *= 6.0f;
    mText->setSize(0, textHeight);

    // Adjust the width relative to the aspect ratio of the screen to make the GUI look coherent
    // regardless of screen type. The 1.778 aspect ratio value is the 16:9 reference.
    float aspectValue = 1.778f / Renderer::getScreenAspectRatio();
    float width = glm::clamp(0.50f * aspectValue, 0.40f, 0.70f) * Renderer::getScreenWidth();

    setSize(width, mTitle->getFont()->getHeight() + textHeight + mButtonGrid->getSize().y +
                       mButtonGrid->getSize().y / 2.0f);
    setPosition((Renderer::getScreenWidth() - mSize.x) / 2.0f,
                (Renderer::getScreenHeight() - mSize.y) / 2.0f);
    mText->startEditing();
}

void GuiTextEditPopup::onSizeChanged()
{
    mBackground.fitTo(mSize, glm::vec3{}, glm::vec2{-32.0f, -32.0f});

    mText->setSize(mSize.x - 40.0f, mText->getSize().y);

    // Update grid.
    mGrid.setRowHeightPerc(0, mTitle->getFont()->getHeight() / mSize.y);
    mGrid.setRowHeightPerc(2, mButtonGrid->getSize().y / mSize.y);
    mGrid.setSize(mSize);
}

bool GuiTextEditPopup::input(InputConfig* config, Input input)
{
    if (GuiComponent::input(config, input))
        return true;

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
    return false;
}

std::vector<HelpPrompt> GuiTextEditPopup::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts = mGrid.getHelpPrompts();
    prompts.push_back(HelpPrompt("b", "back"));
    return prompts;
}
