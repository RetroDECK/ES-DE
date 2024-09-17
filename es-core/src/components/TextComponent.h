//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
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
// autoCalcExtent(1, 0)       - Automatically expand horizontally, line breaks are removed.
// autoCalcExtent(0, 0)       - Wrap and abbreviate inside the width and height boundaries.
// autoCalcExtent(0, 1)       - Limit size horizontally and automatically expand vertically.
// autoCalcExtent(1, 1)       - Automatically expand horizontally and wrap by line break.

// The sizing logic above translates to the following theme configuration:
// <size>0 0</size>           - autoCalcExtent(1, 0)
// <size>width 0</size>       - autoCalcExtent(0, 1)
// <size>width height</size>  - autoCalcExtent(0, 0)

class TextComponent : public GuiComponent
{
public:
    TextComponent();
    TextComponent(const std::string& text,
                  const std::shared_ptr<Font>& font,
                  unsigned int color = 0x000000FF,
                  Alignment horizontalAlignment = ALIGN_LEFT,
                  Alignment verticalAlignment = ALIGN_CENTER,
                  glm::ivec2 autoCalcExtent = {1, 0},
                  glm::vec3 pos = {0.0f, 0.0f, 0.0f},
                  glm::vec2 size = {0.0f, 0.0f},
                  unsigned int bgcolor = 0x00000000,
                  float lineSpacing = 1.5f,
                  float relativeScale = 1.0f,
                  bool horizontalScrolling = false,
                  float scrollSpeedMultiplier = 1.0f,
                  float scrollDelay = 1500.0f,
                  float scrollGap = 1.5f,
                  float maxLength = 0.0f);

    void setFont(const std::shared_ptr<Font>& font);
    void setUppercase(bool uppercase);
    void setLowercase(bool lowercase);
    void setCapitalize(bool capitalize);
    void onSizeChanged() override { onTextChanged(); }
    void setText(const std::string& text, bool update = true, float maxLength = 0.0f);
    void setHiddenText(const std::string& text) { mHiddenText = text; }
    void setAutoCalcExtent(glm::ivec2 extent) override { mAutoCalcExtent = extent; }
    const glm::ivec2 getAutoCalcExtent() { return mAutoCalcExtent; }
    void setColor(unsigned int color) override;
    void setHorizontalAlignment(Alignment align);
    void setVerticalAlignment(Alignment align) { mVerticalAlignment = align; }
    void setLineSpacing(float spacing);
    float getLineSpacing() override { return mLineSpacing; }
    void setTextShaping(bool state) { mFont->setTextShaping(state); }
    void setNoTopMargin(bool margin);
    void setNeedGlyphsPos(bool state) { mNeedGlyphsPos = state; }
    void setRemoveLineBreaks(bool state) override { mRemoveLineBreaks = state; }
    void setNoSizeUpdate(bool state) { mNoSizeUpdate = state; }
    const glm::vec2 getGlyphPosition(int cursor);
    void setBackgroundColor(unsigned int color) override;
    void setRenderBackground(bool render) { mRenderBackground = render; }
    void setBackgroundMargins(const glm::vec2 margins) { mBackgroundMargins = margins; }
    void setBackgroundCornerRadius(const float radius) { mBackgroundCornerRadius = radius; }
    // Used by some components that render the debug overlay themselves.
    void setDebugRendering(bool state) { mDebugRendering = state; }

    void render(const glm::mat4& parentTrans) override;
    void onFocusLost() override { resetComponent(); }

    std::string getValue() const override { return mText; }
    void setValue(const std::string& value) override;

    std::string getHiddenValue() const override { return mHiddenText; }
    void setHiddenValue(const std::string& value) override { setHiddenText(value); }

    const std::string getDefaultValue() const { return mDefaultValue; }

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

    const TextCache* getTextCache() override
    {
        return (mTextCache == nullptr ? nullptr : mTextCache.get());
    }

    // Horizontal scrolling for single-line content that is too long to fit.
    void setHorizontalScrolling(bool state) override;
    void setHorizontalScrollingSpeedMultiplier(float speed) { mScrollSpeedMultiplier = speed; }
    void setHorizontalScrollingDelay(float delay) { mScrollDelay = delay; }
    void setHorizontalScrollingGap(float gap) { mScrollGap = gap; }

    void resetComponent() override
    {
        mScrollOffset1 = 0;
        mScrollOffset2 = 0;
        mScrollTime = 0;
    }

    void update(int deltaTime) override;

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

    static inline std::vector<std::string> supportedMetadataTypes {"name",
                                                                   "description",
                                                                   "rating",
                                                                   "developer",
                                                                   "publisher",
                                                                   "genre",
                                                                   "players",
                                                                   "favorite",
                                                                   "completed",
                                                                   "kidgame",
                                                                   "broken",
                                                                   "playcount",
                                                                   "controller",
                                                                   "altemulator",
                                                                   "emulator",
                                                                   "manual",
                                                                   "physicalName",
                                                                   "physicalNameExtension",
                                                                   "systemName",
                                                                   "systemFullname",
                                                                   "sourceSystemName",
                                                                   "sourceSystemFullname"};

    Renderer* mRenderer;
    std::string mDefaultValue;
    unsigned int mColor;
    unsigned int mBgColor;
    glm::vec2 mBackgroundMargins;
    float mBackgroundCornerRadius;
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
    float mRelativeScale;
    bool mNoTopMargin;
    bool mNeedGlyphsPos;
    bool mRemoveLineBreaks;
    bool mNoSizeUpdate;
    bool mSelectable;
    bool mHorizontalScrolling;
    bool mDebugRendering;
    float mScrollSpeed;
    float mScrollSpeedMultiplier;
    float mScrollDelay;
    float mScrollGap;
    float mScrollOffset1;
    float mScrollOffset2;
    float mScrollTime;
    float mMaxLength;
};

#endif // ES_CORE_COMPONENTS_TEXT_COMPONENT_H
