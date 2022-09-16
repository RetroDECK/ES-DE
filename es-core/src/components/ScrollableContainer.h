//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ScrollableContainer.h
//
//  Component containing scrollable information, used for the game
//  description text in the scraper and gamelist views.
//

#ifndef ES_CORE_COMPONENTS_SCROLLABLE_CONTAINER_H
#define ES_CORE_COMPONENTS_SCROLLABLE_CONTAINER_H

// Time in ms to wait before scrolling starts.
#define AUTO_SCROLL_DELAY 4500.0f
// Time in ms before resetting to the top after we reach the bottom.
#define AUTO_SCROLL_RESET_DELAY 7000.0f
// Relative scrolling speed (lower is faster).
#define AUTO_SCROLL_SPEED 4.0f

#include "GuiComponent.h"
#include "renderers/Renderer.h"

class ScrollableContainer : public GuiComponent
{
public:
    ScrollableContainer();

    glm::vec2 getScrollPos() const { return mScrollPos; }
    void setScrollPos(const glm::vec2& pos) { mScrollPos = pos; }

    const bool isScrollable() const override { return true; }

    void setAutoScroll(bool autoScroll);
    void setScrollParameters(float autoScrollDelayConstant,
                             float autoScrollResetDelayConstant,
                             float autoScrollSpeedConstant) override;
    void reset();

    void applyTheme(const std::shared_ptr<ThemeData>& theme,
                    const std::string& view,
                    const std::string& element,
                    unsigned int properties) override;

    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;

private:
    Renderer* mRenderer;
    glm::vec2 mScrollPos;
    glm::vec2 mScrollDir;

    float mAdjustedHeight;
    float mAutoScrollResetDelayConstant;
    float mAutoScrollDelayConstant;
    float mAutoScrollSpeedConstant;
    float mResolutionModifier;
    float mClipSpacing;

    int mAutoScrollDelay;
    int mAutoScrollSpeed;
    int mAutoScrollAccumulator;
    int mAutoScrollResetAccumulator;
    int mAdjustedAutoScrollSpeed;

    bool mVerticalSnap;
    bool mAtEnd;
    bool mUpdatedSize;
};

#endif // ES_CORE_COMPONENTS_SCROLLABLE_CONTAINER_H
