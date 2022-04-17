//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  CarouselComponent.h
//
//  Carousel.
//

#ifndef ES_CORE_COMPONENTS_CAROUSEL_COMPONENT_H
#define ES_CORE_COMPONENTS_CAROUSEL_COMPONENT_H

#include "Log.h"
#include "Sound.h"
#include "animations/LambdaAnimation.h"
#include "components/IList.h"
#include "components/primary/PrimaryComponent.h"
#include "resources/Font.h"

struct CarouselElement {
    std::shared_ptr<GuiComponent> item;
    std::string itemPath;
    std::string defaultItemPath;
};

template <typename T>
class CarouselComponent : public PrimaryComponent<T>, protected IList<CarouselElement, T>
{
    using List = IList<CarouselElement, T>;

protected:
    using List::mCursor;
    using List::mEntries;
    using List::mScrollVelocity;
    using List::mSize;
    using List::mWindow;

    using GuiComponent::mDefaultZIndex;
    using GuiComponent::mOrigin;
    using GuiComponent::mPosition;
    using GuiComponent::mZIndex;

public:
    using Entry = typename IList<CarouselElement, T>::Entry;

    enum class CarouselType {
        HORIZONTAL, // Replace with AllowShortEnumsOnASingleLine: false (clang-format >=11.0).
        VERTICAL,
        VERTICAL_WHEEL,
        HORIZONTAL_WHEEL,
        NO_CAROUSEL
    };

    CarouselComponent();

    void addEntry(Entry& entry, const std::shared_ptr<ThemeData>& theme = nullptr);
    Entry& getEntry(int index) { return mEntries.at(index); }
    const CarouselType getType() { return mType; }
    const std::string& getItemType() { return mItemType; }
    void setItemType(std::string itemType) { mItemType = itemType; }
    const std::string& getDefaultItem() { return mDefaultItem; }
    void setDefaultItem(std::string defaultItem) { mDefaultItem = defaultItem; }

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

