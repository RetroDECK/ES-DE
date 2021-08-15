//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  TextListComponent.h
//
//  Text list used for displaying and navigating the gamelist views.
//

#ifndef ES_APP_COMPONENTS_TEXT_LIST_COMPONENT_H
#define ES_APP_COMPONENTS_TEXT_LIST_COMPONENT_H

#include "Log.h"
#include "Sound.h"
#include "components/IList.h"
#include "math/Misc.h"
#include "resources/Font.h"
#include "utils/StringUtil.h"

#include <memory>

class TextCache;

struct TextListData {
    unsigned int colorId;
    std::shared_ptr<TextCache> textCache;
};

// A graphical list. Supports multiple colors for rows and scrolling.
template <typename T> class TextListComponent : public IList<TextListData, T>
{
protected:
    using IList<TextListData, T>::mEntries;
    using IList<TextListData, T>::listUpdate;
    using IList<TextListData, T>::listInput;
    using IList<TextListData, T>::listRenderTitleOverlay;
    using IList<TextListData, T>::getTransform;
    using IList<TextListData, T>::mSize;
    using IList<TextListData, T>::mCursor;
    using IList<TextListData, T>::mWindow;
    using IList<TextListData, T>::IList;

public:
    using IList<TextListData, T>::size;
    using IList<TextListData, T>::isScrolling;
    using IList<TextListData, T>::stopScrolling;

    TextListComponent(Window* window);

    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;
    void applyTheme(const std::shared_ptr<ThemeData>& theme,
                    const std::string& view,
                    const std::string& element,
                    unsigned int properties) override;

    void add(const std::string& name, const T& obj, unsigned int colorId);

    enum Alignment {
        ALIGN_LEFT, // Replace with AllowShortEnumsOnASingleLine: false (clang-format >=11.0).
        ALIGN_CENTER,
        ALIGN_RIGHT
    };

    void setAlignment(Alignment align) { mAlignment = align; }

    void setCursorChangedCallback(const std::function<void(CursorState state)>& func)
    {
        mCursorChangedCallback = func;
    }

    void setFont(const std::shared_ptr<Font>& font)
    {
        mFont = font;
        for (auto it = mEntries.begin(); it != mEntries.end(); it++)
            it->data.textCache.reset();
    }

    void setUppercase(bool /*uppercase*/)
    {
        mUppercase = true;
        for (auto it = mEntries.begin(); it != mEntries.end(); it++)
            it->data.textCache.reset();
    }

    void setSelectorHeight(float selectorScale) { mSelectorHeight = selectorScale; }
    void setSelectorOffsetY(float selectorOffsetY) { mSelectorOffsetY = selectorOffsetY; }
    void setSelectorColor(unsigned int color) { mSelectorColor = color; }
    void setSelectorColorEnd(unsigned int color) { mSelectorColorEnd = color; }
    void setSelectorColorGradientHorizontal(bool horizontal)
    {
        mSelectorColorGradientHorizontal = horizontal;
    }
    void setSelectedColor(unsigned int color) { mSelectedColor = color; }
    void setColor(unsigned int id, unsigned int color) { mColors[id] = color; }
    void setLineSpacing(float lineSpacing) { mLineSpacing = lineSpacing; }

protected:
    virtual void onScroll() override
    {
        if (!NavigationSounds::getInstance()->isPlayingThemeNavigationSound(SCROLLSOUND))
            NavigationSounds::getInstance()->playThemeNavigationSound(SCROLLSOUND);
    }
    virtual void onCursorChanged(const CursorState& state) override;

private:
    int mMarqueeOffset;
    int mMarqueeOffset2;
    int mMarqueeTime;
    bool mMarqueeScroll;

    Alignment mAlignment;
    float mHorizontalMargin;

    std::function<void(CursorState state)> mCursorChangedCallback;

    std::shared_ptr<Font> mFont;
    bool mUppercase;
    float mLineSpacing;
    float mSelectorHeight;
    float mSelectorOffsetY;
    unsigned int mSelectorColor;
    unsigned int mSelectorColorEnd;
    bool mSelectorColorGradientHorizontal = true;
    unsigned int mSelectedColor;
    static const unsigned int COLOR_ID_COUNT = 2;
    unsigned int mColors[COLOR_ID_COUNT];

    ImageComponent mSelectorImage;
};

