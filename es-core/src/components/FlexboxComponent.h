//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  FlexboxComponent.h
//
//  Flexbox layout component.
//  Used by gamelist views.
//

#ifndef ES_APP_COMPONENTS_FLEXBOX_COMPONENT_H
#define ES_APP_COMPONENTS_FLEXBOX_COMPONENT_H

#include "GuiComponent.h"
#include "renderers/Renderer.h"

// Definitions for the option values.
#define DIRECTION_ROW "row"
#define DIRECTION_COLUMN "column"
#define ITEM_ALIGN_START "start"
#define ITEM_ALIGN_END "end"
#define ITEM_ALIGN_CENTER "center"
#define ITEM_ALIGN_STRETCH "stretch"

// Default values.
#define DEFAULT_DIRECTION DIRECTION_ROW
#define DEFAULT_ALIGN ITEM_ALIGN_CENTER
#define DEFAULT_ITEMS_PER_LINE 4
#define DEFAULT_MARGIN_X 10.0f
#define DEFAULT_MARGIN_Y 10.0f
#define DEFAULT_ITEM_SIZE_X 64.0f

class TextureResource;

class FlexboxComponent : public GuiComponent
{
public:
    FlexboxComponent(Window* window);

    // Getters/Setters for rendering options.
    void setDirection(std::string value);
    std::string getDirection();
    void setAlign(std::string value);
    std::string getAlign();
    void setItemsPerLine(unsigned int value);
    unsigned int getItemsPerLine();
    void setItemMargin(glm::vec2 value);
    glm::vec2 getItemMargin();
    void setItemWidth(float value);
    float getItemWidth();

    void onSizeChanged() override;
    void render(const glm::mat4& parentTrans) override;
    virtual void applyTheme(const std::shared_ptr<ThemeData>& theme,
                            const std::string& view,
                            const std::string& element,
                            unsigned int properties) override;
    virtual std::vector<HelpPrompt> getHelpPrompts() override;

private:
    // Calculate flexbox layout.
    void computeLayout();

    // Rendering options.
    std::string mDirection;
    std::string mAlign;
    unsigned int mItemsPerLine;
    glm::vec2 mItemMargin;
    float mItemWidth;
    bool mLayoutValid;
};

#endif // ES_APP_COMPONENTS_FLEXBOX_COMPONENT_H
