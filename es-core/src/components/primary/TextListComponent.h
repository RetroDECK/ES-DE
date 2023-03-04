//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  TextListComponent.h
//
//  Text list, usable in both the system and gamelist views.
//

#ifndef ES_CORE_COMPONENTS_PRIMARY_TEXT_LIST_COMPONENT_H
#define ES_CORE_COMPONENTS_PRIMARY_TEXT_LIST_COMPONENT_H

#include "Log.h"
#include "Sound.h"
#include "components/IList.h"
#include "components/primary/PrimaryComponent.h"
#include "resources/Font.h"

enum class TextListEntryType {
    PRIMARY,
    SECONDARY
};

struct TextListData {
    TextListEntryType entryType;
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

    void setCancelTransitionsCallback(const std::function<void()>& func) override
    {
        mCancelTransitionsCallback = func;
    }
    void setCursorChangedCallback(const std::function<void(CursorState state)>& func) override
    {
        mCursorChangedCallback = func;
    }

    void setFont(const std::shared_ptr<Font>& font)
    {
        mFont = font;
        for (auto it = mEntries.begin(); it != mEntries.end(); ++it)
            it->data.textCache.reset();
    }

    const std::string& getIndicators() const { return mIndicators; }
    const std::string& getCollectionIndicators() const { return mCollectionIndicators; }
    const LetterCase getLetterCase() const override { return mLetterCase; }
    const LetterCase getLetterCaseAutoCollections() const override
    {
        return mLetterCaseAutoCollections;
    }
    const LetterCase getLetterCaseCustomCollections() const override
    {
        return mLetterCaseCustomCollections;
    }
    const bool getSystemNameSuffix() const override { return mSystemNameSuffix; }
    const LetterCase getLetterCaseSystemNameSuffix() const override
    {
        return mLetterCaseSystemNameSuffix;
    }

private:
    void onShow() override { mLoopTime = 0; }
    void onScroll() override
    {
        if (mGamelistView &&
            !NavigationSounds::getInstance().isPlayingThemeNavigationSound(SCROLLSOUND))
            NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
        else if (!mGamelistView)
            NavigationSounds::getInstance().playThemeNavigationSound(SYSTEMBROWSESOUND);
    }
    void onCursorChanged(const CursorState& state) override;
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
    const bool getFadeAbovePrimary() const override { return mFadeAbovePrimary; }

    Renderer* mRenderer;
    std::function<void()> mCancelTransitionsCallback;
    std::function<void(CursorState state)> mCursorChangedCallback;
    float mCamOffset;
    int mPreviousScrollVelocity;
    int mLoopOffset1;
    int mLoopOffset2;
    int mLoopTime;
    bool mLoopScroll;
    bool mGamelistView;
    bool mLegacyMode;

    std::shared_ptr<Font> mFont;
    float mSelectorHeight;
    float mSelectorHorizontalOffset;
    float mSelectorVerticalOffset;
    unsigned int mSelectorColor;
    unsigned int mSelectorColorEnd;
    bool mSelectorColorGradientHorizontal;
    ImageComponent mSelectorImage;
    unsigned int mPrimaryColor;
    unsigned int mSecondaryColor;
    unsigned int mSelectedColor;
    unsigned int mSelectedSecondaryColor;
    unsigned int mSelectedBackgroundColor;
    unsigned int mSelectedSecondaryBackgroundColor;
    PrimaryAlignment mAlignment;
    float mHorizontalMargin;
    LetterCase mLetterCase;
    LetterCase mLetterCaseAutoCollections;
    LetterCase mLetterCaseCustomCollections;
    float mLineSpacing;
    std::string mIndicators;
    std::string mCollectionIndicators;
    bool mSystemNameSuffix;
    LetterCase mLetterCaseSystemNameSuffix;
    bool mFadeAbovePrimary;
};

