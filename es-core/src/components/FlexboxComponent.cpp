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

FlexboxComponent::FlexboxComponent(Window* window, unsigned int assumeChildren)
    : GuiComponent(window)
    , mDirection(DEFAULT_DIRECTION)
    , mWrap(DEFAULT_WRAP)
    , mJustifyContent(DEFAULT_JUSTIFY_CONTENT)
    , mAlign(DEFAULT_ALIGN)
    , mAssumeChildren(assumeChildren)
{

    // Initialize contents of the flexbox.
    mSlots = std::vector<std::string>();
    mComponents = std::map<std::string, GuiComponent>();

    // Initialize flexbox layout.
    mVertices = std::map<std::string, glm::vec4>();

    // TODO: Should be dependent on the direction property.
    mSize = glm::vec2{64.0f * mAssumeChildren, 64.0f};

    // TODO: Add definition for default value.
    mMargin = glm::vec2{10.0f, 10.0f};

    // Calculate flexbox layout.
    updateVertices();
}

void FlexboxComponent::onSizeChanged()
{
    // TODO: Should be dependent on the direction property.
    if (mSize.y == 0.0f)
        mSize.y = mSize.x / mAssumeChildren;
    else if (mSize.x == 0.0f)
        mSize.x = mSize.y * mAssumeChildren;

    updateVertices();
}

void FlexboxComponent::updateVertices()
{
    // The maximum number of components to be displayed.
    const float numSlots = mAssumeChildren;

    // The available size to draw in.
    const auto size = getSize();

    // Compute the number of rows and columns and the item max dimensions.
    int rows;
    int columns;
    float itemWidth;
    float itemHeight;
    if (mDirection == DIRECTION_ROW) {
        if (mWrap != WRAP_NOWRAP) {
            // Suppose we have i rows, what would be the average area of an icon? Compute for a
            // small number of rows.
            std::vector<float> areas;
            for (int i = 1; i < 10; i++) {

                float area = size.x * size.y;

                // Number of vertical gaps.
                int verticalGaps = i - 1;

                // Area of vertical gaps.
                area -= verticalGaps * mMargin.y * size.x;

                // Height per item.
                float iHeight = (size.y - verticalGaps * mMargin.y) / i;

                // Width per item. (Approximation)
                // TODO: this is an approximation!
                // Solve: area - (iHeight * (iWidth + mMargin.x) * numSlots) + mMargin.x * iHeight =
                // 0;
                float iWidth = ((area + mMargin.x * iHeight) / (iHeight * numSlots)) - mMargin.x;

                // Average area available per badge
                float avgArea = iHeight * iWidth;

                // Push to the areas array.
                areas.push_back(avgArea);
            }

            // Determine the number of rows based on what results in the largest area per badge
            // based on available space.
            rows = std::max_element(areas.begin(), areas.end()) - areas.begin() + 1;

            // Obtain final item dimensions.
            itemHeight = (size.y - (rows - 1) * mMargin.y) / rows;
            itemWidth = areas[rows - 1] / itemHeight;

            // Compute number of columns.
            if (rows == 1)
                columns = mAssumeChildren;
            else
                columns = std::round((size.x + mMargin.x) / (itemWidth + mMargin.x));
        }
        else {
            rows = 1;
            columns = mAssumeChildren;
            itemHeight = size.y;
            itemWidth = size.x / (mAssumeChildren + (mAssumeChildren - 1) * mMargin.x);
        }
    }
    else {
        // TODO: Add computation for column direction.
    }

    // Compute the exact positions and sizes of the components.
    mVertices.clear();
    if (mDirection == DIRECTION_ROW) {

        // Start row.
        int row = mWrap == WRAP_REVERSE ? rows : 1;
        int item = 0;

        // Iterate through all the rows.
        for (int c = 0; c < rows && item < mSlots.size(); c++) {

            // Pre-compute dimensions of all items in this row.
            std::vector<float> widths;
            std::vector<float> heights;
            int itemTemp = item;
            for (int column = 0; column < columns && itemTemp < mSlots.size(); column++) {
                glm::vec componentSize = mComponents.find(mSlots[itemTemp])->second.getSize();
                float aspectRatioTexture = componentSize.x / componentSize.y;
                float aspectRatioItemSpace = itemWidth / itemHeight;
                if (aspectRatioTexture > aspectRatioItemSpace) {
                    widths.push_back(itemWidth);
                    heights.push_back(itemWidth / aspectRatioTexture);
                }
                else {
                    widths.push_back(itemHeight * aspectRatioTexture);
                    heights.push_back(itemHeight);
                }
                itemTemp++;
            }

            // Iterate through the columns.
            float xpos = 0;
            for (int column = 0; column < columns && item < mSlots.size(); column++) {

                // We always go from left to right.
                // Here we compute the coordinates of the items.

                // Compute final badge x position.
                float x;
                float totalWidth =
                    std::accumulate(widths.begin(), widths.end(), decltype(widths)::value_type(0)) +
                    (widths.size() - 1) * mMargin.x;
                if (mJustifyContent == "start") {
                    x = xpos;
                    xpos += widths[column] + mMargin.x;
                }
                else if (mJustifyContent == "end") {
                    if (column == 0)
                        xpos += size.x - totalWidth;
                    x = xpos;
                    xpos += widths[column] + mMargin.x;
                }
                else if (mJustifyContent == "center") {
                    if (column == 0)
                        xpos += (size.x - totalWidth) / 2;
                    x = xpos;
                    xpos += widths[column] + mMargin.x;
                }
                else if (mJustifyContent == "space-between") {
                    float gapSize = (size.x - totalWidth) / (widths.size() - 1);
                    x = xpos;
                    xpos += widths[column] + gapSize;
                }
                else if (mJustifyContent == "space-around") {
                    float gapSize = (size.x - totalWidth) / (widths.size() - 1);
                    xpos += gapSize / 2;
                    x = xpos;
                    xpos += widths[column] + gapSize / 2;
                }
                else if (mJustifyContent == "space-evenly") {
                    float gapSize = (size.x - totalWidth) / (widths.size() + 1);
                    xpos += gapSize;
                    x = xpos;
                }

                // Compute final badge y position.
                float y = row * itemHeight;
                if (mAlign == "end") {
                    y += itemHeight - heights[column];
                }
                else if (mAlign == "center") {
                    y += (itemHeight - heights[column]) / 2;
                }
                if (mAlign == "stretch") {
                    heights[column] = itemHeight;
                }

                LOG(LogError) << "Computed Final Item Position. Row: " << row
                              << ", Column: " << column << ", Item: " << item << ", pos: (" << x
                              << ", " << y << "), size: (" << widths[column] << ", "
                              << heights[column] << ")";

                // Store the item's layout.
                mVertices[mSlots[item]] = {x, y, widths[column], heights[column]};

                // Increment item;
                item++;
            }

            // Iterate the row.
            mWrap == WRAP_REVERSE ? row-- : row++;
        }
    }
}