    bool isScrolling() const override { return List::isScrolling(); }
    void stopScrolling() override
    {
        List::stopScrolling();
        // Only finish the animation if we're in the gamelist view.
        if constexpr (std::is_same_v<T, FileData*>)
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

    Renderer* mRenderer;
    std::function<void(CursorState state)> mCursorChangedCallback;
    std::function<void()> mCancelTransitionsCallback;

    float mEntryCamOffset;
    int mPreviousScrollVelocity;
    bool mTriggerJump;

    CarouselType mType;
    std::string mItemType;
    std::string mDefaultItem;
    std::shared_ptr<Font> mFont;
    unsigned int mTextColor;
    unsigned int mTextBackgroundColor;
    std::string mText;
    float mLineSpacing;
    Alignment mItemHorizontalAlignment;
    Alignment mItemVerticalAlignment;
    float mMaxItemCount;
    glm::vec2 mItemSize;
    float mItemScale;
    float mItemRotation;
    glm::vec2 mItemRotationOrigin;
    unsigned int mCarouselColor;
    unsigned int mCarouselColorEnd;
    bool mColorGradientHorizontal;
};

template <typename T>
CarouselComponent<T>::CarouselComponent()
    : IList<CarouselElement, T> {LIST_SCROLL_STYLE_SLOW,
                                 (std::is_same_v<T, SystemData*> ?
                                      ListLoopType::LIST_ALWAYS_LOOP :
                                      ListLoopType::LIST_PAUSE_AT_END_ON_JUMP)}
    , mRenderer {Renderer::getInstance()}
    , mEntryCamOffset {0.0f}
    , mPreviousScrollVelocity {0}
    , mTriggerJump {false}
    , mType {CarouselType::HORIZONTAL}
    , mFont {Font::get(FONT_SIZE_LARGE)}
    , mTextColor {0x000000FF}
    , mTextBackgroundColor {0xFFFFFF00}
    , mLineSpacing {1.5f}
    , mItemHorizontalAlignment {ALIGN_CENTER}
    , mItemVerticalAlignment {ALIGN_CENTER}
    , mMaxItemCount {3.0f}
    , mItemSize {Renderer::getScreenWidth() * 0.25f, Renderer::getScreenHeight() * 0.155f}
    , mItemScale {1.2f}
    , mItemRotation {7.5f}
    , mItemRotationOrigin {-3.0f, 0.5f}
    , mCarouselColor {0}
    , mCarouselColorEnd {0}
    , mColorGradientHorizontal {true}
{
}

template <typename T>
void CarouselComponent<T>::addEntry(Entry& entry, const std::shared_ptr<ThemeData>& theme)
{
    bool legacyMode {theme->isLegacyTheme()};

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
                auto item = std::make_shared<ImageComponent>(false, false);
                item->setLinearInterpolation(true);
                item->setMaxSize(glm::round(mItemSize * mItemScale));
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
            auto item = std::make_shared<ImageComponent>(false, false);
            item->setLinearInterpolation(true);
            item->setImage(entry.data.itemPath);
            item->setMaxSize(glm::round(mItemSize * mItemScale));
            item->applyTheme(theme, "system", "", ThemeFlags::ALL);
            item->setRotateByTargetSize(true);
            entry.data.item = item;
        }
        else if (entry.data.defaultItemPath != "" &&
                 ResourceManager::getInstance().fileExists(entry.data.defaultItemPath)) {
            auto defaultItem = std::make_shared<ImageComponent>(false, false);
            defaultItem->setLinearInterpolation(true);
            defaultItem->setImage(entry.data.defaultItemPath);
            defaultItem->setMaxSize(glm::round(mItemSize * mItemScale));
            defaultItem->applyTheme(theme, "system", "", ThemeFlags::ALL);
            defaultItem->setRotateByTargetSize(true);
            entry.data.item = defaultItem;
        }
    }

    if (!entry.data.item) {
        // If no item image is present, add item text as fallback.
        auto text = std::make_shared<TextComponent>(entry.name, mFont, 0x000000FF, ALIGN_CENTER);
        text->setSize(mItemSize * mItemScale);
        if (legacyMode) {
            text->applyTheme(theme, "system", "text_logoText",
                             ThemeFlags::FONT_PATH | ThemeFlags::FONT_SIZE | ThemeFlags::COLOR |
                                 ThemeFlags::LETTER_CASE | ThemeFlags::FORCE_UPPERCASE |
                                 ThemeFlags::LINE_SPACING | ThemeFlags::TEXT);
        }
        if (!legacyMode) {
            text->setLineSpacing(mLineSpacing);
            if constexpr (std::is_same_v<T, SystemData*>) {
                if (mText != "")
                    text->setValue(mText);
            }
            text->setColor(mTextColor);
            text->setBackgroundColor(mTextBackgroundColor);
            text->setRenderBackground(true);
        }
        entry.data.item = text;

        text->setHorizontalAlignment(mItemHorizontalAlignment);
        text->setVerticalAlignment(mItemVerticalAlignment);
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

    glm::vec2 denormalized {mItemSize * entry.data.item->getOrigin()};
    entry.data.item->setPosition(glm::vec3 {denormalized.x, denormalized.y, 0.0f});

    List::add(entry);
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
        if constexpr (std::is_same_v<T, FileData*>) {
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
        if constexpr (std::is_same_v<T, FileData*>) {
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
        if constexpr (std::is_same_v<T, SystemData*>) {
            if (config->isMappedLike("up", input) || config->isMappedLike("down", input) ||
                config->isMappedLike("left", input) || config->isMappedLike("right", input))
                List::listInput(0);
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
    if (mEntries.size() == 0)
        return;

    glm::mat4 carouselTrans {parentTrans};
    carouselTrans = glm::translate(carouselTrans, glm::vec3 {mPosition.x, mPosition.y, 0.0f});
    carouselTrans = glm::translate(
        carouselTrans, glm::vec3 {mOrigin.x * mSize.x * -1.0f, mOrigin.y * mSize.y * -1.0f, 0.0f});

    mRenderer->pushClipRect(
        glm::ivec2 {static_cast<int>(glm::clamp(std::round(carouselTrans[3].x), 0.0f,
                                                mRenderer->getScreenWidth())),
                    static_cast<int>(glm::clamp(std::round(carouselTrans[3].y), 0.0f,
                                                mRenderer->getScreenHeight()))},
        glm::ivec2 {static_cast<int>(std::min(std::round(mSize.x), mRenderer->getScreenWidth())),
                    static_cast<int>(std::min(std::round(mSize.y), mRenderer->getScreenHeight()))});

    mRenderer->setMatrix(carouselTrans);

    // In image debug mode, draw a green rectangle covering the entire carousel area.
    if (Settings::getInstance()->getBool("DebugImage"))
        mRenderer->drawRect(0.0f, 0.0f, mSize.x, mSize.y, 0x00FF0033, 0x00FF0033);

    // Background box behind the items.
    mRenderer->drawRect(0.0f, 0.0f, mSize.x, mSize.y, mCarouselColor, mCarouselColorEnd,
                        mColorGradientHorizontal);

    // Draw the items.
    // itemSpacing will also include the size of the item itself.
    glm::vec2 itemSpacing {0.0f, 0.0f};
    float xOff {0.0f};
    float yOff {0.0f};

    switch (mType) {
        case CarouselType::HORIZONTAL_WHEEL:
        case CarouselType::VERTICAL_WHEEL:
            xOff = std::round((mSize.x - mItemSize.x) / 2.0f - (mEntryCamOffset * itemSpacing.y));
            yOff = (mSize.y - mItemSize.y) / 2.0f;
            break;
        case CarouselType::VERTICAL:
            itemSpacing.y =
                ((mSize.y - (mItemSize.y * mMaxItemCount)) / (mMaxItemCount)) + mItemSize.y;
            yOff = (mSize.y - mItemSize.y) / 2.0f - (mEntryCamOffset * itemSpacing.y);
            if (mItemHorizontalAlignment == ALIGN_LEFT)
                xOff = mItemSize.x / 10.0f;
            else if (mItemHorizontalAlignment == ALIGN_RIGHT)
                xOff = mSize.x - (mItemSize.x * 1.1f);
            else
                xOff = (mSize.x - mItemSize.x) / 2.0f;
            break;
        case CarouselType::HORIZONTAL:
        default:
            itemSpacing.x =
                ((mSize.x - (mItemSize.x * mMaxItemCount)) / (mMaxItemCount)) + mItemSize.x;
            xOff = std::round((mSize.x - mItemSize.x) / 2.0f - (mEntryCamOffset * itemSpacing.x));
            if (mItemVerticalAlignment == ALIGN_TOP)
                yOff = mItemSize.y / 10.0f;
            else if (mItemVerticalAlignment == ALIGN_BOTTOM)
                yOff = mSize.y - (mItemSize.y * 1.1f);
            else
                yOff = (mSize.y - mItemSize.y) / 2.0f;
            break;
    }

    int center {static_cast<int>(mEntryCamOffset)};
    int itemInclusion {static_cast<int>(std::ceil(mMaxItemCount / 2.0f))};
    bool singleEntry {mEntries.size() == 1};

    for (int i = center - itemInclusion; i < center + itemInclusion + 2; ++i) {
        int index {i};

        // If there is only a single entry, then only render the item once (in the center).
        if (singleEntry) {
            mEntries.at(0).data.item->render(
                glm::translate(carouselTrans, glm::vec3 {0 + xOff, 0 + yOff, 0.0f}));
            break;
        }

        while (index < 0)
            index += static_cast<int>(mEntries.size());
        while (index >= static_cast<int>(mEntries.size()))
            index -= static_cast<int>(mEntries.size());

        glm::mat4 itemTrans {carouselTrans};
        itemTrans = glm::translate(
            itemTrans, glm::vec3 {i * itemSpacing.x + xOff, i * itemSpacing.y + yOff, 0.0f});

        float distance {i - mEntryCamOffset};

        float scale {1.0f + ((mItemScale - 1.0f) * (1.0f - fabsf(distance)))};
        scale = std::min(mItemScale, std::max(1.0f, scale));
        scale /= mItemScale;

        int opacity {
            static_cast<int>(std::round(0x80 + ((0xFF - 0x80) * (1.0f - fabsf(distance)))))};
        opacity = std::max(static_cast<int>(0x80), opacity);

        const std::shared_ptr<GuiComponent>& comp {mEntries.at(index).data.item};

        if (comp == nullptr)
            continue;

        if (mType == CarouselType::VERTICAL_WHEEL || mType == CarouselType::HORIZONTAL_WHEEL) {
            comp->setRotationDegrees(mItemRotation * distance);
            comp->setRotationOrigin(mItemRotationOrigin);
        }

        // When running at lower resolutions, prevent the scale-down to go all the way to
        // the minimum value. This avoids potential single-pixel alignment issues when the
        // item can't be vertically placed exactly in the middle of the carousel. Although
        // the problem theoretically exists at all resolutions, it's not visble at around
        // 1080p and above.
        if (std::min(Renderer::getScreenWidth(), Renderer::getScreenHeight()) < 1080.0f)
            scale = glm::clamp(scale, 1.0f / mItemScale + 0.01f, 1.0f);

        comp->setScale(scale);
        comp->setOpacity(static_cast<float>(opacity) / 255.0f);
        comp->render(itemTrans);
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
    mSize.y = Renderer::getScreenHeight() * 0.2325f;
    mPosition.x = 0.0f;
    mPosition.y = floorf(0.5f * (Renderer::getScreenHeight() - mSize.y));
    mCarouselColor = 0xFFFFFFD8;
    mCarouselColorEnd = 0xFFFFFFD8;
    mZIndex = mDefaultZIndex;
    mText = "";

    if (!elem)
        return;

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
            LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                               "<type> defined as \""
                            << type << "\"";
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
                               "<gradientType> defined as \""
                            << gradientType << "\"";
        }
    }

    if (!theme->isLegacyTheme()) {
        if (elem->has("itemScale"))
            mItemScale = glm::clamp(elem->get<float>("itemScale"), 0.5f, 3.0f);
        if (elem->has("itemSize")) {
            // Keep size within a 0.05 and 1.0 multiple of the screen size.
            glm::vec2 itemSize {elem->get<glm::vec2>("itemSize")};
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

        if (elem->has("maxItemCount"))
            mMaxItemCount = glm::clamp(elem->get<float>("maxItemCount"), 0.5f, 30.0f);

        if (elem->has("itemRotation"))
            mItemRotation = elem->get<float>("itemRotation");
        if (elem->has("itemRotationOrigin"))
            mItemRotationOrigin = elem->get<glm::vec2>("itemRotationOrigin");

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
                                   "<itemHorizontalAlignment> defined as \""
                                << alignment << "\"";
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
                                   "<itemVerticalAlignment> defined as \""
                                << alignment << "\"";
                mItemVerticalAlignment = ALIGN_CENTER;
            }
        }
    }

    // Start of legacy themes only section.

    if (elem->has("logoScale"))
        mItemScale = glm::clamp(elem->get<float>("logoScale"), 0.5f, 3.0f);
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
        mItemSize = itemSize * glm::vec2(Renderer::getScreenWidth(), Renderer::getScreenHeight());
    }

    if (elem->has("maxLogoCount")) {
        if (theme->isLegacyTheme())
            mMaxItemCount = std::ceil(glm::clamp(elem->get<float>("maxLogoCount"), 0.5f, 30.0f));
        else
            mMaxItemCount = glm::clamp(elem->get<float>("maxLogoCount"), 0.5f, 30.0f);
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
                               "<logoAlignment> defined as \""
                            << alignment << "\"";
            mItemHorizontalAlignment = ALIGN_CENTER;
            mItemVerticalAlignment = ALIGN_CENTER;
        }
    }

    // End of legacy theme section.

    mFont = Font::getFromTheme(elem, properties, mFont);

    if (elem->has("textColor"))
        mTextColor = elem->get<unsigned int>("textColor");
    if (elem->has("textBackgroundColor"))
        mTextBackgroundColor = elem->get<unsigned int>("textBackgroundColor");

    if (elem->has("lineSpacing"))
        mLineSpacing = glm::clamp(elem->get<float>("lineSpacing"), 0.5f, 3.0f);

    std::string letterCase;

    if (elem->has("letterCase"))
        letterCase = elem->get<std::string>("letterCase");

    if (elem->has("text")) {
        if (letterCase == "uppercase") {
            mText = Utils::String::toUpper(elem->get<std::string>("text"));
        }
        else if (letterCase == "lowercase") {
            mText = Utils::String::toLower(elem->get<std::string>("text"));
        }
        else if (letterCase == "capitalize") {
            mText = Utils::String::toCapitalized(elem->get<std::string>("text"));
        }
        else if (letterCase == "none") {
            mText = elem->get<std::string>("text");
        }
        else {
            LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                               "<letterCase> defined as \""
                            << letterCase << "\"";
            mText = elem->get<std::string>("text");
        }
    }

    GuiComponent::applyTheme(theme, view, element, ALL);

    mSize.x = glm::clamp(mSize.x, mRenderer->getScreenWidth() * 0.05f,
                         mRenderer->getScreenWidth() * 1.5f);
    mSize.y = glm::clamp(mSize.y, mRenderer->getScreenHeight() * 0.05f,
                         mRenderer->getScreenHeight() * 1.5f);
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

    // No need to animate transition, we're not going anywhere.
    if (endPos == mEntryCamOffset)
        return;

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

    if (mCursorChangedCallback)
        mCursorChangedCallback(state);
}

#endif // ES_CORE_COMPONENTS_CAROUSEL_COMPONENT_H
