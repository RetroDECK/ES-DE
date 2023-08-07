//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  TextComponent.cpp
//
//  Displays text.
//

#include "components/TextComponent.h"

#include "Log.h"
#include "Settings.h"
#include "Window.h"
#include "utils/StringUtil.h"

TextComponent::TextComponent()
    : mFont {Font::get(FONT_SIZE_MEDIUM)}
    , mRenderer {Renderer::getInstance()}
    , mColor {0x000000FF}
    , mBgColor {0x00000000}
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
    , mNoTopMargin {false}
    , mSelectable {false}
    , mVerticalAutoSizing {false}
    , mLoopHorizontal {false}
    , mLoopScroll {false}
    , mLoopSpeed {0.0f}
    , mLoopSpeedMultiplier {1.0f}
    , mLoopDelay {1500.0f}
    , mLoopOffset1 {0}
    , mLoopOffset2 {0}
    , mLoopTime {0}
{
}

TextComponent::TextComponent(const std::string& text,
                             const std::shared_ptr<Font>& font,
                             unsigned int color,
                             Alignment horizontalAlignment,
                             Alignment verticalAlignment,
                             glm::vec3 pos,
                             glm::vec2 size,
                             unsigned int bgcolor)
    : mFont {nullptr}
    , mRenderer {Renderer::getInstance()}
    , mColor {0x000000FF}
    , mBgColor {0x00000000}
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
    , mLineSpacing {1.5f}
    , mNoTopMargin {false}
    , mSelectable {false}
    , mVerticalAutoSizing {false}
    , mLoopHorizontal {false}
    , mLoopScroll {false}
    , mLoopSpeed {0.0f}
    , mLoopSpeedMultiplier {1.0f}
    , mLoopDelay {1500.0f}
    , mLoopOffset1 {0}
    , mLoopOffset2 {0}
    , mLoopTime {0}
{
    setFont(font);
    setColor(color);
    setBackgroundColor(bgcolor);
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
    mColor = color;
    mColorOpacity = static_cast<float>(mColor & 0x000000FF) / 255.0f;
    onColorChanged();
}

//  Set the color of the background box.
void TextComponent::setBackgroundColor(unsigned int color)
{
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
    mSaturation = saturation;
    if (mTextCache)
        mTextCache->setSaturation(saturation);
}

void TextComponent::setDimming(float dimming)
{
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
    mUppercase = uppercase;
    if (uppercase) {
        mLowercase = false;
        mCapitalize = false;
    }
    onTextChanged();
}

void TextComponent::setLowercase(bool lowercase)
{
    mLowercase = lowercase;
    if (lowercase) {
        mUppercase = false;
        mCapitalize = false;
    }
    onTextChanged();
}

void TextComponent::setCapitalize(bool capitalize)
{
    mCapitalize = capitalize;
    if (capitalize) {
        mUppercase = false;
        mLowercase = false;
    }
    onTextChanged();
}

