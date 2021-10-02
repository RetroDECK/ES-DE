//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  FlexboxComponent.h
//
//  Flexbox layout component.
//  Used by gamelist views.
//

#ifndef ES_CORE_COMPONENTS_FLEXBOX_COMPONENT_H
#define ES_CORE_COMPONENTS_FLEXBOX_COMPONENT_H

#include "GuiComponent.h"
#include "renderers/Renderer.h"

// Default values.
#define DEFAULT_DIRECTION Direction::row
#define DEFAULT_ALIGN Align::center
#define DEFAULT_ITEMS_PER_LINE 4
#define DEFAULT_MARGIN_X 10.0f
#define DEFAULT_MARGIN_Y 10.0f
#define DEFAULT_ITEM_SIZE_X 64.0f

class FlexboxComponent : public GuiComponent
{
public:
    enum class Direction : char { row, column };
    enum class Align : char { start, end, center, stretch };

    explicit FlexboxComponent(Window* window);

    // Getters/Setters for rendering options.
    [[nodiscard]] Direction getDirection() const { return mDirection; };
    void setDirection(Direction value)
    {
        mDirection = value;
        mLayoutValid = false;
    };
    [[nodiscard]] Align getAlign() const { return mAlign; };
    void setAlign(Align value)
    {
        mAlign = value;
        mLayoutValid = false;
    };
    [[nodiscard]] unsigned int getItemsPerLine() const { return mItemsPerLine; };
    void setItemsPerLine(unsigned int value)
    {
        mItemsPerLine = value;
        mLayoutValid = false;
    };
    [[nodiscard]] glm::vec2 getItemMargin() const { return mItemMargin; };
    void setItemMargin(glm::vec2 value)
    {
        mItemMargin = value;
        mLayoutValid = false;
    };
    [[nodiscard]] float getItemWidth() const { return mItemWidth; };
    void setItemWidth(float value)
    {
        mItemWidth = value;
        mLayoutValid = false;
    };

    void onSizeChanged() override;
    void render(const glm::mat4& parentTrans) override;
    void applyTheme(const std::shared_ptr<ThemeData>& theme,
                    const std::string& view,
                    const std::string& element,
                    unsigned int properties) override;
    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    // Calculate flexbox layout.
    void computeLayout();

    // Rendering options.
    Direction mDirection;
    Align mAlign;
    unsigned int mItemsPerLine;
    glm::vec2 mItemMargin;
    float mItemWidth;
    bool mLayoutValid;
};

#endif // ES_CORE_COMPONENTS_FLEXBOX_COMPONENT_H
