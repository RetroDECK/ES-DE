//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  CarouselComponent.h
//
//  Carousel.
//

#ifndef ES_CORE_COMPONENTS_CAROUSEL_COMPONENT_H
#define ES_CORE_COMPONENTS_CAROUSEL_COMPONENT_H

#include "Sound.h"
#include "animations/LambdaAnimation.h"
#include "components/IList.h"
#include "components/primary/PrimaryComponent.h"
#include "resources/Font.h"

struct CarouselEntry {
    std::shared_ptr<GuiComponent> item;
    std::string itemPath;
    std::string defaultItemPath;
};

template <typename T>
class CarouselComponent : public PrimaryComponent<T>, protected IList<CarouselEntry, T>
{
    using List = IList<CarouselEntry, T>;

protected:
    using List::mCursor;
    using List::mEntries;
    using List::mScrollVelocity;
    using List::mSize;
    using List::mWindow;

    using GuiComponent::mDefaultZIndex;
    using GuiComponent::mZIndex;

public:
    using Entry = typename IList<CarouselEntry, T>::Entry;

    enum class CarouselType {
        HORIZONTAL,
        VERTICAL,
        VERTICAL_WHEEL,
        HORIZONTAL_WHEEL,
        NO_CAROUSEL
    };

    CarouselComponent();

    void addEntry(Entry& entry, const std::shared_ptr<ThemeData>& theme);
    void updateEntry(Entry& entry, const std::shared_ptr<ThemeData>& theme);
    Entry& getEntry(int index) { return mEntries.at(index); }
    void onDemandTextureLoad() override;
    const CarouselType getType() { return mType; }
    const std::string& getItemType() { return mItemType; }
    void setItemType(std::string itemType) { mItemType = itemType; }
    const std::string& getDefaultItem() { return mDefaultItem; }
    void setDefaultItem(std::string defaultItem) { mDefaultItem = defaultItem; }
    bool isScrolling() const override { return List::isScrolling(); }

    void setCursorChangedCallback(const std::function<void(CursorState state)>& func) override
    {
        mCursorChangedCallback = func;
    }
    void setCancelTransitionsCallback(const std::function<void()>& func) override
    {
        mCancelTransitionsCallback = func;
    }

    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;
    void applyTheme(const std::shared_ptr<ThemeData>& theme,
                    const std::string& view,
                    const std::string& element,
                    unsigned int properties) override;

private:
    void onCursorChanged(const CursorState& state) override;
    void onScroll() override
    {
        NavigationSounds::getInstance().playThemeNavigationSound(SYSTEMBROWSESOUND);
    }

    void stopScrolling() override
    {
        List::stopScrolling();
        // Only finish the animation if we're in the gamelist view.
        if (mGamelistView)
            GuiComponent::finishAnimation(0);
    }
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
    std::function<void(CursorState state)> mCursorChangedCallback;
    std::function<void()> mCancelTransitionsCallback;

    float mEntryCamOffset;
    float mEntryCamTarget;
    int mPreviousScrollVelocity;
    bool mPositiveDirection;
    bool mTriggerJump;
    bool mGamelistView;
    bool mUppercase;
    bool mLowercase;
    bool mCapitalize;

    CarouselType mType;
    std::string mItemType;
    std::string mDefaultItem;
    bool mLegacyMode;
    std::shared_ptr<Font> mFont;
    unsigned int mTextColor;
    unsigned int mTextBackgroundColor;
    std::string mText;
    float mLineSpacing;
    Alignment mItemHorizontalAlignment;
    Alignment mItemVerticalAlignment;
    Alignment mWheelHorizontalAlignment;
    float mUnfocusedItemOpacity;
    float mMaxItemCount;
    int mItemsBeforeCenter;
    int mItemsAfterCenter;
    glm::vec2 mItemSize;
    bool mLinearInterpolation;
    bool mInstantItemTransitions;
    bool mItemAxisHorizontal;
    bool mFadeAbovePrimary;
    float mItemScale;
    float mItemRotation;
    glm::vec2 mItemRotationOrigin;
    unsigned int mCarouselColor;
    unsigned int mCarouselColorEnd;
    bool mColorGradientHorizontal;
    bool mReflections;
    float mReflectionsOpacity;
    float mReflectionsFalloff;
    float mHorizontalOffset;
    float mVerticalOffset;
};

template <typename T>
CarouselComponent<T>::CarouselComponent()
    : IList<CarouselEntry, T> {LIST_SCROLL_STYLE_SLOW,
                               (std::is_same_v<T, SystemData*> ?
                                    ListLoopType::LIST_ALWAYS_LOOP :
                                    ListLoopType::LIST_PAUSE_AT_END_ON_JUMP)}
    , mRenderer {Renderer::getInstance()}
    , mEntryCamOffset {0.0f}
    , mEntryCamTarget {0.0f}
    , mPreviousScrollVelocity {0}
    , mPositiveDirection {false}
    , mTriggerJump {false}
    , mGamelistView {std::is_same_v<T, FileData*> ? true : false}
    , mUppercase {false}
    , mLowercase {false}
    , mCapitalize {false}
    , mType {CarouselType::HORIZONTAL}
    , mLegacyMode {false}
    , mFont {Font::get(FONT_SIZE_LARGE)}
    , mTextColor {0x000000FF}
    , mTextBackgroundColor {0xFFFFFF00}
    , mLineSpacing {1.5f}
    , mItemHorizontalAlignment {ALIGN_CENTER}
    , mItemVerticalAlignment {ALIGN_CENTER}
    , mWheelHorizontalAlignment {ALIGN_CENTER}
    , mUnfocusedItemOpacity {0.5f}
    , mMaxItemCount {3.0f}
    , mItemsBeforeCenter {8}
    , mItemsAfterCenter {8}
    , mItemSize {glm::vec2 {Renderer::getScreenWidth() * 0.25f,
                            Renderer::getScreenHeight() * 0.155f}}
    , mLinearInterpolation {false}
    , mInstantItemTransitions {false}
    , mItemAxisHorizontal {false}
    , mFadeAbovePrimary {false}
    , mItemScale {1.2f}
    , mItemRotation {7.5f}
    , mItemRotationOrigin {-3.0f, 0.5f}
    , mCarouselColor {0}
    , mCarouselColorEnd {0}
    , mColorGradientHorizontal {true}
    , mReflections {false}
    , mReflectionsOpacity {0.5f}
    , mReflectionsFalloff {1.0f}
    , mHorizontalOffset {0.0f}
    , mVerticalOffset {0.0f}
{
}

