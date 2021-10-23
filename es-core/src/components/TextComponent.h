//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  TextComponent.h
//
//  Displays text.
//

#ifndef ES_CORE_COMPONENTS_TEXT_COMPONENT_H
#define ES_CORE_COMPONENTS_TEXT_COMPONENT_H

#include "GuiComponent.h"
#include "resources/Font.h"

class ThemeData;

// Used to display text.
// TextComponent::setSize(x, y) works a little differently than most components:
//  * (0, 0)                    - Will automatically calculate a size that fits
//                                the text on one line (expand horizontally).
//  * (x != 0, 0)               - Wrap text so that it does not reach beyond x. Will
//                                automatically calculate a vertical size (expand vertically).
//  * (x != 0, y <= fontHeight) - Will truncate text so it fits within this box.
class TextComponent : public GuiComponent
{
public:
    TextComponent(Window* window);
    TextComponent(Window* window,
                  const std::string& text,
                  const std::shared_ptr<Font>& font,
                  unsigned int color = 0x000000FF,
                  Alignment align = ALIGN_LEFT,
                  glm::vec3 pos = {},
                  glm::vec2 size = {},
                  unsigned int bgcolor = 0x00000000);

    void setFont(const std::shared_ptr<Font>& font);
    void setUppercase(bool uppercase);
    void onSizeChanged() override;
    void setText(const std::string& text, bool update = true);
    void setHiddenText(const std::string& text) { mHiddenText = text; }
    void setColor(unsigned int color) override;
    void setHorizontalAlignment(Alignment align);
    void setVerticalAlignment(Alignment align) { mVerticalAlignment = align; }
    void setLineSpacing(float spacing);
    float getLineSpacing() override { return mLineSpacing; }
    void setNoTopMargin(bool margin);
    void setBackgroundColor(unsigned int color);
    void setRenderBackground(bool render) { mRenderBackground = render; }

    void render(const glm::mat4& parentTrans) override;

    std::string getValue() const override { return mText; }
    void setValue(const std::string& value) override { setText(value); }

    std::string getHiddenValue() const override { return mHiddenText; }
    void setHiddenValue(const std::string& value) override { setHiddenText(value); }

    unsigned char getOpacity() const override { return mColor & 0x000000FF; }
    void setOpacity(unsigned char opacity) override;

    void setSelectable(bool status) { mSelectable = status; }

    virtual void applyTheme(const std::shared_ptr<ThemeData>& theme,
                            const std::string& view,
                            const std::string& element,
                            unsigned int properties) override;

    virtual std::vector<HelpPrompt> getHelpPrompts() override;

    unsigned int getColor() const override { return mColor; }
    std::shared_ptr<Font> getFont() const override { return mFont; }
    Alignment getHorizontalAlignment() { return mHorizontalAlignment; }
    Alignment getVerticalAlignment() { return mVerticalAlignment; }

protected:
    virtual void onTextChanged();

    std::string mText;
    std::string mHiddenText;
    std::shared_ptr<Font> mFont;

private:
    void calculateExtent();
    void onColorChanged();

    unsigned int mColor;
    unsigned int mBgColor;
    unsigned char mColorOpacity;
    unsigned char mBgColorOpacity;
    bool mRenderBackground;

    bool mUppercase;
    glm::ivec2 mAutoCalcExtent;
    std::shared_ptr<TextCache> mTextCache;
    Alignment mHorizontalAlignment;
    Alignment mVerticalAlignment;
    float mLineSpacing;
    bool mNoTopMargin;
    bool mSelectable;
};

#endif // ES_CORE_COMPONENTS_TEXT_COMPONENT_H
