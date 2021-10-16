//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  FlexboxComponent.cpp
//
//  Flexbox layout component.
//

#define DEFAULT_DIRECTION "row"
#define DEFAULT_ALIGNMENT "left"
#define DEFAULT_ITEMS_PER_LINE 4
#define DEFAULT_LINES 2
#define DEFAULT_ITEM_PLACEMENT "center"
#define DEFAULT_MARGIN_X std::roundf(0.01f * Renderer::getScreenWidth())
#define DEFAULT_MARGIN_Y std::roundf(0.01f * Renderer::getScreenHeight())

#include "components/FlexboxComponent.h"

#include "Settings.h"
#include "ThemeData.h"

FlexboxComponent::FlexboxComponent(Window* window,
                                   std::vector<std::pair<std::string, ImageComponent>>& images)
    : GuiComponent{window}
    , mImages(images)
    , mDirection{DEFAULT_DIRECTION}
    , mAlignment{DEFAULT_ALIGNMENT}
    , mItemsPerLine{DEFAULT_ITEMS_PER_LINE}
    , mLines{DEFAULT_LINES}
    , mItemPlacement{DEFAULT_ITEM_PLACEMENT}
    , mItemMargin{glm::vec2{DEFAULT_MARGIN_X, DEFAULT_MARGIN_Y}}
    , mLayoutValid{false}
{
}

void FlexboxComponent::render(const glm::mat4& parentTrans)
{
    if (!isVisible())
        return;

    if (!mLayoutValid)
        computeLayout();

    glm::mat4 trans{parentTrans * getTransform()};
    Renderer::setMatrix(trans);

    if (Settings::getInstance()->getBool("DebugImage"))
        Renderer::drawRect(0.0f, 0.0f, mSize.x, mSize.y, 0xFF000033, 0xFF000033);

    for (auto& image : mImages) {
        if (mOpacity == 255) {
            image.second.render(trans);
        }
        else {
            image.second.setOpacity(mOpacity);
            image.second.render(trans);
            image.second.setOpacity(255);
        }
    }
}