template <typename T>
void CarouselComponent<T>::addEntry(Entry& entry, const std::shared_ptr<ThemeData>& theme)
{
    bool legacyMode {theme->isLegacyTheme()};
    bool dynamic {true};

    if (!mGamelistView)
        dynamic = false;

    if (legacyMode) {
        const ThemeData::ThemeElement* itemElem {
            theme->getElement("system", "image_logo", "image")};

        if (itemElem) {
            std::string path;
            if (itemElem->has("path"))
                path = itemElem->get<std::string>("path");
            std::string defaultPath {
                itemElem->has("default") ? itemElem->get<std::string>("default") : ""};
            if ((!path.empty() && ResourceManager::getInstance().fileExists(path)) ||
                (!defaultPath.empty() && ResourceManager::getInstance().fileExists(defaultPath))) {
                auto item = std::make_shared<ImageComponent>(false, dynamic);
                item->setLinearInterpolation(mLinearInterpolation);
                item->setMipmapping(true);
                item->setMaxSize(glm::round(mItemSize * (mItemScale >= 1.0f ? mItemScale : 1.0f)));
                item->applyTheme(theme, "system", "image_logo",
                                 ThemeFlags::PATH | ThemeFlags::COLOR);
                item->setRotateByTargetSize(true);
                entry.data.item = item;
            }
        }
    }
    else {
        if (entry.data.itemPath != "" &&
            ResourceManager::getInstance().fileExists(entry.data.itemPath)) {
            auto item = std::make_shared<ImageComponent>(false, dynamic);
            item->setLinearInterpolation(mLinearInterpolation);
            item->setMipmapping(true);
            item->setMaxSize(glm::round(mItemSize * (mItemScale >= 1.0f ? mItemScale : 1.0f)));
            item->setImage(entry.data.itemPath);
            item->applyTheme(theme, "system", "", ThemeFlags::ALL);
            item->setRotateByTargetSize(true);
            entry.data.item = item;
        }
        else if (entry.data.defaultItemPath != "" &&
                 ResourceManager::getInstance().fileExists(entry.data.defaultItemPath)) {
            auto defaultItem = std::make_shared<ImageComponent>(false, dynamic);
            defaultItem->setLinearInterpolation(mLinearInterpolation);
            defaultItem->setMipmapping(true);
            defaultItem->setMaxSize(
                glm::round(mItemSize * (mItemScale >= 1.0f ? mItemScale : 1.0f)));
            defaultItem->setImage(entry.data.defaultItemPath);
            defaultItem->applyTheme(theme, "system", "", ThemeFlags::ALL);
            defaultItem->setRotateByTargetSize(true);
            entry.data.item = defaultItem;
        }
    }

    if (!entry.data.item) {
        // If no item image is present, add item text as fallback.
        std::string nameEntry;
        if (!mGamelistView)
            nameEntry = entry.name;
        else if (mUppercase)
            nameEntry = Utils::String::toUpper(entry.name);
        else if (mLowercase)
            nameEntry = Utils::String::toLower(entry.name);
        else if (mCapitalize)
            nameEntry = Utils::String::toCapitalized(entry.name);
        else
            nameEntry = entry.name;

        auto text = std::make_shared<TextComponent>(
            nameEntry, mFont, 0x000000FF, mItemHorizontalAlignment, mItemVerticalAlignment,
            glm::vec3 {0.0f, 0.0f, 0.0f},
            glm::round(mItemSize * (mItemScale >= 1.0f ? mItemScale : 1.0f)), 0x00000000);
        if (legacyMode) {
            text->applyTheme(theme, "system", "text_logoText",
                             ThemeFlags::FONT_PATH | ThemeFlags::FONT_SIZE | ThemeFlags::COLOR |
                                 ThemeFlags::LETTER_CASE | ThemeFlags::FORCE_UPPERCASE |
                                 ThemeFlags::LINE_SPACING | ThemeFlags::TEXT);
        }
        if (!legacyMode) {
            text->setLineSpacing(mLineSpacing);
            if (!mGamelistView) {
                if (mText != "")
                    text->setValue(mText);
            }
            text->setColor(mTextColor);
            text->setBackgroundColor(mTextBackgroundColor);
            text->setRenderBackground(true);
        }
        entry.data.item = text;
    }

    // Set origin for the items based on their alignment so they line up properly.
    if (mItemHorizontalAlignment == ALIGN_LEFT)
        entry.data.item->setOrigin(0.0f, 0.5f);
    else if (mItemHorizontalAlignment == ALIGN_RIGHT)
        entry.data.item->setOrigin(1.0f, 0.5f);
    else
        entry.data.item->setOrigin(0.5f, 0.5f);

    if (mItemVerticalAlignment == ALIGN_TOP)
        entry.data.item->setOrigin(entry.data.item->getOrigin().x, 0.0f);
    else if (mItemVerticalAlignment == ALIGN_BOTTOM)
        entry.data.item->setOrigin(entry.data.item->getOrigin().x, 1.0f);
    else
        entry.data.item->setOrigin(entry.data.item->getOrigin().x, 0.5f);

    glm::vec2 denormalized {glm::round(mItemSize * entry.data.item->getOrigin())};
    entry.data.item->setPosition(glm::vec3 {denormalized.x, denormalized.y, 0.0f});

    List::add(entry);

    mText = "";
}

template <typename T>
void CarouselComponent<T>::updateEntry(Entry& entry, const std::shared_ptr<ThemeData>& theme)
{
    if (entry.data.itemPath != "") {
        auto item = std::make_shared<ImageComponent>(false, true);
        item->setLinearInterpolation(mLinearInterpolation);
        item->setMipmapping(true);
        item->setMaxSize(glm::round(mItemSize * (mItemScale >= 1.0f ? mItemScale : 1.0f)));
        item->setImage(entry.data.itemPath);
        item->applyTheme(theme, "system", "", ThemeFlags::ALL);
        item->setRotateByTargetSize(true);
        entry.data.item = item;
    }
    else {
        return;
    }

    // Set origin for the items based on their alignment so they line up properly.
    if (mItemHorizontalAlignment == ALIGN_LEFT)
        entry.data.item->setOrigin(0.0f, 0.5f);
    else if (mItemHorizontalAlignment == ALIGN_RIGHT)
        entry.data.item->setOrigin(1.0f, 0.5f);
    else
        entry.data.item->setOrigin(0.5f, 0.5f);

    if (mItemVerticalAlignment == ALIGN_TOP)
        entry.data.item->setOrigin(entry.data.item->getOrigin().x, 0.0f);
    else if (mItemVerticalAlignment == ALIGN_BOTTOM)
        entry.data.item->setOrigin(entry.data.item->getOrigin().x, 1.0f);
    else
        entry.data.item->setOrigin(entry.data.item->getOrigin().x, 0.5f);

    glm::vec2 denormalized {glm::round(mItemSize * entry.data.item->getOrigin())};
    entry.data.item->setPosition(glm::vec3 {denormalized.x, denormalized.y, 0.0f});
}

