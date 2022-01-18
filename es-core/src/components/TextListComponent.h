//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  TextListComponent.h
//
//  Text list used for displaying and navigating the gamelist views.
//

#ifndef ES_CORE_COMPONENTS_TEXT_LIST_COMPONENT_H
#define ES_CORE_COMPONENTS_TEXT_LIST_COMPONENT_H

#include "Log.h"
#include "Sound.h"
#include "components/IList.h"
#include "resources/Font.h"
#include "utils/StringUtil.h"

#include <memory>

class TextCache;

struct TextListData {
    unsigned int colorId;
    std::shared_ptr<TextCache> textCache;
};

// A scrollable text list supporting multiple row colors.
template <typename T> class TextListComponent : public IList<TextListData, T>
{
    using List = IList<TextListData, T>;

protected:
    using List::mCursor;
    using List::mEntries;
    using List::mSize;
    using List::mWindow;

public:
    using GuiComponent::setColor;
    using List::size;

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

    void setAlignment(Alignment align)
    {
        // Set alignment.
        mAlignment = align;
    }

    void setCursorChangedCallback(const std::function<void(CursorState state)>& func)
    {
        mCursorChangedCallback = func;
    }

    void setFont(const std::shared_ptr<Font>& font)
    {
        mFont = font;
        for (auto it = mEntries.begin(); it != mEntries.end(); ++it)
            it->data.textCache.reset();
    }

    void setUppercase(bool uppercase)
    {
        mUppercase = uppercase;
        for (auto it = mEntries.begin(); it != mEntries.end(); ++it)
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
    void onScroll() override
    {
        if (!NavigationSounds::getInstance().isPlayingThemeNavigationSound(SCROLLSOUND))
            NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
    }
    void onCursorChanged(const CursorState& state) override;

private:
    int mLoopOffset;
    int mLoopOffset2;
    int mLoopTime;
    bool mLoopScroll;

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
    : IList<TextListData, T> {window}
    , mSelectorImage {window}
{
    mLoopOffset = 0;
    mLoopOffset2 = 0;
    mLoopTime = 0;
    mLoopScroll = false;

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

    glm::mat4 trans {parentTrans * List::getTransform()};
    std::shared_ptr<Font>& font {mFont};

    int startEntry {0};
    float y {0.0f};

    const float entrySize {std::max(floorf(font->getHeight(1.0f)),
                                    floorf(static_cast<float>(font->getSize())) * mLineSpacing)};
    const float lineSpacingHeight {floorf(font->getHeight(mLineSpacing) - font->getHeight(1.0f))};

    // This extra vertical margin is technically incorrect, but it adds a little extra leeway
    // to avoid removing the last row on some older theme sets. There was a sizing bug in the
    // RetroPie fork of EmulationStation and some theme authors set sizes that are just slightly
    // too small for the last row to show up when the sizing calculation is done correctly.
    const float extraMargin {(Renderer::getScreenHeightModifier() >= 1.0f ? 3.0f : 0.0f)};

    // Number of entries that can fit on the screen simultaneously.
    int screenCount {
        static_cast<int>(floorf((mSize.y + lineSpacingHeight / 2.0f + extraMargin) / entrySize))};

    if (size() >= screenCount) {
        startEntry = mCursor - screenCount / 2;
        if (startEntry < 0)
            startEntry = 0;
        if (startEntry >= size() - screenCount)
            startEntry = size() - screenCount;
    }

    int listCutoff {startEntry + screenCount};
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
            Renderer::drawRect(0.0f, (mCursor - startEntry) * entrySize + mSelectorOffsetY, mSize.x,
                               mSelectorHeight, mSelectorColor, mSelectorColorEnd,
                               mSelectorColorGradientHorizontal);
        }
    }

    if (Settings::getInstance()->getBool("DebugText")) {
        Renderer::drawRect(mHorizontalMargin, 0.0f, mSize.x - mHorizontalMargin * 2.0f, mSize.y,
                           0x00000033, 0x00000033);
        Renderer::drawRect(0.0f, 0.0f, mSize.x, mSize.y, 0x0000FF33, 0x0000FF33);
    }

    // Clip to inside margins.
    glm::vec3 dim {mSize.x, mSize.y, 0.0f};
    dim.x = (trans[0].x * dim.x + trans[3].x) - trans[3].x;
    dim.y = (trans[1].y * dim.y + trans[3].y) - trans[3].y;

    Renderer::pushClipRect(
        glm::ivec2 {static_cast<int>(std::round(trans[3].x + mHorizontalMargin)),
                    static_cast<int>(std::round(trans[3].y))},
        glm::ivec2 {static_cast<int>(std::round(dim.x - mHorizontalMargin * 2.0f)),
                    static_cast<int>(std::round(dim.y))});

    for (int i = startEntry; i < listCutoff; ++i) {
        typename IList<TextListData, T>::Entry& entry {mEntries.at(static_cast<unsigned int>(i))};

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

        glm::vec3 offset {0.0f, y, 0.0f};

        switch (mAlignment) {
            case ALIGN_LEFT:
                offset.x = mHorizontalMargin;
                break;
            case ALIGN_CENTER:
                offset.x =
                    static_cast<float>((mSize.x - entry.data.textCache->metrics.size.x) / 2.0f);
                if (offset.x < mHorizontalMargin)
                    offset.x = mHorizontalMargin;
                break;
            case ALIGN_RIGHT:
                offset.x = (mSize.x - entry.data.textCache->metrics.size.x);
                offset.x -= mHorizontalMargin;
                if (offset.x < mHorizontalMargin)
                    offset.x = mHorizontalMargin;
                break;
        }

        // Render text.
        glm::mat4 drawTrans {trans};

        // Currently selected item text might be looping.
        if (mCursor == i && mLoopOffset > 0)
            drawTrans = glm::translate(
                drawTrans, offset - glm::vec3 {static_cast<float>(mLoopOffset), 0.0f, 0.0f});
        else
            drawTrans = glm::translate(drawTrans, offset);

        // Needed to avoid flickering when returning to the start position.
        if (mLoopOffset == 0 && mLoopOffset2 == 0)
            mLoopScroll = false;

        Renderer::setMatrix(drawTrans);
        font->renderTextCache(entry.data.textCache.get());

        // Render currently selected row again if text is moved far enough for it to repeat.
        if ((mCursor == i && mLoopOffset2 < 0) || (mCursor == i && mLoopScroll)) {
            mLoopScroll = true;
            drawTrans = trans;
            drawTrans = glm::translate(
                drawTrans, offset - glm::vec3 {static_cast<float>(mLoopOffset2), 0.0f, 0.0f});
            Renderer::setMatrix(drawTrans);
            font->renderTextCache(entry.data.textCache.get());
        }
        y += entrySize;
    }
    Renderer::popClipRect();
    List::listRenderTitleOverlay(trans);
    GuiComponent::renderChildren(trans);
}

