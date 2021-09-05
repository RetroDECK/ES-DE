//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  BadgesComponent.cpp
//
//  Game badges icons.
//  Used by gamelist views.
//

#include "components/BadgesComponent.h"
#include <numeric>

#include "Settings.h"
#include "ThemeData.h"
#include "resources/TextureResource.h"

BadgesComponent::BadgesComponent(Window* window)
    : GuiComponent(window)
    , mDirection(DEFAULT_DIRECTION)
    , mWrap(DEFAULT_WRAP)
    , mJustifyContent(DEFAULT_JUSTIFY_CONTENT)
    , mAlign(DEFAULT_ALIGN)
{
    mSlots = std::vector<std::string>();
    mSlots.push_back(SLOT_FAVORITE);
    mSlots.push_back(SLOT_COMPLETED);
    mSlots.push_back(SLOT_KIDS);
    mSlots.push_back(SLOT_BROKEN);

    mBadgeIcons = std::map<std::string, std::string>();
    mBadgeIcons[SLOT_FAVORITE] = ":/graphics/badge_favorite.png";
    mBadgeIcons[SLOT_COMPLETED] = ":/graphics/badge_completed.png";
    mBadgeIcons[SLOT_KIDS] = ":/graphics/badge_kidgame.png";
    mBadgeIcons[SLOT_BROKEN] = ":/graphics/badge_broken.png";

    mTextures = std::map<std::string, std::shared_ptr<TextureResource>>();
    mTextures[SLOT_FAVORITE] = TextureResource::get(mBadgeIcons[SLOT_FAVORITE], true);
    mTextures[SLOT_COMPLETED] = TextureResource::get(mBadgeIcons[SLOT_COMPLETED], true);
    mTextures[SLOT_KIDS] = TextureResource::get(mBadgeIcons[SLOT_KIDS], true);
    mTextures[SLOT_BROKEN] = TextureResource::get(mBadgeIcons[SLOT_BROKEN], true);

    mVertices = std::map<std::string, Renderer::Vertex[4]>();

    // TODO: Should be dependent on the direction property.
    mSize = glm::vec2{64.0f * NUM_SLOTS, 64.0f};

    // TODO: Add definition for default value.
    mMargin = glm::vec2{10.0f, 10.0f};

    updateVertices();
}

void BadgesComponent::setValue(const std::string& value)
{
    if (value.empty()) {
        mSlots.clear();
    }
    else {
        // Start by clearing the slots.
        mSlots.clear();

        // Interpret the value and iteratively fill mSlots. The value is a space separated list of
        // strings.
        std::string temp;
        std::istringstream ss(value);
        while (std::getline(ss, temp, ' ')) {
            if (!(temp == SLOT_FAVORITE || temp == SLOT_COMPLETED || temp == SLOT_KIDS ||
                  temp == SLOT_BROKEN))
                LOG(LogError) << "Badge slot '" << temp << "' is invalid.";
            else
                mSlots.push_back(temp);
        }
    }

    updateVertices();
}

std::string BadgesComponent::getValue() const
{
    std::stringstream ss;
    for (auto& slot : mSlots)
        ss << slot << ' ';
    std::string r = ss.str();
    r.pop_back();
    return r;
}

void BadgesComponent::onSizeChanged()
{
    // TODO: Should be dependent on the direction property.
    if (mSize.y == 0.0f)
        mSize.y = mSize.x / NUM_SLOTS;
    else if (mSize.x == 0.0f)
        mSize.x = mSize.y * NUM_SLOTS;

    if (mSize.y > 0.0f) {
        size_t heightPx = static_cast<size_t>(std::round(mSize.y));
        for (auto const& tex : mTextures)
            tex.second->rasterizeAt(heightPx, heightPx);
    }

    updateVertices();
}

