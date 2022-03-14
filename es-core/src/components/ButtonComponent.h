//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ButtonComponent.h
//
//  Basic on/off button.
//

#ifndef ES_CORE_COMPONENTS_BUTTON_COMPONENT_H
#define ES_CORE_COMPONENTS_BUTTON_COMPONENT_H

#include "GuiComponent.h"
#include "components/NinePatchComponent.h"

class TextCache;

class ButtonComponent : public GuiComponent
{
public:
    ButtonComponent(const std::string& text = "",
                    const std::string& helpText = "",
                    const std::function<void()>& func = nullptr,
                    bool upperCase = true,
                    bool flatStyle = false);

    void onSizeChanged() override;
    void onFocusGained() override;
    void onFocusLost() override;

    void setText(const std::string& text, const std::string& helpText, bool upperCase = true);
    const std::string& getText() const { return mText; }

    void setPressedFunc(std::function<void()> f) { mPressedFunc = f; }
    void setEnabled(bool state) override;

    void setPadding(const glm::vec4 padding);
    glm::vec4 getPadding() { return mPadding; }

    void setFlatColorFocused(unsigned int color) { mFlatColorFocused = color; }
    void setFlatColorUnfocused(unsigned int color) { mFlatColorUnfocused = color; }

    const std::function<void()>& getPressedFunc() const { return mPressedFunc; }

    bool input(InputConfig* config, Input input) override;
    void render(const glm::mat4& parentTrans) override;

    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    unsigned int getCurTextColor() const;
    void updateImage();

    Renderer* mRenderer;
    NinePatchComponent mBox;

    std::shared_ptr<Font> mFont;
    std::unique_ptr<TextCache> mTextCache;
    std::function<void()> mPressedFunc;

    glm::vec4 mPadding;

    std::string mText;
    std::string mHelpText;

    bool mFocused;
    bool mEnabled;
    bool mFlatStyle;

    unsigned int mTextColorFocused;
    unsigned int mTextColorUnfocused;
    unsigned int mFlatColorFocused;
    unsigned int mFlatColorUnfocused;
};

#endif // ES_CORE_COMPONENTS_BUTTON_COMPONENT_H
