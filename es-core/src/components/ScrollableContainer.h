//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ScrollableContainer.h
//
//  Area containing scrollable information, for example the game description
//  text container in the detailed, video and grid views.
//

#ifndef ES_CORE_COMPONENTS_SCROLLABLE_CONTAINER_H
#define ES_CORE_COMPONENTS_SCROLLABLE_CONTAINER_H

// Time in ms to wait before scrolling starts.
#define AUTO_SCROLL_DELAY 4500.0f
// Time in ms before resetting to the top after we reach the bottom.
#define AUTO_SCROLL_RESET_DELAY 7000.0f
// Relative scrolling speed (lower is faster).
#define AUTO_SCROLL_SPEED 90

#include "GuiComponent.h"

class ScrollableContainer : public GuiComponent
{
public:
    ScrollableContainer(Window* window);

    Vector2f getScrollPos() const { return mScrollPos; }
    void setScrollPos(const Vector2f& pos) { mScrollPos = pos; }

    void setAutoScroll(bool autoScroll);
    void setScrollParameters(float autoScrollDelayConstant,
                             float autoScrollResetDelayConstant,
                             int autoScrollSpeedConstant) override;
    void reset();

    void update(int deltaTime) override;
    void render(const Transform4x4f& parentTrans) override;

private:
    Vector2f getContentSize();

    Vector2f mScrollPos;
    Vector2f mScrollDir;

    float mFontSize;
    float mSmallFontSize;

    float mAutoScrollResetDelayConstant;
    float mAutoScrollDelayConstant;
    int mAutoScrollSpeedConstant;

    float mResolutionModifier;
    int mAutoScrollDelay;
    int mAutoScrollSpeed;
    int mAutoScrollAccumulator;
    int mAutoScrollResetAccumulator;
    bool mAtEnd;
};

#endif // ES_CORE_COMPONENTS_SCROLLABLE_CONTAINER_H
