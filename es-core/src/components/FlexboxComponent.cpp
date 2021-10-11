//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  FlexboxComponent.cpp
//
//  Flexbox layout component.
//  Used by gamelist views.
//

#define DEFAULT_DIRECTION Direction::row
#define DEFAULT_ALIGN Align::center
#define DEFAULT_ITEMS_PER_LINE 4
#define DEFAULT_LINES 1
#define DEFAULT_MARGIN_X 10.0f
#define DEFAULT_MARGIN_Y 10.0f

#include "components/FlexboxComponent.h"

#include "ThemeData.h"

FlexboxComponent::FlexboxComponent(Window* window,
                                   std::vector<std::pair<std::string, ImageComponent>>& images)
    : GuiComponent{window}
    , mDirection{DEFAULT_DIRECTION}
    , mAlign{DEFAULT_ALIGN}
    , mImages(images)
    , mItemsPerLine{DEFAULT_ITEMS_PER_LINE}
    , mLines{DEFAULT_LINES}
    , mItemMargin{glm::vec2{DEFAULT_MARGIN_X, DEFAULT_MARGIN_Y}}
    , mLayoutValid{false}
{
}

void FlexboxComponent::computeLayout()
{
    // Start placing items in the top-left.
    float anchorX{0.0f};
    float anchorY{0.0f};
    float anchorOriginX{0.0f};
    float anchorOriginY{0.0f};

    // Translation directions when placing items.
    glm::ivec2 directionLine{1, 0};
    glm::ivec2 directionRow{0, 1};

    // Change direction.
    if (mDirection == Direction::column) {
        directionLine = {0, 1};
        directionRow = {1, 0};
    }

    // Compute maximum image dimensions.
    glm::vec2 grid;
    if (mDirection == Direction::row)
        grid = {mItemsPerLine, mLines};
    else
        grid = {mLines, mItemsPerLine};
    glm::vec2 maxItemSize{(mSize + mItemMargin - grid * mItemMargin) / grid};

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
        image.second.setResize(newSize.x, newSize.y);
    }

    // Pre-compute layout parameters.
    float lineWidth = (mDirection == Direction::row ? (maxItemSize.y + mItemMargin.y) :
                                                      (maxItemSize.x + mItemMargin.x));
    float anchorXStart{anchorX};
    float anchorYStart{anchorY};

    int i = 0;

    // Iterate through the images.
    for (auto& image : mImages) {
        if (!image.second.isVisible())
            continue;

        auto size{image.second.getSize()};

        // Top-left anchor position.
        float x{anchorX - anchorOriginX * size.x};
        float y{anchorY - anchorOriginY * size.y};

        // Apply alignment
        if (mAlign == Align::end) {
            x += directionLine.x == 0 ? (maxItemSize.x - size.x) : 0;
            y += directionLine.y == 0 ? (maxItemSize.y - size.y) : 0;
        }
        else if (mAlign == Align::center) {
            x += directionLine.x == 0 ? (maxItemSize.x - size.x) / 2 : 0;
            y += directionLine.y == 0 ? (maxItemSize.y - size.y) / 2 : 0;
        }
        else if (mAlign == Align::stretch && mDirection == Direction::row) {
            image.second.setSize(image.second.getSize().x, maxItemSize.y);
        }

        // Apply origin.
        if (mOrigin.x > 0 && mOrigin.x <= 1)
            x -= mOrigin.x * mSize.x;
        if (mOrigin.y > 0 && mOrigin.y <= 1)
            y -= mOrigin.y * mSize.y;

        // Store final item position.
        image.second.setPosition(getPosition().x + x, getPosition().y + y);

        // Translate anchor.
        if ((i++ + 1) % std::max(1, static_cast<int>(mItemsPerLine)) != 0) {
            // Translate on same line.
            anchorX += (size.x + mItemMargin.x) * static_cast<float>(directionLine.x);
            anchorY += (size.y + mItemMargin.y) * static_cast<float>(directionLine.y);
        }
        else {
            // Translate to first position of next line.
            if (directionRow.x == 0) {
                anchorY += lineWidth * static_cast<float>(directionRow.y);
                anchorX = anchorXStart;
            }
            else {
                anchorX += lineWidth * static_cast<float>(directionRow.x);
                anchorY = anchorYStart;
            }
        }
    }

    mLayoutValid = true;
}

void FlexboxComponent::render(const glm::mat4& parentTrans)
{
    if (!isVisible())
        return;

    if (!mLayoutValid)
        computeLayout();

    for (auto& image : mImages)
        image.second.render(parentTrans);
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

    const ThemeData::ThemeElement* elem = theme->getElement(view, element, "badges");
    if (!elem)
        return;

    if (properties & DIRECTION && elem->has("direction"))
        mDirection =
            elem->get<std::string>("direction") == "row" ? Direction::row : Direction::column;

    if (elem->has("align")) {
        const auto a = elem->get<std::string>("align");
        mAlign = (a == "start" ?
                      Align::start :
                      (a == "end" ? Align::end : (a == "center" ? Align::center : Align::stretch)));
    }

    if (elem->has("itemsPerLine"))
        mItemsPerLine = elem->get<float>("itemsPerLine");

    if (elem->has("lines"))
        mLines = elem->get<float>("lines");

    if (elem->has("itemMargin"))
        mItemMargin = elem->get<glm::vec2>("itemMargin") * scale;

    GuiComponent::applyTheme(theme, view, element, properties);

    // Layout no longer valid.
    mLayoutValid = false;
}