template <typename T>
TextListComponent<T>::TextListComponent(Window* window)
    : IList<TextListData, T>(window)
    , mSelectorImage(window)
{
    mMarqueeOffset = 0;
    mMarqueeOffset2 = 0;
    mMarqueeTime = 0;
    mMarqueeScroll = false;

    mHorizontalMargin = 0.0f;
    mAlignment = ALIGN_CENTER;

    mFont = Font::get(FONT_SIZE_MEDIUM);
    mUppercase = false;
    mLineSpacing = 1.5f;
    mSelectorHeight = mFont->getSize() * 1.5f;
    mSelectorOffsetY = 0;
    mSelectorColor = 0x000000FF;
    mSelectorColorEnd = 0x000000FF;
    mSelectorColorGradientHorizontal = true;
    mSelectedColor = 0;
    mColors[0] = 0x0000FFFF;
    mColors[1] = 0x00FF00FF;
}

template <typename T> void TextListComponent<T>::render(const glm::mat4& parentTrans)
{
    if (size() == 0)
        return;

    glm::mat4 trans = parentTrans * getTransform();
    std::shared_ptr<Font>& font = mFont;

    int startEntry = 0;
    float y = 0;

    const float entrySize =
        std::max(font->getHeight(1.0), static_cast<float>(font->getSize())) * mLineSpacing;

    // Number of entries that can fit on the screen simultaneously.
    int screenCount = static_cast<int>(mSize.y() / entrySize + 0.5f);

    if (size() >= screenCount) {
        startEntry = mCursor - screenCount / 2;
        if (startEntry < 0)
            startEntry = 0;
        if (startEntry >= size() - screenCount)
            startEntry = size() - screenCount;
    }

    int listCutoff = startEntry + screenCount;
    if (listCutoff > size())
        listCutoff = size();

    // Draw selector bar.
    if (startEntry < listCutoff) {
        if (mSelectorImage.hasImage()) {
            mSelectorImage.setPosition(0.0f, (mCursor - startEntry) * entrySize + mSelectorOffsetY,
                                       0.0f);
            mSelectorImage.render(trans);
        }
        else {
            Renderer::setMatrix(trans);
            Renderer::drawRect(0.0f, (mCursor - startEntry) * entrySize + mSelectorOffsetY,
                               mSize.x(), mSelectorHeight, mSelectorColor, mSelectorColorEnd,
                               mSelectorColorGradientHorizontal);
        }
    }

    if (Settings::getInstance()->getBool("DebugText")) {
        Renderer::drawRect(mHorizontalMargin, 0.0f, mSize.x() - mHorizontalMargin * 2.0f, mSize.y(),
                           0x00000033, 0x00000033);
        Renderer::drawRect(0.0f, 0.0f, mSize.x(), mSize.y(), 0x0000FF33, 0x0000FF33);
    }

    // Clip to inside margins.
    glm::vec3 dim(mSize.x(), mSize.y(), 0.0f);
    dim.x = (trans[0].x * dim.x + trans[3].x) - trans[3].x;
    dim.y = (trans[1].y * dim.y + trans[3].y) - trans[3].y;

    Renderer::pushClipRect(
        Vector2i(static_cast<int>(trans[3].x + mHorizontalMargin), static_cast<int>(trans[3].y)),
        Vector2i(static_cast<int>(dim.x - mHorizontalMargin * 2.0f), static_cast<int>(dim.y)));

    for (int i = startEntry; i < listCutoff; i++) {
        typename IList<TextListData, T>::Entry& entry = mEntries.at(static_cast<unsigned int>(i));

        unsigned int color;
        if (mCursor == i && mSelectedColor)
            color = mSelectedColor;
        else
            color = mColors[entry.data.colorId];

        if (!entry.data.textCache)
            entry.data.textCache = std::unique_ptr<TextCache>(font->buildTextCache(
                mUppercase ? Utils::String::toUpper(entry.name) : entry.name, 0, 0, 0x000000FF));

        // If a game is marked as hidden, lower the text opacity a lot.
        // If a game is marked to not be counted, lower the opacity a moderate amount.
        if (entry.object->getHidden())
            entry.data.textCache->setColor(color & 0xFFFFFF44);
        else if (!entry.object->getCountAsGame())
            entry.data.textCache->setColor(color & 0xFFFFFF77);
        else
            entry.data.textCache->setColor(color);

        glm::vec3 offset(0.0f, y, 0.0f);

        switch (mAlignment) {
            case ALIGN_LEFT:
                offset.x = mHorizontalMargin;
                break;
            case ALIGN_CENTER:
                offset.x =
                    static_cast<float>((mSize.x() - entry.data.textCache->metrics.size.x()) / 2.0f);
                if (offset.x < mHorizontalMargin)
                    offset.x = mHorizontalMargin;
                break;
            case ALIGN_RIGHT:
                offset.x = (mSize.x() - entry.data.textCache->metrics.size.x());
                offset.x -= mHorizontalMargin;
                if (offset.x < mHorizontalMargin)
                    offset.x = mHorizontalMargin;
                break;
        }

        // Render text.
        glm::mat4 drawTrans = trans;

        // Currently selected item text might be scrolling.
        if (mCursor == i && mMarqueeOffset > 0)
            drawTrans = glm::translate(
                drawTrans, offset - glm::vec3(static_cast<float>(mMarqueeOffset), 0.0f, 0.0f));
        else
            drawTrans = glm::translate(drawTrans, offset);

        // Needed to avoid flickering when returning to the start position.
        if (mMarqueeOffset == 0 && mMarqueeOffset2 == 0)
            mMarqueeScroll = false;

        Renderer::setMatrix(drawTrans);
        font->renderTextCache(entry.data.textCache.get());

        // Render currently selected row again if marquee is scrolled far enough for it to repeat.
        if ((mCursor == i && mMarqueeOffset2 < 0) || (mCursor == i && mMarqueeScroll)) {
            mMarqueeScroll = true;
            drawTrans = trans;
            drawTrans = glm::translate(
                drawTrans, offset - glm::vec3(static_cast<float>(mMarqueeOffset2), 0.0f, 0.0f));
            Renderer::setMatrix(drawTrans);
            font->renderTextCache(entry.data.textCache.get());
        }
        y += entrySize;
    }
    Renderer::popClipRect();
    listRenderTitleOverlay(trans);
    GuiComponent::renderChildren(trans);
}

