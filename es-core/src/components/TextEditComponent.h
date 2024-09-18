//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  TextEditComponent.h
//
//  Component for editing text fields.
//  TODO: Add support for editing shaped text.
//

#ifndef ES_CORE_COMPONENTS_TEXT_EDIT_COMPONENT_H
#define ES_CORE_COMPONENTS_TEXT_EDIT_COMPONENT_H

#include "GuiComponent.h"
#include "components/NinePatchComponent.h"
#include "components/TextComponent.h"

class TextEditComponent : public GuiComponent
{
public:
    TextEditComponent(bool multiLine);
    ~TextEditComponent();

    void textInput(const std::string& text, const bool pasting = false) override;
    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;

    void onFocusGained() override;
    void onFocusLost() override;

    void onSizeChanged() override;

    void setText(const std::string& val, bool update = true);
    std::string getValue() const override;

    void startEditing();
    void stopEditing();

    bool isEditing() const { return mEditing; }
    std::shared_ptr<Font> getFont() const override { return mEditText->getFont(); }

    void setCursor(size_t pos);
    void setMaskInput(bool state) { mMaskInput = state; }

    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    void onTextChanged();
    void onCursorChanged();

    void updateCursorRepeat(int deltaTime);
    void moveCursor(int amt);

    glm::vec2 getTextAreaPos() const;
    glm::vec2 getTextAreaSize() const;

    Renderer* mRenderer;
    std::string mText;
    bool mFocused;
    bool mEditing;
    bool mMaskInput;
    bool mMultiLine;
    int mCursor; // Cursor position in source text.
    int mCursorShapedText; // Cursor position in shaped text.
    int mBlinkTime;

    int mCursorRepeatTimer;
    int mCursorRepeatDir;

    glm::vec2 mScrollOffset;
    glm::vec2 mCursorPos;

    NinePatchComponent mBox;
    std::unique_ptr<TextComponent> mEditText;
};

#endif // ES_CORE_COMPONENTS_TEXT_EDIT_COMPONENT_H
