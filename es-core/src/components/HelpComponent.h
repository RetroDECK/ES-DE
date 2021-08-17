//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  HelpComponent.h
//
//  Help information in icon and text pairs.
//

#ifndef ES_CORE_COMPONENTS_HELP_COMPONENT_H
#define ES_CORE_COMPONENTS_HELP_COMPONENT_H

#include "GuiComponent.h"
#include "HelpStyle.h"

class ComponentGrid;
class ImageComponent;
class TextureResource;

class HelpComponent : public GuiComponent
{
public:
    HelpComponent(Window* window);

    void assignIcons();

    void clearPrompts();
    void setPrompts(const std::vector<HelpPrompt>& prompts);

    void render(const glm::mat4& parent) override;
    void setOpacity(unsigned char opacity) override;

    void setStyle(const HelpStyle& style);

private:
    std::shared_ptr<TextureResource> getIconTexture(const char* name);
    std::map<std::string, std::shared_ptr<TextureResource>> mIconCache;

    std::shared_ptr<ComponentGrid> mGrid;
    void updateGrid();

    std::vector<HelpPrompt> mPrompts;
    HelpStyle mStyle;
};

#endif // ES_CORE_COMPONENTS_HELP_COMPONENT_H
