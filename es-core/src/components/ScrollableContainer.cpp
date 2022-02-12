//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ScrollableContainer.cpp
//
//  Component containing scrollable information, used for the game
//  description text in the scraper and gamelist views.
//

#include "components/ScrollableContainer.h"

#include "Window.h"
#include "animations/LambdaAnimation.h"
#include "renderers/Renderer.h"
#include "resources/Font.h"

ScrollableContainer::ScrollableContainer()
    : mScrollPos {0.0f, 0.0f}
    , mScrollDir {0.0f, 0.0f}
    , mClipSpacing {0.0f}
    , mAutoScrollDelay {0}
    , mAutoScrollSpeed {0}
    , mAutoScrollAccumulator {0}
    , mAutoScrollResetAccumulator {0}
    , mAdjustedAutoScrollSpeed {0}
    , mUpdatedSize {false}
{
    // Set the modifier to get equivalent scrolling speed regardless of screen resolution.
    mResolutionModifier = Renderer::getScreenHeightModifier();

    mAutoScrollResetDelayConstant = AUTO_SCROLL_RESET_DELAY;
    mAutoScrollDelayConstant = AUTO_SCROLL_DELAY;
    mAutoScrollSpeedConstant = AUTO_SCROLL_SPEED;
}

void ScrollableContainer::setAutoScroll(bool autoScroll)
{
    if (autoScroll) {
        mScrollDir = glm::vec2 {0.0f, 1.0f};
        mAutoScrollDelay = static_cast<int>(mAutoScrollDelayConstant);
        reset();
    }
    else {
        mScrollDir = glm::vec2 {};
        mAutoScrollDelay = 0;
        mAutoScrollSpeed = 0;
        mAutoScrollAccumulator = 0;
    }
}

void ScrollableContainer::setScrollParameters(float autoScrollDelayConstant,
                                              float autoScrollResetDelayConstant,
                                              float autoScrollSpeedConstant)
{
    mAutoScrollResetDelayConstant = glm::clamp(autoScrollResetDelayConstant, 1000.0f, 10000.0f);
    mAutoScrollDelayConstant = glm::clamp(autoScrollDelayConstant, 1000.0f, 10000.0f);
    mAutoScrollSpeedConstant = AUTO_SCROLL_SPEED / glm::clamp(autoScrollSpeedConstant, 0.1f, 10.0f);
}

void ScrollableContainer::reset()
{
    mScrollPos = glm::vec2 {};
    mAutoScrollResetAccumulator = 0;
    mAutoScrollAccumulator = -mAutoScrollDelay + mAutoScrollSpeed;
    mAtEnd = false;
    // This is needed to resize to the font height boundary when we keep running in the background
    // while launching games.
    if (!mChildren.empty()) {
        float combinedHeight {
            mChildren.front()->getFont()->getHeight(mChildren.front()->getLineSpacing())};
        if (mChildren.front()->getSize().y > mSize.y) {
            float numLines {mSize.y / combinedHeight};
            mSize.y = floorf(numLines) * combinedHeight;
            if (mSize.y == 0.0f)
                mSize.y = combinedHeight;
        }
    }
}

void ScrollableContainer::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                     const std::string& view,
                                     const std::string& element,
                                     unsigned int properties)
{
    using namespace ThemeFlags;
    GuiComponent::applyTheme(theme, view, element, properties);

    const ThemeData::ThemeElement* elem {theme->getElement(view, element, "text")};
    if (!elem || !elem->has("container"))
        return;

    if (elem->has("containerScrollSpeed")) {
        mAutoScrollSpeedConstant =
            AUTO_SCROLL_SPEED / glm::clamp(elem->get<float>("containerScrollSpeed"), 0.1f, 10.0f);
    }

    if (elem->has("containerStartDelay")) {
        mAutoScrollDelayConstant =
            glm::clamp(elem->get<float>("containerStartDelay"), 0.0f, 10.0f) * 1000.0f;
    }

    if (elem->has("containerResetDelay")) {
        mAutoScrollResetDelayConstant =
            glm::clamp(elem->get<float>("containerResetDelay"), 0.0f, 20.0f) * 1000.0f;
    }
}