void TextComponent::render(const glm::mat4& parentTrans)
{
    if (!isVisible() || mThemeOpacity == 0.0f || mSize.x == 0.0f || mSize.y == 0.0f)
        return;

    glm::mat4 trans {parentTrans * getTransform()};
    mRenderer->setMatrix(trans);

    // Draw the overall textbox area. If we're inside a vertical scrollable container then
    // this area is rendered inside that component instead of here.
    if (Settings::getInstance()->getBool("DebugText")) {
        if (!mParent || !mParent->isScrollable())
            mRenderer->drawRect(0.0f, 0.0f, mSize.x, mSize.y, 0x0000FF33, 0x0000FF33);
    }

    if (mLoopHorizontal && mTextCache != nullptr) {
        // Clip everything to be inside our bounds.
        glm::vec3 dim {mSize.x, mSize.y, 0.0f};
        dim.x = (trans[0].x * dim.x + trans[3].x) - trans[3].x;
        dim.y = (trans[1].y * dim.y + trans[3].y) - trans[3].y;

        const int clipRectPosX {static_cast<int>(std::round(trans[3].x))};
        const int clipRectPosY {static_cast<int>(std::round(trans[3].y))};
        const int clipRectSizeX {static_cast<int>(std::round(dim.x))};
        const int clipRectSizeY {static_cast<int>(std::round(dim.y) + 1.0f)};

        mRenderer->pushClipRect(glm::ivec2 {clipRectPosX, clipRectPosY},
                                glm::ivec2 {clipRectSizeX, clipRectSizeY});

        float offsetX {0.0f};

        if (mTextCache->metrics.size.x < mSize.x) {
            if (mHorizontalAlignment == Alignment::ALIGN_CENTER)
                offsetX = static_cast<float>((mSize.x - mTextCache->metrics.size.x) / 2.0f);
            else if (mHorizontalAlignment == Alignment::ALIGN_RIGHT)
                offsetX = mSize.x - mTextCache->metrics.size.x;
        }

        trans = glm::translate(trans,
                               glm::vec3 {offsetX - static_cast<float>(mLoopOffset1), 0.0f, 0.0f});
    }

    auto renderFunc = [this](glm::mat4 trans) {
        if (mRenderBackground)
            mRenderer->drawRect(0.0f, 0.0f, mSize.x, mSize.y, mBgColor, mBgColor, false,
                                mOpacity * mThemeOpacity, mDimming);
        if (mTextCache) {
            const glm::vec2& textSize {mTextCache->metrics.size};
            float yOff {0.0f};

            if (mSize.y > textSize.y) {
                switch (mVerticalAlignment) {
                    case ALIGN_TOP: {
                        yOff = 0.0f;
                        break;
                    }
                    case ALIGN_BOTTOM: {
                        yOff = mSize.y - textSize.y;
                        break;
                    }
                    case ALIGN_CENTER: {
                        yOff = (mSize.y - textSize.y) / 2.0f;
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
            else {
                // If height is smaller than the font height, then always center vertically.
                yOff = (mSize.y - textSize.y) / 2.0f;
            }

            trans = glm::translate(trans, glm::vec3 {0.0f, yOff, 0.0f});
            mRenderer->setMatrix(trans);

            // Draw the text area, where the text is actually located.
            if (Settings::getInstance()->getBool("DebugText")) {
                switch (mHorizontalAlignment) {
                    case ALIGN_LEFT: {
                        mRenderer->drawRect(0.0f, 0.0f, mTextCache->metrics.size.x,
                                            mTextCache->metrics.size.y, 0x00000033, 0x00000033);
                        break;
                    }
                    case ALIGN_CENTER: {
                        mRenderer->drawRect(
                            mLoopHorizontal ? 0.0f : (mSize.x - mTextCache->metrics.size.x) / 2.0f,
                            0.0f, mTextCache->metrics.size.x, mTextCache->metrics.size.y,
                            0x00000033, 0x00000033);
                        break;
                    }
                    case ALIGN_RIGHT: {
                        mRenderer->drawRect(mLoopHorizontal ? 0.0f :
                                                              mSize.x - mTextCache->metrics.size.x,
                                            0.0f, mTextCache->metrics.size.x,
                                            mTextCache->metrics.size.y, 0x00000033, 0x00000033);
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
            mFont->renderTextCache(mTextCache.get());
        }
    };

    renderFunc(trans);

    if (mLoopHorizontal && mTextCache != nullptr && mTextCache->metrics.size.x > mSize.x) {
        // Needed to avoid flickering when returning to the start position.
        if (mLoopOffset1 == 0 && mLoopOffset2 == 0)
            mLoopScroll = false;
        // Render again if text has moved far enough for it to repeat.
        if (mLoopOffset2 < 0 || (mLoopDelay != 0.0f && mLoopScroll)) {
            mLoopScroll = true;
            trans = glm::translate(parentTrans * getTransform(),
                                   glm::vec3 {static_cast<float>(-mLoopOffset2), 0.0f, 0.0f});
            mRenderer->setMatrix(trans);
            renderFunc(trans);
        }
    }

    if (mLoopHorizontal && mTextCache != nullptr)
        mRenderer->popClipRect();
}

void TextComponent::setValue(const std::string& value)
{
    if (value == "unknown" && mDefaultValue != "" &&
        (mThemeMetadata == "developer" || mThemeMetadata == "publisher" ||
         mThemeMetadata == "genre" || mThemeMetadata == "players")) {
        setText(mDefaultValue);
    }
    else if (mLoopHorizontal) {
        setText(Utils::String::replace(value, "\n", ""));
    }
    else {
        setText(value);
    }
}

void TextComponent::setHorizontalLooping(bool state)
{
    resetLooping();
    mLoopHorizontal = state;

    if (mLoopHorizontal)
        mLoopSpeed =
            mFont->sizeText("ABCDEFGHIJKLMNOPQRSTUVWXYZ").x * 0.247f * mLoopSpeedMultiplier;
}

void TextComponent::update(int deltaTime)
{
    if (mLoopHorizontal && mTextCache != nullptr) {
        // Don't scroll if the media viewer or screensaver is active or if text scrolling
        // is disabled;
        if (mWindow->isMediaViewerActive() || mWindow->isScreensaverActive() ||
            !mWindow->getAllowTextScrolling()) {
            if (mLoopTime != 0 && !mWindow->isLaunchScreenDisplayed())
                resetLooping();
            return;
        }

        assert(mLoopSpeed != 0.0f);

        mLoopOffset1 = 0;
        mLoopOffset2 = 0;

        if (mTextCache->metrics.size.x > mSize.x) {
            // Loop the text.
            const float scrollLength {mTextCache->metrics.size.x};
            const float returnLength {mLoopSpeed * 1.5f / mLoopSpeedMultiplier};
            const float scrollTime {(scrollLength * 1000.0f) / mLoopSpeed};
            const float returnTime {(returnLength * 1000.0f) / mLoopSpeed};
            const int maxTime {static_cast<int>(mLoopDelay + scrollTime + returnTime)};

            mLoopTime += deltaTime;
            while (mLoopTime > maxTime)
                mLoopTime -= maxTime;

            mLoopOffset1 = static_cast<int>(Utils::Math::loop(mLoopDelay, scrollTime + returnTime,
                                                              static_cast<float>(mLoopTime),
                                                              scrollLength + returnLength));

            if (mLoopOffset1 > (scrollLength - (mSize.x - returnLength)))
                mLoopOffset2 = static_cast<int>(mLoopOffset1 - (scrollLength + returnLength));
            else if (mLoopOffset2 < 0)
                mLoopOffset2 = 0;
        }
    }
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
        // This can happen under special circumstances like when a blank/dummy font is used.
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

    const bool isMultiline {mAutoCalcExtent.y == 1 || mSize.y > lineHeight};

    if (mLoopHorizontal) {
        mTextCache = std::shared_ptr<TextCache>(font->buildTextCache(text, 0.0f, 0.0f, mColor));
    }
    else if (isMultiline && !isScrollable) {
        const std::string wrappedText {
            font->wrapText(text, mSize.x, (mVerticalAutoSizing ? 0.0f : mSize.y - lineHeight),
                           mLineSpacing, isMultiline)};
        mTextCache = std::shared_ptr<TextCache>(
            font->buildTextCache(wrappedText, glm::vec2 {0.0f, 0.0f}, mColor, mSize.x,
                                 mHorizontalAlignment, mLineSpacing, mNoTopMargin));
    }
    else {
        mTextCache = std::shared_ptr<TextCache>(font->buildTextCache(
            font->wrapText(text, mSize.x, 0.0f, mLineSpacing, isMultiline), glm::vec2 {0.0f, 0.0f},
            mColor, mSize.x, mHorizontalAlignment, mLineSpacing, mNoTopMargin));
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
    mHorizontalAlignment = align;
    onTextChanged();
}

void TextComponent::setLineSpacing(float spacing)
{
    mLineSpacing = spacing;
    onTextChanged();
}

void TextComponent::setNoTopMargin(bool margin)
{
    mNoTopMargin = margin;
    onTextChanged();
}

std::vector<HelpPrompt> TextComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    if (mSelectable)
        prompts.push_back(HelpPrompt("a", "select"));
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
                    mLoopSpeedMultiplier =
                        glm::clamp(elem->get<float>("containerScrollSpeed"), 0.1f, 10.0f);
                }
                if (elem->has("containerStartDelay")) {
                    mLoopDelay =
                        glm::clamp(elem->get<float>("containerStartDelay"), 0.0f, 10.0f) * 1000.0f;
                }
                mLoopHorizontal = true;
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
                        mThemeMetadata == "genre" || mThemeMetadata == "players") {
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

    setFont(Font::getFromTheme(elem, properties, mFont, maxHeight, false));

    // We need to do this after setting the font as the loop speed is calculated from its size.
    if (mLoopHorizontal)
        setHorizontalLooping(true);
}
