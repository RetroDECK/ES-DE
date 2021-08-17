//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ScrollableContainer.cpp
//
//  Area containing scrollable information, for example the game description
//  text container in the detailed, video and grid views.
//

#include "components/ScrollableContainer.h"

#include "Window.h"
#include "animations/LambdaAnimation.h"
#include "renderers/Renderer.h"
#include "resources/Font.h"

ScrollableContainer::ScrollableContainer(Window* window)
    : GuiComponent(window)
    , mAutoScrollDelay(0)
    , mAutoScrollSpeed(0)
    , mAutoScrollAccumulator(0)
    , mScrollPos(0, 0)
    , mScrollDir(0, 0)
    , mAutoScrollResetAccumulator(0)
    , mFontSize(0.0f)
{
    // Set the modifier to get equivalent scrolling speed regardless of screen resolution.
    mResolutionModifier = Renderer::getScreenHeightModifier();
    mSmallFontSize = static_cast<float>(Font::get(FONT_SIZE_SMALL)->getSize());

    // For narrower aspect ratios than 16:9 there is a need to set an additional compensation
    // to get somehow consistent scrolling speeds.
    float aspectCompensation = static_cast<float>(Renderer::getScreenHeight()) /
                                   static_cast<float>(Renderer::getScreenWidth()) -
                               0.5625f;
    if (aspectCompensation > 0)
        mResolutionModifier += aspectCompensation * 4.0f;

    mAutoScrollResetDelayConstant = AUTO_SCROLL_RESET_DELAY;
    mAutoScrollDelayConstant = AUTO_SCROLL_DELAY;
    mAutoScrollSpeedConstant = AUTO_SCROLL_SPEED;
}

void ScrollableContainer::setAutoScroll(bool autoScroll)
{
    if (autoScroll) {
        mScrollDir = glm::vec2{0.0f, 1.0f};
        mAutoScrollDelay = static_cast<int>(mAutoScrollDelayConstant);
        mAutoScrollSpeed = mAutoScrollSpeedConstant;
        mAutoScrollSpeed =
            static_cast<int>(static_cast<float>(mAutoScrollSpeedConstant) / mResolutionModifier);
        reset();
    }
    else {
        mScrollDir = glm::vec2{};
        mAutoScrollDelay = 0;
        mAutoScrollSpeed = 0;
        mAutoScrollAccumulator = 0;
    }
}

void ScrollableContainer::setScrollParameters(float autoScrollDelayConstant,
                                              float autoScrollResetDelayConstant,
                                              int autoScrollSpeedConstant)
{
    mAutoScrollResetDelayConstant = autoScrollResetDelayConstant;
    mAutoScrollDelayConstant = autoScrollDelayConstant;
    mAutoScrollSpeedConstant = autoScrollSpeedConstant;
}

void ScrollableContainer::reset()
{
    mScrollPos = glm::vec2{};
    mAutoScrollResetAccumulator = 0;
    mAutoScrollAccumulator = -mAutoScrollDelay + mAutoScrollSpeed;
    mAtEnd = false;
}

void ScrollableContainer::update(int deltaTime)
{
    // Don't scroll if the media viewer or screensaver is active or if text scrolling is disabled;
    if (mWindow->isMediaViewerActive() || mWindow->isScreensaverActive() ||
        !mWindow->getAllowTextScrolling()) {
        if (mScrollPos != glm::vec2{} && !mWindow->isLaunchScreenDisplayed())
            reset();
        return;
    }

    const glm::vec2 contentSize{getContentSize()};
    int adjustedAutoScrollSpeed = mAutoScrollSpeed;

    // Adjust the scrolling speed based on the width of the container.
    float widthModifier = contentSize.x / static_cast<float>(Renderer::getScreenWidth());
    adjustedAutoScrollSpeed = static_cast<int>(adjustedAutoScrollSpeed * widthModifier);

    // Also adjust the scrolling speed based on the size of the font.
    float fontSizeModifier = mSmallFontSize / mFontSize;
    adjustedAutoScrollSpeed =
        static_cast<int>(adjustedAutoScrollSpeed * fontSizeModifier * fontSizeModifier);

    if (adjustedAutoScrollSpeed < 0)
        adjustedAutoScrollSpeed = 1;

    if (adjustedAutoScrollSpeed != 0) {
        mAutoScrollAccumulator += deltaTime;
        while (mAutoScrollAccumulator >= adjustedAutoScrollSpeed) {
            if (contentSize.y > mSize.y)
                mScrollPos += mScrollDir;
            mAutoScrollAccumulator -= adjustedAutoScrollSpeed;
        }
    }

    // Clip scrolling within bounds.
    if (mScrollPos.x < 0.0f)
        mScrollPos.x = 0.0f;
    if (mScrollPos.y < 0.0f)
        mScrollPos.y = 0.0f;

    if (mScrollPos.x + getSize().x > contentSize.x) {
        mScrollPos.x = contentSize.x - getSize().x;
        mAtEnd = true;
    }

    if (contentSize.y < getSize().y) {
        mScrollPos.y = 0.0f;
    }
    else if (mScrollPos.y + getSize().y > contentSize.y) {
        mScrollPos.y = contentSize.y - getSize().y;
        mAtEnd = true;
    }

    if (mAtEnd) {
        mAutoScrollResetAccumulator += deltaTime;
        if (mAutoScrollResetAccumulator >= static_cast<int>(mAutoScrollResetDelayConstant)) {
            // Fade in the text as it resets to the start position.
            auto func = [this](float t) {
                this->setOpacity(static_cast<unsigned char>(Math::lerp(0.0f, 1.0f, t) * 255));
                mScrollPos = glm::vec2{};
                mAutoScrollResetAccumulator = 0;
                mAutoScrollAccumulator = -mAutoScrollDelay + mAutoScrollSpeed;
                mAtEnd = false;
            };
            this->setAnimation(new LambdaAnimation(func, 300), 0, nullptr, false);
        }
    }

    GuiComponent::update(deltaTime);
}

void ScrollableContainer::render(const glm::mat4& parentTrans)
{
    if (!isVisible())
        return;

    glm::mat4 trans{parentTrans * getTransform()};

    glm::ivec2 clipPos{static_cast<int>(trans[3].x), static_cast<int>(trans[3].y)};

    glm::vec3 dimScaled{};
    dimScaled.x = std::fabs(trans[3].x + mSize.x);
    dimScaled.y = std::fabs(trans[3].y + mSize.y);

    glm::ivec2 clipDim{static_cast<int>(dimScaled.x - trans[3].x),
                       static_cast<int>(dimScaled.y - trans[3].y)};

    Renderer::pushClipRect(clipPos, clipDim);

    trans = glm::translate(trans, -glm::vec3{mScrollPos.x, mScrollPos.y, 0.0f});
    Renderer::setMatrix(trans);

    GuiComponent::renderChildren(trans);
    Renderer::popClipRect();
}

glm::vec2 ScrollableContainer::getContentSize()
{
    glm::vec2 max{};
    for (unsigned int i = 0; i < mChildren.size(); i++) {
        glm::vec2 pos{mChildren.at(i)->getPosition().x, mChildren.at(i)->getPosition().y};
        glm::vec2 bottomRight{mChildren.at(i)->getSize() + pos};
        if (bottomRight.x > max.x)
            max.x = bottomRight.x;
        if (bottomRight.y > max.y)
            max.y = bottomRight.y;
        if (!mFontSize)
            mFontSize = static_cast<float>(mChildren.at(i)->getFont()->getSize());
    }

    return max;
}
