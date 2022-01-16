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

FlexboxComponent::FlexboxComponent(Window* window, std::vector<FlexboxItem>& items)
    : GuiComponent {window}
    , mItems {items}
    , mDirection {DEFAULT_DIRECTION}
    , mAlignment {DEFAULT_ALIGNMENT}
    , mLines {DEFAULT_LINES}
    , mItemsPerLine {DEFAULT_ITEMS_PER_LINE}
    , mItemPlacement {DEFAULT_ITEM_PLACEMENT}
    , mItemMargin {glm::vec2 {DEFAULT_MARGIN_X, DEFAULT_MARGIN_Y}}
    , mOverlayPosition {0.5f, 0.5f}
    , mOverlaySize {0.5f}
    , mLayoutValid {false}
{
}

void FlexboxComponent::render(const glm::mat4& parentTrans)
{
    if (!isVisible())
        return;

    if (!mLayoutValid)
        computeLayout();

    glm::mat4 trans {parentTrans * getTransform()};
    Renderer::setMatrix(trans);

    if (Settings::getInstance()->getBool("DebugImage"))
        Renderer::drawRect(0.0f, 0.0f, ceilf(mSize.x), ceilf(mSize.y), 0xFF000033, 0xFF000033);

    for (auto& item : mItems) {
        if (!item.visible)
            continue;
        if (mOpacity == 255) {
            item.baseImage.render(trans);
            if (item.overlayImage.getTexture() != nullptr)
                item.overlayImage.render(trans);
        }
        else {
            item.baseImage.setOpacity(mOpacity);
            item.baseImage.render(trans);
            item.baseImage.setOpacity(255);
            if (item.overlayImage.getTexture() != nullptr) {
                item.overlayImage.setOpacity(mOpacity);
                item.overlayImage.render(trans);
                item.overlayImage.setOpacity(255);
            }
        }
    }
}

void FlexboxComponent::setItemMargin(glm::vec2 value)
{
    if (value.x == -1.0f)
        mItemMargin.x = std::roundf(value.y * Renderer::getScreenHeight());
    else
        mItemMargin.x = std::roundf(value.x * Renderer::getScreenWidth());

    if (value.y == -1.0f)
        mItemMargin.y = std::roundf(value.x * Renderer::getScreenWidth());
    else
        mItemMargin.y = std::roundf(value.y * Renderer::getScreenHeight());

    mLayoutValid = false;
}