template <typename T>
TextListComponent<T>::TextListComponent()
    : IList<TextListData, T> {(std::is_same_v<T, SystemData*> ?
                                   IList<TextListData, T>::LIST_SCROLL_STYLE_SLOW :
                                   IList<TextListData, T>::LIST_SCROLL_STYLE_QUICK),
                              ListLoopType::LIST_PAUSE_AT_END}
    , mRenderer {Renderer::getInstance()}
    , mCamOffset {0.0f}
    , mPreviousScrollVelocity {0}
    , mLoopOffset1 {0}
    , mLoopOffset2 {0}
    , mLoopTime {0}
    , mLoopScroll {false}
    , mGamelistView {std::is_same_v<T, FileData*> ? true : false}
    , mLegacyMode {false}
    , mFont {Font::get(FONT_SIZE_MEDIUM_FIXED)}
    , mSelectorHeight {mFont->getSize() * 1.5f}
    , mSelectorHorizontalOffset {0.0f}
    , mSelectorVerticalOffset {0.0f}
    , mSelectorColor {0x333333FF}
    , mSelectorColorEnd {0x333333FF}
    , mSelectorColorGradientHorizontal {true}
    , mPrimaryColor {0x0000FFFF}
    , mSecondaryColor {0x00FF00FF}
    , mSelectedColor {0x0000FFFF}
    , mSelectedSecondaryColor {0x00FF00FF}
    , mSelectedBackgroundColor {0x00000000}
    , mSelectedSecondaryBackgroundColor {0x00000000}
    , mAlignment {PrimaryAlignment::ALIGN_CENTER}
    , mHorizontalMargin {0.0f}
    , mLetterCase {LetterCase::NONE}
    , mLetterCaseAutoCollections {LetterCase::UNDEFINED}
    , mLetterCaseCustomCollections {LetterCase::UNDEFINED}
    , mLineSpacing {1.5f}
    , mIndicators {"symbols"}
    , mCollectionIndicators {"symbols"}
    , mSystemNameSuffix {true}
    , mLetterCaseSystemNameSuffix {LetterCase::UPPERCASE}
    , mFadeAbovePrimary {false}
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
                if (mCursor != 0) {
                    if (mCancelTransitionsCallback)
                        mCancelTransitionsCallback();
                    List::listInput(-10);
                }
                return true;
            }
            if (config->isMappedLike("rightshoulder", input)) {
                if (mCursor != size() - 1) {
                    if (mCancelTransitionsCallback)
                        mCancelTransitionsCallback();
                    List::listInput(10);
                }
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
                if (isScrolling())
                    onCursorChanged(CursorState::CURSOR_STOPPED);
                List::listInput(0);
            }
        }
    }

    return GuiComponent::input(config, input);
}

template <typename T> void TextListComponent<T>::update(int deltaTime)
{
    List::listUpdate(deltaTime);

    if ((mWindow->isMediaViewerActive() || mWindow->isScreensaverActive() ||
         !mWindow->getAllowTextScrolling())) {
        mLoopTime = 0;
    }
    else {
        // Always reset the loop offsets.
        mLoopOffset1 = 0;
        mLoopOffset2 = 0;

        // If we're not scrolling and this object's text exceeds our size, then loop it.
        const float limit {mSize.x - mHorizontalMargin * 2.0f};
        float length {0.0f};

        if (mEntries.at(static_cast<unsigned int>(mCursor)).data.textCache != nullptr)
            length = mEntries.at(static_cast<unsigned int>(mCursor)).data.textCache->getSize().x;

        if (length != 0.0f && length > limit) {
            // Loop the text.
            const float speed {mFont->sizeText("ABCDEFGHIJKLMNOPQRSTUVWXYZ").x * 0.247f};
            const float delay {3000.0f};
            const float scrollLength {length};
            const float returnLength {speed * 1.5f};
            const float scrollTime {(scrollLength * 1000.0f) / speed};
            const float returnTime {(returnLength * 1000.0f) / speed};
            const int maxTime {static_cast<int>(delay + scrollTime + returnTime)};

            mLoopTime += deltaTime;
            while (mLoopTime > maxTime)
                mLoopTime -= maxTime;

            mLoopOffset1 = static_cast<int>(Utils::Math::loop(delay, scrollTime + returnTime,
                                                              static_cast<float>(mLoopTime),
                                                              scrollLength + returnLength));

            if (mLoopOffset1 > (scrollLength - (limit - returnLength)))
                mLoopOffset2 = static_cast<int>(mLoopOffset1 - (scrollLength + returnLength));
        }
    }

    GuiComponent::update(deltaTime);
}

