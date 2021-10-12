//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  FlexboxComponent.h
//
//  Flexbox layout component.
//

#ifndef ES_CORE_COMPONENTS_FLEXBOX_COMPONENT_H
#define ES_CORE_COMPONENTS_FLEXBOX_COMPONENT_H

#include "GuiComponent.h"
#include "components/ImageComponent.h"

class FlexboxComponent : public GuiComponent
{
public:
    explicit FlexboxComponent(Window* window,
                              std::vector<std::pair<std::string, ImageComponent>>& images);

    // Getters/setters for the layout.
    void setDirection(const std::string& direction)
    {
        assert(direction == "row" || direction == "column");
        mDirection = direction;
    }

    std::string getAlignment() const { return mAlignment; }
    void setAlignment(const std::string& value)
    {
        assert(value == "left" || value == "right");
        mAlignment = value;
        mLayoutValid = false;
    }

    unsigned int getItemsPerLine() const { return mItemsPerLine; }
    void setItemsPerLine(unsigned int value)
    {
        mItemsPerLine = value;
        mLayoutValid = false;
    }

    unsigned int getLines() const { return mLines; }
    void setLines(unsigned int value)
    {
        mLines = value;
        mLayoutValid = false;
    }

    std::string getItemPlacement() const { return mItemPlacement; }
    void setItemPlacement(const std::string& value)
    {
        assert(value == "start" || value == "center" || value == "end" || value == "stretch");
        mItemPlacement = value;
        mLayoutValid = false;
    }

    glm::vec2 getItemMargin() const { return mItemMargin; }
    void setItemMargin(glm::vec2 value)
    {
        mItemMargin.x = std::roundf(value.x * Renderer::getScreenWidth());
        mItemMargin.y = std::roundf(value.y * Renderer::getScreenHeight());
        mLayoutValid = false;
    }

    void onSizeChanged() override { mLayoutValid = false; }
    void render(const glm::mat4& parentTrans) override;

private:
    // Calculate flexbox layout.
    void computeLayout();

    std::vector<std::pair<std::string, ImageComponent>>& mImages;

    // Layout options.
    std::string mDirection;
    std::string mAlignment;
    unsigned int mItemsPerLine;
    unsigned int mLines;
    std::string mItemPlacement;
    glm::vec2 mItemMargin;

    bool mLayoutValid;
};

#endif // ES_CORE_COMPONENTS_FLEXBOX_COMPONENT_H