template <typename T> void CarouselComponent<T>::onDemandTextureLoad()
{
    if constexpr (std::is_same_v<T, FileData*>) {
        const int numEntries {static_cast<int>(mEntries.size())};
        const int center {getCursor()};
        const bool isWheel {mType == CarouselType::VERTICAL_WHEEL ||
                            mType == CarouselType::HORIZONTAL_WHEEL};
        int centerOffset {0};
        int itemInclusion {0};
        int itemInclusionBefore {0};
        int itemInclusionAfter {0};

        if (isWheel) {
            itemInclusion = 1;
            itemInclusionBefore = mItemsBeforeCenter - 1;
            itemInclusionAfter = mItemsAfterCenter;
        }
        else {
            itemInclusion = static_cast<int>(std::ceil((mMaxItemCount + 1) / 2.0f));
            itemInclusionBefore = -1;
            // If the carousel is offset we need to load additional textures to fully populate
            // the visible entries.
            if (mType == CarouselType::HORIZONTAL && mHorizontalOffset != 0.0f) {
                const float itemSpacing {
                    ((mSize.x - (mItemSize.x * mMaxItemCount)) / mMaxItemCount) + mItemSize.x};

                centerOffset = static_cast<int>(std::ceil(mSize.x * std::fabs(mHorizontalOffset) /
                                                          std::min(mItemSize.x, itemSpacing)));
                if (mHorizontalOffset < 0.0f)
                    itemInclusionAfter += centerOffset;
                else
                    itemInclusionBefore += centerOffset;

                if (mHorizontalOffset > 0.0f)
                    centerOffset = -centerOffset;
            }
            else if (mType == CarouselType::VERTICAL && mVerticalOffset != 0.0f) {
                const float itemSpacing {
                    ((mSize.y - (mItemSize.y * mMaxItemCount)) / mMaxItemCount) + mItemSize.y};

                centerOffset = static_cast<int>(std::ceil(mSize.y * std::fabs(mVerticalOffset) /
                                                          std::min(mItemSize.y, itemSpacing)));
                if (mVerticalOffset < 0.0f)
                    itemInclusionAfter += centerOffset;
                else
                    itemInclusionBefore += centerOffset;

                if (mVerticalOffset > 0.0f)
                    centerOffset = -centerOffset;
            }
        }

        for (int i = center - itemInclusion - itemInclusionBefore;
             i < center + itemInclusion + itemInclusionAfter; ++i) {
            int cursor {i};

            while (cursor < 0)
                cursor += numEntries;
            while (cursor >= numEntries)
                cursor -= numEntries;

            auto& entry = mEntries.at(cursor);

            if (entry.data.itemPath == "") {
                FileData* game {entry.object};

                if (mItemType == "" || mItemType == "marquee")
                    entry.data.itemPath = game->getMarqueePath();
                else if (mItemType == "cover")
                    entry.data.itemPath = game->getCoverPath();
                else if (mItemType == "backcover")
                    entry.data.itemPath = game->getBackCoverPath();
                else if (mItemType == "3dbox")
                    entry.data.itemPath = game->get3DBoxPath();
                else if (mItemType == "physicalmedia")
                    entry.data.itemPath = game->getPhysicalMediaPath();
                else if (mItemType == "screenshot")
                    entry.data.itemPath = game->getScreenshotPath();
                else if (mItemType == "titlescreen")
                    entry.data.itemPath = game->getTitleScreenPath();
                else if (mItemType == "miximage")
                    entry.data.itemPath = game->getMiximagePath();
                else if (mItemType == "fanart")
                    entry.data.itemPath = game->getFanArtPath();
                else if (mItemType == "none") // Display the game name as text.
                    return;

                auto theme = game->getSystem()->getTheme();
                updateEntry(entry, theme);
            }
        }
    }
}

template <typename T> bool CarouselComponent<T>::input(InputConfig* config, Input input)
{
    if (input.value != 0) {
        switch (mType) {
            case CarouselType::VERTICAL:
            case CarouselType::VERTICAL_WHEEL:
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
                break;
            case CarouselType::HORIZONTAL:
            case CarouselType::HORIZONTAL_WHEEL:
            default:
                if (config->isMappedLike("left", input)) {
                    if (mCancelTransitionsCallback)
                        mCancelTransitionsCallback();
                    List::listInput(-1);
                    return true;
                }
                if (config->isMappedLike("right", input)) {
                    if (mCancelTransitionsCallback)
                        mCancelTransitionsCallback();
                    List::listInput(1);
                    return true;
                }
                break;
        }
        if (mGamelistView) {
            if (config->isMappedLike("leftshoulder", input)) {
                if (mCancelTransitionsCallback)
                    mCancelTransitionsCallback();
                if (mEntries.size() < 10 && getCursor() != 0) {
                    mTriggerJump = true;
                    return this->listFirstRow();
                }
                else {
                    List::listInput(-10);
                    return true;
                }
            }
            if (config->isMappedLike("rightshoulder", input)) {
                if (mCancelTransitionsCallback)
                    mCancelTransitionsCallback();
                if (mEntries.size() < 10 && getCursor() != static_cast<int>(mEntries.size()) - 1) {
                    mTriggerJump = true;
                    return this->listLastRow();
                }
                else {
                    List::listInput(10);
                    return true;
                }
            }
            if (config->isMappedLike("lefttrigger", input)) {
                if (getCursor() == 0)
                    return true;
                mTriggerJump = true;
                if (mCancelTransitionsCallback)
                    mCancelTransitionsCallback();
                return this->listFirstRow();
            }
            if (config->isMappedLike("righttrigger", input)) {
                if (getCursor() == static_cast<int>(mEntries.size()) - 1)
                    return true;
                mTriggerJump = true;
                if (mCancelTransitionsCallback)
                    mCancelTransitionsCallback();
                return this->listLastRow();
            }
        }
    }
    else {
        if (mGamelistView) {
            if (config->isMappedLike("up", input) || config->isMappedLike("down", input) ||
                config->isMappedLike("left", input) || config->isMappedLike("right", input) ||
                config->isMappedLike("leftshoulder", input) ||
                config->isMappedLike("rightshoulder", input) ||
                config->isMappedLike("lefttrigger", input) ||
                config->isMappedLike("righttrigger", input)) {
                onCursorChanged(CursorState::CURSOR_STOPPED);
                List::listInput(0);
                mTriggerJump = false;
            }
        }
        else {
            if (config->isMappedLike("up", input) || config->isMappedLike("down", input) ||
                config->isMappedLike("left", input) || config->isMappedLike("right", input)) {
                if (isScrolling())
                    onCursorChanged(CursorState::CURSOR_STOPPED);
                List::listInput(0);
            }
        }
    }

    return GuiComponent::input(config, input);
}

