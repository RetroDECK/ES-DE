//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiTextEditPopup.h
//
//  Text edit popup.
//  Has a default mode and a complex mode, both with various options passed as arguments.
//

#ifndef ES_CORE_GUIS_GUI_TEXT_EDIT_POPUP_H
#define ES_CORE_GUIS_GUI_TEXT_EDIT_POPUP_H

#include "GuiComponent.h"
#include "components/ButtonComponent.h"
#include "components/ComponentGrid.h"
#include "components/TextComponent.h"
#include "components/TextEditComponent.h"
#include "utils/LocalizationUtil.h"

class GuiTextEditPopup : public GuiComponent
{
public:
    GuiTextEditPopup(const HelpStyle& helpstyle,
                     const std::string& title,
                     const std::string& initValue,
                     const std::function<void(const std::string&)>& okCallback,
                     bool multiLine,
                     const std::string& acceptBtnText = _("OK"),
                     const std::string& saveConfirmationText = _("SAVE CHANGES?"),
                     const std::string& infoString = "",
                     const std::string& defaultValue = "",
                     const std::string& loadBtnHelpText = _("LOAD DEFAULT"),
                     const std::string& clearBtnHelpText = _("CLEAR"),
                     const std::string& cancelBtnHelpText = _("DISCARD CHANGES"));

    void onSizeChanged() override;
    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;

    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override { return mHelpStyle; }

private:
    void updateDeleteRepeat(int deltaTime);

    NinePatchComponent mBackground;
    ComponentGrid mGrid;
    HelpStyle mHelpStyle;

    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<TextComponent> mInfoString;
    std::shared_ptr<TextComponent> mDefaultValue;
    std::shared_ptr<TextEditComponent> mText;
    std::shared_ptr<ComponentGrid> mButtonGrid;

    std::string mInitValue;
    std::string mAcceptBtnText;
    std::string mSaveConfirmationText;
    std::string mLoadBtnHelpText;
    std::string mClearBtnHelpText;
    std::string mCancelBtnHelpText;

    std::function<void(const std::string&)> mOkCallback;

    bool mMultiLine;
    bool mComplexMode;
    bool mDeleteRepeat;

    int mDeleteRepeatTimer;
};

#endif // ES_CORE_GUIS_GUI_TEXT_EDIT_POPUP_H
