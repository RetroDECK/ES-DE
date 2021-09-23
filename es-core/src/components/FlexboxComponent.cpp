//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  FlexboxComponent.cpp
//
//  Flexbox layout component.
//  Used by gamelist views.
//

#include "components/FlexboxComponent.h"
#include <numeric>

#include "Settings.h"
#include "ThemeData.h"
#include "resources/TextureResource.h"

FlexboxComponent::FlexboxComponent(Window* window)
    : GuiComponent(window)
    , mDirection(DEFAULT_DIRECTION)
    , mAlign(DEFAULT_ALIGN)
    , mItemsPerLine(DEFAULT_ITEMS_PER_LINE)
    , mItemWidth(DEFAULT_ITEM_SIZE_X)
{
    // Initialize item margins.
    mItemMargin = glm::vec2{DEFAULT_MARGIN_X, DEFAULT_MARGIN_Y};

    // Layout validity
    mLayoutValid = false;
}

// Getters/Setters for rendering options.
void FlexboxComponent::setDirection(std::string value)
{
    mDirection = value;
    mLayoutValid = false;
}
std::string FlexboxComponent::getDirection() { return mDirection; }
void FlexboxComponent::setAlign(std::string value)
{
    mAlign = value;
    mLayoutValid = false;
}
std::string FlexboxComponent::getAlign() { return mAlign; }
void FlexboxComponent::setItemsPerLine(unsigned int value)
{
    mItemsPerLine = value;
    mLayoutValid = false;
}
unsigned int FlexboxComponent::getItemsPerLine() { return mItemsPerLine; }
void FlexboxComponent::setItemMargin(glm::vec2 value)
{
    mItemMargin = value;
    mLayoutValid = false;
}
glm::vec2 FlexboxComponent::getItemMargin() { return mItemMargin; }
void FlexboxComponent::setItemWidth(float value)
{
    mItemWidth = value;
    mLayoutValid = false;
}
float FlexboxComponent::getItemWidth() { return mItemWidth; }

void FlexboxComponent::onSizeChanged() {
    mLayoutValid = false;
}

void FlexboxComponent::computeLayout()
{
    // Start placing items in the top-left;
    float anchorX = 0;
    float anchorY = 0;
    float anchorOriginX = 0;
    float anchorOriginY = 0;

    // Translation directions when placing items.
    glm::vec2 directionLine = {1, 0};
    glm::vec2 directionRow = {0, 1};

    // Change direction.
    if (mDirection == DIRECTION_COLUMN) {
        directionLine = {0, 1};
        directionRow = {1, 0};
    }

    // Set children sizes.
    glm::vec2 maxItemSize = {0.0f, 0.0f};
    for (auto i : mChildren) {
        auto oldSize = i->getSize();
        if (oldSize.x == 0)
            oldSize.x = DEFAULT_ITEM_SIZE_X;
        glm::vec2 newSize = {mItemWidth, oldSize.y * (mItemWidth / oldSize.x)};
        i->setSize(newSize);
        maxItemSize = {std::max(maxItemSize.x, newSize.x), std::max(maxItemSize.y, newSize.y)};
    }

    // Pre-compute layout parameters;
    int n = mChildren.size();
    int nLines = std::max(1, (int)std::ceil(n / std::max(1, (int)mItemsPerLine)));
    float lineWidth =
        (mDirection == "row" ? (maxItemSize.y + mItemMargin.y) : (maxItemSize.x + mItemMargin.x));
    float anchorXStart = anchorX;
    float anchorYStart = anchorY;

    // Iterate through the children.
    for (int i = 0; i < n; i++) {
        GuiComponent* child = mChildren[i];
        auto size = child->getSize();

        // Top-left anchor position.
        float x = anchorX - anchorOriginX * size.x;
        float y = anchorY - anchorOriginY * size.y;

        // Apply item margin.
        x += mItemMargin.x * (directionLine.x >= 0.0f ? 1.0f : -1.0f);
        y += mItemMargin.y * (directionLine.y >= 0.0f ? 1.0f : -1.0f);

        // Apply alignment
        if (mAlign == ITEM_ALIGN_END) {
            x += directionLine.x == 0 ? (maxItemSize.x - size.x) : 0;
            y += directionLine.y == 0 ? (maxItemSize.y - size.y) : 0;
        }
        else if (mAlign == ITEM_ALIGN_CENTER) {
            x += directionLine.x == 0 ? (maxItemSize.x - size.x) / 2 : 0;
            y += directionLine.y == 0 ? (maxItemSize.y - size.y) / 2 : 0;
        }
        else if (mAlign == ITEM_ALIGN_STRETCH && mDirection == "row") {
            child->setSize(child->getSize().x, maxItemSize.y);
        }

        // Store final item position.
        child->setPosition(getPosition().x + x, getPosition().y + y);

        // Translate anchor.
        if ((i + 1) % std::max(1, (int)mItemsPerLine) != 0) {
            // Translate on same line.
            anchorX += (size.x + mItemMargin.x) * directionLine.x;
            anchorY += (size.y + mItemMargin.y) * directionLine.y;
        }
        else {
            // Translate to first position of next line.
            if (directionRow.x == 0) {
                anchorY += lineWidth * directionRow.y;
                anchorX = anchorXStart;
            } else {
                anchorX += lineWidth * directionRow.x;
                anchorY = anchorYStart;
            }
        }
    }

    mLayoutValid = true;
}

void FlexboxComponent::render(const glm::mat4& parentTrans) {
    if (!isVisible())
        return;

    if (!mLayoutValid)
        computeLayout();

    renderChildren(parentTrans);
}

void FlexboxComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                  const std::string& view,
                                  const std::string& element,
                                  unsigned int properties)
{
    using namespace ThemeFlags;

    glm::vec2 scale{getParent() ? getParent()->getSize() :
                                  glm::vec2{static_cast<float>(Renderer::getScreenWidth()),
                                            static_cast<float>(Renderer::getScreenHeight())}};

    // TODO: How to do this without explicit 'badges' property?
    const ThemeData::ThemeElement* elem = theme->getElement(view, element, "badges");
    if (!elem)
        return;

    if (properties & DIRECTION && elem->has("direction"))
        mDirection = elem->get<std::string>("direction");

    if (elem->has("align"))
        mAlign = elem->get<std::string>("align");

    if (elem->has("itemsPerLine"))
        mItemsPerLine = elem->get<float>("itemsPerLine");

    if (elem->has("itemMargin"))
        mItemMargin = elem->get<glm::vec2>("itemMargin");

    if (elem->has("itemWidth"))
        mItemWidth = elem->get<float>("itemWidth") * scale.x;

    GuiComponent::applyTheme(theme, view, element, properties);

    // Layout no longer valid.
    mLayoutValid = false;
}

std::vector<HelpPrompt> FlexboxComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    return prompts;
}
