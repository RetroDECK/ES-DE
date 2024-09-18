//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  TextListComponent.h
//
//  Text list, usable in both the system and gamelist views.
//

#ifndef ES_CORE_COMPONENTS_PRIMARY_TEXT_LIST_COMPONENT_H
#define ES_CORE_COMPONENTS_PRIMARY_TEXT_LIST_COMPONENT_H

#include "Log.h"
#include "Sound.h"
#include "components/IList.h"
#include "components/TextComponent.h"
#include "components/primary/PrimaryComponent.h"

enum class TextListEntryType {
    PRIMARY,
    SECONDARY
};

struct TextListData {
    TextListEntryType entryType;
    std::shared_ptr<TextComponent> entryName;
};

template <typename T>
class TextListComponent : public PrimaryComponent<T>, private IList<TextListData, T>
{
    using List = IList<TextListData, T>;

protected:
    using List::mCursor;
    using List::mEntries;
    using List::mLastCursor;
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
    void onShowPrimary() override { mEntries.at(mCursor).data.entryName->resetComponent(); }
    void onScroll() override
    {
        if (mGamelistView &&
            (!isScrolling() ||
             !NavigationSounds::getInstance().isPlayingThemeNavigationSound(SCROLLSOUND)))
            NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
        else if (!mGamelistView &&
                 (!isScrolling() || !NavigationSounds::getInstance().isPlayingThemeNavigationSound(
                                        SYSTEMBROWSESOUND)))
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
    bool mGamelistView;

    std::shared_ptr<Font> mFont;
    float mSelectorWidth;
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
    glm::vec2 mSelectedBackgroundMargins;
    float mSelectedBackgroundCornerRadius;
    bool mHorizontalScrolling;
    float mHorizontalScrollSpeed;
    float mHorizontalScrollDelay;
    float mTextHorizontalScrollGap;
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
    : IList<TextListData, T> {IList<TextListData, T>::LIST_SCROLL_STYLE_QUICK,
                              ListLoopType::LIST_PAUSE_AT_END}
    , mRenderer {Renderer::getInstance()}
    , mCamOffset {0.0f}
    , mPreviousScrollVelocity {0}
    , mGamelistView {std::is_same_v<T, FileData*> ? true : false}
    , mFont {Font::get(FONT_SIZE_MEDIUM_FIXED)}
    , mSelectorWidth {mSize.x}
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
    , mSelectedBackgroundMargins {0.0f, 0.0f}
    , mSelectedBackgroundCornerRadius {0.0f}
    , mHorizontalScrolling {true}
    , mHorizontalScrollSpeed {1.0f}
    , mHorizontalScrollDelay {3000.0f}
    , mTextHorizontalScrollGap {1.5f}
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
    if (mHorizontalScrolling) {
        entry.data.entryName = std::make_shared<TextComponent>(
            entry.name, mFont, 0x000000FF, ALIGN_LEFT, ALIGN_CENTER, glm::ivec2 {1, 0},
            glm::vec3 {0.0f, 0.0f, 0.0f}, glm::vec2 {0.0f, mFont->getSize() * 1.5f});
    }
    else {
        entry.data.entryName = std::make_shared<TextComponent>(
            entry.name, mFont, 0x000000FF, ALIGN_LEFT, ALIGN_CENTER, glm::ivec2 {1, 0},
            glm::vec3 {0.0f, 0.0f, 0.0f}, glm::vec2 {0.0f, mFont->getSize() * 1.5f}, 0x00000000,
            1.5f, 1.0f, false, 1.0f, 1500.0f, 1.5f, mSize.x - (mHorizontalMargin * 2.0f));
    }

    if (mHorizontalScrolling) {
        glm::vec2 textSize {entry.data.entryName->getSize()};
        if (textSize.x > mSize.x - (mHorizontalMargin * 2.0f)) {
            // Set the text width to the width of the textlist to trigger horizontal scrolling.
            entry.data.entryName->setHorizontalScrollingSpeedMultiplier(mHorizontalScrollSpeed);
            entry.data.entryName->setHorizontalScrollingDelay(mHorizontalScrollDelay);
            entry.data.entryName->setHorizontalScrollingGap(mTextHorizontalScrollGap);
            entry.data.entryName->setHorizontalScrolling(true);
            textSize.x = mSize.x - (mHorizontalMargin * 2.0f);
            entry.data.entryName->setSize(textSize);
        }
    }

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
    mEntries.at(mCursor).data.entryName->update(deltaTime);
    List::listUpdate(deltaTime);
    GuiComponent::update(deltaTime);
}

template <typename T> void TextListComponent<T>::render(const glm::mat4& parentTrans)
{
    if (size() == 0)
        return;

    glm::mat4 trans {parentTrans * List::getTransform()};

    int startEntry {0};
    int screenCount {0};
    float offsetY {0.0f};
    float entrySize {0.0f};
    float lineSpacingHeight {0.0f};

    entrySize = mFont->getSize() * mLineSpacing;
    lineSpacingHeight = (mFont->getSize() * mLineSpacing) - mFont->getSize();

    // Number of entries that can fit on the screen simultaneously.
    screenCount = static_cast<int>(std::floor((mSize.y + lineSpacingHeight / 2.0f) / entrySize));

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
                                mSelectorWidth, mSelectorHeight, mSelectorColor, mSelectorColorEnd,
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

    float horizontalOffset {0.0f};
    if (mAlignment == PrimaryAlignment::ALIGN_LEFT && mSelectorHorizontalOffset < 0.0f)
        horizontalOffset = mSelectorHorizontalOffset;
    else if (mAlignment == PrimaryAlignment::ALIGN_RIGHT && mSelectorHorizontalOffset > 0.0f)
        horizontalOffset = mSelectorHorizontalOffset;

    mRenderer->pushClipRect(
        glm::ivec2 {static_cast<int>(std::round(trans[3].x + horizontalOffset + mHorizontalMargin +
                                                -mSelectedBackgroundMargins.x)),
                    static_cast<int>(std::round(trans[3].y))},
        glm::ivec2 {static_cast<int>(std::round((dim.x - mHorizontalMargin * 2.0f) +
                                                mSelectedBackgroundMargins.x +
                                                mSelectedBackgroundMargins.y)),
                    static_cast<int>(std::round(dim.y))});

    for (int i {startEntry}; i < listCutoff; ++i) {
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

        if constexpr (std::is_same_v<T, FileData*>) {
            // If a game is marked as hidden, lower the text opacity a lot.
            // If a game is marked to not be counted, lower the opacity a moderate amount.
            if (entry.object->getHidden())
                entry.data.entryName->setColor(color & 0xFFFFFF44);
            else if (!entry.object->getCountAsGame())
                entry.data.entryName->setColor(color & 0xFFFFFF77);
            else
                entry.data.entryName->setColor(color);
        }
        else {
            entry.data.entryName->setColor(color);
        }

        glm::vec3 offset {0.0f, offsetY, 0.0f};

        switch (mAlignment) {
            case PrimaryAlignment::ALIGN_LEFT:
                offset.x = mHorizontalMargin;
                break;
            case PrimaryAlignment::ALIGN_CENTER:
                offset.x = static_cast<float>((mSize.x - entry.data.entryName->getSize().x) / 2.0f);
                if (offset.x < mHorizontalMargin)
                    offset.x = mHorizontalMargin;
                break;
            case PrimaryAlignment::ALIGN_RIGHT:
                offset.x = (mSize.x - entry.data.entryName->getSize().x);
                offset.x -= mHorizontalMargin;
                if (offset.x < mHorizontalMargin)
                    offset.x = mHorizontalMargin;
                break;
        }

        // Render text.
        glm::mat4 drawTrans {trans};

        drawTrans = glm::translate(drawTrans, glm::round(offset));
        mRenderer->setMatrix(drawTrans);

        if (i == mCursor && backgroundColor != 0x00000000) {
            if (mSelectorHorizontalOffset != 0.0f || mSelectedBackgroundMargins.x != 0.0f) {
                drawTrans = glm::translate(
                    drawTrans, glm::vec3 {mSelectorHorizontalOffset - mSelectedBackgroundMargins.x,
                                          0.0f, 0.0f});
                mRenderer->setMatrix(drawTrans);
            }

            mRenderer->drawRect(0.0f, mSelectorVerticalOffset,
                                entry.data.entryName->getSize().x + mSelectedBackgroundMargins.x +
                                    mSelectedBackgroundMargins.y,
                                mSelectorHeight, backgroundColor, backgroundColor, false, 1.0f,
                                1.0f, Renderer::BlendFactor::SRC_ALPHA,
                                Renderer::BlendFactor::ONE_MINUS_SRC_ALPHA,
                                mSelectedBackgroundCornerRadius);

            if (mSelectorHorizontalOffset != 0.0f || mSelectedBackgroundMargins.x != 0.0f) {
                drawTrans = glm::translate(
                    drawTrans, glm::vec3 {-mSelectorHorizontalOffset + mSelectedBackgroundMargins.x,
                                          0.0f, 0.0f});
                mRenderer->setMatrix(drawTrans);
            }
        }

        entry.data.entryName->render(drawTrans);

        offsetY += entrySize;
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

    if (elem->has("selectedBackgroundMargins")) {
        const glm::vec2 selectedBackgroundMargins {
            glm::clamp(elem->get<glm::vec2>("selectedBackgroundMargins"), 0.0f, 0.5f)};
        mSelectedBackgroundMargins = selectedBackgroundMargins * Renderer::getScreenWidth();
    }

    if (elem->has("selectedBackgroundCornerRadius")) {
        mSelectedBackgroundCornerRadius =
            glm::clamp(elem->get<float>("selectedBackgroundCornerRadius"), 0.0f, 0.5f) *
            mRenderer->getScreenWidth();
    }

    if (elem->has("textHorizontalScrolling"))
        mHorizontalScrolling = elem->get<bool>("textHorizontalScrolling");

    if (elem->has("textHorizontalScrollSpeed")) {
        mHorizontalScrollSpeed =
            glm::clamp(elem->get<float>("textHorizontalScrollSpeed"), 0.1f, 10.0f);
    }

    if (elem->has("textHorizontalScrollDelay")) {
        mHorizontalScrollDelay =
            glm::clamp(elem->get<float>("textHorizontalScrollDelay"), 0.0f, 10.0f) * 1000.0f;
    }

    if (elem->has("textHorizontalScrollGap")) {
        mTextHorizontalScrollGap =
            glm::clamp(elem->get<float>("textHorizontalScrollGap"), 0.1f, 5.0f);
    }

    mFont = Font::getFromTheme(elem, properties, mFont);

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

    mSelectorHorizontalOffset = 0.0f;
    mSelectorVerticalOffset = 0.0f;

    if (properties & LINE_SPACING) {
        if (elem->has("lineSpacing"))
            mLineSpacing = glm::clamp(elem->get<float>("lineSpacing"), 0.5f, 3.0f);
        if (elem->has("selectorHeight"))
            mSelectorHeight = glm::clamp(elem->get<float>("selectorHeight"), 0.0f, 1.0f) *
                              Renderer::getScreenHeight();
        else
            mSelectorHeight = mFont->getSize() * 1.5f;
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

    mSize.x = glm::clamp(mSize.x, mRenderer->getScreenWidth() * 0.05f,
                         mRenderer->getScreenWidth() * 1.0f);
    mSize.y = glm::clamp(mSize.y, mRenderer->getScreenHeight() * 0.05f,
                         mRenderer->getScreenHeight() * 1.0f);

    if (elem->has("selectorWidth"))
        mSelectorWidth =
            glm::clamp(elem->get<float>("selectorWidth"), 0.0f, 1.0f) * Renderer::getScreenWidth();
    else
        mSelectorWidth = mSize.x;

    if (elem->has("selectorImagePath")) {
        const std::string& path {elem->get<std::string>("selectorImagePath")};
        bool tile {elem->has("selectorImageTile") && elem->get<bool>("selectorImageTile")};
        mSelectorImage.setImage(path, tile);
        mSelectorImage.setSize(mSelectorWidth, mSelectorHeight);
        mSelectorImage.setResize(mSelectorWidth, mSelectorHeight);
        mSelectorImage.setColorShift(mSelectorColor);
        mSelectorImage.setColorShiftEnd(mSelectorColorEnd);
    }
    else {
        mSelectorImage.setImage("");
    }

    if (elem->has("fadeAbovePrimary"))
        mFadeAbovePrimary = elem->get<bool>("fadeAbovePrimary");
}

template <typename T> void TextListComponent<T>::onCursorChanged(const CursorState& state)
{
    if (mEntries.size() > static_cast<size_t>(mLastCursor))
        mEntries.at(mLastCursor).data.entryName->resetComponent();

    if constexpr (std::is_same_v<T, SystemData*>) {
        float startPos {mCamOffset};
        float posMax {static_cast<float>(mEntries.size())};
        float endPos {static_cast<float>(mCursor)};

        float animTime {400.0f};
        float timeDiff {1.0f};

        // If startPos is inbetween two positions then reduce the time slightly as the distance
        // will be shorter meaning the animation would play for too long if not compensated for.
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
