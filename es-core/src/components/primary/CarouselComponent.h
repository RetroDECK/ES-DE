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
    std::shared_ptr<GuiComponent> logo;
    std::string logoPath;
    std::string defaultLogoPath;
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
    Alignment mLogoHorizontalAlignment;
    Alignment mLogoVerticalAlignment;
    float mMaxLogoCount;
    glm::vec2 mLogoSize;
    float mLogoScale;
    float mLogoRotation;
    glm::vec2 mLogoRotationOrigin;
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
    , mLogoHorizontalAlignment {ALIGN_CENTER}
    , mLogoVerticalAlignment {ALIGN_CENTER}
    , mMaxLogoCount {3.0f}
    , mLogoSize {Renderer::getScreenWidth() * 0.25f, Renderer::getScreenHeight() * 0.155f}
    , mLogoScale {1.2f}
    , mLogoRotation {7.5f}
    , mLogoRotationOrigin {-3.0f, 0.5f}
    , mCarouselColor {0}
    , mCarouselColorEnd {0}
    , mColorGradientHorizontal {true}
{
}

template <typename T>
void CarouselComponent<T>::addEntry(Entry& entry, const std::shared_ptr<ThemeData>& theme)
{
    bool legacyMode {theme->isLegacyTheme()};

    // Make logo.
    if (legacyMode) {
        const ThemeData::ThemeElement* logoElem {
            theme->getElement("system", "image_logo", "image")};

        if (logoElem) {
            std::string path;
            if (logoElem->has("path"))
                path = logoElem->get<std::string>("path");
            std::string defaultPath {
                logoElem->has("default") ? logoElem->get<std::string>("default") : ""};
            if ((!path.empty() && ResourceManager::getInstance().fileExists(path)) ||
                (!defaultPath.empty() && ResourceManager::getInstance().fileExists(defaultPath))) {
                auto logo = std::make_shared<ImageComponent>(false, false);
                logo->setLinearInterpolation(true);
                logo->setMaxSize(glm::round(mLogoSize * mLogoScale));
                logo->applyTheme(theme, "system", "image_logo",
                                 ThemeFlags::PATH | ThemeFlags::COLOR);
                logo->setRotateByTargetSize(true);
                entry.data.logo = logo;
            }
        }
    }
    else {
        if (entry.data.logoPath != "" &&
            ResourceManager::getInstance().fileExists(entry.data.logoPath)) {
            auto logo = std::make_shared<ImageComponent>(false, false);
            logo->setLinearInterpolation(true);
            logo->setImage(entry.data.logoPath);
            logo->setMaxSize(glm::round(mLogoSize * mLogoScale));
            logo->applyTheme(theme, "system", "", ThemeFlags::ALL);
            logo->setRotateByTargetSize(true);
            entry.data.logo = logo;
        }
        else if (entry.data.defaultLogoPath != "" &&
                 ResourceManager::getInstance().fileExists(entry.data.defaultLogoPath)) {
            auto defaultLogo = std::make_shared<ImageComponent>(false, false);
            defaultLogo->setLinearInterpolation(true);
            defaultLogo->setImage(entry.data.defaultLogoPath);
            defaultLogo->setMaxSize(glm::round(mLogoSize * mLogoScale));
            defaultLogo->applyTheme(theme, "system", "", ThemeFlags::ALL);
            defaultLogo->setRotateByTargetSize(true);
            entry.data.logo = defaultLogo;
        }
    }

    if (!entry.data.logo) {
        // If no logo image is present, add logo text as fallback.
        auto text = std::make_shared<TextComponent>(entry.name, mFont, 0x000000FF, ALIGN_CENTER);
        text->setSize(mLogoSize * mLogoScale);
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
        entry.data.logo = text;

        text->setHorizontalAlignment(mLogoHorizontalAlignment);
        text->setVerticalAlignment(mLogoVerticalAlignment);
    }

    // Set origin for the logos based on their alignment so they line up properly.
    if (mLogoHorizontalAlignment == ALIGN_LEFT)
        entry.data.logo->setOrigin(0, 0.5);
    else if (mLogoHorizontalAlignment == ALIGN_RIGHT)
        entry.data.logo->setOrigin(1.0, 0.5);
    else
        entry.data.logo->setOrigin(0.5, 0.5);

    if (mLogoVerticalAlignment == ALIGN_TOP)
        entry.data.logo->setOrigin(entry.data.logo->getOrigin().x, 0);
    else if (mLogoVerticalAlignment == ALIGN_BOTTOM)
        entry.data.logo->setOrigin(entry.data.logo->getOrigin().x, 1);
    else
        entry.data.logo->setOrigin(entry.data.logo->getOrigin().x, 0.5);

    glm::vec2 denormalized {mLogoSize * entry.data.logo->getOrigin()};
    entry.data.logo->setPosition(glm::vec3 {denormalized.x, denormalized.y, 0.0f});

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

    // Background box behind logos.
    mRenderer->drawRect(0.0f, 0.0f, mSize.x, mSize.y, mCarouselColor, mCarouselColorEnd,
                        mColorGradientHorizontal);

    // Draw logos.
    // logoSpacing will also include the size of the logo itself.
    glm::vec2 logoSpacing {};
    float xOff {0.0f};
    float yOff {0.0f};

    switch (mType) {
        case CarouselType::HORIZONTAL_WHEEL:
        case CarouselType::VERTICAL_WHEEL:
            xOff = std::round((mSize.x - mLogoSize.x) / 2.0f - (mEntryCamOffset * logoSpacing.y));
            yOff = (mSize.y - mLogoSize.y) / 2.0f;
            break;
        case CarouselType::VERTICAL:
            logoSpacing.y =
                ((mSize.y - (mLogoSize.y * mMaxLogoCount)) / (mMaxLogoCount)) + mLogoSize.y;
            yOff = (mSize.y - mLogoSize.y) / 2.0f - (mEntryCamOffset * logoSpacing.y);
            if (mLogoHorizontalAlignment == ALIGN_LEFT)
                xOff = mLogoSize.x / 10.0f;
            else if (mLogoHorizontalAlignment == ALIGN_RIGHT)
                xOff = mSize.x - (mLogoSize.x * 1.1f);
            else
                xOff = (mSize.x - mLogoSize.x) / 2.0f;
            break;
        case CarouselType::HORIZONTAL:
        default:
            logoSpacing.x =
                ((mSize.x - (mLogoSize.x * mMaxLogoCount)) / (mMaxLogoCount)) + mLogoSize.x;
            xOff = std::round((mSize.x - mLogoSize.x) / 2.0f - (mEntryCamOffset * logoSpacing.x));
            if (mLogoVerticalAlignment == ALIGN_TOP)
                yOff = mLogoSize.y / 10.0f;
            else if (mLogoVerticalAlignment == ALIGN_BOTTOM)
                yOff = mSize.y - (mLogoSize.y * 1.1f);
            else
                yOff = (mSize.y - mLogoSize.y) / 2.0f;
            break;
    }

    int center {static_cast<int>(mEntryCamOffset)};
    int logoInclusion {static_cast<int>(std::ceil(mMaxLogoCount / 2.0f))};
    bool singleEntry {mEntries.size() == 1};

    for (int i = center - logoInclusion; i < center + logoInclusion + 2; ++i) {
        int index {i};

        // If there is only a single system, then only render the logo once (in the center).
        if (singleEntry) {
            mEntries.at(0).data.logo->render(
                glm::translate(carouselTrans, glm::vec3 {0 + xOff, 0 + yOff, 0.0f}));
            break;
        }

        while (index < 0)
            index += static_cast<int>(mEntries.size());
        while (index >= static_cast<int>(mEntries.size()))
            index -= static_cast<int>(mEntries.size());

        glm::mat4 logoTrans {carouselTrans};
        logoTrans = glm::translate(
            logoTrans, glm::vec3 {i * logoSpacing.x + xOff, i * logoSpacing.y + yOff, 0.0f});

        float distance {i - mEntryCamOffset};

        float scale {1.0f + ((mLogoScale - 1.0f) * (1.0f - fabsf(distance)))};
        scale = std::min(mLogoScale, std::max(1.0f, scale));
        scale /= mLogoScale;

        int opacity {
            static_cast<int>(std::round(0x80 + ((0xFF - 0x80) * (1.0f - fabsf(distance)))))};
        opacity = std::max(static_cast<int>(0x80), opacity);

        const std::shared_ptr<GuiComponent>& comp {mEntries.at(index).data.logo};

        if (comp == nullptr)
            continue;

        if (mType == CarouselType::VERTICAL_WHEEL || mType == CarouselType::HORIZONTAL_WHEEL) {
            comp->setRotationDegrees(mLogoRotation * distance);
            comp->setRotationOrigin(mLogoRotationOrigin);
        }

        // When running at lower resolutions, prevent the scale-down to go all the way to
        // the minimum value. This avoids potential single-pixel alignment issues when the
        // logo can't be vertically placed exactly in the middle of the carousel. Although
        // the problem theoretically exists at all resolutions, it's not visble at around
        // 1080p and above.
        if (std::min(Renderer::getScreenWidth(), Renderer::getScreenHeight()) < 1080.0f)
            scale = glm::clamp(scale, 1.0f / mLogoScale + 0.01f, 1.0f);

        comp->setScale(scale);
        comp->setOpacity(static_cast<float>(opacity) / 255.0f);
        comp->render(logoTrans);
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
            mLogoScale = glm::clamp(elem->get<float>("itemScale"), 0.5f, 3.0f);
        if (elem->has("itemSize")) {
            // Keep size within a 0.05 and 1.0 multiple of the screen size.
            glm::vec2 logoSize {elem->get<glm::vec2>("itemSize")};
            if (std::max(logoSize.x, logoSize.y) > 1.0f) {
                logoSize /= std::max(logoSize.x, logoSize.y);
            }
            else if (std::min(logoSize.x, logoSize.y) < 0.005f) {
                float ratio {std::min(logoSize.x, logoSize.y) / 0.005f};
                logoSize /= ratio;
                // Just an extra precaution if a crazy ratio was used.
                logoSize.x = glm::clamp(logoSize.x, 0.005f, 1.0f);
                logoSize.y = glm::clamp(logoSize.y, 0.005f, 1.0f);
            }
            mLogoSize =
                logoSize * glm::vec2(Renderer::getScreenWidth(), Renderer::getScreenHeight());
        }

        if (elem->has("maxItemCount"))
            mMaxLogoCount = glm::clamp(elem->get<float>("maxItemCount"), 0.5f, 30.0f);

        if (elem->has("itemRotation"))
            mLogoRotation = elem->get<float>("itemRotation");
        if (elem->has("itemRotationOrigin"))
            mLogoRotationOrigin = elem->get<glm::vec2>("itemRotationOrigin");

        if (elem->has("itemHorizontalAlignment")) {
            const std::string alignment {elem->get<std::string>("itemHorizontalAlignment")};
            if (alignment == "left" && mType != CarouselType::HORIZONTAL) {
                mLogoHorizontalAlignment = ALIGN_LEFT;
            }
            else if (alignment == "right" && mType != CarouselType::HORIZONTAL) {
                mLogoHorizontalAlignment = ALIGN_RIGHT;
            }
            else if (alignment == "center") {
                mLogoHorizontalAlignment = ALIGN_CENTER;
            }
            else {
                LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                                   "<itemHorizontalAlignment> defined as \""
                                << alignment << "\"";
                mLogoHorizontalAlignment = ALIGN_CENTER;
            }
        }

        if (elem->has("itemVerticalAlignment")) {
            const std::string alignment {elem->get<std::string>("itemVerticalAlignment")};
            if (alignment == "top" && mType != CarouselType::VERTICAL) {
                mLogoVerticalAlignment = ALIGN_TOP;
            }
            else if (alignment == "bottom" && mType != CarouselType::VERTICAL) {
                mLogoVerticalAlignment = ALIGN_BOTTOM;
            }
            else if (alignment == "center") {
                mLogoVerticalAlignment = ALIGN_CENTER;
            }
            else {
                LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                                   "<itemVerticalAlignment> defined as \""
                                << alignment << "\"";
                mLogoVerticalAlignment = ALIGN_CENTER;
            }
        }
    }

    // Start of legacy themes only section.

    if (elem->has("logoScale"))
        mLogoScale = glm::clamp(elem->get<float>("logoScale"), 0.5f, 3.0f);
    if (elem->has("logoSize")) {
        // Keep size within a 0.05 and 1.0 multiple of the screen size.
        glm::vec2 logoSize {elem->get<glm::vec2>("logoSize")};
        if (std::max(logoSize.x, logoSize.y) > 1.0f) {
            logoSize /= std::max(logoSize.x, logoSize.y);
        }
        else if (std::min(logoSize.x, logoSize.y) < 0.005f) {
            float ratio {std::min(logoSize.x, logoSize.y) / 0.005f};
            logoSize /= ratio;
            // Just an extra precaution if a crazy ratio was used.
            logoSize.x = glm::clamp(logoSize.x, 0.005f, 1.0f);
            logoSize.y = glm::clamp(logoSize.y, 0.005f, 1.0f);
        }
        mLogoSize = logoSize * glm::vec2(Renderer::getScreenWidth(), Renderer::getScreenHeight());
    }

    if (elem->has("maxLogoCount")) {
        if (theme->isLegacyTheme())
            mMaxLogoCount = std::ceil(glm::clamp(elem->get<float>("maxLogoCount"), 0.5f, 30.0f));
        else
            mMaxLogoCount = glm::clamp(elem->get<float>("maxLogoCount"), 0.5f, 30.0f);
    }

    if (elem->has("logoRotation"))
        mLogoRotation = elem->get<float>("logoRotation");
    if (elem->has("logoRotationOrigin"))
        mLogoRotationOrigin = elem->get<glm::vec2>("logoRotationOrigin");

    if (elem->has("logoAlignment")) {
        const std::string alignment {elem->get<std::string>("logoAlignment")};
        if (alignment == "left" && mType != CarouselType::HORIZONTAL) {
            mLogoHorizontalAlignment = ALIGN_LEFT;
            mLogoVerticalAlignment = ALIGN_CENTER;
        }
        else if (alignment == "right" && mType != CarouselType::HORIZONTAL) {
            mLogoHorizontalAlignment = ALIGN_RIGHT;
            mLogoVerticalAlignment = ALIGN_CENTER;
        }
        else if (alignment == "top" && mType != CarouselType::VERTICAL) {
            mLogoVerticalAlignment = ALIGN_TOP;
            mLogoHorizontalAlignment = ALIGN_CENTER;
        }
        else if (alignment == "bottom" && mType != CarouselType::VERTICAL) {
            mLogoVerticalAlignment = ALIGN_BOTTOM;
            mLogoHorizontalAlignment = ALIGN_CENTER;
        }
        else if (alignment == "center") {
            mLogoHorizontalAlignment = ALIGN_CENTER;
            mLogoVerticalAlignment = ALIGN_CENTER;
        }
        else {
            LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                               "<logoAlignment> defined as \""
                            << alignment << "\"";
            mLogoHorizontalAlignment = ALIGN_CENTER;
            mLogoVerticalAlignment = ALIGN_CENTER;
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

    // Make sure there are no reverse jumps between logos.
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
