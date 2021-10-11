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
#include "components/ImageComponent.h"

class FlexboxComponent : public GuiComponent
{
public:
    enum class Direction : char {
        row, // Replace with AllowShortEnumsOnASingleLine: false (clang-format >=11.0).
        column
    };

    enum class Align : char {
        start, // Replace with AllowShortEnumsOnASingleLine: false (clang-format >=11.0).
        end,
        center,
        stretch
    };

    explicit FlexboxComponent(Window* window,
                              std::vector<std::pair<std::string, ImageComponent>>& images);

    // Getters/Setters for rendering options.
    Align getAlign() const { return mAlign; }
    void setAlign(Align value)
    {
        mAlign = value;
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

    glm::vec2 getItemMargin() const { return mItemMargin; }
    void setItemMargin(glm::vec2 value)
    {
        mItemMargin = value;
        mLayoutValid = false;
    }

    void onSizeChanged() override { mLayoutValid = false; }
    void render(const glm::mat4& parentTrans) override;
    void applyTheme(const std::shared_ptr<ThemeData>& theme,
                    const std::string& view,
                    const std::string& element,
                    unsigned int properties) override;

private:
    // Calculate flexbox layout.
    void computeLayout();

    // Layout options.
    Direction mDirection;
    Align mAlign;

    std::vector<std::pair<std::string, ImageComponent>>& mImages;

    unsigned int mItemsPerLine;
    unsigned int mLines;

    glm::vec2 mItemMargin;
    bool mLayoutValid;
};

#endif // ES_CORE_COMPONENTS_FLEXBOX_COMPONENT_H
