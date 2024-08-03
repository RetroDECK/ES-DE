//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  TextComponent.cpp
//
//  Displays text.
//

#include "components/TextComponent.h"

#include "Log.h"
#include "Settings.h"
#include "Window.h"
#include "utils/LocalizationUtil.h"
#include "utils/StringUtil.h"

TextComponent::TextComponent()
    : mFont {Font::get(FONT_SIZE_MEDIUM)}
    , mRenderer {Renderer::getInstance()}
    , mColor {0x000000FF}
    , mBgColor {0x00000000}
    , mBackgroundMargins {0.0f, 0.0f}
    , mBackgroundCornerRadius {0.0f}
    , mColorOpacity {1.0f}
    , mBgColorOpacity {0.0f}
    , mRenderBackground {false}
    , mSystemNameSuffix {false}
    , mLetterCaseSystemNameSuffix {LetterCase::UPPERCASE}
    , mUppercase {false}
    , mLowercase {false}
    , mCapitalize {false}
    , mAutoCalcExtent {1, 1}
    , mHorizontalAlignment {ALIGN_LEFT}
    , mVerticalAlignment {ALIGN_CENTER}
    , mLineSpacing {1.5f}
    , mRelativeScale {1.0f}
    , mNoTopMargin {false}
    , mSelectable {false}
    , mVerticalAutoSizing {false}
    , mHorizontalScrolling {false}
    , mScrollSpeed {0.0f}
    , mScrollSpeedMultiplier {1.0f}
    , mScrollDelay {1500.0f}
    , mScrollGap {1.5f}
    , mScrollOffset1 {0.0f}
    , mScrollOffset2 {0.0f}
    , mScrollTime {0.0f}
{
}

TextComponent::TextComponent(const std::string& text,
                             const std::shared_ptr<Font>& font,
                             unsigned int color,
                             Alignment horizontalAlignment,
                             Alignment verticalAlignment,
                             glm::vec3 pos,
                             glm::vec2 size,
                             unsigned int bgcolor,
                             float lineSpacing,
                             float relativeScale,
                             bool horizontalScrolling,
                             float scrollSpeedMultiplier,
                             float scrollDelay,
                             float scrollGap)
    : mFont {nullptr}
    , mRenderer {Renderer::getInstance()}
    , mColor {0x000000FF}
    , mBgColor {0x00000000}
    , mBackgroundMargins {0.0f, 0.0f}
    , mBackgroundCornerRadius {0.0f}
    , mColorOpacity {1.0f}
    , mBgColorOpacity {0.0f}
    , mRenderBackground {false}
    , mSystemNameSuffix {false}
    , mLetterCaseSystemNameSuffix {LetterCase::UPPERCASE}
    , mUppercase {false}
    , mLowercase {false}
    , mCapitalize {false}
    , mAutoCalcExtent {1, 1}
    , mHorizontalAlignment {horizontalAlignment}
    , mVerticalAlignment {verticalAlignment}
    , mLineSpacing {lineSpacing}
    , mRelativeScale {relativeScale}
    , mNoTopMargin {false}
    , mSelectable {false}
    , mVerticalAutoSizing {false}
    , mHorizontalScrolling {horizontalScrolling}
    , mScrollSpeed {0.0f}
    , mScrollSpeedMultiplier {scrollSpeedMultiplier}
    , mScrollDelay {scrollDelay}
    , mScrollGap {scrollGap}
    , mScrollOffset1 {0.0f}
    , mScrollOffset2 {0.0f}
    , mScrollTime {0.0f}
{
    setFont(font);
    setColor(color);
    setBackgroundColor(bgcolor);
    setHorizontalScrolling(mHorizontalScrolling);
    setText(text, false);
    setPosition(pos);
    setSize(size);
}

void TextComponent::onSizeChanged()
{
    mAutoCalcExtent = glm::ivec2 {getSize().x == 0, getSize().y == 0};
    onTextChanged();
}

void TextComponent::setFont(const std::shared_ptr<Font>& font)
{
    if (mFont == font)
        return;

    mFont = font;
    onTextChanged();
}

