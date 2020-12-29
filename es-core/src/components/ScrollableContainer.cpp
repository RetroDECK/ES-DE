//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ScrollableContainer.cpp
//
//  Area containing scrollable information, for example the game description
//  text container in the detailed, video and grid views.
//

#include "components/ScrollableContainer.h"

#include "math/Vector2i.h"
#include "renderers/Renderer.h"
#include "Window.h"

#define AUTO_SCROLL_RESET_DELAY 6200 // Time before resetting to top after we reach the bottom.
#define AUTO_SCROLL_DELAY 2600 // Time to wait before we start to scroll.
#define AUTO_SCROLL_SPEED 42 // Relative scrolling speed (lower is faster).
#define AUTO_WIDTH_MOD 350 // Line width modifier to use to calculate scrolling speed.

ScrollableContainer::ScrollableContainer(
        Window* window)
        : GuiComponent(window),
        mAutoScrollDelay(0),
        mAutoScrollSpeed(0),
        mAutoScrollAccumulator(0),
        mScrollPos(0, 0),
        mScrollDir(0, 0),
        mAutoScrollResetAccumulator(0)
{
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

void ScrollableContainer::setAutoScroll(bool autoScroll)
{
    if (autoScroll) {
        mScrollDir = Vector2f(0, 1);
        mAutoScrollDelay = AUTO_SCROLL_DELAY;
        mAutoScrollSpeed = AUTO_SCROLL_SPEED;
        reset();
    }
    else {
        mScrollDir = Vector2f(0, 0);
        mAutoScrollDelay = 0;
        mAutoScrollSpeed = 0;
        mAutoScrollAccumulator = 0;
    }
}

Vector2f ScrollableContainer::getScrollPos() const
{
    return mScrollPos;
}

void ScrollableContainer::setScrollPos(const Vector2f& pos)
{
    mScrollPos = pos;
}

void ScrollableContainer::update(int deltaTime)
{
    // Don't scroll if the screensaver is active or text scrolling is disabled;
    if (mWindow->isScreensaverActive() || !mWindow->getAllowTextScrolling()) {
        if (mScrollPos != 0)
            reset();
        return;
    }

    const Vector2f contentSize = getContentSize();
    // Scale speed by the text width, more text per line leads to slower scrolling.
    const float widthMod = contentSize.x() / AUTO_WIDTH_MOD;

    if (mAutoScrollSpeed != 0) {
        mAutoScrollAccumulator += deltaTime / static_cast<int>(widthMod);

        while (mAutoScrollAccumulator >= mAutoScrollSpeed) {
            mScrollPos += mScrollDir;
            mAutoScrollAccumulator -= mAutoScrollSpeed;
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
        if (mAutoScrollResetAccumulator >= AUTO_SCROLL_RESET_DELAY)
            reset();
    }

    GuiComponent::update(deltaTime);
}

// This should probably return a box to allow for when controls don't start at 0,0.
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
    }

    return max;
}

void ScrollableContainer::reset()
{
    mScrollPos = Vector2f(0, 0);
    mAutoScrollResetAccumulator = 0;
    mAutoScrollAccumulator = -mAutoScrollDelay + mAutoScrollSpeed;
    mAtEnd = false;
}