template <typename T> void TextListComponent<T>::render(const glm::mat4& parentTrans)
{
    if ((mWindow->isMediaViewerActive() || mWindow->isScreensaverActive() ||
         !mWindow->getAllowTextScrolling())) {
        mLoopOffset1 = 0;
        mLoopOffset2 = 0;
    }

    if (size() == 0)
        return;

    glm::mat4 trans {parentTrans * List::getTransform()};
    std::shared_ptr<Font>& font {mFont};

    int startEntry {0};
    int screenCount {0};
    float y {0.0f};

    float entrySize {0.0f};
    float lineSpacingHeight {0.0f};

    // The vertical spacing between rows for legacy themes is very inaccurate and will look
    // different depending on the resolution, but it's done for maximum backward compatibility.
    if (mLegacyMode) {
        font->useLegacyMaxGlyphHeight();
        entrySize = std::max(font->getHeight(mLineSpacing), font->getSize() * mLineSpacing);
        lineSpacingHeight = std::floor(font->getSize() * mLineSpacing - font->getSize());
    }
    else {
        entrySize = font->getSize() * mLineSpacing;
        lineSpacingHeight = font->getSize() * mLineSpacing - font->getSize() * 1.0f;
    }

    if (mLegacyMode) {
        // This extra vertical margin is technically incorrect, but it adds a little extra leeway
        // to avoid removing the last row on some older theme sets. There was a sizing bug in the
        // RetroPie fork of EmulationStation and some theme authors set sizes that are just slightly
        // too small for the last row to show up when the sizing calculation is done correctly.
        const float extraMargin {(Renderer::getScreenHeightModifier() >= 1.0f ? 3.0f : 0.0f)};
        // Number of entries that can fit on the screen simultaneously.
        screenCount = static_cast<int>(
            floorf((mSize.y + lineSpacingHeight / 2.0f + extraMargin) / entrySize));
    }
    else {
        // Number of entries that can fit on the screen simultaneously.
        screenCount =
            static_cast<int>(std::floor((mSize.y + lineSpacingHeight / 2.0f) / entrySize));
    }

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
            mSelectorImage.setPosition(mSelectorHorizontalOffset,
                                       (mCursor - startEntry) * entrySize + mSelectorVerticalOffset,
                                       0.0f);
            mSelectorImage.render(trans);
        }
        else {
            mRenderer->setMatrix(trans);
            mRenderer->drawRect(mSelectorHorizontalOffset,
                                (mCursor - startEntry) * entrySize + mSelectorVerticalOffset,
                                mSize.x, mSelectorHeight, mSelectorColor, mSelectorColorEnd,
                                mSelectorColorGradientHorizontal);
        }
    }

    if (Settings::getInstance()->getBool("DebugText")) {
        mRenderer->setMatrix(trans);
        mRenderer->drawRect(mHorizontalMargin, 0.0f, mSize.x - mHorizontalMargin * 2.0f, mSize.y,
                            0x00000033, 0x00000033);
        mRenderer->drawRect(0.0f, 0.0f, mSize.x, mSize.y, 0x00FF0033, 0x00FF0033);
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
        unsigned int color {0x00000000};
        unsigned int backgroundColor {0x00000000};

        if (entry.data.entryType == TextListEntryType::PRIMARY) {
            color = (mCursor == i ? mSelectedColor : mPrimaryColor);
            backgroundColor = (mCursor == i ? mSelectedBackgroundColor : 0x00000000);
        }
        else {
            color = (mCursor == i ? mSelectedSecondaryColor : mSecondaryColor);
            backgroundColor = (mCursor == i ? mSelectedSecondaryBackgroundColor : 0x00000000);
        }

        if (!entry.data.textCache) {
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
        if (mCursor == i && mLoopOffset1 > 0) {
            drawTrans = glm::translate(
                drawTrans, offset - glm::vec3 {static_cast<float>(mLoopOffset1), 0.0f, 0.0f});
        }
        else {
            drawTrans = glm::translate(drawTrans, offset);
        }

        // Needed to avoid flickering when returning to the start position.
        if (mLoopOffset1 == 0 && mLoopOffset2 == 0)
            mLoopScroll = false;

        mRenderer->setMatrix(drawTrans);

        if (i == mCursor && backgroundColor != 0x00000000) {
            mRenderer->drawRect(mSelectorHorizontalOffset, 0.0f,
                                entry.data.textCache->metrics.size.x, mSelectorHeight,
                                backgroundColor, backgroundColor);
        }

        font->renderTextCache(entry.data.textCache.get());

        // Render currently selected row again if text is moved far enough for it to repeat.
        if ((mCursor == i && mLoopOffset2 < 0) || (mCursor == i && mLoopScroll)) {
            mLoopScroll = true;
            drawTrans = trans;
            drawTrans = glm::translate(
                drawTrans, offset - glm::vec3 {static_cast<float>(mLoopOffset2), 0.0f, 0.0f});
            mRenderer->setMatrix(drawTrans);
            if (i == mCursor && backgroundColor != 0x00000000) {
                mRenderer->drawRect(mSelectorHorizontalOffset, 0.0f,
                                    entry.data.textCache->metrics.size.x, mSelectorHeight,
                                    backgroundColor, backgroundColor);
            }
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
    mSize.x = Renderer::getScreenWidth();
    mSize.y = Renderer::getScreenHeight() * 0.8f;
    GuiComponent::mPosition.x = 0.0f;
    GuiComponent::mPosition.y = Renderer::getScreenHeight() * 0.1f;
    setAlignment(PrimaryAlignment::ALIGN_LEFT);

    GuiComponent::applyTheme(theme, view, element, properties);

    using namespace ThemeFlags;
    const ThemeData::ThemeElement* elem {theme->getElement(view, element, "textlist")};

    if (!elem)
        return;

    mLegacyMode = theme->isLegacyTheme();

    if (properties & COLOR) {
        if (elem->has("selectorColor")) {
            mSelectorColor = elem->get<unsigned int>("selectorColor");
            mSelectorColorEnd = mSelectorColor;
        }
        if (elem->has("selectorColorEnd"))
            mSelectorColorEnd = elem->get<unsigned int>("selectorColorEnd");
        if (elem->has("selectorGradientType")) {
            const std::string& gradientType {elem->get<std::string>("selectorGradientType")};
            if (gradientType == "horizontal") {
                mSelectorColorGradientHorizontal = true;
            }
            else if (gradientType == "vertical") {
                mSelectorColorGradientHorizontal = false;
            }
            else {
                mSelectorColorGradientHorizontal = true;
                LOG(LogWarning) << "TextListComponent: Invalid theme configuration, property "
                                   "\"selectorGradientType\" for element \""
                                << element.substr(9) << "\" defined as \"" << gradientType << "\"";
            }
        }
        if (elem->has("primaryColor"))
            mPrimaryColor = elem->get<unsigned int>("primaryColor");
        if (elem->has("secondaryColor"))
            mSecondaryColor = elem->get<unsigned int>("secondaryColor");
        if (elem->has("selectedColor"))
            mSelectedColor = elem->get<unsigned int>("selectedColor");
        else
            mSelectedColor = mPrimaryColor;
        if (elem->has("selectedSecondaryColor"))
            mSelectedSecondaryColor = elem->get<unsigned int>("selectedSecondaryColor");
        else
            mSelectedSecondaryColor = mSelectedColor;
        if (elem->has("selectedBackgroundColor"))
            mSelectedBackgroundColor = elem->get<unsigned int>("selectedBackgroundColor");
        if (elem->has("selectedSecondaryBackgroundColor"))
            mSelectedSecondaryBackgroundColor =
                elem->get<unsigned int>("selectedSecondaryBackgroundColor");
        else
            mSelectedSecondaryBackgroundColor = mSelectedBackgroundColor;
    }

    setFont(Font::getFromTheme(elem, properties, mFont, 0.0f, false, mLegacyMode));
    if (mLegacyMode)
        mFont->useLegacyMaxGlyphHeight();
    const float selectorHeight {mFont->getHeight(mLineSpacing)};
    mSelectorHeight = selectorHeight;

    if (properties & ALIGNMENT) {
        if (elem->has("horizontalAlignment")) {
            const std::string& horizontalAlignment {elem->get<std::string>("horizontalAlignment")};
            if (horizontalAlignment == "left") {
                setAlignment(PrimaryAlignment::ALIGN_LEFT);
            }
            else if (horizontalAlignment == "center") {
                setAlignment(PrimaryAlignment::ALIGN_CENTER);
            }
            else if (horizontalAlignment == "right") {
                setAlignment(PrimaryAlignment::ALIGN_RIGHT);
            }
            else {
                LOG(LogWarning) << "TextListComponent: Invalid theme configuration, property "
                                   "\"horizontalAlignment\" for element \""
                                << element.substr(9) << "\" defined as \"" << horizontalAlignment
                                << "\"";
            }
        }
        else if (elem->has("alignment")) {
            // Legacy themes only.
            const std::string& alignment {elem->get<std::string>("alignment")};
            if (alignment == "left") {
                setAlignment(PrimaryAlignment::ALIGN_LEFT);
            }
            else if (alignment == "center") {
                setAlignment(PrimaryAlignment::ALIGN_CENTER);
            }
            else if (alignment == "right") {
                setAlignment(PrimaryAlignment::ALIGN_RIGHT);
            }
            else {
                LOG(LogWarning) << "TextListComponent: Invalid theme configuration, property "
                                   "\"alignment\" for element \""
                                << element.substr(9) << "\" defined as \"" << alignment << "\"";
            }
        }
        if (elem->has("horizontalMargin")) {
            mHorizontalMargin =
                elem->get<float>("horizontalMargin") *
                (this->mParent ? this->mParent->getSize().x : Renderer::getScreenWidth());
        }
    }

    if (properties & LETTER_CASE && elem->has("letterCase")) {
        const std::string& letterCase {elem->get<std::string>("letterCase")};
        if (letterCase == "uppercase") {
            mLetterCase = LetterCase::UPPERCASE;
        }
        else if (letterCase == "lowercase") {
            mLetterCase = LetterCase::LOWERCASE;
        }
        else if (letterCase == "capitalize") {
            mLetterCase = LetterCase::CAPITALIZE;
        }
        else if (letterCase != "none") {
            LOG(LogWarning) << "TextListComponent: Invalid theme configuration, property "
                               "\"letterCase\" for element \""
                            << element.substr(9) << "\" defined as \"" << letterCase << "\"";
        }
    }

    if (properties & LETTER_CASE && elem->has("letterCaseAutoCollections")) {
        const std::string& letterCase {elem->get<std::string>("letterCaseAutoCollections")};
        if (letterCase == "uppercase") {
            mLetterCaseAutoCollections = LetterCase::UPPERCASE;
        }
        else if (letterCase == "lowercase") {
            mLetterCaseAutoCollections = LetterCase::LOWERCASE;
        }
        else if (letterCase == "capitalize") {
            mLetterCaseAutoCollections = LetterCase::CAPITALIZE;
        }
        else if (letterCase == "none") {
            mLetterCaseAutoCollections = LetterCase::NONE;
        }
        else {
            LOG(LogWarning) << "TextListComponent: Invalid theme configuration, property "
                               "\"letterCaseAutoCollections\" for element \""
                            << element.substr(9) << "\" defined as \"" << letterCase << "\"";
        }
    }

    if (properties & LETTER_CASE && elem->has("letterCaseCustomCollections")) {
        const std::string& letterCase {elem->get<std::string>("letterCaseCustomCollections")};
        if (letterCase == "uppercase") {
            mLetterCaseCustomCollections = LetterCase::UPPERCASE;
        }
        else if (letterCase == "lowercase") {
            mLetterCaseCustomCollections = LetterCase::LOWERCASE;
        }
        else if (letterCase == "capitalize") {
            mLetterCaseCustomCollections = LetterCase::CAPITALIZE;
        }
        else if (letterCase == "none") {
            mLetterCaseCustomCollections = LetterCase::NONE;
        }
        else {
            LOG(LogWarning) << "TextListComponent: Invalid theme configuration, property "
                               "\"letterCaseCustomCollections\" for element \""
                            << element.substr(9) << "\" defined as \"" << letterCase << "\"";
        }
    }

    // Legacy themes only.
    if (properties & FORCE_UPPERCASE && elem->has("forceUppercase")) {
        if (elem->get<bool>("forceUppercase"))
            mLetterCase = LetterCase::UPPERCASE;
    }

    mSelectorHorizontalOffset = 0.0f;
    mSelectorVerticalOffset = 0.0f;

    if (properties & LINE_SPACING) {
        if (elem->has("lineSpacing"))
            mLineSpacing = glm::clamp(elem->get<float>("lineSpacing"), 0.5f, 3.0f);
        if (elem->has("selectorHeight"))
            mSelectorHeight = glm::clamp(elem->get<float>("selectorHeight"), 0.0f, 1.0f) *
                              Renderer::getScreenHeight();
        if (elem->has("selectorHorizontalOffset")) {
            const float scale {this->mParent ? this->mParent->getSize().x :
                                               Renderer::getScreenWidth()};
            mSelectorHorizontalOffset =
                glm::clamp(elem->get<float>("selectorHorizontalOffset"), -1.0f, 1.0f) * scale;
        }
        if (elem->has("selectorVerticalOffset")) {
            const float scale {this->mParent ? this->mParent->getSize().y :
                                               Renderer::getScreenHeight()};
            mSelectorVerticalOffset =
                glm::clamp(elem->get<float>("selectorVerticalOffset"), -1.0f, 1.0f) * scale;
        }
        else if (elem->has("selectorOffsetY")) {
            const float scale {this->mParent ? this->mParent->getSize().y :
                                               Renderer::getScreenHeight()};
            mSelectorVerticalOffset =
                glm::clamp(elem->get<float>("selectorOffsetY"), -1.0f, 1.0f) * scale;
        }
    }

    if (elem->has("indicators")) {
        const std::string& indicators {elem->get<std::string>("indicators")};
        if (indicators == "symbols" || indicators == "ascii" || indicators == "none") {
            mIndicators = indicators;
        }
        else {
            mIndicators = "symbols";
            LOG(LogWarning) << "TextListComponent: Invalid theme configuration, property "
                               "\"indicators\" for element \""
                            << element.substr(9) << "\" defined as \"" << indicators << "\"";
        }
    }

    if (elem->has("collectionIndicators")) {
        const std::string& collectionIndicators {elem->get<std::string>("collectionIndicators")};
        if (collectionIndicators == "symbols" || collectionIndicators == "ascii") {
            mCollectionIndicators = collectionIndicators;
        }
        else {
            mCollectionIndicators = "symbols";
            LOG(LogWarning) << "TextListComponent: Invalid theme configuration, property "
                               "\"collectionIndicators\" for element \""
                            << element.substr(9) << "\" defined as \"" << collectionIndicators
                            << "\"";
        }
    }

    if (mGamelistView && elem->has("systemNameSuffix"))
        mSystemNameSuffix = elem->get<bool>("systemNameSuffix");

    if (mGamelistView && properties & LETTER_CASE && elem->has("letterCaseSystemNameSuffix")) {
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
            LOG(LogWarning) << "TextListComponent: Invalid theme configuration, property "
                               "\"letterCaseSystemNameSuffix\" for element \""
                            << element.substr(9) << "\" defined as \"" << letterCase << "\"";
        }
    }

    if (elem->has("selectorImagePath")) {
        const std::string& path {elem->get<std::string>("selectorImagePath")};
        bool tile {elem->has("selectorImageTile") && elem->get<bool>("selectorImageTile")};
        mSelectorImage.setImage(path, tile);
        mSelectorImage.setSize(mSize.x, mSelectorHeight);
        mSelectorImage.setResize(mSize.x, mSelectorHeight);
        mSelectorImage.setColorShift(mSelectorColor);
        mSelectorImage.setColorShiftEnd(mSelectorColorEnd);
    }
    else {
        mSelectorImage.setImage("");
    }

    if (elem->has("fadeAbovePrimary"))
        mFadeAbovePrimary = elem->get<bool>("fadeAbovePrimary");

    mSize.x = glm::clamp(mSize.x, mRenderer->getScreenWidth() * 0.05f,
                         mRenderer->getScreenWidth() * 1.0f);
    mSize.y = glm::clamp(mSize.y, mRenderer->getScreenHeight() * 0.05f,
                         mRenderer->getScreenHeight() * 1.0f);
}

template <typename T> void TextListComponent<T>::onCursorChanged(const CursorState& state)
{
    mLoopTime = 0;

    if constexpr (std::is_same_v<T, SystemData*>) {
        float startPos {mCamOffset};
        float posMax {static_cast<float>(mEntries.size())};
        float endPos {static_cast<float>(mCursor)};

        float animTime {400.0f};
        float timeDiff {1.0f};

        // If startPos is inbetween two positions then reduce the time slightly as the distance will
        // be shorter meaning the animation would play for too long if not compensated for.
        if (mScrollVelocity == 1)
            timeDiff = endPos - startPos;
        else if (mScrollVelocity == -1)
            timeDiff = startPos - endPos;

        if (timeDiff != 1.0f)
            animTime =
                glm::clamp(std::fabs(glm::mix(0.0f, animTime, timeDiff * 1.5f)), 200.0f, animTime);

        Animation* anim {new LambdaAnimation(
            [this, startPos, endPos, posMax](float t) {
                // Non-linear interpolation.
                t = 1.0f - (1.0f - t) * (1.0f - t);
                float f {(endPos * t) + (startPos * (1.0f - t))};

                if (f < 0)
                    f += posMax;
                if (f >= posMax)
                    f -= posMax;

                mCamOffset = f;
            },
            static_cast<int>(animTime))};

        GuiComponent::setAnimation(anim, 0, nullptr, false, 0);
    }

    if (mCursorChangedCallback)
        mCursorChangedCallback(state);
}

#endif // ES_CORE_COMPONENTS_PRIMARY_TEXT_LIST_COMPONENT_H