void BadgesComponent::updateVertices()
{
    mVertices.clear();

    /*const float numSlots = mSlots.size();
    float s;
    if (mDirection == DIRECTION_ROW)
        s = std::min( getSize().x / numSlots, getSize().y );
    else
        s = std::min( getSize().y / numSlots, getSize().x );
    const long color = 4278190080;

    int i = 0;
    for (auto & slot : mSlots)
    {
        // clang-format off
        mVertices[slot][0] = {{0.0f, 0.0f}, {0.0f, 1.0f}, color};
        mVertices[slot][1] = {{0.0f, s},    {0.0f, 0.0f}, color};
        mVertices[slot][2] = {{s   , 0.0f}, {1.0f, 1.0f}, color};
        mVertices[slot][3] = {{s   , s},    {1.0f, 0.0f}, color};
        // clang-format on
        i++;
    }*/

    // The maximum number of badges to be displayed.
    const float numSlots = NUM_SLOTS;

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
                columns = NUM_SLOTS;
            else
                columns = std::round((size.x + mMargin.x) / (itemWidth + mMargin.x));
        }
        else {
            rows = 1;
            columns = NUM_SLOTS;
            itemHeight = size.y;
            itemWidth = size.x / (NUM_SLOTS + (NUM_SLOTS - 1) * mMargin.x);
        }
    }
    else {
        // TODO: Add computation for column direction.
    }

    const long color = 4278190080;
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
                glm::vec texSize = mTextures[mSlots[itemTemp]]->getSize();
                float aspectRatioTexture = texSize.x / texSize.y;
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

                // Store the item's vertices and apply texture mapping.
                // clang-format off
                mVertices[mSlots[item]][0] = {{x, y},                                       {0.0f, 1.0f}, color};
                mVertices[mSlots[item]][1] = {{x, y+heights[column]},                       {0.0f, 0.0f}, color};
                mVertices[mSlots[item]][2] = {{x+widths[column]   , y},                     {1.0f, 1.0f}, color};
                mVertices[mSlots[item]][3] = {{x+widths[column]   , y+heights[column]},     {1.0f, 0.0f}, color};
                // clang-format on

                // Increment item;
                item++;
            }

            // Iterate the row.
            mWrap == WRAP_REVERSE ? row-- : row++;
        }
    }
}

void BadgesComponent::render(const glm::mat4& parentTrans)
{
    if (!isVisible())
        return;

    glm::mat4 trans{parentTrans * getTransform()};

    Renderer::setMatrix(trans);

    if (mOpacity > 0) {
        if (Settings::getInstance()->getBool("DebugImage"))
            Renderer::drawRect(0.0f, 0.0f, mSize.x, mSize.y, 0xFF000033, 0xFF000033);

        for (auto& slot : mSlots) {
            if (mTextures[slot] == nullptr)
                continue;

            if (mTextures[slot]->bind()) {
                Renderer::drawTriangleStrips(mVertices[slot], 4);
                Renderer::bindTexture(0);
            }

            // TODO: update render matrix to position of next slot
            // trans = glm::translate(trans, {0.0f, 0.0f, 1.0f});
        }
    }

    renderChildren(trans);
}

void BadgesComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                 const std::string& view,
                                 const std::string& element,
                                 unsigned int properties)
{
    using namespace ThemeFlags;

    const ThemeData::ThemeElement* elem = theme->getElement(view, element, "badges");
    if (!elem)
        return;

    bool imgChanged = false;
    for (auto& slot : mSlots) {
        if (properties & PATH && elem->has(slot)) {
            mBadgeIcons[slot] = elem->get<std::string>(slot);
            mTextures[slot] = TextureResource::get(mBadgeIcons[slot], true);
            imgChanged = true;
        }
    }

    if (properties & DIRECTION && elem->has("direction"))
        mDirection = elem->get<std::string>("direction");

    if (elem->has("wrap"))
        mWrap = elem->get<std::string>("wrap");

    if (elem->has("justifyContent"))
        mJustifyContent = elem->get<std::string>("justifyContent");

    if (elem->has("align"))
        mAlign = elem->get<std::string>("align");

    if (elem->has("slots"))
        setValue(elem->get<std::string>("slots"));

    GuiComponent::applyTheme(theme, view, element, properties);

    if (imgChanged)
        onSizeChanged();
}

std::vector<HelpPrompt> BadgesComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    return prompts;
}