template <typename T> bool TextListComponent<T>::input(InputConfig* config, Input input)
{
    if (size() > 0) {
        if (input.value != 0) {
            if (config->isMappedLike("down", input)) {
                List::listInput(1);
                return true;
            }

            if (config->isMappedLike("up", input)) {
                List::listInput(-1);
                return true;
            }
            if (config->isMappedLike("rightshoulder", input)) {
                List::listInput(10);
                return true;
            }

            if (config->isMappedLike("leftshoulder", input)) {
                List::listInput(-10);
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
                List::stopScrolling();
        }
    }

    return GuiComponent::input(config, input);
}

template <typename T> void TextListComponent<T>::update(int deltaTime)
{
    List::listUpdate(deltaTime);

    if (mWindow->isScreensaverActive() || !mWindow->getAllowTextScrolling())
        List::stopScrolling();

    if (!List::isScrolling() && size() > 0) {
        // Always reset the loop offsets.
        mLoopOffset = 0;
        mLoopOffset2 = 0;

        // If we're not scrolling and this object's text exceeds our size, then loop it.
        const float textLength {mFont
                                    ->sizeText(Utils::String::toUpper(
                                        mEntries.at(static_cast<unsigned int>(mCursor)).name))
                                    .x};
        const float limit {mSize.x - mHorizontalMargin * 2.0f};

        if (textLength > limit) {
            // Loop the text.
            const float speed {mFont->sizeText("ABCDEFGHIJKLMNOPQRSTUVWXYZ").x * 0.247f};
            const float delay {3000.0f};
            const float scrollLength {textLength};
            const float returnLength {speed * 1.5f};
            const float scrollTime {(scrollLength * 1000.0f) / speed};
            const float returnTime {(returnLength * 1000.0f) / speed};
            const int maxTime {static_cast<int>(delay + scrollTime + returnTime)};

            mLoopTime += deltaTime;
            while (mLoopTime > maxTime)
                mLoopTime -= maxTime;

            mLoopOffset = static_cast<int>(Utils::Math::loop(delay, scrollTime + returnTime,
                                                             static_cast<float>(mLoopTime),
                                                             scrollLength + returnLength));

            if (mLoopOffset > (scrollLength - (limit - returnLength)))
                mLoopOffset2 = static_cast<int>(mLoopOffset - (scrollLength + returnLength));
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
    mLoopOffset = 0;
    mLoopOffset2 = 0;
    mLoopTime = 0;

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

    const ThemeData::ThemeElement* elem {theme->getElement(view, element, "textlist")};
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
    const float selectorHeight {
        std::max(mFont->getHeight(1.0), static_cast<float>(mFont->getSize())) * mLineSpacing};
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
                                (this->mParent ? this->mParent->getSize().x :
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
            float scale = this->mParent ? this->mParent->getSize().y :
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
        mSelectorImage.setSize(mSize.x, mSelectorHeight);
        mSelectorImage.setColorShift(mSelectorColor);
        mSelectorImage.setColorShiftEnd(mSelectorColorEnd);
    }
    else {
        mSelectorImage.setImage("");
    }
}

#endif // ES_CORE_COMPONENTS_TEXT_LIST_COMPONENT_H
