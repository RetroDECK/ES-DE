//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiTextEditPopup.h
//
//  Simple text edit popup with a title, a text input box and OK and Cancel buttons.
//

#ifndef ES_CORE_GUIS_GUI_TEXT_EDIT_POPUP_H
#define ES_CORE_GUIS_GUI_TEXT_EDIT_POPUP_H

#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"
#include "GuiComponent.h"

class TextComponent;
class TextEditComponent;

class GuiTextEditPopup : public GuiComponent
{
public:
    GuiTextEditPopup(
            Window* window,
            const HelpStyle& helpstyle,
            const std::string& title,
            const std::string& initValue,
            const std::function<void(const std::string&)>& okCallback,
            bool multiLine,
            const std::string& acceptBtnText = "OK",
            const std::string& saveConfirmationText = "SAVE CHANGES?");

    bool input(InputConfig* config, Input input) override;
    void onSizeChanged() override;

    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override { return mHelpStyle; };

private:
    NinePatchComponent mBackground;
    ComponentGrid mGrid;

    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<TextEditComponent> mText;
    std::shared_ptr<ComponentGrid> mButtonGrid;

    HelpStyle mHelpStyle;
    bool mMultiLine;
    std::string mInitValue;
    std::function<void(const std::string&)> mOkCallback;
    std::string mSaveConfirmationText;
};

#endif // ES_CORE_GUIS_GUI_TEXT_EDIT_POPUP_H
