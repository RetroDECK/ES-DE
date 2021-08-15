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
#include "math/Vector2i.h"
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
        mScrollDir = Vector2f(0, 1);
        mAutoScrollDelay = static_cast<int>(mAutoScrollDelayConstant);
        mAutoScrollSpeed = mAutoScrollSpeedConstant;
        mAutoScrollSpeed =
            static_cast<int>(static_cast<float>(mAutoScrollSpeedConstant) / mResolutionModifier);
        reset();
    }
    else {
        mScrollDir = Vector2f(0, 0);
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
    mScrollPos = Vector2f(0, 0);
    mAutoScrollResetAccumulator = 0;
    mAutoScrollAccumulator = -mAutoScrollDelay + mAutoScrollSpeed;
    mAtEnd = false;
}

void ScrollableContainer::update(int deltaTime)
{
    // Don't scroll if the media viewer or screensaver is active or if text scrolling is disabled;
    if (mWindow->isMediaViewerActive() || mWindow->isScreensaverActive() ||
        !mWindow->getAllowTextScrolling()) {
        if (mScrollPos != 0 && !mWindow->isLaunchScreenDisplayed())
            reset();
        return;
    }

    const Vector2f contentSize = getContentSize();
    int adjustedAutoScrollSpeed = mAutoScrollSpeed;

    // Adjust the scrolling speed based on the width of the container.
    float widthModifier = contentSize.x() / static_cast<float>(Renderer::getScreenWidth());
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
            if (contentSize.y() > mSize.y())
                mScrollPos += mScrollDir;
            mAutoScrollAccumulator -= adjustedAutoScrollSpeed;
        }
    }

    // Clip scrolling within bounds.
    if (mScrollPos.x() < 0)
        mScrollPos[0] = 0;
    if (mScrollPos.y() < 0)
        mScrollPos[1] = 0;

    if (mScrollPos.x() + getSize().x() > contentSize.x()) {
        mScrollPos[0] = contentSize.x() - getSize().x();
        mAtEnd = true;
    }

    if (contentSize.y() < getSize().y()) {
        mScrollPos[1] = 0;
    }
    else if (mScrollPos.y() + getSize().y() > contentSize.y()) {
        mScrollPos[1] = contentSize.y() - getSize().y();
        mAtEnd = true;
    }

    if (mAtEnd) {
        mAutoScrollResetAccumulator += deltaTime;
        if (mAutoScrollResetAccumulator >= static_cast<int>(mAutoScrollResetDelayConstant)) {
            // Fade in the text as it resets to the start position.
            auto func = [this](float t) {
                this->setOpacity(static_cast<unsigned char>(Math::lerp(0.0f, 1.0f, t) * 255));
                mScrollPos = Vector2f(0, 0);
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

    glm::mat4 trans = parentTrans * getTransform();

    Vector2i clipPos(static_cast<int>(trans[3].x), static_cast<int>(trans[3].y));

    glm::vec3 dimScaled {};

    dimScaled.x = std::fabs(trans[3].x + mSize.x());
    dimScaled.y = std::fabs(trans[3].y + mSize.y());

    Vector2i clipDim(static_cast<int>(dimScaled.x - trans[3].x),
                     static_cast<int>(dimScaled.y - trans[3].y));

    Renderer::pushClipRect(clipPos, clipDim);

    trans = glm::translate(trans, -glm::vec3(mScrollPos.x(), mScrollPos.y(), 0.0f));
    Renderer::setMatrix(trans);

    GuiComponent::renderChildren(trans);
    Renderer::popClipRect();
}

Vector2f ScrollableContainer::getContentSize()
{
    Vector2f max(0, 0);
    for (unsigned int i = 0; i < mChildren.size(); i++) {
        Vector2f pos(mChildren.at(i)->getPosition()[0], mChildren.at(i)->getPosition()[1]);
        Vector2f bottomRight = mChildren.at(i)->getSize() + pos;
        if (bottomRight.x() > max.x())
            max.x() = bottomRight.x();
        if (bottomRight.y() > max.y())
            max.y() = bottomRight.y();
        if (!mFontSize)
            mFontSize = static_cast<float>(mChildren.at(i)->getFont()->getSize());
    }

    return max;
}
