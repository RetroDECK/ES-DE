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

#include "GuiComponent.h"

class ScrollableContainer : public GuiComponent
{
public:
    ScrollableContainer(Window* window);

    Vector2f getScrollPos() const;
    void setScrollPos(const Vector2f& pos);
    void setAutoScroll(bool autoScroll);
    void reset();

    void update(int deltaTime) override;
    void render(const Transform4x4f& parentTrans) override;

private:
    Vector2f getContentSize();

    Vector2f mScrollPos;
    Vector2f mScrollDir;
    float mResolutionModifier;
    int mAutoScrollDelay;
    int mAutoScrollSpeed;
    int mAutoScrollAccumulator;
    int mAutoScrollResetAccumulator;
    bool mAtEnd;
};

#endif // ES_CORE_COMPONENTS_SCROLLABLE_CONTAINER_H