void FlexboxComponent::computeLayout()
{
    // If we're not clamping itemMargin to a reasonable value, all kinds of weird rendering
    // issues could occur.
    mItemMargin.x = glm::clamp(mItemMargin.x, 0.0f, mSize.x / 2.0f);
    mItemMargin.y = glm::clamp(mItemMargin.y, 0.0f, mSize.y / 2.0f);

    // Also keep the size within reason.
    mSize.x = glm::clamp(mSize.x, static_cast<float>(Renderer::getScreenWidth()) * 0.03f,
                         static_cast<float>(Renderer::getScreenWidth()));
    mSize.y = glm::clamp(mSize.y, static_cast<float>(Renderer::getScreenHeight()) * 0.03f,
                         static_cast<float>(Renderer::getScreenHeight()));

    if (mItemsPerLine * mLines < mItems.size()) {
        LOG(LogWarning)
            << "FlexboxComponent: Invalid theme configuration, the number of badges"
               " exceeds the product of <lines> times <itemsPerLine>, setting <itemsPerLine> to "
            << mItems.size();
        mItemsPerLine = static_cast<unsigned int>(mItems.size());
    }

    glm::vec2 grid {};

    if (mDirection == "row")
        grid = {mItemsPerLine, mLines};
    else
        grid = {mLines, mItemsPerLine};

    glm::vec2 maxItemSize {(mSize + mItemMargin - grid * mItemMargin) / grid};

    float rowHeight {0.0f};
    bool firstItem {true};

    // Calculate maximum item dimensions.
    for (auto& item : mItems) {
        if (!item.visible)
            continue;

        glm::vec2 sizeDiff {item.baseImage.getSize() / maxItemSize};

        // The first item dictates the maximum width for the rest.
        if (firstItem) {
            maxItemSize.x = (item.baseImage.getSize() / std::max(sizeDiff.x, sizeDiff.y)).x;
            sizeDiff = item.baseImage.getSize() / maxItemSize;
            item.baseImage.setSize((item.baseImage.getSize() / std::max(sizeDiff.x, sizeDiff.y)));
            firstItem = false;
        }
        else {
            item.baseImage.setSize((item.baseImage.getSize() / std::max(sizeDiff.x, sizeDiff.y)));
        }

        if (item.baseImage.getSize().y > rowHeight)
            rowHeight = item.baseImage.getSize().y;
    }

    // Update the maximum item height.
    maxItemSize.y = 0.0f;
    for (auto& item : mItems) {
        if (!item.visible)
            continue;
        if (item.baseImage.getSize().y > maxItemSize.y)
            maxItemSize.y = item.baseImage.getSize().y;
    }

    maxItemSize = glm::round(maxItemSize);

    bool alignRight {mAlignment == "right"};
    float alignRightComp {0.0f};

    // If right-aligning, move the overall container contents during grid setup.
    if (alignRight && mDirection == "row")
        alignRightComp =
            std::round(mSize.x - ((maxItemSize.x + mItemMargin.x) * grid.x) + mItemMargin.x);

    std::vector<glm::vec2> itemPositions;

    // Lay out the grid.
    if (mDirection == "row") {
        for (int y = 0; y < grid.y; ++y) {
            for (int x = 0; x < grid.x; ++x) {
                itemPositions.emplace_back(
                    glm::vec2 {(x * (maxItemSize.x + mItemMargin.x) + alignRightComp),
                               y * (rowHeight + mItemMargin.y)});
            }
        }
    }
    else if (mDirection == "column" && !alignRight) {
        for (int x = 0; x < grid.x; ++x) {
            for (int y = 0; y < grid.y; ++y) {
                itemPositions.emplace_back(glm::vec2 {(x * (maxItemSize.x + mItemMargin.x)),
                                                      y * (rowHeight + mItemMargin.y)});
            }
        }
    }
    else { // Right-aligned.
        for (int x = 0; x < grid.x; ++x) {
            for (int y = 0; y < grid.y; ++y) {
                itemPositions.emplace_back(
                    glm::vec2 {(mSize.x - (x * (maxItemSize.x + mItemMargin.x)) - maxItemSize.x),
                               y * (rowHeight + mItemMargin.y)});
            }
        }
    }

    int pos {0};
    float lastY {0.0f};
    float itemsOnLastRow {0};

    // Position items on the grid.
    for (auto& item : mItems) {
        if (!item.visible)
            continue;

        if (mDirection == "row" && pos > 0) {
            if (itemPositions[pos - 1].y < itemPositions[pos].y) {
                lastY = itemPositions[pos].y;
                itemsOnLastRow = 0;
            }
        }

        float verticalOffset {0.0f};

        // For any items that do not fill the maximum height, position these either on
        // top/start (implicit), center or bottom/end.
        if (item.baseImage.getSize().y < maxItemSize.y) {
            if (mItemPlacement == "center") {
                verticalOffset = std::floor((maxItemSize.y - item.baseImage.getSize().y) / 2.0f);
            }
            else if (mItemPlacement == "end") {
                verticalOffset = maxItemSize.y - item.baseImage.getSize().y;
            }
        }

        item.baseImage.setPosition(itemPositions[pos].x, itemPositions[pos].y + verticalOffset,
                                   0.0f);

        // Optional overlay image.
        if (item.overlayImage.getTexture() != nullptr) {
            item.overlayImage.setResize(item.baseImage.getSize().x * mOverlaySize, 0.0f);
            item.overlayImage.setPosition(
                item.baseImage.getPosition().x + (item.baseImage.getSize().x * mOverlayPosition.x) -
                    item.overlayImage.getSize().x / 2.0f,
                item.baseImage.getPosition().y + (item.baseImage.getSize().y * mOverlayPosition.y) -
                    item.overlayImage.getSize().y / 2.0f);
        }

        // This rasterizes the SVG images so they look nice and smooth.
        item.baseImage.setResize(item.baseImage.getSize());

        ++itemsOnLastRow;
        ++pos;
    }

    // Apply right-align to the items if we're using row mode.
    if (alignRight && mDirection == "row") {
        for (auto& item : mItems) {
            if (!item.visible)
                continue;
            glm::vec3 currPos {item.baseImage.getPosition()};
            if (currPos.y == lastY) {
                const float offset {(grid.x - itemsOnLastRow) * (maxItemSize.x + mItemMargin.x)};
                item.baseImage.setPosition(currPos.x + offset, currPos.y, currPos.z);
                if (item.overlayImage.getTexture() != nullptr) {
                    currPos = item.overlayImage.getPosition();
                    item.overlayImage.setPosition(currPos.x + offset, currPos.y, currPos.z);
                }
            }
        }
    }

    mLayoutValid = true;
}