template <typename T> void CarouselComponent<T>::update(int deltaTime)
{
    List::listUpdate(deltaTime);
    GuiComponent::update(deltaTime);
}

template <typename T> void CarouselComponent<T>::render(const glm::mat4& parentTrans)
{
    const float camOffset {mInstantItemTransitions ? mEntryCamTarget : mEntryCamOffset};

    int numEntries {static_cast<int>(mEntries.size())};
    if (numEntries == 0)
        return;

    glm::mat4 carouselTrans {parentTrans};
    carouselTrans = glm::translate(
        carouselTrans, glm::vec3 {GuiComponent::mPosition.x, GuiComponent::mPosition.y, 0.0f});
    carouselTrans =
        glm::translate(carouselTrans, glm::vec3 {GuiComponent::mOrigin.x * mSize.x * -1.0f,
                                                 GuiComponent::mOrigin.y * mSize.y * -1.0f, 0.0f});

    if (carouselTrans[3].x + mSize.x <= 0.0f || carouselTrans[3].y + mSize.y <= 0.0f)
        return;

    const float sizeX {carouselTrans[3].x < 0.0f ? mSize.x + carouselTrans[3].x : mSize.x};
    const float sizeY {carouselTrans[3].y < 0.0f ? mSize.y + carouselTrans[3].y : mSize.y};

    glm::ivec2 clipPos {static_cast<int>(glm::clamp(std::round(carouselTrans[3].x), 0.0f,
                                                    mRenderer->getScreenWidth())),
                        static_cast<int>(glm::clamp(std::round(carouselTrans[3].y), 0.0f,
                                                    mRenderer->getScreenHeight()))};
    glm::ivec2 clipDim {
        static_cast<int>(std::min(std::round(sizeX), mRenderer->getScreenWidth())),
        static_cast<int>(std::min(std::round(sizeY), mRenderer->getScreenHeight()))};

    mRenderer->pushClipRect(clipPos, clipDim);
    mRenderer->setMatrix(carouselTrans);

    // In image debug mode, draw a green rectangle covering the entire carousel area.
    if (Settings::getInstance()->getBool("DebugImage"))
        mRenderer->drawRect(0.0f, 0.0f, mSize.x, mSize.y, 0x00FF0033, 0x00FF0033);

    // Background box behind the items.
    mRenderer->drawRect(0.0f, 0.0f, mSize.x, mSize.y, mCarouselColor, mCarouselColorEnd,
                        mColorGradientHorizontal);

    const bool isWheel {mType == CarouselType::VERTICAL_WHEEL ||
                        mType == CarouselType::HORIZONTAL_WHEEL};

    // Draw the items.
    // itemSpacing will also include the size of the item itself.
    glm::vec2 itemSpacing {0.0f, 0.0f};
    float xOff {0.0f};
    float yOff {0.0f};
    float scaleSize {mItemSize.x * mItemScale - mItemSize.x};

    if (isWheel) {
        xOff = (mSize.x - mItemSize.x) / 2.0f;
        yOff = (mSize.y - mItemSize.y) / 2.0f;
        // Alignment of the actual carousel inside to the overall component area.
        if (mLegacyMode) {
            if (mItemHorizontalAlignment == ALIGN_LEFT)
                xOff = mSize.x / 10.0f;
            else if (mItemHorizontalAlignment == ALIGN_RIGHT)
                xOff = mSize.x - (mItemSize.x * 1.1f);
            else
                xOff = (mSize.x - mItemSize.x) / 2.0f;
        }
        else {
            if (mWheelHorizontalAlignment == ALIGN_RIGHT) {
                xOff += mSize.x / 2.0f;
                if (mItemHorizontalAlignment == ALIGN_LEFT)
                    xOff -= mItemSize.x / 2.0f + scaleSize;
                else if (mItemHorizontalAlignment == ALIGN_RIGHT)
                    xOff -= mItemSize.x / 2.0f;
                else
                    xOff -= mItemSize.x / 2.0f + scaleSize / 2.0f;
            }
            else if (mWheelHorizontalAlignment == ALIGN_LEFT) {
                xOff -= (mSize.x / 2.0f);
                if (mItemHorizontalAlignment == ALIGN_LEFT)
                    xOff += mItemSize.x / 2.0f;
                else if (mItemHorizontalAlignment == ALIGN_RIGHT)
                    xOff += mItemSize.x / 2.0f + scaleSize;
                else
                    xOff += mItemSize.x / 2.0f + scaleSize / 2.0f;
            }
            else if (mWheelHorizontalAlignment == ALIGN_CENTER &&
                     mItemHorizontalAlignment != ALIGN_CENTER) {
                if (mItemHorizontalAlignment == ALIGN_RIGHT)
                    xOff += scaleSize / 2.0f;
                else if (mItemHorizontalAlignment == ALIGN_LEFT)
                    xOff -= scaleSize / 2.0f;
            }
        }
    }
    else if (mType == CarouselType::VERTICAL) {
        itemSpacing.y = ((mSize.y - (mItemSize.y * mMaxItemCount)) / mMaxItemCount) + mItemSize.y;
        yOff = (mSize.y - mItemSize.y) / 2.0f - (camOffset * itemSpacing.y);
        if (mItemHorizontalAlignment == ALIGN_LEFT) {
            if (mLegacyMode)
                xOff = mItemSize.x / 10.0f;
            else
                xOff = 0.0f;
        }
        else if (mItemHorizontalAlignment == ALIGN_RIGHT) {
            if (mLegacyMode)
                xOff = mSize.x - mItemSize.x * 1.1f;
            else
                xOff = mSize.x - mItemSize.x;
        }
        else {
            xOff = (mSize.x - mItemSize.x) / 2.0f;
        }
    }
    else { // HORIZONTAL.
        itemSpacing.x = ((mSize.x - (mItemSize.x * mMaxItemCount)) / mMaxItemCount) + mItemSize.x;
        xOff = (mSize.x - mItemSize.x) / 2.0f - (camOffset * itemSpacing.x);
        if (mItemVerticalAlignment == ALIGN_TOP) {
            if (mLegacyMode)
                yOff = mItemSize.y / 10.0f;
            else
                yOff = 0.0f;
        }
        else if (mItemVerticalAlignment == ALIGN_BOTTOM) {
            if (mLegacyMode)
                yOff = mSize.y - (mItemSize.y * 1.1f);
            else
                yOff = mSize.y - mItemSize.y - (mReflections ? ((mItemSize.y * mItemScale)) : 0.0f);
        }
        else {
            if (mLegacyMode)
                yOff = (mSize.y - mItemSize.y) / 2.0f;
            else
                yOff = (mSize.y - (mItemSize.y * (mReflections ? 2.0f : 1.0f))) / 2.0f;
        }
    }

    if (!mLegacyMode) {
        xOff += mSize.x * mHorizontalOffset;
        yOff += mSize.y * mVerticalOffset;
    }

    int center {0};
    int centerOffset {0};
    // Needed to make sure that overlapping items are renderered correctly.
    if (mPositiveDirection)
        center = static_cast<int>(std::floor(camOffset));
    else
        center = static_cast<int>(std::ceil(camOffset));

    int itemInclusion {0};
    int itemInclusionBefore {0};
    int itemInclusionAfter {0};

    if (mLegacyMode || mType == CarouselType::HORIZONTAL || mType == CarouselType::VERTICAL) {
        itemInclusion = static_cast<int>(std::ceil(mMaxItemCount / 2.0f)) + 1;
        itemInclusionAfter = 2;
        // If the carousel is offset we need to calculate the center offset so the items are
        // rendered in the correct order (as seen when overlapping).
        if (mType == CarouselType::HORIZONTAL && mHorizontalOffset != 0.0f) {
            centerOffset = static_cast<int>(std::ceil(mSize.x * std::fabs(mHorizontalOffset) /
                                                      std::min(mItemSize.x, itemSpacing.x)));
            if (mHorizontalOffset < 0.0f)
                itemInclusionAfter += centerOffset;
            else
                itemInclusionBefore += centerOffset;

            if (mHorizontalOffset > 0.0f)
                centerOffset = -centerOffset;
        }
        else if (mType == CarouselType::VERTICAL && mVerticalOffset != 0.0f) {
            centerOffset = static_cast<int>(std::ceil(mSize.y * std::fabs(mVerticalOffset) /
                                                      std::min(mItemSize.y, itemSpacing.y)));
            if (mVerticalOffset < 0.0f)
                itemInclusionAfter += centerOffset;
            else
                itemInclusionBefore += centerOffset;

            if (mVerticalOffset > 0.0f)
                centerOffset = -centerOffset;
        }
    }
    else {
        // For the wheel types.
        itemInclusion = 1;
        itemInclusionBefore = mItemsBeforeCenter - 1;
        itemInclusionAfter = mItemsAfterCenter;
    }

    const bool singleEntry {numEntries == 1};

    struct renderStruct {
        int index;
        float distance;
        float scale;
        float opacity;
        glm::mat4 trans;
    };

    std::vector<renderStruct> renderItems;
    std::vector<renderStruct> renderItemsSorted;

    for (int i = center - itemInclusion - itemInclusionBefore;
         i < center + itemInclusion + itemInclusionAfter; ++i) {
        int index {i};

        while (index < 0)
            index += numEntries;
        while (index >= numEntries)
            index -= numEntries;

        float distance {i - camOffset};

        if (singleEntry)
            distance = 0.0f;

        float scale {0.0f};

        if (mItemScale >= 1.0f) {
            scale = 1.0f + ((mItemScale - 1.0f) * (1.0f - fabsf(distance)));
            scale = std::min(mItemScale, std::max(1.0f, scale));
            scale /= mItemScale;
        }
        else {
            scale = 1.0f + ((1.0f - mItemScale) * (fabsf(distance) - 1.0f));
            scale = std::max(mItemScale, std::min(1.0f, scale));
        }

        glm::mat4 itemTrans {carouselTrans};
        if (singleEntry)
            itemTrans = glm::translate(carouselTrans, glm::vec3 {xOff, yOff, 0.0f});
        else
            itemTrans = glm::translate(itemTrans, glm::vec3 {(i * itemSpacing.x) + xOff,
                                                             (i * itemSpacing.y) + yOff, 0.0f});

        float opacity {0.0f};

        if (distance == 0.0f || mUnfocusedItemOpacity == 1.0f) {
            opacity = 1.0f;
        }
        else if (fabsf(distance) >= 1.0f) {
            opacity = mUnfocusedItemOpacity;
        }
        else {
            float maxDiff {1.0f - mUnfocusedItemOpacity};
            opacity = mUnfocusedItemOpacity + (maxDiff - (maxDiff * fabsf(distance)));
        }

        renderStruct renderItem;
        renderItem.index = index;
        renderItem.distance = distance;
        renderItem.scale = scale;
        renderItem.opacity = opacity;
        renderItem.trans = itemTrans;

        renderItems.emplace_back(renderItem);

        if (singleEntry)
            break;
    }

    int belowCenter {static_cast<int>(std::round((renderItems.size() - centerOffset - 1) / 2))};
    if (renderItems.size() == 1) {
        renderItemsSorted.emplace_back(renderItems.front());
    }
    else {
        // Make sure that overlapping items are rendered in the correct order.
        size_t zeroDistanceEntry {0};

        for (int i = 0; i < belowCenter; ++i)
            renderItemsSorted.emplace_back(renderItems[i]);

        for (int i = static_cast<int>(renderItems.size()) - 1; i > belowCenter - 1; --i) {
            if (isWheel && (mPositiveDirection ? std::ceil(renderItems[i].distance) :
                                                 std::floor(renderItems[i].distance)) == 0) {
                zeroDistanceEntry = i;
                continue;
            }
            renderItemsSorted.emplace_back(renderItems[i]);
        }

        if (isWheel)
            renderItemsSorted.emplace_back(renderItems[zeroDistanceEntry]);
    }

    for (auto& renderItem : renderItemsSorted) {
        const std::shared_ptr<GuiComponent>& comp {mEntries.at(renderItem.index).data.item};

        if (comp == nullptr)
            continue;

        if (isWheel) {
            glm::mat4 positionCalc {renderItem.trans};
            const float xOffTrans {(GuiComponent::mOrigin.x - mItemRotationOrigin.x) * mItemSize.x};
            const float yOffTrans {mItemAxisHorizontal ?
                                       0.0f :
                                       (GuiComponent::mOrigin.y - mItemRotationOrigin.y) *
                                           mItemSize.y};

            // Transform to offset point.
            positionCalc = glm::translate(positionCalc,
                                          glm::vec3 {xOffTrans * -1.0f, yOffTrans * -1.0f, 0.0f});

            // Apply rotation transform.
            positionCalc =
                glm::rotate(positionCalc, glm::radians(mItemRotation * renderItem.distance),
                            glm::vec3 {0.0f, 0.0f, 1.0f});

            // Transform back to original point.
            positionCalc = glm::translate(positionCalc, glm::vec3 {xOffTrans, yOffTrans, 0.0f});

            if (mItemAxisHorizontal) {
                // Only keep position and discard the rotation data.
                renderItem.trans[3].x = positionCalc[3].x;
                renderItem.trans[3].y = positionCalc[3].y;
            }
            else {
                renderItem.trans = positionCalc;
                renderItem.trans = positionCalc;
            }
        }

        float metadataOpacity {1.0f};

        if constexpr (std::is_same_v<T, FileData*>) {
            // If a game is marked as hidden, lower the opacity a lot.
            // If a game is marked to not be counted, lower the opacity a moderate amount.
            if (mEntries.at(renderItem.index).object->getHidden())
                metadataOpacity = 0.4f;
            else if (!mEntries.at(renderItem.index).object->getCountAsGame())
                metadataOpacity = 0.7f;
        }

        comp->setScale(renderItem.scale);
        comp->setOpacity(renderItem.opacity * metadataOpacity);
        comp->render(renderItem.trans);

        // TODO: Rewrite to use "real" reflections instead of this hack.
        // Don't attempt to add reflections for text entries.
        if (mReflections && (mEntries.at(renderItem.index).data.itemPath != "" ||
                             mEntries.at(renderItem.index).data.defaultItemPath != "")) {
            glm::mat4 reflectionTrans {glm::translate(
                renderItem.trans, glm::vec3 {0.0f, comp->getSize().y * renderItem.scale, 0.0f})};
            float falloff {glm::clamp(mReflectionsFalloff, 0.0f, 1.0f)};
            falloff = mReflectionsOpacity * (1.0f - falloff);
            comp->setOpacity(comp->getOpacity() * mReflectionsOpacity);
            if (mReflectionsFalloff > 0.0f)
                comp->setReflectionsFalloff(comp->getSize().y / mReflectionsFalloff);
            comp->setFlipY(true);
            comp->render(reflectionTrans);
            comp->setFlipY(false);
            comp->setReflectionsFalloff(0.0f);
        }

        if (singleEntry)
            break;
    }
    mRenderer->popClipRect();
}

