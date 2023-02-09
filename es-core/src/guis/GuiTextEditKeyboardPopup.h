//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiTextEditKeyboardPopup.h
//
//  Text edit popup with a virtual keyboard.
//  Has a default mode and a complex mode, both with various options passed as arguments.
//

#ifndef ES_CORE_GUIS_GUI_TEXT_EDIT_KEYBOARD_POPUP_H
#define ES_CORE_GUIS_GUI_TEXT_EDIT_KEYBOARD_POPUP_H

#include "GuiComponent.h"
#include "components/ButtonComponent.h"
#include "components/ComponentGrid.h"
#include "components/TextEditComponent.h"

class GuiTextEditKeyboardPopup : public GuiComponent
{
public:
    GuiTextEditKeyboardPopup(const HelpStyle& helpstyle,
                             const float verticalPosition,
                             const std::string& title,
                             const std::string& initValue,
                             const std::function<void(const std::string&)>& okCallback,
                             bool multiLine,
                             const std::string& acceptBtnHelpText = "OK",
                             const std::string& saveConfirmationText = "SAVE CHANGES?",
                             const std::string& infoString = "",
                             const std::string& defaultValue = "",
                             const std::string& loadBtnHelpText = "LOAD DEFAULT",
                             const std::string& clearBtnHelpText = "CLEAR",
                             const std::string& cancelBtnHelpText = "DISCARD CHANGES");

    void onSizeChanged() override;
    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;

    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override { return mHelpStyle; }

private:
    class KeyboardButton
    {
    public:
        std::shared_ptr<ButtonComponent> button;
        const std::string key;
        const std::string shiftedKey;
        const std::string altedKey;
        const std::string altshiftedKey;
        KeyboardButton(const std::shared_ptr<ButtonComponent> b,
                       const std::string& k,
                       const std::string& sk,
                       const std::string& ak,
                       const std::string& ask)
            : button {b}
            , key {k}
            , shiftedKey {sk}
            , altedKey {ak}
            , altshiftedKey {ask} {};
    };

    void updateDeleteRepeat(int deltaTime);
    void updateNavigationRepeat(int deltaTime);

    void shiftKeys();
    void altKeys();
    void altShiftKeys();

    std::shared_ptr<ButtonComponent> makeButton(const std::string& key,
                                                const std::string& shiftedKey,
                                                const std::string& altedKey,
                                                const std::string& altshiftedKey);
    std::vector<KeyboardButton> mKeyboardButtons;

    std::shared_ptr<ButtonComponent> mShiftButton;
    std::shared_ptr<ButtonComponent> mAltButton;

    Renderer* mRenderer;
    NinePatchComponent mBackground;
    ComponentGrid mGrid;
    HelpStyle mHelpStyle;

    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<TextComponent> mInfoString;
    std::shared_ptr<TextComponent> mDefaultValue;
    std::shared_ptr<TextEditComponent> mText;
    std::shared_ptr<ComponentGrid> mKeyboardGrid;

    std::string mInitValue;
    std::string mAcceptBtnHelpText;
    std::string mSaveConfirmationText;
    std::string mLoadBtnHelpText;
    std::string mClearBtnHelpText;
    std::string mCancelBtnHelpText;

    std::function<void(const std::string&)> mOkCallback;

    bool mMultiLine;
    bool mComplexMode;
    bool mDeleteRepeat;
    bool mShift;
    bool mAlt;

    float mVerticalPosition;

    int mHorizontalKeyCount;
    int mDeleteRepeatTimer;
    int mNavigationRepeatTimer;
    int mNavigationRepeatDirX;
    int mNavigationRepeatDirY;
};

#endif // ES_CORE_GUIS_GUI_TEXT_EDIT_KEYBOARD_POPUP_H