//  Set the color of the font/text.
void TextComponent::setColor(unsigned int color)
{
    if (mColor == color)
        return;

    mColor = color;
    mColorOpacity = static_cast<float>(mColor & 0x000000FF) / 255.0f;
    onColorChanged();
}

//  Set the color of the background box.
void TextComponent::setBackgroundColor(unsigned int color)
{
    if (mBgColor == color)
        return;

    mBgColor = color;
    mBgColorOpacity = static_cast<float>(mBgColor & 0x000000FF) / 255.0f;
}

void TextComponent::setOpacity(float opacity)
{
    float textOpacity {opacity * mColorOpacity};
    mColor = (mColor & 0xFFFFFF00) | static_cast<unsigned char>(textOpacity * 255.0f);

    onColorChanged();
    GuiComponent::setOpacity(opacity);

    if (mTextCache)
        mTextCache->setOpacity(mThemeOpacity);
}

void TextComponent::setSaturation(float saturation)
{
    if (mSaturation == saturation)
        return;

    mSaturation = saturation;
    if (mTextCache)
        mTextCache->setSaturation(saturation);
}

void TextComponent::setDimming(float dimming)
{
    if (mDimming == dimming)
        return;

    mDimming = dimming;
    if (mTextCache)
        mTextCache->setDimming(dimming);
}

void TextComponent::setText(const std::string& text, bool update)
{
    if (mText == text)
        return;

    mText = text;

    if (update)
        onTextChanged();
}

void TextComponent::setUppercase(bool uppercase)
{
    if (mUppercase == uppercase)
        return;

    mUppercase = uppercase;
    if (uppercase) {
        mLowercase = false;
        mCapitalize = false;
    }
    onTextChanged();
}

void TextComponent::setLowercase(bool lowercase)
{
    if (mLowercase == lowercase)
        return;

    mLowercase = lowercase;
    if (lowercase) {
        mUppercase = false;
        mCapitalize = false;
    }
    onTextChanged();
}

void TextComponent::setCapitalize(bool capitalize)
{
    if (mCapitalize == capitalize)
        return;

    mCapitalize = capitalize;
    if (capitalize) {
        mUppercase = false;
        mLowercase = false;
    }
    onTextChanged();
}