template <typename T> bool TextListComponent<T>::input(InputConfig* config, Input input)
{
    if (size() > 0) {
        if (input.value != 0) {
            if (config->isMappedLike("down", input)) {
                listInput(1);
                return true;
            }

            if (config->isMappedLike("up", input)) {
                listInput(-1);
                return true;
            }
            if (config->isMappedLike("rightshoulder", input)) {
                listInput(10);
                return true;
            }

            if (config->isMappedLike("leftshoulder", input)) {
                listInput(-10);
                return true;
            }

            if (config->isMappedLike("righttrigger", input)) {
                return this->listLastRow();
            }

            if (config->isMappedLike("lefttrigger", input)) {
                return this->listFirstRow();
            }
        }
        else {
            if (config->isMappedLike("down", input) || config->isMappedLike("up", input) ||
                config->isMappedLike("rightshoulder", input) ||
                config->isMappedLike("leftshoulder", input) ||
                config->isMappedLike("lefttrigger", input) ||
                config->isMappedLike("righttrigger", input))
                stopScrolling();
        }
    }

    return GuiComponent::input(config, input);
}

template <typename T> void TextListComponent<T>::update(int deltaTime)
{
    listUpdate(deltaTime);

    if (mWindow->isScreensaverActive() || !mWindow->getAllowTextScrolling())
        stopScrolling();

    if (!isScrolling() && size() > 0) {
        // Always reset the marquee offsets.
        mMarqueeOffset = 0;
        mMarqueeOffset2 = 0;

        // If we're not scrolling and this object's text exceeds our size, then marquee it.
        const float textLength = mFont
                                     ->sizeText(Utils::String::toUpper(
                                         mEntries.at(static_cast<unsigned int>(mCursor)).name))
                                     .x();
        const float limit = mSize.x() - mHorizontalMargin * 2.0f;

        if (textLength > limit) {
            // Loop.
            // Pixels per second (based on nes-mini font at 1920x1080 to produce a speed of 200).
            const float speed = mFont->sizeText("ABCDEFGHIJKLMNOPQRSTUVWXYZ").x() * 0.247f;
            const float delay = 3000.0f;
            const float scrollLength = textLength;
            const float returnLength = speed * 1.5f;
            const float scrollTime = (scrollLength * 1000.0f) / speed;
            const float returnTime = (returnLength * 1000.0f) / speed;
            const int maxTime = static_cast<int>(delay + scrollTime + returnTime);

            mMarqueeTime += deltaTime;
            while (mMarqueeTime > maxTime)
                mMarqueeTime -= maxTime;

            mMarqueeOffset = static_cast<int>(Math::Scroll::loop(delay, scrollTime + returnTime,
                                                                 static_cast<float>(mMarqueeTime),
                                                                 scrollLength + returnLength));

            if (mMarqueeOffset > (scrollLength - (limit - returnLength)))
                mMarqueeOffset2 = static_cast<int>(mMarqueeOffset - (scrollLength + returnLength));
        }
    }

    GuiComponent::update(deltaTime);
}

