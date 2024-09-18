//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiMsgBox.h
//
//  Popup message dialog with a notification text and a choice of one,
//  two or three buttons.
//

#ifndef ES_CORE_GUIS_GUI_MSG_BOX_H
#define ES_CORE_GUIS_GUI_MSG_BOX_H

#include "GuiComponent.h"
#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"
#include "utils/LocalizationUtil.h"

class ButtonComponent;
class TextComponent;

class GuiMsgBox : public GuiComponent
{
public:
    GuiMsgBox(const HelpStyle& helpstyle,
              const std::string& text,
              const std::string& name1 = _("OK"),
              const std::function<void()>& func1 = nullptr,
              const std::string& name2 = "",
              const std::function<void()>& func2 = nullptr,
              const std::string& name3 = "",
              const std::function<void()>& func3 = nullptr,
              const std::function<void()>& backFunc = nullptr,
              const bool disableBackButton = false,
              const bool deleteOnButtonPress = true,
              const float maxWidthMultiplier = 0.0f);

    void calculateSize();

    void changeText(const std::string& newText);

    bool input(InputConfig* config, Input input) override;
    void onSizeChanged() override;

    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override { return mHelpStyle; }

private:
    void deleteMeAndCall(const std::function<void()>& func);

    Renderer* mRenderer;
    NinePatchComponent mBackground;
    ComponentGrid mGrid;

    HelpStyle mHelpStyle;
    std::shared_ptr<TextComponent> mMsg;
    std::vector<std::shared_ptr<ButtonComponent>> mButtons;
    std::shared_ptr<ComponentGrid> mButtonGrid;
    const std::function<void()> mBackFunc;
    bool mDisableBackButton;
    bool mDeleteOnButtonPress;
    float mMaxWidthMultiplier;
};

#endif // ES_CORE_GUIS_GUI_MSG_BOX_H
