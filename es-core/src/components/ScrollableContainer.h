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

#define AUTO_SCROLL_RESET_DELAY 3500.0f // Time before resetting to top after we reach the bottom.
#define AUTO_SCROLL_DELAY 2000.0f // Time to wait before we start to scroll.
#define AUTO_SCROLL_SPEED 38 // Relative scrolling speed (lower is faster).
#define AUTO_WIDTH_MOD 350.0f // Line width modifier to use to calculate scrolling speed.

#include "GuiComponent.h"

class ScrollableContainer : public GuiComponent
{
public:
    ScrollableContainer(Window* window);

    Vector2f getScrollPos() const;
    void setScrollPos(const Vector2f& pos);
    void setAutoScroll(bool autoScroll);
    void setScrollParameters(float autoScrollResetDelayConstant, float autoScrollDelayConstant,
            int autoScrollSpeedConstant, float autoWidthModConstant) override;
    void reset();

    void update(int deltaTime) override;
    void render(const Transform4x4f& parentTrans) override;

private:
    Vector2f getContentSize();

    Vector2f mScrollPos;
    Vector2f mScrollDir;

    float mAutoScrollResetDelayConstant;
    float mAutoScrollDelayConstant;
    int mAutoScrollSpeedConstant;
    float mAutoWidthModConstant;

    float mResolutionModifier;
    int mAutoScrollDelay;
    int mAutoScrollSpeed;
    int mAutoScrollAccumulator;
    int mAutoScrollResetAccumulator;
    bool mAtEnd;
};

#endif // ES_CORE_COMPONENTS_SCROLLABLE_CONTAINER_H
