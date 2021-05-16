//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ScrollableContainer.cpp
//
//  Area containing scrollable information, for example the game description
//  text container in the detailed, video and grid views.
//

#include "components/ScrollableContainer.h"

#include "animations/LambdaAnimation.h"
#include "math/Vector2i.h"
#include "renderers/Renderer.h"
#include "resources/Font.h"
#include "Window.h"

ScrollableContainer::ScrollableContainer(
        Window* window)
        : GuiComponent(window),
        mAutoScrollDelay(0),
        mAutoScrollSpeed(0),
        mAutoScrollAccumulator(0),
        mScrollPos(0, 0),
        mScrollDir(0, 0),
        mAutoScrollResetAccumulator(0),
        mFontSize(0.0f)
{
    // Set the modifier to get equivalent scrolling speed regardless of screen resolution.
    mResolutionModifier = Renderer::getScreenHeightModifier();
    mSmallFontSize = static_cast<float>(Font::get(FONT_SIZE_SMALL)->getSize());

    // For narrower aspect ratios than 16:9 there is a need to set an additional compensation
    // to get somehow consistent scrolling speeds.
    float aspectCompensation = static_cast<float>(Renderer::getScreenHeight()) /
            static_cast<float>(Renderer::getScreenWidth()) - 0.5625f;
    if (aspectCompensation > 0)
        mResolutionModifier += aspectCompensation * 4.0f;

    mAutoScrollResetDelayConstant = AUTO_SCROLL_RESET_DELAY;
    mAutoScrollDelayConstant = AUTO_SCROLL_DELAY;
    mAutoScrollSpeedConstant = AUTO_SCROLL_SPEED;
}

Vector2f ScrollableContainer::getScrollPos() const
{
    return mScrollPos;
}

void ScrollableContainer::setScrollPos(const Vector2f& pos)
{
    mScrollPos = pos;
}

void ScrollableContainer::setAutoScroll(bool autoScroll)
{
    if (autoScroll) {
        mScrollDir = Vector2f(0, 1);
        mAutoScrollDelay = static_cast<int>(mAutoScrollDelayConstant);
        mAutoScrollSpeed = mAutoScrollSpeedConstant;
        mAutoScrollSpeed = static_cast<int>(static_cast<float>(mAutoScrollSpeedConstant) /
                mResolutionModifier);
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
        float autoScrollResetDelayConstant, int autoScrollSpeedConstant)
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
        if (mScrollPos != 0)
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
    adjustedAutoScrollSpeed = static_cast<int>(adjustedAutoScrollSpeed *
            fontSizeModifier * fontSizeModifier);

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

void ScrollableContainer::render(const Transform4x4f& parentTrans)
{
    if (!isVisible())
        return;

    Transform4x4f trans = parentTrans * getTransform();

    Vector2i clipPos(static_cast<int>(trans.translation().x()),
            static_cast<int>(trans.translation().y()));

    Vector3f dimScaled = trans * Vector3f(mSize.x(), mSize.y(), 0);
    Vector2i clipDim(static_cast<int>((dimScaled.x()) - trans.translation().x()),
            static_cast<int>((dimScaled.y()) - trans.translation().y()));

    Renderer::pushClipRect(clipPos, clipDim);

    trans.translate(-Vector3f(mScrollPos.x(), mScrollPos.y(), 0));
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
