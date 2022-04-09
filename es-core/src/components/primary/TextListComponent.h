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
#include "components/primary/PrimaryComponent.h"
#include "resources/Font.h"

struct TextListData {
    unsigned int colorId;
    std::shared_ptr<TextCache> textCache;
};

template <typename T>
class TextListComponent : public PrimaryComponent<T>, private IList<TextListData, T>
{
    using List = IList<TextListData, T>;

protected:
    using List::mCursor;
    using List::mEntries;
    using List::mScrollVelocity;
    using List::mSize;
    using List::mWindow;

public:
    using List::size;
    using Entry = typename IList<TextListData, T>::Entry;
    using PrimaryAlignment = typename PrimaryComponent<T>::PrimaryAlignment;
    using GuiComponent::setColor;

    TextListComponent();

    void addEntry(Entry& entry, const std::shared_ptr<ThemeData>& theme = nullptr);

    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;
    void applyTheme(const std::shared_ptr<ThemeData>& theme,
                    const std::string& view,
                    const std::string& element,
                    unsigned int properties) override;

    void setAlignment(PrimaryAlignment align) override { mAlignment = align; }

    void setCursorChangedCallback(const std::function<void(CursorState state)>& func) override
    {
        mCursorChangedCallback = func;
    }
    void setCancelTransitionsCallback(const std::function<void()>& func) override
    {
        mCancelTransitionsCallback = func;
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

        if (uppercase) {
            mLowercase = false;
            mCapitalize = false;
        }

        for (auto it = mEntries.begin(); it != mEntries.end(); ++it)
            it->data.textCache.reset();
    }

    void setLowercase(bool lowercase)
    {
        mLowercase = lowercase;

        if (lowercase) {
            mUppercase = false;
            mCapitalize = false;
        }

        for (auto it = mEntries.begin(); it != mEntries.end(); ++it)
            it->data.textCache.reset();
    }