// List management stuff.
template <typename T>
void TextListComponent<T>::add(const std::string& name, const T& obj, unsigned int color)
{
    assert(color < COLOR_ID_COUNT);

    typename IList<TextListData, T>::Entry entry;
    entry.name = name;
    entry.object = obj;
    entry.data.colorId = color;
    static_cast<IList<TextListData, T>*>(this)->add(entry);
}

template <typename T> void TextListComponent<T>::onCursorChanged(const CursorState& state)
{
    mMarqueeOffset = 0;
    mMarqueeOffset2 = 0;
    mMarqueeTime = 0;

    if (mCursorChangedCallback)
        mCursorChangedCallback(state);
}

template <typename T>
void TextListComponent<T>::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                      const std::string& view,
                                      const std::string& element,
                                      unsigned int properties)
{
    GuiComponent::applyTheme(theme, view, element, properties);

    const ThemeData::ThemeElement* elem = theme->getElement(view, element, "textlist");
    if (!elem)
        return;

    using namespace ThemeFlags;
    if (properties & COLOR) {
        if (elem->has("selectorColor")) {
            setSelectorColor(elem->get<unsigned int>("selectorColor"));
            setSelectorColorEnd(elem->get<unsigned int>("selectorColor"));
        }
        if (elem->has("selectorColorEnd"))
            setSelectorColorEnd(elem->get<unsigned int>("selectorColorEnd"));
        if (elem->has("selectorGradientType"))
            setSelectorColorGradientHorizontal(
                !(elem->get<std::string>("selectorGradientType").compare("horizontal")));
        if (elem->has("selectedColor"))
            setSelectedColor(elem->get<unsigned int>("selectedColor"));
        if (elem->has("primaryColor"))
            setColor(0, elem->get<unsigned int>("primaryColor"));
        if (elem->has("secondaryColor"))
            setColor(1, elem->get<unsigned int>("secondaryColor"));
    }

    setFont(Font::getFromTheme(elem, properties, mFont));
    const float selectorHeight =
        std::max(mFont->getHeight(1.0), static_cast<float>(mFont->getSize())) * mLineSpacing;
    setSelectorHeight(selectorHeight);

    if (properties & ALIGNMENT) {
        if (elem->has("alignment")) {
            const std::string& str = elem->get<std::string>("alignment");
            if (str == "left")
                setAlignment(ALIGN_LEFT);
            else if (str == "center")
                setAlignment(ALIGN_CENTER);
            else if (str == "right")
                setAlignment(ALIGN_RIGHT);
            else
                LOG(LogError) << "Unknown TextListComponent alignment \"" << str << "\"!";
        }
        if (elem->has("horizontalMargin")) {
            mHorizontalMargin = elem->get<float>("horizontalMargin") *
                                (this->mParent ? this->mParent->getSize().x() :
                                                 static_cast<float>(Renderer::getScreenWidth()));
        }
    }

    if (properties & FORCE_UPPERCASE && elem->has("forceUppercase"))
        setUppercase(elem->get<bool>("forceUppercase"));

    if (properties & LINE_SPACING) {
        if (elem->has("lineSpacing"))
            setLineSpacing(elem->get<float>("lineSpacing"));
        if (elem->has("selectorHeight"))
            setSelectorHeight(elem->get<float>("selectorHeight") * Renderer::getScreenHeight());
        if (elem->has("selectorOffsetY")) {
            float scale = this->mParent ? this->mParent->getSize().y() :
                                          static_cast<float>(Renderer::getScreenHeight());
            setSelectorOffsetY(elem->get<float>("selectorOffsetY") * scale);
        }
        else {
            setSelectorOffsetY(0.0);
        }
    }

    if (elem->has("selectorImagePath")) {
        std::string path = elem->get<std::string>("selectorImagePath");
        bool tile = elem->has("selectorImageTile") && elem->get<bool>("selectorImageTile");
        mSelectorImage.setImage(path, tile);
        mSelectorImage.setSize(mSize.x(), mSelectorHeight);
        mSelectorImage.setColorShift(mSelectorColor);
        mSelectorImage.setColorShiftEnd(mSelectorColorEnd);
    }
    else {
        mSelectorImage.setImage("");
    }
}

#endif // ES_APP_COMPONENTS_TEXT_LIST_COMPONENT_H
