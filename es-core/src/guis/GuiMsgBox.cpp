//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiMsgBox.cpp
//
//  Popup message dialog with a notification text and a choice of one,
//  two or three buttons.
//

#include "guis/GuiMsgBox.h"

#include "components/ButtonComponent.h"
#include "components/MenuComponent.h"

#define HORIZONTAL_PADDING_PX 20.0f

GuiMsgBox::GuiMsgBox(Window* window,
                     const HelpStyle &helpstyle,
                     const std::string &text,
                     const std::string &name1,
                     const std::function<void()> &func1,
                     const std::string &name2,
                     const std::function<void()> &func2,
                     const std::string &name3,
                     const std::function<void()> &func3,
                     bool disableBackButton,
                     bool deleteOnButtonPress)
        : GuiComponent(window), mBackground(window, ":/graphics/frame.svg"), mGrid(window, glm::ivec2{1, 2}),
          mHelpStyle(helpstyle), mDisableBackButton(disableBackButton), mDeleteOnButtonPress(deleteOnButtonPress)
{
    // Adjust the width relative to the aspect ratio of the screen to make the GUI look coherent
    // regardless of screen type. The 1.778 aspect ratio value is the 16:9 reference.
    float aspectValue = 1.778f / Renderer::getScreenAspectRatio();

    float width =
        floorf(glm::clamp(0.60f * aspectValue, 0.60f, 0.80f) * Renderer::getScreenWidth());
    float minWidth =
        floorf(glm::clamp(0.30f * aspectValue, 0.10f, 0.50f) * Renderer::getScreenWidth());

    mMsg = std::make_shared<TextComponent>(mWindow, text, Font::get(FONT_SIZE_MEDIUM), 0x777777FF,
                                           ALIGN_CENTER);
    mGrid.setEntry(mMsg, glm::ivec2{0, 0}, false, false);

    // Create the buttons.
    mButtons.push_back(std::make_shared<ButtonComponent>(
        mWindow, name1, name1, std::bind(&GuiMsgBox::deleteMeAndCall, this, func1)));
    if (!name2.empty())
        mButtons.push_back(std::make_shared<ButtonComponent>(
            mWindow, name2, name2, std::bind(&GuiMsgBox::deleteMeAndCall, this, func2)));
    if (!name3.empty())
        mButtons.push_back(std::make_shared<ButtonComponent>(
            mWindow, name3, name3, std::bind(&GuiMsgBox::deleteMeAndCall, this, func3)));

    // Set accelerator automatically (button to press when "B" is pressed).
    if (mButtons.size() == 1) {
        mAcceleratorFunc = mButtons.front()->getPressedFunc();
    }
    else {
        for (auto it = mButtons.cbegin(); it != mButtons.cend(); it++) {
            if (Utils::String::toUpper((*it)->getText()) == "OK" ||
                Utils::String::toUpper((*it)->getText()) == "NO") {
                mAcceleratorFunc = (*it)->getPressedFunc();
                break;
            }
        }
    }

    // Put the buttons into a ComponentGrid.
    mButtonGrid = makeButtonGrid(mWindow, mButtons);
    mGrid.setEntry(mButtonGrid, glm::ivec2{0, 1}, true, false, glm::ivec2{1, 1},
                   GridFlags::BORDER_TOP);

    // Decide final width.
    if (mMsg->getSize().x < width && mButtonGrid->getSize().x < width) {
        // mMsg and buttons are narrower than width.
        width = std::max(mButtonGrid->getSize().x, mMsg->getSize().x);
        width = std::max(width, minWidth);
    }
    else if (mButtonGrid->getSize().x > width) {
        width = mButtonGrid->getSize().x;
    }

    // Now that we know width, we can find height.
    mMsg->setSize(width, 0.0f); // mMsg->getSize.y() now returns the proper length.
    const float msgHeight =
        std::max(Font::get(FONT_SIZE_LARGE)->getHeight(), mMsg->getSize().y * 1.225f);
    setSize(width + HORIZONTAL_PADDING_PX * 2.0f * Renderer::getScreenWidthModifier(),
            msgHeight + mButtonGrid->getSize().y);

    // Center for good measure.
    setPosition((Renderer::getScreenWidth() - mSize.x) / 2.0f,
                (Renderer::getScreenHeight() - mSize.y) / 2.0f);

    addChild(&mBackground);
    addChild(&mGrid);
}

void GuiMsgBox::changeText(const std::string& newText)
{
    mMsg->setText(newText);
    mMsg->setSize(mMsg->getFont()->sizeText(newText));

    // Adjust the width depending on the aspect ratio of the screen, to make the screen look
    // somewhat coherent regardless of screen type. The 1.778 aspect ratio value is the 16:9
    // reference.
    float aspectValue = 1.778f / Renderer::getScreenAspectRatio();

    float width =
        floorf(glm::clamp(0.60f * aspectValue, 0.60f, 0.80f) * Renderer::getScreenWidth());
    float minWidth = Renderer::getScreenWidth() * 0.3f;

    // Decide final width.
    if (mMsg->getSize().x < width && mButtonGrid->getSize().x < width) {
        // mMsg and buttons are narrower than width.
        width = std::max(mButtonGrid->getSize().x, mMsg->getSize().x);
        width = std::max(width, minWidth);
    }
    else if (mButtonGrid->getSize().x > mSize.x) {
        width = mButtonGrid->getSize().x;
    }

    // Now that we know width, we can find height.
    mMsg->setSize(width, 0); // mMsg->getSize.y() now returns the proper length.
    const float msgHeight =
        std::max(Font::get(FONT_SIZE_LARGE)->getHeight(), mMsg->getSize().y * 1.225f);
    setSize(width + HORIZONTAL_PADDING_PX * 2.0f * Renderer::getScreenWidthModifier(),
            msgHeight + mButtonGrid->getSize().y);
}

bool GuiMsgBox::input(InputConfig* config, Input input)
{
    // Special case for when GuiMsgBox comes up to report errors before
    // anything has been configured.
    if (config->getDeviceId() == DEVICE_KEYBOARD && !config->isConfigured() && input.value &&
        (input.id == SDLK_RETURN || input.id == SDLK_ESCAPE || input.id == SDLK_SPACE)) {
        mAcceleratorFunc();
        return true;
    }

    if (!mDisableBackButton) {
        if (mAcceleratorFunc && config->isMappedTo("b", input) && input.value != 0) {
            mAcceleratorFunc();
            return true;
        }
    }

    return GuiComponent::input(config, input);
}

void GuiMsgBox::onSizeChanged()
{
    mGrid.setSize(mSize);
    mGrid.setRowHeightPerc(1, mButtonGrid->getSize().y / mSize.y);

    // Update messagebox size.
    mMsg->setSize(mSize.x - HORIZONTAL_PADDING_PX * 2.0f * Renderer::getScreenWidthModifier(),
                  mGrid.getRowHeight(0));
    mGrid.onSizeChanged();

    mBackground.fitTo(mSize, glm::vec3{}, glm::vec2{-32.0f, -32.0f});
}

void GuiMsgBox::deleteMeAndCall(const std::function<void()>& func)
{
    auto funcCopy = func;

    if (mDeleteOnButtonPress)
        delete this;

    if (funcCopy)
        funcCopy();
}

std::vector<HelpPrompt> GuiMsgBox::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts = mGrid.getHelpPrompts();

    // If there is only one button, then remove the "Choose" help symbol
    // as there is no way to make a choice.
    if (mButtons.size() == 1)
        prompts.pop_back();

    if (!mDisableBackButton)
        prompts.push_back(HelpPrompt("b", "Back"));

    return prompts;
}