void FlexboxComponent::computeLayout()
{
    // Start placing items in the top-left.
    float anchorX{0.0f};
    float anchorY{0.0f};

    // Translation directions when placing items.
    glm::vec2 directionLine{1, 0};
    glm::vec2 directionRow{0, 1};

    // Change direction.
    if (mDirection == "column") {
        directionLine = {0, 1};
        directionRow = {1, 0};
    }

    // If we're not clamping itemMargin to a reasonable value, all kinds of weird rendering
    // issues could occur.
    mItemMargin.x = glm::clamp(mItemMargin.x, 0.0f, mSize.x / 2.0f);
    mItemMargin.y = glm::clamp(mItemMargin.y, 0.0f, mSize.y / 2.0f);

    // Also keep the size within reason.
    mSize.x = glm::clamp(mSize.x, static_cast<float>(Renderer::getScreenWidth()) * 0.03f,
                         static_cast<float>(Renderer::getScreenWidth()));
    mSize.y = glm::clamp(mSize.y, static_cast<float>(Renderer::getScreenHeight()) * 0.03f,
                         static_cast<float>(Renderer::getScreenHeight()));

    // Compute maximum image dimensions.
    glm::vec2 grid;
    if (mDirection == "row")
        grid = {mItemsPerLine, mLines};
    else
        grid = {mLines, mItemsPerLine};

    glm::vec2 maxItemSize{(mSize + mItemMargin - grid * mItemMargin) / grid};
    maxItemSize.x = floorf(maxItemSize.x);
    maxItemSize.y = floorf(maxItemSize.y);

    if (grid.x * grid.y < static_cast<float>(mImages.size())) {
        LOG(LogWarning) << "FlexboxComponent: Invalid theme configuration, the number of badges "
                           "exceeds the product of <lines> times <itemsPerLine>";
    }

    glm::vec2 sizeChange{0.0f, 0.0f};

    // Set final image dimensions.
    for (auto& image : mImages) {
        if (!image.second.isVisible())
            continue;
        auto oldSize{image.second.getSize()};
        if (oldSize.x == 0)
            oldSize.x = maxItemSize.x;
        glm::vec2 sizeMaxX{maxItemSize.x, oldSize.y * (maxItemSize.x / oldSize.x)};
        glm::vec2 sizeMaxY{oldSize.x * (maxItemSize.y / oldSize.y), maxItemSize.y};
        glm::vec2 newSize;
        if (sizeMaxX.y > maxItemSize.y)
            newSize = sizeMaxY;
        else if (sizeMaxY.x > maxItemSize.x)
            newSize = sizeMaxX;
        else
            newSize = sizeMaxX.x * sizeMaxX.y >= sizeMaxY.x * sizeMaxY.y ? sizeMaxX : sizeMaxY;

        if (image.second.getSize() != newSize)
            image.second.setResize(newSize.x, newSize.y);

        // In case maxItemSize needs to be updated.
        if (newSize.x != sizeChange.x)
            sizeChange.x = newSize.x;
        if (newSize.y != sizeChange.y)
            sizeChange.y = newSize.y;
    }

    if (maxItemSize.x != sizeChange.x)
        maxItemSize.x = sizeChange.x;
    if (maxItemSize.y != sizeChange.y)
        maxItemSize.y = sizeChange.y;

    // Pre-compute layout parameters.
    float anchorXStart{anchorX};
    float anchorYStart{anchorY};

    int i = 0;

    // Iterate through the images.
    for (auto& image : mImages) {
        if (!image.second.isVisible())
            continue;

        auto size{image.second.getSize()};

        // Top-left anchor position.
        float x{anchorX};
        float y{anchorY};

        // Apply alignment.
        if (mItemPlacement == "end") {
            x += directionLine.x == 0 ? (maxItemSize.x - size.x) : 0;
            y += directionLine.y == 0 ? (maxItemSize.y - size.y) : 0;
        }
        else if (mItemPlacement == "center") {
            x += directionLine.x == 0 ? (maxItemSize.x - size.x) / 2 : 0;
            y += directionLine.y == 0 ? (maxItemSize.y - size.y) / 2 : 0;
        }
        else if (mItemPlacement == "stretch" && mDirection == "row") {
            image.second.setSize(image.second.getSize().x, maxItemSize.y);
        }

        // Apply overall container alignment.
        if (mAlignment == "right")
            x += (mSize.x - maxItemSize.x * grid.x - (grid.x - 1) * mItemMargin.x);

        // Store final item position.
        image.second.setPosition(x, y);

        // Translate anchor.
        if ((i++ + 1) % std::max(1, static_cast<int>(mItemsPerLine)) != 0) {
            // Translate on same line.
            anchorX += (size.x + mItemMargin.x) * static_cast<float>(directionLine.x);
            anchorY += (size.y + mItemMargin.y) * static_cast<float>(directionLine.y);
        }
        else {
            // Translate to first position of next line.
            if (directionRow.x == 0) {
                anchorY += size.y + mItemMargin.y;
                anchorX = anchorXStart;
            }
            else {
                anchorX += size.x + mItemMargin.x;
                anchorY = anchorYStart;
            }
        }
    }

    // Apply right-align
    if (mAlignment == "right") {
        unsigned int m = i % std::max(1, static_cast<int>(mItemsPerLine));
        unsigned int n = m > 0 ? mItemsPerLine - m : m;
        i = 0;
        unsigned int line = 1;
        for (auto& image : mImages) {
            if (!image.second.isVisible())
                continue;
            if (line == mLines)
                image.second.setPosition(
                    image.second.getPosition().x +
                        floorf((maxItemSize.x + mItemMargin.x) * static_cast<float>(n)),
                    image.second.getPosition().y);
            if ((i++ + 1) % std::max(1, static_cast<int>(mItemsPerLine)) == 0)
                line++;
        }
    }

    mLayoutValid = true;
}