void TextComponent::render(const glm::mat4& parentTrans)
{
    if (!isVisible() || mTextCache == nullptr || mThemeOpacity == 0.0f || mSize.x == 0.0f ||
        mSize.y == 0.0f)
        return;

    glm::mat4 trans {parentTrans * getTransform()};
    mRenderer->setMatrix(trans);

    float offsetX {0.0f};

    if (mHorizontalScrolling) {
        if (mTextCache->metrics.size.x < mSize.x) {
            // This is needed for text that does not fill the entire width and thus gets aligned.
            if (mHorizontalAlignment == Alignment::ALIGN_CENTER)
                offsetX = ((mSize.x * mRelativeScale) - mTextCache->metrics.size.x) / 2.0f;
            else if (mHorizontalAlignment == Alignment::ALIGN_RIGHT)
                offsetX = (mSize.x * mRelativeScale) - mTextCache->metrics.size.x;
        }

        if (offsetX < 0.0f)
            offsetX = 0.0f;

        // Clip the texture using a fragment shader which allows for rotation and other benefits
        // as compared to using the pushClipRect() function.
        mTextCache->setClipRegion(glm::vec4 {mScrollOffset1, 0.0f,
                                             (mSize.x * mRelativeScale) + mScrollOffset1,
                                             mTextCache->metrics.size.y});

        trans = glm::translate(trans, glm::vec3 {offsetX - mScrollOffset1, 0.0f, 0.0f});
    }

    auto renderFunc = [this](glm::mat4 trans, bool secondPass) {
        if (mRenderBackground && !secondPass)
            if (mBackgroundMargins.x > 0.0f) {
                trans = glm::translate(trans, glm::vec3 {-mBackgroundMargins.x, 0.0f, 0.0f});
                mRenderer->setMatrix(trans);
            }

        mRenderer->drawRect(0.0f, 0.0f, mSize.x + mBackgroundMargins.x + mBackgroundMargins.y,
                            mSize.y, mBgColor, mBgColor, false, mOpacity * mThemeOpacity, mDimming,
                            Renderer::BlendFactor::SRC_ALPHA,
                            Renderer::BlendFactor::ONE_MINUS_SRC_ALPHA, mBackgroundCornerRadius);

        if (mBackgroundMargins.x > 0.0f) {
            trans = glm::translate(trans, glm::vec3 {mBackgroundMargins.x, 0.0f, 0.0f});
            mRenderer->setMatrix(trans);
        }

        if (mTextCache) {
            const float textHeight {mTextCache->metrics.size.y};
            float yOff {0.0f};
            float yOffDebugOverlay {0.0f};

            if (mSize.y > textHeight) {
                switch (mVerticalAlignment) {
                    case ALIGN_TOP: {
                        yOff = 0.0f;
                        break;
                    }
                    case ALIGN_BOTTOM: {
                        yOff = mSize.y - textHeight;
                        break;
                    }
                    case ALIGN_CENTER: {
                        yOff = (mSize.y - textHeight) / 2.0f;
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
            else {
                // If height is smaller than the font height, then centering is done in
                // Font::buildTextCache()
                yOffDebugOverlay = (mSize.y - textHeight) / 2.0f;
            }

            // Draw the overall textbox area. If we're inside a vertical scrollable container then
            // this area is rendered inside that component instead of here.
            if (!secondPass && Settings::getInstance()->getBool("DebugText")) {
                if (!mParent || !mParent->isScrollable())
                    mRenderer->drawRect(0.0f, 0.0f, mSize.x, mSize.y, 0x0000FF33, 0x0000FF33);
            }

            trans = glm::translate(trans, glm::vec3 {0.0f, std::round(yOff), 0.0f});
            mRenderer->setMatrix(trans);

            if (Settings::getInstance()->getBool("DebugText")) {
                const float relativeScaleOffset {(mSize.x - (mSize.x * mRelativeScale)) / 2.0f};
                if (mHorizontalScrolling && !secondPass) {
                    if (mScrollOffset1 <= mTextCache->metrics.size.x) {
                        const float width {mTextCache->metrics.size.x - mScrollOffset1};
                        mRenderer->drawRect(
                            mScrollOffset1 + relativeScaleOffset, yOffDebugOverlay,
                            width > mSize.x * mRelativeScale ? mSize.x * mRelativeScale : width,
                            mTextCache->metrics.size.y, 0x00000033, 0x00000033);
                    }
                }
                else if (mHorizontalScrolling && secondPass) {
                    if ((mSize.x * mRelativeScale) - -mScrollOffset2 > 0.0f) {
                        mRenderer->drawRect(relativeScaleOffset, yOffDebugOverlay,
                                            (mSize.x * mRelativeScale) - -mScrollOffset2,
                                            mTextCache->metrics.size.y, 0x00000033, 0x00000033);
                    }
                }
                else {
                    switch (mHorizontalAlignment) {
                        case ALIGN_LEFT: {
                            mRenderer->drawRect(0.0f, yOffDebugOverlay, mTextCache->metrics.size.x,
                                                mTextCache->metrics.size.y, 0x00000033, 0x00000033);
                            break;
                        }
                        case ALIGN_CENTER: {
                            mRenderer->drawRect((mSize.x - mTextCache->metrics.size.x) / 2.0f,
                                                yOffDebugOverlay, mTextCache->metrics.size.x,
                                                mTextCache->metrics.size.y, 0x000033, 0x00000033);
                            break;
                        }
                        case ALIGN_RIGHT: {
                            mRenderer->drawRect(mSize.x - mTextCache->metrics.size.x,
                                                yOffDebugOverlay, mTextCache->metrics.size.x,
                                                mTextCache->metrics.size.y, 0x00000033, 0x00000033);
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
            }
            // We need to adjust positioning if the relative scale multiplier is in use.
            if (mRelativeScale < 1.0f) {
                trans = glm::translate(
                    trans, glm::vec3 {(mSize.x - (mSize.x * mRelativeScale)) / 2.0f, 0.0f, 0.0f});
                mRenderer->setMatrix(trans);
            }

            mFont->renderTextCache(mTextCache.get());
        }
    };

    renderFunc(trans, false);

    // Render again if the text has moved far enough to repeat.
    if (mHorizontalScrolling && mTextCache->metrics.size.x > mSize.x * mRelativeScale) {
        if (mScrollOffset2 < 0.0f) {
            mTextCache->setClipRegion(glm::vec4 {mScrollOffset2, 0.0f,
                                                 (mSize.x * mRelativeScale) + mScrollOffset2,
                                                 mTextCache->metrics.size.y});
            trans = glm::translate(parentTrans * getTransform(),
                                   glm::vec3 {offsetX - mScrollOffset2, 0.0f, 0.0f});
            renderFunc(trans, true);
        }
    }
}

void TextComponent::setValue(const std::string& value)
{
    if (value == "unknown" && mDefaultValue != "" &&
        (mThemeMetadata == "developer" || mThemeMetadata == "publisher" ||
         mThemeMetadata == "genre" || mThemeMetadata == "players")) {
        setText(mDefaultValue);
    }
    else if (mHorizontalScrolling) {
        setText(Utils::String::replace(value, "\n", " "));
    }
    else {
        setText(value);
    }
}

void TextComponent::setHorizontalScrolling(bool state)
{
    resetComponent();
    mHorizontalScrolling = state;

    if (mHorizontalScrolling) {
        mScrollSpeed =
            mFont->sizeText("ABCDEFGHIJKLMNOPQRSTUVWXYZ").x * 0.247f * mScrollSpeedMultiplier;
    }
    else if (mTextCache != nullptr) {
        mTextCache->setClipRegion(
            glm::vec4 {0.0f, 0.0f, mSize.x * mRelativeScale, mTextCache->metrics.size.y});
    }
}

void TextComponent::update(int deltaTime)
{
    if (mHorizontalScrolling && mTextCache != nullptr) {
        // Don't scroll if the media viewer or screensaver is active or if text scrolling
        // is disabled;
        if (mWindow->isMediaViewerActive() || mWindow->isScreensaverActive() ||
            !mWindow->getAllowTextScrolling()) {
            if (mScrollTime != 0 && !mWindow->isLaunchScreenDisplayed())
                resetComponent();
            return;
        }

        assert(mScrollSpeed != 0.0f);

        mScrollOffset1 = 0.0f;
        mScrollOffset2 = 0.0f;

        if (mTextCache->metrics.size.x > mSize.x * mRelativeScale) {
            const float scrollLength {mTextCache->metrics.size.x};
            const float returnLength {mScrollSpeed * mScrollGap / mScrollSpeedMultiplier};
            const float scrollTime {(scrollLength * 1000.0f) / mScrollSpeed};
            const float returnTime {(returnLength * 1000.0f) / mScrollSpeed};
            const float maxTime {mScrollDelay + scrollTime + returnTime};

            mScrollTime += deltaTime;

            while (mScrollTime > maxTime)
                mScrollTime -= maxTime;

            mScrollOffset1 = Utils::Math::loop(mScrollDelay, scrollTime + returnTime, mScrollTime,
                                               scrollLength + returnLength);

            if (mScrollOffset1 > (scrollLength - (mSize.x * mRelativeScale - returnLength)))
                mScrollOffset2 = mScrollOffset1 - (scrollLength + returnLength);
            else if (mScrollOffset2 < 0)
                mScrollOffset2 = 0;
        }
    }

    updateSelf(deltaTime);
}

void TextComponent::onTextChanged()
{
    mTextCache.reset();

    if (!mVerticalAutoSizing)
        mVerticalAutoSizing = (mSize.x != 0.0f && mSize.y == 0.0f);

    std::string text;

    if (mText != "") {
        if (mUppercase)
            text = Utils::String::toUpper(mText);
        else if (mLowercase)
            text = Utils::String::toLower(mText);
        else if (mCapitalize)
            text = Utils::String::toCapitalized(mText);
        else
            text = mText; // Original case.
    }

    if (mFont && mAutoCalcExtent.x) {
        mSize = mFont->sizeText(text, mLineSpacing);
        if (mSize.x == 0.0f)
            return;
    }

    if (!mFont || text.empty() || mSize.x < 0.0f)
        return;

    float lineHeight {0.0f};
    const bool isScrollable {mParent && mParent->isScrollable()};
    std::shared_ptr<Font> font {mFont};

    // Used to initialize all glyphs, which is needed to populate mMaxGlyphHeight.
    lineHeight = mFont->loadGlyphs(text + "\n") * mLineSpacing;

    const bool isMultiline {mAutoCalcExtent.y == 1 || mSize.y * mRelativeScale > lineHeight};
    float offsetY {0.0f};

    if (mHorizontalScrolling) {
        if (lineHeight > mSize.y && mSize.y != 0.0f)
            offsetY = (mSize.y - lineHeight) / 2.0f;
        mTextCache = std::shared_ptr<TextCache>(
            font->buildTextCache(text, 0.0f, offsetY, mColor, mLineSpacing));
    }
    else if (isMultiline && !isScrollable) {
        const std::string wrappedText {
            font->wrapText(text, mSize.x * mRelativeScale,
                           (mVerticalAutoSizing ? 0.0f : (mSize.y * mRelativeScale) - lineHeight),
                           mLineSpacing, isMultiline)};
        mTextCache = std::shared_ptr<TextCache>(font->buildTextCache(
            wrappedText, glm::vec2 {0.0f, 0.0f}, mColor, mSize.x * mRelativeScale,
            mHorizontalAlignment, mLineSpacing, mNoTopMargin));
    }
    else {
        if (!isMultiline && lineHeight > mSize.y)
            offsetY = (mSize.y - lineHeight) / 2.0f;
        mTextCache = std::shared_ptr<TextCache>(
            font->buildTextCache(font->wrapText(text, mSize.x, 0.0f, mLineSpacing, isMultiline),
                                 glm::vec2 {0.0f, offsetY}, mColor, mSize.x, mHorizontalAlignment,
                                 mLineSpacing, mNoTopMargin));
    }

    if (mAutoCalcExtent.y)
        mSize.y = mTextCache->metrics.size.y;

    if (mOpacity != 1.0f || mThemeOpacity != 1.0f)
        setOpacity(mOpacity);

    // This is required to set the color transparency.
    onColorChanged();
}

void TextComponent::onColorChanged()
{
    if (mTextCache)
        mTextCache->setColor(mColor);
}

void TextComponent::setHorizontalAlignment(Alignment align)
{
    if (mHorizontalAlignment == align)
        return;

    mHorizontalAlignment = align;
    onTextChanged();
}

void TextComponent::setLineSpacing(float spacing)
{
    if (mLineSpacing == spacing)
        return;

    mLineSpacing = spacing;
    onTextChanged();
}

void TextComponent::setNoTopMargin(bool margin)
{
    if (mNoTopMargin == margin)
        return;

    mNoTopMargin = margin;
    onTextChanged();
}

std::vector<HelpPrompt> TextComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    if (mSelectable)
        prompts.push_back(HelpPrompt("a", _("select")));
    return prompts;
}

void TextComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
                               const std::string& view,
                               const std::string& element,
                               unsigned int properties)
{
    using namespace ThemeFlags;
    GuiComponent::applyTheme(theme, view, element, properties);

    std::string elementType {"text"};
    std::string componentName {"TextComponent"};

    if (element.substr(0, 13) == "gamelistinfo_") {
        elementType = "gamelistinfo";
        componentName = "gamelistInfoComponent";
    }

    const ThemeData::ThemeElement* elem {theme->getElement(view, element, elementType)};
    if (!elem)
        return;

    if (properties & ThemeFlags::POSITION && elem->has("stationary")) {
        const std::string& stationary {elem->get<std::string>("stationary")};
        if (stationary == "never")
            mStationary = Stationary::NEVER;
        else if (stationary == "always")
            mStationary = Stationary::ALWAYS;
        else if (stationary == "withinView")
            mStationary = Stationary::WITHIN_VIEW;
        else if (stationary == "betweenViews")
            mStationary = Stationary::BETWEEN_VIEWS;
        else
            LOG(LogWarning) << componentName
                            << ": Invalid theme configuration, property "
                               "\"stationary\" for element \""
                            << element.substr(elementType == "gamelistinfo" ? 13 : 5)
                            << "\" defined as \"" << stationary << "\"";
    }

    if (elem->has("metadataElement") && elem->get<bool>("metadataElement"))
        mComponentThemeFlags |= ComponentThemeFlags::METADATA_ELEMENT;

    if (properties & COLOR && elem->has("color"))
        setColor(elem->get<unsigned int>("color"));

    setRenderBackground(false);
    if (properties & COLOR && elem->has("backgroundColor")) {
        setBackgroundColor(elem->get<unsigned int>("backgroundColor"));
        setRenderBackground(true);
    }

    if (properties & ALIGNMENT && elem->has("horizontalAlignment")) {
        const std::string& horizontalAlignment {elem->get<std::string>("horizontalAlignment")};
        if (horizontalAlignment == "left")
            setHorizontalAlignment(ALIGN_LEFT);
        else if (horizontalAlignment == "center")
            setHorizontalAlignment(ALIGN_CENTER);
        else if (horizontalAlignment == "right")
            setHorizontalAlignment(ALIGN_RIGHT);
        else
            LOG(LogWarning) << componentName
                            << ": Invalid theme configuration, property "
                               "\"horizontalAlignment\" for element \""
                            << element.substr(elementType == "gamelistinfo" ? 13 : 5)
                            << "\" defined as \"" << horizontalAlignment << "\"";
    }

    if (properties & ALIGNMENT && elem->has("verticalAlignment")) {
        const std::string& verticalAlignment {elem->get<std::string>("verticalAlignment")};
        if (verticalAlignment == "top")
            setVerticalAlignment(ALIGN_TOP);
        else if (verticalAlignment == "center")
            setVerticalAlignment(ALIGN_CENTER);
        else if (verticalAlignment == "bottom")
            setVerticalAlignment(ALIGN_BOTTOM);
        else
            LOG(LogWarning) << componentName
                            << ": Invalid theme configuration, property "
                               "\"verticalAlignment\" for element \""
                            << element.substr(elementType == "gamelistinfo" ? 13 : 5)
                            << "\" defined as \"" << verticalAlignment << "\"";
    }

    if (elem->has("container") && elem->get<bool>("container")) {
        if (elem->has("containerType")) {
            const std::string& containerType {elem->get<std::string>("containerType")};
            if (containerType == "horizontal") {
                if (elem->has("containerScrollSpeed")) {
                    mScrollSpeedMultiplier =
                        glm::clamp(elem->get<float>("containerScrollSpeed"), 0.1f, 10.0f);
                }
                if (elem->has("containerStartDelay")) {
                    mScrollDelay =
                        glm::clamp(elem->get<float>("containerStartDelay"), 0.0f, 10.0f) * 1000.0f;
                }
                if (elem->has("containerScrollGap")) {
                    mScrollGap = glm::clamp(elem->get<float>("containerScrollGap"), 0.1f, 5.0f);
                }
                mHorizontalScrolling = true;
            }
            else if (containerType != "vertical") {
                LOG(LogError) << "TextComponent: Invalid theme configuration, property "
                                 "\"containerType\" for element \""
                              << element.substr(5) << "\" defined as \"" << containerType << "\"";
            }
        }
    }

    if (properties & TEXT && elem->has("text"))
        setText(elem->get<std::string>("text"));

    if (properties & METADATA && elem->has("systemdata")) {
        mThemeSystemdata = "";
        const std::string& systemdata {elem->get<std::string>("systemdata")};

        for (auto& type : supportedSystemdataTypes) {
            if (type == systemdata) {
                mThemeSystemdata = type;
                break;
            }
        }
        if (mThemeSystemdata == "") {
            LOG(LogError) << "TextComponent: Invalid theme configuration, property "
                             "\"systemdata\" for element \""
                          << element.substr(5) << "\" defined as \"" << systemdata << "\"";
        }
    }

    bool systemdataAndMetadata {false};

    if (elem->has("systemdata") && elem->has("metadata")) {
        systemdataAndMetadata = true;
        LOG(LogWarning) << "TextComponent: Invalid theme configuration, element \""
                        << element.substr(5)
                        << "\" has both \"systemdata\" and \"metadata\" properties defined";
    }

    if (!systemdataAndMetadata && properties & METADATA && elem->has("metadata")) {
        mThemeMetadata = "";
        const std::string& metadata {elem->get<std::string>("metadata")};

        for (auto& type : supportedMetadataTypes) {
            if (type == metadata) {
                mThemeMetadata = type;
                if (elem->has("defaultValue")) {
                    if (mThemeMetadata == "developer" || mThemeMetadata == "publisher" ||
                        mThemeMetadata == "genre" || mThemeMetadata == "players" ||
                        mThemeMetadata == "systemName" || mThemeMetadata == "systemFullname" ||
                        mThemeMetadata == "sourceSystemName" ||
                        mThemeMetadata == "sourceSystemFullname") {
                        const std::string& defaultValue {elem->get<std::string>("defaultValue")};
                        if (defaultValue == ":space:")
                            mDefaultValue = " ";
                        else
                            mDefaultValue = defaultValue;
                    }
                }
                if (mThemeMetadata == "name" || mThemeMetadata == "description") {
                    if (elem->has("systemNameSuffix"))
                        mSystemNameSuffix = elem->get<bool>("systemNameSuffix");
                    else
                        mSystemNameSuffix = true;
                }
                break;
            }
        }
        if (mThemeMetadata == "") {
            LOG(LogError) << "TextComponent: Invalid theme configuration, property "
                             "\"metadata\" for element \""
                          << element.substr(5) << "\" defined as \"" << metadata << "\"";
        }
    }

    if (elem->has("backgroundMargins")) {
        const glm::vec2 backgroundMargins {
            glm::clamp(elem->get<glm::vec2>("backgroundMargins"), 0.0f, 0.5f)};
        mBackgroundMargins = backgroundMargins * Renderer::getScreenWidth();
    }

    if (elem->has("backgroundCornerRadius")) {
        mBackgroundCornerRadius =
            glm::clamp(elem->get<float>("backgroundCornerRadius"), 0.0f, 0.5f) *
            mRenderer->getScreenWidth();
    }

    if (properties & LETTER_CASE && elem->has("letterCaseSystemNameSuffix")) {
        const std::string& letterCase {elem->get<std::string>("letterCaseSystemNameSuffix")};
        if (letterCase == "uppercase") {
            mLetterCaseSystemNameSuffix = LetterCase::UPPERCASE;
        }
        else if (letterCase == "lowercase") {
            mLetterCaseSystemNameSuffix = LetterCase::LOWERCASE;
        }
        else if (letterCase == "capitalize") {
            mLetterCaseSystemNameSuffix = LetterCase::CAPITALIZE;
        }
        else {
            LOG(LogWarning) << "TextComponent: Invalid theme configuration, property "
                               "\"letterCaseSystemNameSuffix\" for element \""
                            << element.substr(5) << "\" defined as \"" << letterCase << "\"";
        }
    }

    if (properties & LETTER_CASE && elem->has("letterCase")) {
        const std::string& letterCase {elem->get<std::string>("letterCase")};
        if (letterCase == "uppercase") {
            setUppercase(true);
        }
        else if (letterCase == "lowercase") {
            setLowercase(true);
        }
        else if (letterCase == "capitalize") {
            setCapitalize(true);
        }
        else if (letterCase != "none") {
            LOG(LogWarning) << "TextComponent: Invalid theme configuration, property "
                               "\"letterCase\" for element \""
                            << element.substr(5) << "\" defined as \"" << letterCase << "\"";
        }
    }

    float maxHeight {0.0f};

    if (elem->has("size")) {
        const glm::vec2 size {elem->get<glm::vec2>("size")};
        if (size.x != 0.0f && size.y != 0.0f)
            maxHeight = mSize.y * 2.0f;
    }

    if (properties & LINE_SPACING && elem->has("lineSpacing"))
        setLineSpacing(glm::clamp(elem->get<float>("lineSpacing"), 0.5f, 3.0f));

    setFont(Font::getFromTheme(elem, properties, mFont, maxHeight));

    // We need to do this after setting the font as the scroll speed is calculated from its size.
    if (mHorizontalScrolling)
        setHorizontalScrolling(true);
}