    void setCapitalize(bool capitalize)
    {
        mCapitalize = capitalize;

        if (capitalize) {
            mUppercase = false;
            mLowercase = false;
        }

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
    bool isScrolling() const override { return List::isScrolling(); }
    void stopScrolling() override { List::stopScrolling(); }
    const int getScrollingVelocity() override { return List::getScrollingVelocity(); }
    void clear() override { List::clear(); }
    const T& getSelected() const override { return List::getSelected(); }
    const T& getNext() const override { return List::getNext(); }
    const T& getPrevious() const override { return List::getPrevious(); }
    const T& getFirst() const override { return List::getFirst(); }
    const T& getLast() const override { return List::getLast(); }
    bool setCursor(const T& obj) override { return List::setCursor(obj); }
    bool remove(const T& obj) override { return List::remove(obj); }
    int size() const override { return List::size(); }

    int getCursor() override { return mCursor; }
    const size_t getNumEntries() override { return mEntries.size(); }

    Renderer* mRenderer;
    std::function<void()> mCancelTransitionsCallback;
    float mCamOffset;
    int mPreviousScrollVelocity;

    int mLoopOffset;
    int mLoopOffset2;
    int mLoopTime;
    bool mLoopScroll;

    PrimaryAlignment mAlignment;
    float mHorizontalMargin;

    std::function<void(CursorState state)> mCursorChangedCallback;
    ImageComponent mSelectorImage;

    std::shared_ptr<Font> mFont;
    bool mUppercase;
    bool mLowercase;
    bool mCapitalize;
    float mLineSpacing;
    float mSelectorHeight;
    float mSelectorOffsetY;
    unsigned int mSelectorColor;
    unsigned int mSelectorColorEnd;
    bool mSelectorColorGradientHorizontal = true;
    unsigned int mSelectedColor;
    static const unsigned int COLOR_ID_COUNT {2};
    unsigned int mColors[COLOR_ID_COUNT];
};

template <typename T>
TextListComponent<T>::TextListComponent()
    : IList<TextListData, T> {(std::is_same_v<T, SystemData*> ? LIST_SCROLL_STYLE_SLOW :
                                                                LIST_SCROLL_STYLE_QUICK),
                              ListLoopType::LIST_PAUSE_AT_END}
    , mRenderer {Renderer::getInstance()}
    , mCamOffset {0.0f}
    , mPreviousScrollVelocity {0}
    , mLoopOffset {0}
    , mLoopOffset2 {0}
    , mLoopTime {0}
    , mLoopScroll {false}
    , mAlignment {PrimaryAlignment::ALIGN_CENTER}
    , mHorizontalMargin {0.0f}
    , mFont {Font::get(FONT_SIZE_MEDIUM)}
    , mUppercase {false}
    , mLowercase {false}
    , mCapitalize {false}
    , mLineSpacing {1.5f}
    , mSelectorHeight {mFont->getSize() * 1.5f}
    , mSelectorOffsetY {0.0f}
    , mSelectorColor {0x000000FF}
    , mSelectorColorEnd {0x000000FF}
    , mSelectorColorGradientHorizontal {true}
    , mSelectedColor {0}
    , mColors {0x0000FFFF, 0x00FF00FF}
{
}

template <typename T>
void TextListComponent<T>::addEntry(Entry& entry, const std::shared_ptr<ThemeData>& theme)
{
    List::add(entry);
}

template <typename T> bool TextListComponent<T>::input(InputConfig* config, Input input)
{
    if (size() > 0) {
        if (input.value != 0) {
            if (config->isMappedLike("up", input)) {
                if (mCancelTransitionsCallback)
                    mCancelTransitionsCallback();
                List::listInput(-1);
                return true;
            }
            if (config->isMappedLike("down", input)) {
                if (mCancelTransitionsCallback)
                    mCancelTransitionsCallback();
                List::listInput(1);
                return true;
            }
            if (config->isMappedLike("leftshoulder", input)) {
                if (mCancelTransitionsCallback)
                    mCancelTransitionsCallback();
                List::listInput(-10);
                return true;
            }
            if (config->isMappedLike("rightshoulder", input)) {
                if (mCancelTransitionsCallback)
                    mCancelTransitionsCallback();
                List::listInput(10);
                return true;
            }
            if (config->isMappedLike("lefttrigger", input)) {
                if (getCursor() == 0)
                    return true;
                if (mCancelTransitionsCallback)
                    mCancelTransitionsCallback();
                return this->listFirstRow();
            }
            if (config->isMappedLike("righttrigger", input)) {
                if (getCursor() == static_cast<int>(mEntries.size()) - 1)
                    return true;
                if (mCancelTransitionsCallback)
                    mCancelTransitionsCallback();
                return this->listLastRow();
            }
        }
        else {
            if (config->isMappedLike("up", input) || config->isMappedLike("down", input) ||
                config->isMappedLike("leftshoulder", input) ||
                config->isMappedLike("rightshoulder", input) ||
                config->isMappedLike("lefttrigger", input) ||
                config->isMappedLike("righttrigger", input)) {
                if constexpr (std::is_same_v<T, SystemData*>)
                    List::listInput(0);
                else
                    List::stopScrolling();
            }
        }
    }

    return GuiComponent::input(config, input);
}

template <typename T> void TextListComponent<T>::update(int deltaTime)
{
    List::listUpdate(deltaTime);

    if (mWindow->isScreensaverActive() || !mWindow->getAllowTextScrolling())
        List::stopScrolling();

    if (!isScrolling() && size() > 0) {
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
            mRenderer->setMatrix(trans);
            mRenderer->drawRect(0.0f, (mCursor - startEntry) * entrySize + mSelectorOffsetY,
                                mSize.x, mSelectorHeight, mSelectorColor, mSelectorColorEnd,
                                mSelectorColorGradientHorizontal);
        }
    }