void FlexboxComponent::render(const glm::mat4& parentTrans)
{
    if (!isVisible())
        return;

    // Render all the child components.
    for (unsigned int i = 0; i < mSlots.size(); i++) {
        glm::vec4 v = mVertices[mSlots[i]];
        auto c = mComponents.find(mSlots[i])->second;
        glm::vec2 oldSize = c.getSize();
        c.setPosition(v.x, v.y);
        c.setSize(v.z, v.w);
        c.render(parentTrans);
        c.setSize(oldSize);
    }

    renderChildren(parentTrans);
}

void FlexboxComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                  const std::string& view,
                                  const std::string& element,
                                  unsigned int properties)
{
    using namespace ThemeFlags;

    // TODO: How to do this without explicit 'badges' property?
    const ThemeData::ThemeElement* elem = theme->getElement(view, element, "badges");
    if (!elem)
        return;

    if (properties & DIRECTION && elem->has("direction"))
        mDirection = elem->get<std::string>("direction");

    if (elem->has("wrap"))
        mWrap = elem->get<std::string>("wrap");

    if (elem->has("justifyContent"))
        mJustifyContent = elem->get<std::string>("justifyContent");

    if (elem->has("align"))
        mAlign = elem->get<std::string>("align");

    GuiComponent::applyTheme(theme, view, element, properties);

    // Trigger layout computation.
    onSizeChanged();
}

std::vector<HelpPrompt> FlexboxComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    return prompts;
}
