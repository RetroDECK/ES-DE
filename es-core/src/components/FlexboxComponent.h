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
    struct FlexboxItem {
        // Optional label, mostly a convenience for the calling class to track items.
        std::string label;

        // Main image that governs grid sizing and placement.
        ImageComponent baseImage;

        // Optional overlay image that can be sized and positioned relative to the base image.
        ImageComponent overlayImage;
        glm::vec2 overlayPosition {0.5f, 0.5f};
        float overlaySize {0.5f};

        bool visible {false};
    };

    FlexboxComponent(std::vector<FlexboxItem>& items);

    // Getters/setters for the layout.
    const std::string& getDirection() const { return mDirection; }
    void setDirection(const std::string& direction)
    {
        assert(direction == "row" || direction == "column");
        mDirection = direction;
    }

    const std::string& getAlignment() const { return mAlignment; }
    void setAlignment(const std::string& value)
    {
        assert(value == "left" || value == "right" || value == "center");
        mAlignment = value;
        mLayoutValid = false;
    }

    unsigned int getLines() const { return mLines; }
    void setLines(unsigned int value)
    {
        mLines = value;
        mLayoutValid = false;
    }

    unsigned int getItemsPerLine() const { return mItemsPerLine; }
    void setItemsPerLine(unsigned int value)
    {
        mItemsPerLine = value;
        mLayoutValid = false;
    }

    const std::string& getItemPlacement() const { return mItemPlacement; }
    void setItemPlacement(const std::string& value)
    {
        assert(value == "start" || value == "center" || value == "end" || value == "stretch");
        mItemPlacement = value;
        mLayoutValid = false;
    }

    const glm::vec2& getItemMargin() const { return mItemMargin; }
    void setItemMargin(glm::vec2 value);

    void onSizeChanged() override { mLayoutValid = false; }
    void render(const glm::mat4& parentTrans) override;

private:
    // Calculate flexbox layout.
    void calculateLayout();

    Renderer* mRenderer;
    std::vector<FlexboxItem>& mItems;

    // Layout options.
    std::string mDirection;
    std::string mAlignment;
    unsigned int mLines;
    unsigned int mItemsPerLine;
    std::string mItemPlacement;
    glm::vec2 mItemMargin;

    bool mLayoutValid;
};

#endif // ES_CORE_COMPONENTS_FLEXBOX_COMPONENT_H
