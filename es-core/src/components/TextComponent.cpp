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
#include "utils/StringUtil.h"

TextComponent::TextComponent(Window* window)
    : GuiComponent{window}
    , mFont{Font::get(FONT_SIZE_MEDIUM)}
    , mColor{0x000000FF}
    , mBgColor{0}
    , mRenderBackground{false}
    , mUppercase{false}
    , mAutoCalcExtent{1, 1}
    , mHorizontalAlignment{ALIGN_LEFT}
    , mVerticalAlignment{ALIGN_CENTER}
    , mLineSpacing{1.5f}
    , mNoTopMargin{false}
    , mSelectable{false}
{
}

TextComponent::TextComponent(Window* window,
                             const std::string& text,
                             const std::shared_ptr<Font>& font,
                             unsigned int color,
                             Alignment align,
                             glm::vec3 pos,
                             glm::vec2 size,
                             unsigned int bgcolor)
    : GuiComponent{window}
    , mFont{nullptr}
    , mColor{0x000000FF}
    , mBgColor{0}
    , mRenderBackground{false}
    , mUppercase{false}
    , mAutoCalcExtent{1, 1}
    , mHorizontalAlignment{align}
    , mVerticalAlignment{ALIGN_CENTER}
    , mLineSpacing{1.5f}
    , mNoTopMargin{false}
    , mSelectable{false}
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
    mAutoCalcExtent = glm::ivec2{(getSize().x == 0), (getSize().y == 0)};
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
    mColorOpacity = mColor & 0x000000FF;
    onColorChanged();
}

//  Set the color of the background box.
void TextComponent::setBackgroundColor(unsigned int color)
{
    mBgColor = color;
    mBgColorOpacity = mBgColor & 0x000000FF;
}

//  Scale the opacity.
void TextComponent::setOpacity(unsigned char opacity)
{
    // This function is mostly called to do fade in and fade out of the text component element.
    // Therefore we assume here that opacity is a fractional value (expressed as an unsigned
    // char 0 - 255) of the opacity originally set with setColor() or setBackgroundColor().
    unsigned char o = static_cast<unsigned char>(static_cast<float>(opacity) / 255.0f *
                                                 static_cast<float>(mColorOpacity));
    mColor = (mColor & 0xFFFFFF00) | static_cast<unsigned char>(o);

    unsigned char bgo = static_cast<unsigned char>(static_cast<float>(opacity) / 255.0f *
                                                   static_cast<float>(mBgColorOpacity));
    mBgColor = (mBgColor & 0xFFFFFF00) | static_cast<unsigned char>(bgo);

    onColorChanged();
    GuiComponent::setOpacity(opacity);
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
    onTextChanged();
}

void TextComponent::render(const glm::mat4& parentTrans)
{
    if (!isVisible())
        return;

    glm::mat4 trans{parentTrans * getTransform()};

    if (mRenderBackground) {
        Renderer::setMatrix(trans);
        Renderer::drawRect(0.0f, 0.0f, mSize.x, mSize.y, mBgColor, mBgColor);
    }

    if (mTextCache) {
        const glm::vec2& textSize{mTextCache->metrics.size};
        float yOff = 0.0f;
        switch (mVerticalAlignment) {
            case ALIGN_TOP: {
                yOff = 0.0f;
                break;
            }
            case ALIGN_BOTTOM: {
                yOff = (getSize().y - textSize.y);
                break;
            }
            case ALIGN_CENTER: {
                yOff = (getSize().y - textSize.y) / 2.0f;
                break;
            }
            default: {
                break;
            }
        }
        glm::vec3 off{0.0f, yOff, 0.0f};

        if (Settings::getInstance()->getBool("DebugText")) {
            // Draw the "textbox" area, what we are aligned within.
            Renderer::setMatrix(trans);
            Renderer::drawRect(0.0f, 0.0f, mSize.x, mSize.y, 0x0000FF33, 0x0000FF33);
        }

        trans = glm::translate(trans, off);
        Renderer::setMatrix(trans);

        // Draw the text area, where the text actually is located.
        if (Settings::getInstance()->getBool("DebugText")) {
            switch (mHorizontalAlignment) {
                case ALIGN_LEFT: {
                    Renderer::drawRect(0.0f, 0.0f, mTextCache->metrics.size.x,
                                       mTextCache->metrics.size.y, 0x00000033, 0x00000033);
                    break;
                }
                case ALIGN_CENTER: {
                    Renderer::drawRect((mSize.x - mTextCache->metrics.size.x) / 2.0f, 0.0f,
                                       mTextCache->metrics.size.x, mTextCache->metrics.size.y,
                                       0x00000033, 0x00000033);
                    break;
                }
                case ALIGN_RIGHT: {
                    Renderer::drawRect(mSize.x - mTextCache->metrics.size.x, 0.0f,
                                       mTextCache->metrics.size.x, mTextCache->metrics.size.y,
                                       0x00000033, 0x00000033);
                    break;
                }
                default: {
                    break;
                }
            }
        }
        mFont->renderTextCache(mTextCache.get());
    }
}