    if (Settings::getInstance()->getBool("DebugText")) {
        mRenderer->drawRect(mHorizontalMargin, 0.0f, mSize.x - mHorizontalMargin * 2.0f, mSize.y,
                            0x00000033, 0x00000033);
        mRenderer->drawRect(0.0f, 0.0f, mSize.x, mSize.y, 0x0000FF33, 0x0000FF33);
    }

    // Clip to inside margins.
    glm::vec3 dim {mSize.x, mSize.y, 0.0f};
    dim.x = (trans[0].x * dim.x + trans[3].x) - trans[3].x;
    dim.y = (trans[1].y * dim.y + trans[3].y) - trans[3].y;

    mRenderer->pushClipRect(
        glm::ivec2 {static_cast<int>(std::round(trans[3].x + mHorizontalMargin)),
                    static_cast<int>(std::round(trans[3].y))},
        glm::ivec2 {static_cast<int>(std::round(dim.x - mHorizontalMargin * 2.0f)),
                    static_cast<int>(std::round(dim.y))});

    for (int i = startEntry; i < listCutoff; ++i) {
        Entry& entry {mEntries.at(i)};

        unsigned int color;
        if (mCursor == i && mSelectedColor)
            color = mSelectedColor;
        else
            color = mColors[entry.data.colorId];

        if (!entry.data.textCache) {
            if (mUppercase)
                entry.data.textCache = std::unique_ptr<TextCache>(
                    font->buildTextCache(Utils::String::toUpper(entry.name), 0, 0, 0x000000FF));
            else if (mLowercase)
                entry.data.textCache = std::unique_ptr<TextCache>(
                    font->buildTextCache(Utils::String::toLower(entry.name), 0, 0, 0x000000FF));
            else if (mCapitalize)
                entry.data.textCache = std::unique_ptr<TextCache>(font->buildTextCache(
                    Utils::String::toCapitalized(entry.name), 0, 0, 0x000000FF));
            else
                entry.data.textCache =
                    std::unique_ptr<TextCache>(font->buildTextCache(entry.name, 0, 0, 0x000000FF));
        }

        if constexpr (std::is_same_v<T, FileData*>) {
            // If a game is marked as hidden, lower the text opacity a lot.
            // If a game is marked to not be counted, lower the opacity a moderate amount.
            if (entry.object->getHidden())
                entry.data.textCache->setColor(color & 0xFFFFFF44);
            else if (!entry.object->getCountAsGame())
                entry.data.textCache->setColor(color & 0xFFFFFF77);
            else
                entry.data.textCache->setColor(color);
        }
        else {
            entry.data.textCache->setColor(color);
        }

        glm::vec3 offset {0.0f, y, 0.0f};

        switch (mAlignment) {
            case PrimaryAlignment::ALIGN_LEFT:
                offset.x = mHorizontalMargin;
                break;
            case PrimaryAlignment::ALIGN_CENTER:
                offset.x =
                    static_cast<float>((mSize.x - entry.data.textCache->metrics.size.x) / 2.0f);
                if (offset.x < mHorizontalMargin)
                    offset.x = mHorizontalMargin;
                break;
            case PrimaryAlignment::ALIGN_RIGHT:
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

        mRenderer->setMatrix(drawTrans);
        font->renderTextCache(entry.data.textCache.get());

        // Render currently selected row again if text is moved far enough for it to repeat.
        if ((mCursor == i && mLoopOffset2 < 0) || (mCursor == i && mLoopScroll)) {
            mLoopScroll = true;
            drawTrans = trans;
            drawTrans = glm::translate(
                drawTrans, offset - glm::vec3 {static_cast<float>(mLoopOffset2), 0.0f, 0.0f});
            mRenderer->setMatrix(drawTrans);
            font->renderTextCache(entry.data.textCache.get());
        }
        y += entrySize;
    }
    mRenderer->popClipRect();
    if constexpr (std::is_same_v<T, FileData*>)
        List::listRenderTitleOverlay(trans);
    GuiComponent::renderChildren(trans);
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
        if (elem->has("selectorGradientType")) {
            const std::string gradientType {elem->get<std::string>("selectorGradientType")};
            if (gradientType == "horizontal") {
                setSelectorColorGradientHorizontal(true);
            }
            else if (gradientType == "vertical") {
                setSelectorColorGradientHorizontal(false);
            }
            else {
                setSelectorColorGradientHorizontal(true);
                LOG(LogWarning) << "TextListComponent: Invalid theme configuration, property "
                                   "<selectorGradientType> defined as \""
                                << gradientType << "\"";
            }
        }
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
        if (elem->has("horizontalAlignment")) {
            const std::string& str {elem->get<std::string>("horizontalAlignment")};
            if (str == "left")
                setAlignment(PrimaryAlignment::ALIGN_LEFT);
            else if (str == "center")
                setAlignment(PrimaryAlignment::ALIGN_CENTER);
            else if (str == "right")
                setAlignment(PrimaryAlignment::ALIGN_RIGHT);
            else
                LOG(LogWarning) << "TextListComponent: Invalid theme configuration, property "
                                   "<horizontalAlignment> defined as \""
                                << str << "\"";
        }
        // Legacy themes only.
        else if (elem->has("alignment")) {
            const std::string& str {elem->get<std::string>("alignment")};
            if (str == "left")
                setAlignment(PrimaryAlignment::ALIGN_LEFT);
            else if (str == "center")
                setAlignment(PrimaryAlignment::ALIGN_CENTER);
            else if (str == "right")
                setAlignment(PrimaryAlignment::ALIGN_RIGHT);
            else
                LOG(LogWarning) << "TextListComponent: Invalid theme configuration, property "
                                   "<alignment> defined as \""
                                << str << "\"";
        }
        if (elem->has("horizontalMargin")) {
            mHorizontalMargin =
                elem->get<float>("horizontalMargin") *
                (this->mParent ? this->mParent->getSize().x : Renderer::getScreenWidth());
        }
    }

