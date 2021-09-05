//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  BadgesComponent.h
//
//  Game badges icons.
//  Used by gamelist views.
//

#ifndef ES_APP_COMPONENTS_BADGES_COMPONENT_H
#define ES_APP_COMPONENTS_BADGES_COMPONENT_H

#include "GuiComponent.h"
#include "renderers/Renderer.h"

#define DIRECTION_ROW "row"
#define DIRECTION_COLUMN "column"
#define WRAP_WRAP "wrap"
#define WRAP_NOWRAP "nowrap"
#define WRAP_REVERSE "wrap-reverse"
#define JUSTIFY_CONTENT_START "start"
#define JUSTIFY_CONTENT_END "end"
#define JUSTIFY_CONTENT_CENTER "center"
#define JUSTIFY_CONTENT_SPACE_BETWEEN "space-between"
#define JUSTIFY_CONTENT_SPACE_AROUND "space-around"
#define JUSTIFY_CONTENT_SPACE_EVENLY "space-evenly"
#define ITEM_ALIGN_START "start"
#define ITEM_ALIGN_END "end"
#define ITEM_ALIGN_CENTER "center"
#define ITEM_ALIGN_STRETCH "stretch"
#define NUM_SLOTS 4
#define SLOT_FAVORITE "favorite"
#define SLOT_COMPLETED "completed"
#define SLOT_KIDS "kidgame"
#define SLOT_BROKEN "broken"
#define DEFAULT_DIRECTION DIRECTION_ROW
#define DEFAULT_WRAP WRAP_WRAP
#define DEFAULT_JUSTIFY_CONTENT JUSTIFY_CONTENT_START
#define DEFAULT_ALIGN ITEM_ALIGN_CENTER
#define DEFAULT_MARGIN_X = 10.0f
#define DEFAULT_MARGIN_Y = 10.0f

class TextureResource;

class BadgesComponent : public GuiComponent
{
public:
    BadgesComponent(Window* window);

    std::string getValue() const override;
    // Should be a list of strings.
    void setValue(const std::string& value) override;

    void render(const glm::mat4& parentTrans) override;

    void onSizeChanged() override;

    void setDirection(int direction);

    int getDirection();

    void setSlots(std::vector<std::string>);

    std::vector<std::string> getSlots();

    virtual void applyTheme(const std::shared_ptr<ThemeData>& theme,
                            const std::string& view,
                            const std::string& element,
                            unsigned int properties) override;

    virtual std::vector<HelpPrompt> getHelpPrompts() override;

private:
    void updateVertices();
    std::map<std::string, Renderer::Vertex[4]> mVertices;

    std::map<std::string, std::string> mBadgeIcons;
    std::map<std::string, std::shared_ptr<TextureResource>> mTextures;

    std::string mDirection;
    std::string mWrap;
    std::string mJustifyContent;
    std::string mAlign;
    glm::vec2 mMargin;
    std::vector<std::string> mSlots;
};

#endif // ES_APP_COMPONENTS_BADGES_COMPONENT_H