template <typename T>
void CarouselComponent<T>::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                      const std::string& view,
                                      const std::string& element,
                                      unsigned int properties)
{
    using namespace ThemeFlags;
    const ThemeData::ThemeElement* elem {theme->getElement(view, element, "carousel")};

    mSize.x = Renderer::getScreenWidth();
    mSize.y = Renderer::getScreenHeight() * 0.23240f;
    GuiComponent::mPosition.x = 0.0f;
    GuiComponent::mPosition.y = Renderer::getScreenHeight() * 0.38378f;
    mCarouselColor = 0xFFFFFFD8;
    mCarouselColorEnd = 0xFFFFFFD8;
    mZIndex = mDefaultZIndex;
    mText = "";
    mUppercase = false;
    mLowercase = false;
    mCapitalize = false;

    if (!elem)
        return;

    mLegacyMode = theme->isLegacyTheme();

    if (elem->has("type")) {
        const std::string type {elem->get<std::string>("type")};
        if (type == "horizontal") {
            mType = CarouselType::HORIZONTAL;
        }
        else if (type == "horizontal_wheel") {
            mType = CarouselType::HORIZONTAL_WHEEL;
        }
        else if (type == "vertical") {
            mType = CarouselType::VERTICAL;
        }
        else if (type == "vertical_wheel") {
            mType = CarouselType::VERTICAL_WHEEL;
        }
        else {
            LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property \"type\" "
                               "for element \""
                            << element.substr(9) << "\" defined as \"" << type << "\"";
            mType = CarouselType::HORIZONTAL;
        }
    }

    if (elem->has("color")) {
        mCarouselColor = elem->get<unsigned int>("color");
        mCarouselColorEnd = mCarouselColor;
    }
    if (elem->has("colorEnd"))
        mCarouselColorEnd = elem->get<unsigned int>("colorEnd");

    if (elem->has("gradientType")) {
        const std::string gradientType {elem->get<std::string>("gradientType")};
        if (gradientType == "horizontal") {
            mColorGradientHorizontal = true;
        }
        else if (gradientType == "vertical") {
            mColorGradientHorizontal = false;
        }
        else {
            mColorGradientHorizontal = true;
            LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                               "\"gradientType\" for element \""
                            << element.substr(9) << "\" defined as \"" << gradientType << "\"";
        }
    }

    if (!mLegacyMode) {
        mLinearInterpolation = true;

        if (elem->has("maxItemCount")) {
            mMaxItemCount = glm::clamp(elem->get<float>("maxItemCount"), 0.5f, 30.0f);
            if (mType == CarouselType::HORIZONTAL_WHEEL || mType == CarouselType::VERTICAL_WHEEL) {
                LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                                   "\"maxItemCount\" for element \""
                                << element.substr(9) << "\" not applicable to the "
                                << (mType == CarouselType::HORIZONTAL_WHEEL ?
                                        "\"horizontal_wheel\"" :
                                        "\"vertical_wheel\"")
                                << " type";
            }
        }

        if (elem->has("itemsBeforeCenter"))
            mItemsBeforeCenter =
                glm::clamp(static_cast<int>(elem->get<unsigned int>("itemsBeforeCenter")), 0, 20);

        if (elem->has("itemsAfterCenter"))
            mItemsAfterCenter =
                glm::clamp(static_cast<int>(elem->get<unsigned int>("itemsAfterCenter")), 0, 20);

        if (elem->has("itemSize")) {
            const glm::vec2 itemSize {glm::clamp(elem->get<glm::vec2>("itemSize"), 0.05f, 1.0f)};
            mItemSize =
                itemSize * glm::vec2(Renderer::getScreenWidth(), Renderer::getScreenHeight());
        }

        if (elem->has("itemScale"))
            mItemScale = glm::clamp(elem->get<float>("itemScale"), 0.2f, 3.0f);

        if (elem->has("itemTransitions")) {
            const std::string itemTransitions {elem->get<std::string>("itemTransitions")};
            if (itemTransitions == "slide") {
                mInstantItemTransitions = false;
            }
            else if (itemTransitions == "instant") {
                mInstantItemTransitions = true;
            }
            else {
                mInstantItemTransitions = false;
                LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                                   "\"itemTransitions\" for element \""
                                << element.substr(9) << "\" defined as \"" << itemTransitions
                                << "\"";
            }
        }

        if (elem->has("itemInterpolation")) {
            const std::string itemInterpolation {elem->get<std::string>("itemInterpolation")};
            if (itemInterpolation == "linear") {
                mLinearInterpolation = true;
            }
            else if (itemInterpolation == "nearest") {
                mLinearInterpolation = false;
            }
            else {
                mLinearInterpolation = true;
                LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                                   "\"itemInterpolation\" for element \""
                                << element.substr(9) << "\" defined as \"" << itemInterpolation
                                << "\"";
            }
        }

        if (elem->has("itemRotation"))
            mItemRotation = elem->get<float>("itemRotation");

        if (elem->has("itemRotationOrigin"))
            mItemRotationOrigin = elem->get<glm::vec2>("itemRotationOrigin");

        mItemAxisHorizontal =
            (elem->has("itemAxisHorizontal") && elem->get<bool>("itemAxisHorizontal"));

        if (elem->has("itemHorizontalAlignment")) {
            const std::string alignment {elem->get<std::string>("itemHorizontalAlignment")};
            if (alignment == "left" && mType != CarouselType::HORIZONTAL) {
                mItemHorizontalAlignment = ALIGN_LEFT;
            }
            else if (alignment == "right" && mType != CarouselType::HORIZONTAL) {
                mItemHorizontalAlignment = ALIGN_RIGHT;
            }
            else if (alignment == "center") {
                mItemHorizontalAlignment = ALIGN_CENTER;
            }
            else {
                LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                                   "\"itemHorizontalAlignment\" for element \""
                                << element.substr(9) << "\" defined as \"" << alignment << "\"";
                mItemHorizontalAlignment = ALIGN_CENTER;
            }
        }

        if (elem->has("itemVerticalAlignment")) {
            const std::string alignment {elem->get<std::string>("itemVerticalAlignment")};
            if (alignment == "top" && mType != CarouselType::VERTICAL) {
                mItemVerticalAlignment = ALIGN_TOP;
            }
            else if (alignment == "bottom" && mType != CarouselType::VERTICAL) {
                mItemVerticalAlignment = ALIGN_BOTTOM;
            }
            else if (alignment == "center") {
                mItemVerticalAlignment = ALIGN_CENTER;
            }
            else {
                LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                                   "\"itemVerticalAlignment\" for element \""
                                << element.substr(9) << "\" defined as \"" << alignment << "\"";
                mItemVerticalAlignment = ALIGN_CENTER;
            }
        }

        if (elem->has("wheelHorizontalAlignment")) {
            const std::string alignment {elem->get<std::string>("wheelHorizontalAlignment")};
            if (alignment == "left") {
                mWheelHorizontalAlignment = ALIGN_LEFT;
            }
            else if (alignment == "right") {
                mWheelHorizontalAlignment = ALIGN_RIGHT;
            }
            else if (alignment == "center") {
                mWheelHorizontalAlignment = ALIGN_CENTER;
            }
            else {
                LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                                   "\"wheelHorizontalAlignment\" for element \""
                                << element.substr(9) << "\" defined as \"" << alignment << "\"";
                mWheelHorizontalAlignment = ALIGN_CENTER;
            }
        }

        if (elem->has("horizontalOffset"))
            mHorizontalOffset = glm::clamp(elem->get<float>("horizontalOffset"), -1.0f, 1.0f);

        if (elem->has("verticalOffset"))
            mVerticalOffset = glm::clamp(elem->get<float>("verticalOffset"), -1.0f, 1.0f);

        if (elem->has("reflections") && elem->get<bool>("reflections")) {
            if (mType == CarouselType::HORIZONTAL) {
                mReflections = elem->get<bool>("reflections");
            }
            else {
                LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                                   "\"reflections\" for element \""
                                << element.substr(9)
                                << "\" only supported for horizontal carousel type";
            }
        }

        if (elem->has("reflectionsOpacity"))
            mReflectionsOpacity = glm::clamp(elem->get<float>("reflectionsOpacity"), 0.1f, 1.0f);

        if (elem->has("reflectionsFalloff"))
            mReflectionsFalloff = glm::clamp(elem->get<float>("reflectionsFalloff"), 0.0f, 10.0f);

        if (elem->has("unfocusedItemOpacity"))
            mUnfocusedItemOpacity =
                glm::clamp(elem->get<float>("unfocusedItemOpacity"), 0.1f, 1.0f);
    }

    // Legacy themes.
    if (mLegacyMode) {
        // Don't allow logoScale below 1.0 for legacy themes as it introduces compatibility issues.
        if (elem->has("logoScale"))
            mItemScale = glm::clamp(elem->get<float>("logoScale"), 1.0f, 3.0f);
        if (elem->has("logoSize")) {
            // Keep size within a 0.05 and 1.0 multiple of the screen size.
            glm::vec2 itemSize {elem->get<glm::vec2>("logoSize")};
            if (std::max(itemSize.x, itemSize.y) > 1.0f) {
                itemSize /= std::max(itemSize.x, itemSize.y);
            }
            else if (std::min(itemSize.x, itemSize.y) < 0.005f) {
                float ratio {std::min(itemSize.x, itemSize.y) / 0.005f};
                itemSize /= ratio;
                // Just an extra precaution if a crazy ratio was used.
                itemSize.x = glm::clamp(itemSize.x, 0.005f, 1.0f);
                itemSize.y = glm::clamp(itemSize.y, 0.005f, 1.0f);
            }
            mItemSize =
                itemSize * glm::vec2(Renderer::getScreenWidth(), Renderer::getScreenHeight());
        }

        if (elem->has("maxLogoCount")) {
            // For legacy themes we allow a maxLogoCount (maxItemCount) of 0.
            mMaxItemCount = std::ceil(glm::clamp(elem->get<float>("maxLogoCount"), 0.0f, 30.0f));
        }

        if (elem->has("logoRotation"))
            mItemRotation = elem->get<float>("logoRotation");
        if (elem->has("logoRotationOrigin"))
            mItemRotationOrigin = elem->get<glm::vec2>("logoRotationOrigin");

        if (elem->has("logoAlignment")) {
            const std::string alignment {elem->get<std::string>("logoAlignment")};
            if (alignment == "left" && mType != CarouselType::HORIZONTAL) {
                mItemHorizontalAlignment = ALIGN_LEFT;
                mItemVerticalAlignment = ALIGN_CENTER;
            }
            else if (alignment == "right" && mType != CarouselType::HORIZONTAL) {
                mItemHorizontalAlignment = ALIGN_RIGHT;
                mItemVerticalAlignment = ALIGN_CENTER;
            }
            else if (alignment == "top" && mType != CarouselType::VERTICAL) {
                mItemVerticalAlignment = ALIGN_TOP;
                mItemHorizontalAlignment = ALIGN_CENTER;
            }
            else if (alignment == "bottom" && mType != CarouselType::VERTICAL) {
                mItemVerticalAlignment = ALIGN_BOTTOM;
                mItemHorizontalAlignment = ALIGN_CENTER;
            }
            else if (alignment == "center") {
                mItemHorizontalAlignment = ALIGN_CENTER;
                mItemVerticalAlignment = ALIGN_CENTER;
            }
            else {
                LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                                   "\"logoAlignment\" for element \""
                                << element.substr(9) << "\" defined as \"" << alignment << "\"";
                mItemHorizontalAlignment = ALIGN_CENTER;
                mItemVerticalAlignment = ALIGN_CENTER;
            }
        }
    }

    // For non-legacy themes, scale the font size with the itemScale property value.
    mFont = Font::getFromTheme(elem, properties, mFont, 0.0f, mLegacyMode,
                               (mLegacyMode ? 1.0f : (mItemScale >= 1.0f ? mItemScale : 1.0f)));

    if (elem->has("textColor"))
        mTextColor = elem->get<unsigned int>("textColor");
    if (elem->has("textBackgroundColor"))
        mTextBackgroundColor = elem->get<unsigned int>("textBackgroundColor");

    if (elem->has("lineSpacing"))
        mLineSpacing = glm::clamp(elem->get<float>("lineSpacing"), 0.5f, 3.0f);

    std::string letterCase;
    bool hasText {!mGamelistView && elem->has("text")};

    if (elem->has("letterCase")) {
        letterCase = elem->get<std::string>("letterCase");

        if (letterCase == "uppercase") {
            mUppercase = true;
            if (hasText)
                mText = Utils::String::toUpper(elem->get<std::string>("text"));
        }
        else if (letterCase == "lowercase") {
            mLowercase = true;
            if (hasText)
                mText = Utils::String::toLower(elem->get<std::string>("text"));
        }
        else if (letterCase == "capitalize") {
            mCapitalize = true;
            if (hasText)
                mText = Utils::String::toCapitalized(elem->get<std::string>("text"));
        }
        else if (hasText && letterCase == "none") {
            mText = elem->get<std::string>("text");
        }
        else {
            LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                               "\"letterCase\" for element \""
                            << element.substr(9) << "\" defined as \"" << letterCase << "\"";
            if (hasText)
                mText = elem->get<std::string>("text");
        }
    }

    if (elem->has("fadeAbovePrimary"))
        mFadeAbovePrimary = elem->get<bool>("fadeAbovePrimary");

    GuiComponent::applyTheme(theme, view, element, ALL);

    mSize.x = glm::clamp(mSize.x, mRenderer->getScreenWidth() * 0.05f,
                         mRenderer->getScreenWidth() * 2.0f);
    mSize.y = glm::clamp(mSize.y, mRenderer->getScreenHeight() * 0.05f,
                         mRenderer->getScreenHeight() * 2.0f);
}

