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

// TextComponent sizing works in the following ways:
// setSize(0.0f, 0.0f)      - Automatically sizes single-line text by expanding horizontally.
// setSize(width, 0.0f)     - Limits size horizontally and automatically expands vertically.
// setSize(width, height)   - Wraps and abbreviates text inside the width and height boundaries.
class TextComponent : public GuiComponent
{
public:
    TextComponent();
    TextComponent(const std::string& text,
                  const std::shared_ptr<Font>& font,
                  unsigned int color = 0x000000FF,
                  Alignment horizontalAlignment = ALIGN_LEFT,
                  Alignment verticalAlignment = ALIGN_CENTER,
                  glm::vec3 pos = {0.0f, 0.0f, 0.0f},
                  glm::vec2 size = {0.0f, 0.0f},
                  unsigned int bgcolor = 0x00000000);

    void setFont(const std::shared_ptr<Font>& font);
    void setUppercase(bool uppercase);
    void setLowercase(bool lowercase);
    void setCapitalize(bool capitalize);
    void onSizeChanged() override;
    void setText(const std::string& text, bool update = true);
    void setHiddenText(const std::string& text) { mHiddenText = text; }
    void setColor(unsigned int color) override;
    void setHorizontalAlignment(Alignment align);
    void setVerticalAlignment(Alignment align) { mVerticalAlignment = align; }
    void setLineSpacing(float spacing);
    float getLineSpacing() override { return mLineSpacing; }
    void setNoTopMargin(bool margin);
    void setBackgroundColor(unsigned int color) override;
    void setRenderBackground(bool render) { mRenderBackground = render; }

    void render(const glm::mat4& parentTrans) override;

    std::string getValue() const override { return mText; }
    void setValue(const std::string& value) override;

    std::string getHiddenValue() const override { return mHiddenText; }
    void setHiddenValue(const std::string& value) override { setHiddenText(value); }

    float const getOpacity() const override
    {
        return static_cast<float>((mColor & 0x000000FF) / 255.0f);
    }
    float const getColorOpacity() const override { return mColorOpacity; }

    void setOpacity(float opacity) override;
    void setSaturation(float saturation) override;
    void setDimming(float dimming) override;

    void setSelectable(bool status) { mSelectable = status; }

    void applyTheme(const std::shared_ptr<ThemeData>& theme,
                    const std::string& view,
                    const std::string& element,
                    unsigned int properties) override;

    std::vector<HelpPrompt> getHelpPrompts() override;

    unsigned int getColor() const override { return mColor; }
    std::shared_ptr<Font> getFont() const override { return mFont; }
    Alignment getHorizontalAlignment() { return mHorizontalAlignment; }
    Alignment getVerticalAlignment() { return mVerticalAlignment; }
    const bool getSystemNameSuffix() const { return mSystemNameSuffix; }
    const LetterCase getLetterCaseSystemNameSuffix() const { return mLetterCaseSystemNameSuffix; }

    int getTextCacheGlyphHeight() override
    {
        return (mTextCache == nullptr ? 0 : mTextCache->metrics.maxGlyphHeight);
    }

protected:
    virtual void onTextChanged();

    std::string mText;
    std::string mHiddenText;
    std::shared_ptr<Font> mFont;

private:
    void onColorChanged();

    static inline std::vector<std::string> supportedSystemdataTypes {"name",
                                                                     "fullname",
                                                                     "gamecount",
                                                                     "gamecountGames",
                                                                     "gamecountGamesNoText",
                                                                     "gamecountFavorites",
                                                                     "gamecountFavoritesNoText"};

    static inline std::vector<std::string> supportedMetadataTypes {
        "name",       "description",    "rating",           "developer",           "publisher",
        "genre",      "players",        "favorite",         "completed",           "kidgame",
        "broken",     "playcount",      "controller",       "altemulator",         "emulator",
        "systemName", "systemFullname", "sourceSystemName", "sourceSystemFullname"};

    Renderer* mRenderer;
    std::string mDefaultValue;
    unsigned int mColor;
    unsigned int mBgColor;
    float mColorOpacity;
    float mBgColorOpacity;
    bool mRenderBackground;
    bool mSystemNameSuffix;
    LetterCase mLetterCaseSystemNameSuffix;

    bool mUppercase;
    bool mLowercase;
    bool mCapitalize;
    glm::ivec2 mAutoCalcExtent;
    std::shared_ptr<TextCache> mTextCache;
    Alignment mHorizontalAlignment;
    Alignment mVerticalAlignment;
    float mLineSpacing;
    bool mNoTopMargin;
    bool mSelectable;
    bool mVerticalAutoSizing;
    bool mLegacyTheme;
};

#endif // ES_CORE_COMPONENTS_TEXT_COMPONENT_H