void ScrollableContainer::update(int deltaTime)
{
    if (mSize == glm::vec2 {0.0f, 0.0f})
        return;

    // Don't scroll if the media viewer or screensaver is active or if text scrolling is disabled;
    if (mWindow->isMediaViewerActive() || mWindow->isScreensaverActive() ||
        !mWindow->getAllowTextScrolling()) {
        if (mScrollPos != glm::vec2 {} && !mWindow->isLaunchScreenDisplayed())
            reset();
        return;
    }

    const glm::vec2 contentSize {mChildren.front()->getSize()};
    float rowModifier {1.0f};

    float lineSpacing {mChildren.front()->getLineSpacing()};
    float combinedHeight {mChildren.front()->getFont()->getHeight(lineSpacing)};

    // Calculate the spacing which will be used to clip the container.
    if (lineSpacing > 1.2f && mClipSpacing == 0.0f) {
        const float minimumSpacing = mChildren.front()->getFont()->getHeight(1.2f);
        const float currentSpacing = mChildren.front()->getFont()->getHeight(lineSpacing);
        mClipSpacing = std::round((currentSpacing - minimumSpacing) / 2.0f);
    }

    // Resize container to font height boundary to avoid rendering a fraction of the last line.
    if (!mUpdatedSize && contentSize.y > mSize.y) {
        float numLines {mSize.y / combinedHeight};
        mSize.y = floorf(numLines) * combinedHeight;
        mUpdatedSize = true;
    }
    else if (mUpdatedSize) {
        // If there are less than 8 lines of text, accelerate the scrolling further.
        float lines {mSize.y / combinedHeight};
        if (lines < 8.0f)
            rowModifier = lines / 8.0f;
    }

    if (!mAdjustedAutoScrollSpeed) {
        float fontSize {static_cast<float>(mChildren.front()->getFont()->getSize())};
        float width {contentSize.x / (fontSize * 1.3f)};

        // Keep speed adjustments within reason.
        float speedModifier {glm::clamp(width, 10.0f, 40.0f)};

        speedModifier *= mAutoScrollSpeedConstant;
        speedModifier /= mResolutionModifier;
        mAdjustedAutoScrollSpeed = static_cast<int>(speedModifier);
    }

    if (mAdjustedAutoScrollSpeed < 0)
        mAdjustedAutoScrollSpeed = 1;

    if (mAdjustedAutoScrollSpeed != 0) {
        mAutoScrollAccumulator += deltaTime;
        while (mAutoScrollAccumulator >=
               static_cast<int>(rowModifier * static_cast<float>(mAdjustedAutoScrollSpeed))) {
            if (contentSize.y > mSize.y)
                mScrollPos += mScrollDir;
            mAutoScrollAccumulator -=
                static_cast<int>(rowModifier * static_cast<float>(mAdjustedAutoScrollSpeed));
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
            float maxOpacity {static_cast<float>(mChildren.front()->getColor() & 0x000000FF) /
                              255.0f};
            auto func = [this, maxOpacity](float t) {
                unsigned int color {mChildren.front()->getColor()};
                float opacity {glm::mix(0.0f, maxOpacity, t)};
                color = (color & 0xFFFFFF00) + static_cast<unsigned char>(opacity * 255.0f);
                this->mChildren.front()->setColor(color);
                mScrollPos = glm::vec2 {};
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
    if (!isVisible() || mThemeOpacity == 0.0f || mChildren.front()->getValue() == "")
        return;

    glm::mat4 trans {parentTrans * getTransform()};

    glm::ivec2 clipPos {static_cast<int>(trans[3].x), static_cast<int>(trans[3].y)};

    glm::vec3 dimScaled {};
    dimScaled.x = std::fabs(trans[3].x + mSize.x);
    dimScaled.y = std::fabs(trans[3].y + mSize.y);

    glm::ivec2 clipDim {static_cast<int>(ceilf(dimScaled.x - trans[3].x)),
                        static_cast<int>(ceilf(dimScaled.y - trans[3].y))};

    // By effectively clipping the upper and lower boundaries of the container we mostly avoid
    // scrolling outside the vertical starting and ending positions.
    clipPos.y += static_cast<int>(mClipSpacing);
    clipDim.y -= static_cast<int>(mClipSpacing);

    Renderer::pushClipRect(clipPos, clipDim);

    trans = glm::translate(trans, -glm::vec3 {mScrollPos.x, mScrollPos.y, 0.0f});
    Renderer::setMatrix(trans);

    GuiComponent::renderChildren(trans);
    Renderer::popClipRect();
}
