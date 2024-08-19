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
    // Initially set the text component to wrap by line breaks while maintaining the row lengths.
    // This is the "ideal" size for the text as it's exactly how it's written.
    mMsg = std::make_shared<TextComponent>(text, Font::get(FONT_SIZE_MEDIUM), mMenuColorPrimary,
                                           ALIGN_CENTER, ALIGN_CENTER, glm::ivec2 {1, 1});
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

    calculateSize();

    setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f,
                (mRenderer->getScreenHeight() - mSize.y) / 2.0f);

    addChild(&mBackground);
    addChild(&mGrid);
}

void GuiMsgBox::calculateSize()
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

    // Decide final width.
    if (mMsg->getSize().x < width && mButtonGrid->getSize().x < width) {
        // mMsg and buttons are narrower than width.
        width = std::max(mButtonGrid->getSize().x, mMsg->getSize().x);
        width = std::max(width, minWidth);
    }
    else if (mButtonGrid->getSize().x > width) {
        width = mButtonGrid->getSize().x;
    }

    // As the actual rows may be too wide to fit we change to wrapping by our component width
    // while allowing expansion vertically. Setting the width will update the text cache.
    mMsg->setAutoCalcExtent(glm::vec2 {0, 1});
    mMsg->setSize(width, 0.0f);

    const float msgHeight {std::max(Font::get(FONT_SIZE_LARGE)->getHeight(),
                                    mMsg->getSize().y * VERTICAL_PADDING_MODIFIER)};
    setSize(std::round(width + std::ceil(HORIZONTAL_PADDING_PX * 2.0f *
                                         mRenderer->getScreenWidthModifier())),
            std::round(msgHeight + mButtonGrid->getSize().y));
}

void GuiMsgBox::changeText(const std::string& newText)
{
    mMsg->setAutoCalcExtent(glm::vec2 {1, 1});
    mMsg->setText(newText);

    calculateSize();
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

    mMsg->setSize(mSize.x -
                      std::ceil(HORIZONTAL_PADDING_PX * 2.0f * Renderer::getScreenWidthModifier()),
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
        prompts.push_back(HelpPrompt("b", _("back")));

    return prompts;
}
