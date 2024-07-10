//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiMsgBox.cpp
//
//  Popup message dialog with a notification text and a choice of one,
//  two or three buttons.
//

#include "guis/GuiMsgBox.h"

#include "components/ButtonComponent.h"
#include "components/MenuComponent.h"

#define HORIZONTAL_PADDING_PX 20.0f
#define VERTICAL_PADDING_MODIFIER 1.225f

GuiMsgBox::GuiMsgBox(const HelpStyle& helpstyle,
                     const std::string& text,
                     const std::string& name1,
                     const std::function<void()>& func1,
                     const std::string& name2,
                     const std::function<void()>& func2,
                     const std::string& name3,
                     const std::function<void()>& func3,
                     const std::function<void()>& backFunc,
                     const bool disableBackButton,
                     const bool deleteOnButtonPress,
                     const float maxWidthMultiplier)
    : mRenderer {Renderer::getInstance()}
    , mBackground {":/graphics/frame.svg"}
    , mGrid {glm::ivec2 {1, 2}}
    , mHelpStyle {helpstyle}
    , mBackFunc {backFunc}
    , mDisableBackButton {disableBackButton}
    , mDeleteOnButtonPress {deleteOnButtonPress}
    , mMaxWidthMultiplier {maxWidthMultiplier}
{
    // Adjust the width relative to the aspect ratio of the screen to make the GUI look coherent
    // regardless of screen type. The 1.778 aspect ratio value is the 16:9 reference.
    const float aspectValue {1.778f / mRenderer->getScreenAspectRatio()};

    if (mMaxWidthMultiplier == 0.0f)
        mMaxWidthMultiplier = mRenderer->getIsVerticalOrientation() ? 0.90f : 0.80f;

    float width {std::floor(glm::clamp(0.60f * aspectValue, 0.60f, mMaxWidthMultiplier) *
                            mRenderer->getScreenWidth())};
    const float minWidth {
        floorf(glm::clamp(0.30f * aspectValue, 0.10f, 0.50f) * mRenderer->getScreenWidth())};

    mMsg = std::make_shared<TextComponent>(text, Font::get(FONT_SIZE_MEDIUM), mMenuColorPrimary,
                                           ALIGN_CENTER);
    mGrid.setEntry(mMsg, glm::ivec2 {0, 0}, false, false);

    // Create the buttons.
    mButtons.push_back(std::make_shared<ButtonComponent>(
        name1, name1, std::bind(&GuiMsgBox::deleteMeAndCall, this, func1)));
    if (!name2.empty())
        mButtons.push_back(std::make_shared<ButtonComponent>(
            name2, name2, std::bind(&GuiMsgBox::deleteMeAndCall, this, func2)));
    if (!name3.empty())
        mButtons.push_back(std::make_shared<ButtonComponent>(
            name3, name3, std::bind(&GuiMsgBox::deleteMeAndCall, this, func3)));

    // Put the buttons into a ComponentGrid.
    mButtonGrid = MenuComponent::makeButtonGrid(mButtons);
    mGrid.setEntry(mButtonGrid, glm::ivec2 {0, 1}, true, false, glm::ivec2 {1, 1},
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

    // Now that we know the width, we can calculate the height.
    mMsg->setSize(width, 0.0f); // mMsg->getSize.y() now returns the proper length.
    const float msgHeight {std::max(Font::get(FONT_SIZE_LARGE)->getHeight(),
                                    mMsg->getSize().y * VERTICAL_PADDING_MODIFIER)};
    setSize(std::round(width + std::ceil(HORIZONTAL_PADDING_PX * 2.0f *
                                         mRenderer->getScreenWidthModifier())),
            std::round(msgHeight + mButtonGrid->getSize().y));

    setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f,
                (mRenderer->getScreenHeight() - mSize.y) / 2.0f);

    addChild(&mBackground);
    addChild(&mGrid);
}

void GuiMsgBox::changeText(const std::string& newText)
{
    mMsg->setText(newText);
    glm::vec2 newSize {mMsg->getFont()->sizeText(newText)};
    newSize.y *= VERTICAL_PADDING_MODIFIER;
    mMsg->setSize(newSize);

    // Adjust the width depending on the aspect ratio of the screen, to make the screen look
    // somewhat coherent regardless of screen type. The 1.778 aspect ratio value is the 16:9
    // reference.
    const float aspectValue {1.778f / Renderer::getScreenAspectRatio()};

    if (mMaxWidthMultiplier == 0.0f)
        mMaxWidthMultiplier = mRenderer->getIsVerticalOrientation() ? 0.90f : 0.80f;

    float width {floorf(glm::clamp(0.60f * aspectValue, 0.60f, mMaxWidthMultiplier) *
                        mRenderer->getScreenWidth())};
    const float minWidth {mRenderer->getScreenWidth() * 0.3f};

    // Decide final width.
    if (mMsg->getSize().x < width && mButtonGrid->getSize().x < width) {
        // mMsg and buttons are narrower than width.
        width = std::max(mButtonGrid->getSize().x, mMsg->getSize().x);
        width = std::max(width, minWidth);
    }
    else if (mButtonGrid->getSize().x > width) {
        width = mButtonGrid->getSize().x;
    }

    // Now that we know the width, we can calculate the height.
    mMsg->setSize(width, 0.0f); // mMsg->getSize.y() now returns the proper height.
    newSize = mMsg->getSize();
    newSize.y *= VERTICAL_PADDING_MODIFIER;
    mMsg->setSize(newSize);

    const float msgHeight {std::max(Font::get(FONT_SIZE_LARGE)->getHeight(), mMsg->getSize().y)};
    setSize(width + std::ceil(HORIZONTAL_PADDING_PX * 2.0f * mRenderer->getScreenWidthModifier()),
            msgHeight + mButtonGrid->getSize().y);
}

bool GuiMsgBox::input(InputConfig* config, Input input)
{
    if (!mDisableBackButton && config->isMappedTo("b", input) && input.value != 0) {
        if (mBackFunc)
            mBackFunc();
        delete this;
        return true;
    }

    return GuiComponent::input(config, input);
}

void GuiMsgBox::onSizeChanged()
{
    mGrid.setSize(mSize);
    mGrid.setRowHeightPerc(1, mButtonGrid->getSize().y / mSize.y);

    mMsg->setSize(mSize.x - HORIZONTAL_PADDING_PX * 2.0f * Renderer::getScreenWidthModifier(),
                  mGrid.getRowHeight(0));
    mGrid.onSizeChanged();

    mBackground.fitTo(mSize);
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
    std::vector<HelpPrompt> prompts {mGrid.getHelpPrompts()};

    if (!mDisableBackButton)
        prompts.push_back(HelpPrompt("b", "Back"));

    return prompts;
}
