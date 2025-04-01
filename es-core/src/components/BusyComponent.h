//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  BusyComponent.h
//
//  Animated busy indicator.
//

#ifndef ES_CORE_COMPONENTS_BUSY_COMPONENT_H
#define ES_CORE_COMPONENTS_BUSY_COMPONENT_H

#include "GuiComponent.h"
#include "components/BackgroundComponent.h"
#include "components/ComponentGrid.h"
#include "components/TextComponent.h"

class AnimatedImageComponent;
class TextComponent;

class BusyComponent : public GuiComponent
{
public:
    BusyComponent();

    void setText(const std::string& text) { mText->setText(text, true); }
    void onSizeChanged() override;

private:
    Renderer* mRenderer;
    BackgroundComponent mBackground;
    ComponentGrid mGrid;

    std::shared_ptr<AnimatedImageComponent> mAnimation;
    std::shared_ptr<TextComponent> mText;
};

#endif // ES_CORE_COMPONENTS_BUSY_COMPONENT_H
