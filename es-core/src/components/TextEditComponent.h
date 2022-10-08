//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  TextEditComponent.h
//
//  Component for editing text fields in menus.
//

#ifndef ES_CORE_COMPONENTS_TEXT_EDIT_COMPONENT_H
#define ES_CORE_COMPONENTS_TEXT_EDIT_COMPONENT_H

#include "GuiComponent.h"
#include "components/NinePatchComponent.h"
#include "resources/Font.h"

class Font;
class TextCache;

// Used to enter text.
class TextEditComponent : public GuiComponent
{
public:
    TextEditComponent();

    void textInput(const std::string& text) override;
    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;

    void onFocusGained() override;
    void onFocusLost() override;

    void onSizeChanged() override;

    void setValue(const std::string& val) override;
    std::string getValue() const override;

    void startEditing();
    void stopEditing();

    bool isEditing() const { return mEditing; }
    std::shared_ptr<Font> getFont() const override { return mFont; }

    void setCursor(size_t pos);
    void setMaskInput(bool state) { mMaskInput = state; }

    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    void onTextChanged();
    void onCursorChanged();

    void updateCursorRepeat(int deltaTime);
    void moveCursor(int amt);

    bool isMultiline() { return (getSize().y > mFont->getHeight() * 1.25f); }
    glm::vec2 getTextAreaPos() const;
    glm::vec2 getTextAreaSize() const;

    Renderer* mRenderer;
    std::string mText;
    std::string mWrappedText;
    std::string mTextOrig;
    bool mFocused;
    bool mEditing;
    bool mMaskInput;
    int mCursor; // Cursor position in characters.
    int mBlinkTime;

    int mCursorRepeatTimer;
    int mCursorRepeatDir;

    glm::vec2 mScrollOffset;
    glm::vec2 mCursorPos;

    NinePatchComponent mBox;

    std::shared_ptr<Font> mFont;
    std::unique_ptr<TextCache> mTextCache;
};

#endif // ES_CORE_COMPONENTS_TEXT_EDIT_COMPONENT_H