void TextComponent::calculateExtent()
{
    if (mAutoCalcExtent.x) {
        mSize = mFont->sizeText(mUppercase ? Utils::String::toUpper(mText) : mText, mLineSpacing);
    }
    else {
        if (mAutoCalcExtent.y)
            mSize.y = mFont
                          ->sizeWrappedText(mUppercase ? Utils::String::toUpper(mText) : mText,
                                            getSize().x, mLineSpacing)
                          .y;
    }
}

void TextComponent::onTextChanged()
{
    calculateExtent();

    if (!mFont || mText.empty()) {
        mTextCache.reset();
        return;
    }

    std::string text = mUppercase ? Utils::String::toUpper(mText) : mText;

    std::shared_ptr<Font> f = mFont;
    const bool isMultiline = (mSize.y == 0.0f || mSize.y > f->getHeight() * 1.2f);

    bool addAbbrev = false;
    if (!isMultiline) {
        size_t newline = text.find('\n');
        // Single line of text - stop at the first newline since it'll mess everything up.
        text = text.substr(0, newline);
        addAbbrev = newline != std::string::npos;
    }

    glm::vec2 size{f->sizeText(text)};
    if (!isMultiline && mSize.x > 0.0f && text.size() && (size.x > mSize.x || addAbbrev)) {
        // Abbreviate text.
        const std::string abbrev = "...";
        glm::vec2 abbrevSize{f->sizeText(abbrev)};

        while (text.size() && size.x + abbrevSize.x > mSize.x) {
            size_t newSize = Utils::String::prevCursor(text, text.size());
            text.erase(newSize, text.size() - newSize);
            if (!text.empty() && text.back() == ' ')
                text.pop_back();
            size = f->sizeText(text);
        }

        text.append(abbrev);

        mTextCache = std::shared_ptr<TextCache>(
            f->buildTextCache(text, glm::vec2{}, (mColor >> 8 << 8) | mOpacity, mSize.x,
                              mHorizontalAlignment, mLineSpacing, mNoTopMargin));
    }
    else {
        mTextCache = std::shared_ptr<TextCache>(f->buildTextCache(
            f->wrapText(text, mSize.x), glm::vec2{}, (mColor >> 8 << 8) | mOpacity, mSize.x,
            mHorizontalAlignment, mLineSpacing, mNoTopMargin));
    }
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
    GuiComponent::applyTheme(theme, view, element, properties);

    using namespace ThemeFlags;

    const ThemeData::ThemeElement* elem = theme->getElement(view, element, "text");
    if (!elem)
        return;

    if (properties & COLOR && elem->has("color"))
        setColor(elem->get<unsigned int>("color"));

    setRenderBackground(false);
    if (properties & COLOR && elem->has("backgroundColor")) {
        setBackgroundColor(elem->get<unsigned int>("backgroundColor"));
        setRenderBackground(true);
    }

    if (properties & ALIGNMENT && elem->has("alignment")) {
        std::string str = elem->get<std::string>("alignment");
        if (str == "left")
            setHorizontalAlignment(ALIGN_LEFT);
        else if (str == "center")
            setHorizontalAlignment(ALIGN_CENTER);
        else if (str == "right")
            setHorizontalAlignment(ALIGN_RIGHT);
        else
            LOG(LogError) << "Unknown text alignment string: " << str;
    }

    if (properties & TEXT && elem->has("text"))
        setText(elem->get<std::string>("text"));

    if (properties & FORCE_UPPERCASE && elem->has("forceUppercase"))
        setUppercase(elem->get<bool>("forceUppercase"));

    if (properties & LINE_SPACING && elem->has("lineSpacing"))
        setLineSpacing(elem->get<float>("lineSpacing"));

    setFont(Font::getFromTheme(elem, properties, mFont));
}