    if (properties & LETTER_CASE && elem->has("letterCase")) {
        std::string letterCase {elem->get<std::string>("letterCase")};
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
            LOG(LogWarning) << "TextListComponent: Invalid theme configuration, property "
                               "<letterCase> defined as \""
                            << letterCase << "\"";
        }
    }

    // Legacy themes only.
    if (properties & FORCE_UPPERCASE && elem->has("forceUppercase"))
        setUppercase(elem->get<bool>("forceUppercase"));

    if (properties & LINE_SPACING) {
        if (elem->has("lineSpacing"))
            setLineSpacing(glm::clamp(elem->get<float>("lineSpacing"), 0.5f, 3.0f));
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
        mSelectorImage.setResize(mSize.x, mSelectorHeight);
        mSelectorImage.setColorShift(mSelectorColor);
        mSelectorImage.setColorShiftEnd(mSelectorColorEnd);
    }
    else {
        mSelectorImage.setImage("");
    }
}

template <typename T> void TextListComponent<T>::onCursorChanged(const CursorState& state)
{
    mLoopOffset = 0;
    mLoopOffset2 = 0;
    mLoopTime = 0;

    if constexpr (std::is_same_v<T, SystemData*>) {
        float startPos {mCamOffset};
        float posMax {static_cast<float>(mEntries.size())};
        float endPos {static_cast<float>(mCursor)};

        Animation* anim {new LambdaAnimation(
            [this, startPos, endPos, posMax](float t) {
                t -= 1;
                float f {glm::mix(startPos, endPos, t * t * t + 1)};
                if (f < 0)
                    f += posMax;
                if (f >= posMax)
                    f -= posMax;

                mCamOffset = f;
            },
            500)};

        GuiComponent::setAnimation(anim, 0, nullptr, false, 0);
    }

    if (mCursorChangedCallback)
        mCursorChangedCallback(state);
}

#endif // ES_CORE_COMPONENTS_TEXT_LIST_COMPONENT_H