template <typename T> void CarouselComponent<T>::onCursorChanged(const CursorState& state)
{
    float startPos {mEntryCamOffset};
    float posMax {static_cast<float>(mEntries.size())};
    float target {static_cast<float>(mCursor)};

    // Find the shortest path to the target.
    float endPos {target}; // Directly.

    if (mPreviousScrollVelocity > 0 && mScrollVelocity == 0 && mEntryCamOffset > posMax - 1.0f)
        startPos = 0.0f;

    // If quick jumping to the start or end of the list using the trigger button when in
    // the gamelist view, then always animate in the requested direction.
    if (!mTriggerJump) {
        float dist {fabsf(endPos - startPos)};

        if (fabsf(target + posMax - startPos - mScrollVelocity) < dist)
            endPos = target + posMax; // Loop around the end (0 -> max).
        if (fabsf(target - posMax - startPos - mScrollVelocity) < dist)
            endPos = target - posMax; // Loop around the start (max - 1 -> -1).
    }

    // Make sure there are no reverse jumps between items.
    bool changedDirection {false};
    if (mPreviousScrollVelocity != 0 && mPreviousScrollVelocity != mScrollVelocity)
        changedDirection = true;

    if (!changedDirection && mScrollVelocity > 0 && endPos < startPos)
        endPos = endPos + posMax;

    if (!changedDirection && mScrollVelocity < 0 && endPos > startPos)
        endPos = endPos - posMax;

    if (mScrollVelocity != 0)
        mPreviousScrollVelocity = mScrollVelocity;

    // Needed to make sure that overlapping items are renderered correctly.
    if (startPos > endPos)
        mPositiveDirection = true;
    else
        mPositiveDirection = false;

    mEntryCamTarget = endPos;

    Animation* anim {new LambdaAnimation(
        [this, startPos, endPos, posMax](float t) {
            t -= 1;
            float f {glm::mix(startPos, endPos, t * t * t + 1)};
            if (f < 0)
                f += posMax;
            if (f >= posMax)
                f -= posMax;

            mEntryCamOffset = f;
        },
        500)};

    GuiComponent::setAnimation(anim, 0, nullptr, false, 0);

    if (mCursorChangedCallback && !mEntries.empty())
        mCursorChangedCallback(state);
}

#endif // ES_CORE_COMPONENTS_CAROUSEL_COMPONENT_H
