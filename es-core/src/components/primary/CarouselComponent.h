//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  CarouselComponent.h
//
//  Carousel, usable in both the system and gamelist views.
//

#ifndef ES_CORE_COMPONENTS_PRIMARY_CAROUSEL_COMPONENT_H
#define ES_CORE_COMPONENTS_PRIMARY_CAROUSEL_COMPONENT_H

#include "Sound.h"
#include "animations/LambdaAnimation.h"
#include "components/IList.h"
#include "components/primary/PrimaryComponent.h"
#include "resources/Font.h"

struct CarouselEntry {
    std::shared_ptr<GuiComponent> item;
    std::string imagePath;
    std::string defaultImagePath;
};

template <typename T>
class CarouselComponent : public PrimaryComponent<T>, protected IList<CarouselEntry, T>
{
protected:
    using List = IList<CarouselEntry, T>;
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

    enum class ItemStacking {
        CENTERED,
        ASCENDING,
        ASCENDING_RAISED,
        DESCENDING,
        DESCENDING_RAISED
    };

    CarouselComponent();

    void addEntry(Entry& entry, const std::shared_ptr<ThemeData>& theme);
    void updateEntry(Entry& entry, const std::shared_ptr<ThemeData>& theme);
    Entry& getEntry(int index) { return mEntries.at(index); }
    void onDemandTextureLoad() override;
    const CarouselType getType() { return mType; }
    const std::string& getDefaultCarouselImage() const { return mDefaultImagePath; }
    const std::string& getDefaultCarouselFolderImage() const { return mDefaultFolderImagePath; }
    void setDefaultImage(std::string defaultImage) { mDefaultImagePath = defaultImage; }
    void setDefaultFolderImage(std::string defaultImage) { mDefaultFolderImagePath = defaultImage; }
    bool isScrolling() const override { return List::isScrolling(); }
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

    void setCancelTransitionsCallback(const std::function<void()>& func) override
    {
        mCancelTransitionsCallback = func;
    }
    void setCursorChangedCallback(const std::function<void(CursorState state)>& func) override
    {
        mCursorChangedCallback = func;
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
        if (mGamelistView)
            NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
        else
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

    enum class ImageFit {
        CONTAIN,
        FILL,
        COVER
    };

    Renderer* mRenderer;
    std::function<void(CursorState state)> mCursorChangedCallback;
    std::function<void()> mCancelTransitionsCallback;

    float mEntryCamOffset;
    float mEntryCamTarget;
    int mPreviousScrollVelocity;
    bool mPositiveDirection;
    bool mTriggerJump;
    bool mGamelistView;
    bool mLegacyMode;

    CarouselType mType;
    std::vector<std::string> mImageTypes;
    std::string mDefaultImagePath;
    std::string mDefaultFolderImagePath;
    std::shared_ptr<ImageComponent> mDefaultImage;
    float mMaxItemCount;
    int mItemsBeforeCenter;
    int mItemsAfterCenter;
    ItemStacking mItemStacking;
    glm::vec2 mSelectedItemMargins;
    glm::vec2 mItemSize;
    float mItemScale;
    float mItemRotation;
    glm::vec2 mItemRotationOrigin;
    bool mItemAxisHorizontal;
    float mItemAxisRotation;
    bool mLinearInterpolation;
    unsigned int mImageColorShift;
    unsigned int mImageColorShiftEnd;
    bool mImageColorGradientHorizontal;
    unsigned int mImageSelectedColor;
    unsigned int mImageSelectedColorEnd;
    bool mImageSelectedColorGradientHorizontal;
    bool mHasImageSelectedColor;
    float mImageBrightness;
    float mImageSaturation;
    float mItemDiagonalOffset;
    bool mInstantItemTransitions;
    Alignment mItemHorizontalAlignment;
    Alignment mItemVerticalAlignment;
    Alignment mWheelHorizontalAlignment;
    Alignment mWheelVerticalAlignment;
    float mHorizontalOffset;
    float mVerticalOffset;
    bool mReflections;
    float mReflectionsOpacity;
    float mReflectionsFalloff;
    float mUnfocusedItemOpacity;
    float mUnfocusedItemSaturation;
    bool mHasUnfocusedItemSaturation;
    float mUnfocusedItemDimming;
    ImageFit mImagefit;
    unsigned int mCarouselColor;
    unsigned int mCarouselColorEnd;
    bool mColorGradientHorizontal;
    unsigned int mTextColor;
    unsigned int mTextBackgroundColor;
    unsigned int mTextSelectedColor;
    unsigned int mTextSelectedBackgroundColor;
    bool mHasTextSelectedColor;
    std::shared_ptr<Font> mFont;
    LetterCase mLetterCase;
    LetterCase mLetterCaseAutoCollections;
    LetterCase mLetterCaseCustomCollections;
    float mLineSpacing;
    bool mSystemNameSuffix;
    LetterCase mLetterCaseSystemNameSuffix;
    bool mFadeAbovePrimary;
};

template <typename T>
CarouselComponent<T>::CarouselComponent()
    : IList<CarouselEntry, T> {IList<CarouselEntry, T>::LIST_SCROLL_STYLE_SLOW,
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
    , mLegacyMode {false}
    , mType {CarouselType::HORIZONTAL}
    , mMaxItemCount {3.0f}
    , mItemsBeforeCenter {8}
    , mItemsAfterCenter {8}
    , mItemStacking {ItemStacking::CENTERED}
    , mSelectedItemMargins {0.0f, 0.0f}
    , mItemSize {glm::vec2 {Renderer::getScreenWidth() * 0.25f,
                            Renderer::getScreenHeight() * 0.155f}}
    , mItemScale {1.2f}
    , mItemRotation {7.5f}
    , mItemRotationOrigin {-3.0f, 0.5f}
    , mItemAxisHorizontal {false}
    , mItemAxisRotation {0.0f}
    , mLinearInterpolation {false}
    , mImageColorShift {0xFFFFFFFF}
    , mImageColorShiftEnd {0xFFFFFFFF}
    , mImageColorGradientHorizontal {true}
    , mImageSelectedColor {0xFFFFFFFF}
    , mImageSelectedColorEnd {0xFFFFFFFF}
    , mImageSelectedColorGradientHorizontal {true}
    , mHasImageSelectedColor {false}
    , mImageBrightness {0.0f}
    , mImageSaturation {1.0f}
    , mItemDiagonalOffset {0.0f}
    , mInstantItemTransitions {false}
    , mItemHorizontalAlignment {ALIGN_CENTER}
    , mItemVerticalAlignment {ALIGN_CENTER}
    , mWheelHorizontalAlignment {ALIGN_CENTER}
    , mWheelVerticalAlignment {ALIGN_CENTER}
    , mHorizontalOffset {0.0f}
    , mVerticalOffset {0.0f}
    , mReflections {false}
    , mReflectionsOpacity {0.5f}
    , mReflectionsFalloff {1.0f}
    , mUnfocusedItemOpacity {0.5f}
    , mUnfocusedItemSaturation {1.0f}
    , mHasUnfocusedItemSaturation {false}
    , mUnfocusedItemDimming {1.0f}
    , mImagefit {ImageFit::CONTAIN}
    , mCarouselColor {0}
    , mCarouselColorEnd {0}
    , mColorGradientHorizontal {true}
    , mTextColor {0x000000FF}
    , mTextBackgroundColor {0xFFFFFF00}
    , mTextSelectedColor {0x000000FF}
    , mTextSelectedBackgroundColor {0xFFFFFF00}
    , mHasTextSelectedColor {false}
    , mFont {Font::get(FONT_SIZE_LARGE_FIXED)}
    , mLetterCase {LetterCase::NONE}
    , mLetterCaseAutoCollections {LetterCase::UNDEFINED}
    , mLetterCaseCustomCollections {LetterCase::UNDEFINED}
    , mLineSpacing {1.5f}
    , mSystemNameSuffix {true}
    , mLetterCaseSystemNameSuffix {LetterCase::UPPERCASE}
    , mFadeAbovePrimary {false}
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
                item->setMaxSize(glm::round(mItemSize * mItemScale));
                item->applyTheme(theme, "system", "image_logo",
                                 ThemeFlags::PATH | ThemeFlags::COLOR);
                item->setRotateByTargetSize(true);
                entry.data.item = item;
            }
        }
    }
    else {
        if (entry.data.imagePath != "" &&
            ResourceManager::getInstance().fileExists(entry.data.imagePath)) {
            auto item = std::make_shared<ImageComponent>(false, dynamic);
            item->setLinearInterpolation(mLinearInterpolation);
            item->setMipmapping(true);
            if (mImagefit == ImageFit::CONTAIN)
                item->setMaxSize(glm::round(mItemSize * (mItemScale >= 1.0f ? mItemScale : 1.0f)));
            else if (mImagefit == ImageFit::FILL)
                item->setResize(glm::round(mItemSize * (mItemScale >= 1.0f ? mItemScale : 1.0f)));
            else if (mImagefit == ImageFit::COVER)
                item->setCroppedSize(
                    glm::round(mItemSize * (mItemScale >= 1.0f ? mItemScale : 1.0f)));
            item->setImage(entry.data.imagePath);
            item->applyTheme(theme, "system", "", ThemeFlags::ALL);
            if (mImageBrightness != 0.0)
                item->setBrightness(mImageBrightness);
            if (mImageSaturation != 1.0)
                item->setSaturation(mImageSaturation);
            if (mImageColorShift != 0xFFFFFFFF)
                item->setColorShift(mImageColorShift);
            if (mImageColorShiftEnd != mImageColorShift)
                item->setColorShiftEnd(mImageColorShiftEnd);
            if (!mImageColorGradientHorizontal)
                item->setColorGradientHorizontal(false);
            item->setRotateByTargetSize(true);
            entry.data.item = item;
        }
        else if (entry.data.defaultImagePath != "" &&
                 ResourceManager::getInstance().fileExists(entry.data.defaultImagePath)) {
            if (mDefaultImage.get() == nullptr || !mGamelistView) {
                mDefaultImage = std::make_shared<ImageComponent>(false, dynamic);
                mDefaultImage->setLinearInterpolation(mLinearInterpolation);
                mDefaultImage->setMipmapping(true);
                if (mImagefit == ImageFit::CONTAIN)
                    mDefaultImage->setMaxSize(
                        glm::round(mItemSize * (mItemScale >= 1.0f ? mItemScale : 1.0f)));
                else if (mImagefit == ImageFit::FILL)
                    mDefaultImage->setResize(
                        glm::round(mItemSize * (mItemScale >= 1.0f ? mItemScale : 1.0f)));
                else if (mImagefit == ImageFit::COVER)
                    mDefaultImage->setCroppedSize(
                        glm::round(mItemSize * (mItemScale >= 1.0f ? mItemScale : 1.0f)));
                mDefaultImage->setImage(entry.data.defaultImagePath);
                mDefaultImage->applyTheme(theme, "system", "", ThemeFlags::ALL);
                if (mImageBrightness != 0.0)
                    mDefaultImage->setBrightness(mImageBrightness);
                if (mImageSaturation != 1.0)
                    mDefaultImage->setSaturation(mImageSaturation);
                if (mImageColorShift != 0xFFFFFFFF)
                    mDefaultImage->setColorShift(mImageColorShift);
                if (mImageColorShiftEnd != mImageColorShift)
                    mDefaultImage->setColorShiftEnd(mImageColorShiftEnd);
                if (!mImageColorGradientHorizontal)
                    mDefaultImage->setColorGradientHorizontal(false);
                mDefaultImage->setRotateByTargetSize(true);
            }
            // For the gamelist view the default image is applied in onDemandTextureLoad().
            if (!mGamelistView)
                entry.data.item = mDefaultImage;
        }
        else if (!mGamelistView) {
            entry.data.imagePath = "";
        }
    }

    if (!entry.data.item) {
        // If no item image is present, add item text as fallback.
        auto text = std::make_shared<TextComponent>(
            entry.name, mFont, 0x000000FF, mItemHorizontalAlignment, mItemVerticalAlignment,
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
            if (!mGamelistView)
                text->setValue(entry.name);
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
}

template <typename T>
void CarouselComponent<T>::updateEntry(Entry& entry, const std::shared_ptr<ThemeData>& theme)
{
    if (entry.data.imagePath != "") {
        auto item = std::make_shared<ImageComponent>(false, true);
        item->setLinearInterpolation(mLinearInterpolation);
        item->setMipmapping(true);
        if (mImagefit == ImageFit::CONTAIN)
            item->setMaxSize(glm::round(mItemSize * (mItemScale >= 1.0f ? mItemScale : 1.0f)));
        else if (mImagefit == ImageFit::FILL)
            item->setResize(glm::round(mItemSize * (mItemScale >= 1.0f ? mItemScale : 1.0f)));
        else if (mImagefit == ImageFit::COVER)
            item->setCroppedSize(glm::round(mItemSize * (mItemScale >= 1.0f ? mItemScale : 1.0f)));
        item->setImage(entry.data.imagePath);
        item->applyTheme(theme, "system", "", ThemeFlags::ALL);
        if (mImageBrightness != 0.0)
            item->setBrightness(mImageBrightness);
        if (mImageSaturation != 1.0)
            item->setSaturation(mImageSaturation);
        if (mImageColorShift != 0xFFFFFFFF)
            item->setColorShift(mImageColorShift);
        if (mImageColorShiftEnd != mImageColorShift)
            item->setColorShiftEnd(mImageColorShiftEnd);
        if (!mImageColorGradientHorizontal)
            item->setColorGradientHorizontal(false);
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
        if (size() == 0)
            return;

        if (mImageTypes.empty())
            mImageTypes.emplace_back("marquee");

        const int numEntries {size()};
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
            itemInclusion += 1;
        }

        for (int i = center - itemInclusion - itemInclusionBefore;
             i < center + itemInclusion + itemInclusionAfter; ++i) {
            int cursor {i};

            while (cursor < 0)
                cursor += numEntries;
            while (cursor >= numEntries)
                cursor -= numEntries;

            auto& entry = mEntries.at(cursor);

            if (entry.data.imagePath == "") {
                FileData* game {entry.object};

                for (auto& imageType : mImageTypes) {
                    if (imageType == "marquee")
                        entry.data.imagePath = game->getMarqueePath();
                    else if (imageType == "cover")
                        entry.data.imagePath = game->getCoverPath();
                    else if (imageType == "backcover")
                        entry.data.imagePath = game->getBackCoverPath();
                    else if (imageType == "3dbox")
                        entry.data.imagePath = game->get3DBoxPath();
                    else if (imageType == "physicalmedia")
                        entry.data.imagePath = game->getPhysicalMediaPath();
                    else if (imageType == "screenshot")
                        entry.data.imagePath = game->getScreenshotPath();
                    else if (imageType == "titlescreen")
                        entry.data.imagePath = game->getTitleScreenPath();
                    else if (imageType == "miximage")
                        entry.data.imagePath = game->getMiximagePath();
                    else if (imageType == "fanart")
                        entry.data.imagePath = game->getFanArtPath();
                    else if (imageType == "none") // Display the game name as text.
                        break;

                    if (entry.data.imagePath != "")
                        break;
                }

                if (entry.data.imagePath == "")
                    entry.data.imagePath = entry.data.defaultImagePath;

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
                if (isScrolling())
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
    float scaleSize {0.0f};

    if (mType == CarouselType::HORIZONTAL_WHEEL)
        scaleSize = mItemSize.y * mItemScale - mItemSize.y;
    else
        scaleSize = mItemSize.x * mItemScale - mItemSize.x;

    if (isWheel) {
        // Alignment of the actual carousel inside to the overall component area.
        if (mLegacyMode) {
            if (mItemHorizontalAlignment == ALIGN_LEFT)
                xOff = mSize.x / 10.0f;
            else if (mItemHorizontalAlignment == ALIGN_RIGHT)
                xOff = mSize.x - (mItemSize.x * 1.1f);
            else
                xOff = (mSize.x - mItemSize.x) / 2.0f;
            yOff = (mSize.y - mItemSize.y) / 2.0f;
        }
        else {
            if (mType == CarouselType::HORIZONTAL_WHEEL) {
                xOff = (mSize.x / 2.0f) - (mItemSize.y / 2.0f);
                if (mWheelVerticalAlignment == ALIGN_CENTER) {
                    yOff = (mSize.y / 2.0f) + (mItemSize.x / 2.0f);
                    if (mItemVerticalAlignment == ALIGN_TOP)
                        yOff -= scaleSize / 2.0f;
                    else if (mItemVerticalAlignment == ALIGN_BOTTOM)
                        yOff += scaleSize / 2.0f;
                }
                else if (mWheelVerticalAlignment == ALIGN_TOP) {
                    yOff = mItemSize.x - ((mItemSize.x - mItemSize.y) / 2.0f);
                    if (mItemVerticalAlignment == ALIGN_CENTER)
                        yOff += scaleSize / 2.0f;
                    else if (mItemVerticalAlignment == ALIGN_BOTTOM)
                        yOff += scaleSize;
                }
                else if (mWheelVerticalAlignment == ALIGN_BOTTOM) {
                    yOff = mSize.y + ((mItemSize.x - mItemSize.y) / 2.0f);
                    if (mItemVerticalAlignment == ALIGN_CENTER)
                        yOff -= scaleSize / 2.0f;
                    else if (mItemVerticalAlignment == ALIGN_TOP)
                        yOff -= scaleSize / 1.0f;
                }
            }
            else {
                xOff = (mSize.x - mItemSize.x) / 2.0f;
                yOff = (mSize.y - mItemSize.y) / 2.0f;
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
        float saturation;
        float dimming;
        glm::mat4 trans;
    };

    std::vector<renderStruct> renderItems;
    std::vector<renderStruct> renderItemsSorted;

    for (int i {center - itemInclusion - itemInclusionBefore};
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

        // Don't allow scaling below 1.0 for legacy themes as it introduces compatibility issues.
        if (mLegacyMode || mItemScale >= 1.0f) {
            scale = 1.0f + ((mItemScale - 1.0f) * (1.0f - fabsf(distance)));
            scale = std::min(mItemScale, std::max(1.0f, scale));
            scale /= mItemScale;
        }
        else {
            scale = 1.0f + ((1.0f - mItemScale) * (fabsf(distance) - 1.0f));
            scale = std::max(mItemScale, std::min(1.0f, scale));
        }

        glm::vec2 selectedItemMargins {0.0f, 0.0f};

        if (mSelectedItemMargins != glm::vec2 {0.0f, 0.0f}) {
            if (i < camOffset) {
                if (mType == CarouselType::HORIZONTAL)
                    selectedItemMargins.x = -mSelectedItemMargins.x;
                else
                    selectedItemMargins.y = -mSelectedItemMargins.x;
            }
            else if (i > camOffset) {
                if (mType == CarouselType::HORIZONTAL)
                    selectedItemMargins.x = mSelectedItemMargins.y;
                else
                    selectedItemMargins.y = mSelectedItemMargins.y;
            }

            if (std::fabs(distance) < 1.0f)
                selectedItemMargins *= std::fabs(distance);
        }

        glm::mat4 itemTrans {carouselTrans};
        if (singleEntry)
            itemTrans = glm::translate(carouselTrans, glm::vec3 {xOff, yOff, 0.0f});
        else
            itemTrans = glm::translate(
                itemTrans, glm::vec3 {(i * itemSpacing.x) + xOff + selectedItemMargins.x,
                                      (i * itemSpacing.y) + yOff + selectedItemMargins.y, 0.0f});

        if (mType == CarouselType::HORIZONTAL_WHEEL)
            itemTrans = glm::rotate(itemTrans, glm::radians(-90.0f), glm::vec3 {0.0f, 0.0f, 1.0f});

        float opacity {0.0f};
        float saturation {0.0f};
        float dimming {0.0f};

        if (distance == 0.0f || mUnfocusedItemOpacity == 1.0f) {
            opacity = 1.0f;
        }
        else if (fabsf(distance) >= 1.0f) {
            opacity = mUnfocusedItemOpacity;
        }
        else {
            const float maxDiff {1.0f - mUnfocusedItemOpacity};
            opacity = mUnfocusedItemOpacity + (maxDiff - (maxDiff * fabsf(distance)));
        }

        if (mHasUnfocusedItemSaturation) {
            if (distance == 0.0f) {
                saturation = mImageSaturation;
            }
            else if (fabsf(distance) >= 1.0f) {
                saturation = mUnfocusedItemSaturation;
            }
            else {
                const float maxDiff {mImageSaturation - mUnfocusedItemSaturation};
                saturation = mUnfocusedItemSaturation + (maxDiff - (maxDiff * fabsf(distance)));
            }
        }

        if (distance == 0.0f || mUnfocusedItemDimming == 1.0f) {
            dimming = 1.0f;
        }
        else if (fabsf(distance) >= 1.0f) {
            dimming = mUnfocusedItemDimming;
        }
        else {
            const float maxDiff {1.0f - mUnfocusedItemDimming};
            dimming = mUnfocusedItemDimming + (maxDiff - (maxDiff * fabsf(distance)));
        }

        renderStruct renderItem;
        renderItem.index = index;
        renderItem.distance = distance;
        renderItem.scale = scale;
        renderItem.opacity = opacity;
        renderItem.saturation = saturation;
        renderItem.dimming = dimming;
        renderItem.trans = itemTrans;

        renderItems.emplace_back(renderItem);

        if (singleEntry)
            break;
    }

    int belowCenter {static_cast<int>(std::round((renderItems.size() - centerOffset - 1) / 2))};
    if (renderItems.size() == 1) {
        renderItemsSorted.emplace_back(renderItems.front());
    }
    else if (!isWheel && mItemStacking != ItemStacking::CENTERED) {
        if (mItemStacking == ItemStacking::ASCENDING) {
            renderItemsSorted.insert(renderItemsSorted.begin(),
                                     std::make_move_iterator(renderItems.begin()),
                                     std::make_move_iterator(renderItems.end()));
        }
        else if (mItemStacking == ItemStacking::ASCENDING_RAISED) {
            for (size_t i {0}; i < renderItems.size(); ++i) {
                if (i == static_cast<size_t>(belowCenter))
                    continue;
                renderItemsSorted.emplace_back(std::move(renderItems[i]));
            }
            renderItemsSorted.emplace_back(std::move(renderItems[belowCenter]));
        }
        else if (mItemStacking == ItemStacking::DESCENDING) {
            for (size_t i {renderItems.size()}; i > 0; --i)
                renderItemsSorted.emplace_back(std::move(renderItems[i - 1]));
        }
        else if (mItemStacking == ItemStacking::DESCENDING_RAISED) {
            for (size_t i {renderItems.size()}; i > 0; --i) {
                if (i - 1 == static_cast<size_t>(belowCenter))
                    continue;
                renderItemsSorted.emplace_back(std::move(renderItems[i - 1]));
            }
            renderItemsSorted.emplace_back(std::move(renderItems[belowCenter]));
        }
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
            const float xOffTrans {-mItemRotationOrigin.x * mItemSize.x};
            const float yOffTrans {mItemAxisHorizontal ? 0.0f :
                                                         -mItemRotationOrigin.y * mItemSize.y};

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

                if (mType == CarouselType::HORIZONTAL_WHEEL) {
                    // For horizontal wheels we need to rotate all items 90 degrees around their
                    // own axis.
                    const float xOffTransRotate {-(mItemSize.x / 2.0f)};
                    const float yOffTransRotate {-(mItemSize.y / 2.0f)};

                    renderItem.trans =
                        glm::translate(renderItem.trans, glm::vec3 {xOffTransRotate * -1.0f,
                                                                    yOffTransRotate * -1.0f, 0.0f});

                    renderItem.trans = glm::rotate(renderItem.trans, glm::radians(90.0f),
                                                   glm::vec3 {0.0f, 0.0f, 1.0f});

                    renderItem.trans = glm::translate(
                        renderItem.trans, glm::vec3 {xOffTransRotate, yOffTransRotate, 0.0f});
                }
            }
            else if (mType == CarouselType::HORIZONTAL_WHEEL) {
                renderItem.trans = positionCalc;
                renderItem.trans = positionCalc;
                const float xOffTransRotate {-(mItemSize.x / 2.0f)};
                const float yOffTransRotate {-(mItemSize.y / 2.0f)};

                renderItem.trans =
                    glm::translate(renderItem.trans, glm::vec3 {xOffTransRotate * -1.0f,
                                                                yOffTransRotate * -1.0f, 0.0f});

                renderItem.trans = glm::rotate(renderItem.trans, glm::radians(90.0f),
                                               glm::vec3 {0.0f, 0.0f, 1.0f});

                renderItem.trans = glm::translate(
                    renderItem.trans, glm::vec3 {xOffTransRotate, yOffTransRotate, 0.0f});
            }
            else {
                renderItem.trans = positionCalc;
                renderItem.trans = positionCalc;
            }
        }
        else if (mItemAxisRotation != 0.0f) {
            // Rotate items around their own axis.
            const float xOffTransRotate {-(mItemSize.x / 2.0f)};
            const float yOffTransRotate {-(mItemSize.y / 2.0f)};

            renderItem.trans =
                glm::translate(renderItem.trans,
                               glm::vec3 {xOffTransRotate * -1.0f, yOffTransRotate * -1.0f, 0.0f});

            renderItem.trans = glm::rotate(renderItem.trans, glm::radians(mItemAxisRotation),
                                           glm::vec3 {0.0f, 0.0f, 1.0f});

            renderItem.trans = glm::translate(renderItem.trans,
                                              glm::vec3 {xOffTransRotate, yOffTransRotate, 0.0f});
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

        const glm::vec3 origPos {comp->getPosition()};
        if (mItemDiagonalOffset != 0.0f) {
            if (mType == CarouselType::HORIZONTAL)
                comp->setPosition(
                    origPos.x, origPos.y - (mItemDiagonalOffset * renderItem.distance), origPos.z);
            else
                comp->setPosition(origPos.x - (mItemDiagonalOffset * renderItem.distance),
                                  origPos.y, origPos.z);
        }

        comp->setScale(renderItem.scale);
        comp->setOpacity(renderItem.opacity * metadataOpacity);
        if (mHasUnfocusedItemSaturation)
            comp->setSaturation(renderItem.saturation);
        if (mUnfocusedItemDimming != 1.0f)
            comp->setDimming(renderItem.dimming);
        if (renderItem.index == mCursor && std::abs(renderItem.distance) < 1.0f &&
            (mHasImageSelectedColor || mHasTextSelectedColor)) {
            if (mHasTextSelectedColor && mEntries.at(renderItem.index).data.imagePath == "" &&
                mEntries.at(renderItem.index).data.defaultImagePath == "") {
                comp->setColor(mTextSelectedColor);
                if (mTextSelectedBackgroundColor != mTextBackgroundColor)
                    comp->setBackgroundColor(mTextSelectedBackgroundColor);
                comp->render(renderItem.trans);
                comp->setColor(mTextColor);
                if (mTextSelectedBackgroundColor != mTextBackgroundColor)
                    comp->setBackgroundColor(mTextBackgroundColor);
            }
            else if (mHasImageSelectedColor) {
                comp->setColorShift(mImageSelectedColor);
                if (mImageSelectedColorEnd != mImageSelectedColor)
                    comp->setColorShiftEnd(mImageSelectedColorEnd);
                if (mImageSelectedColorGradientHorizontal != mImageColorGradientHorizontal)
                    comp->setColorGradientHorizontal(mImageSelectedColorGradientHorizontal);
                comp->render(renderItem.trans);
                if (mImageSelectedColorGradientHorizontal != mImageColorGradientHorizontal)
                    comp->setColorGradientHorizontal(mImageColorGradientHorizontal);
                comp->setColorShift(mImageColorShift);
                if (mImageColorShiftEnd != mImageColorShift)
                    comp->setColorShiftEnd(mImageColorShiftEnd);
            }
            else {
                comp->render(renderItem.trans);
            }
        }
        else {
            comp->render(renderItem.trans);
        }

        if (mItemDiagonalOffset != 0.0f)
            comp->setPosition(origPos);

        // TODO: Rewrite to use "real" reflections instead of this hack.
        // Don't attempt to add reflections for text entries.
        if (mReflections && (mEntries.at(renderItem.index).data.imagePath != "" ||
                             mEntries.at(renderItem.index).data.defaultImagePath != "")) {
            glm::mat4 reflectionTrans {glm::translate(
                renderItem.trans, glm::vec3 {0.0f, comp->getSize().y * renderItem.scale, 0.0f})};
            float falloff {glm::clamp(mReflectionsFalloff, 0.0f, 1.0f)};
            falloff = mReflectionsOpacity * (1.0f - falloff);
            comp->setOpacity(comp->getOpacity() * mReflectionsOpacity);
            if (mHasUnfocusedItemSaturation)
                comp->setSaturation(renderItem.saturation);
            if (mUnfocusedItemDimming != 1.0f)
                comp->setDimming(renderItem.dimming);
            if (mReflectionsFalloff > 0.0f)
                comp->setReflectionsFalloff(comp->getSize().y / mReflectionsFalloff);
            comp->setFlipY(true);
            if (renderItem.index == mCursor && mHasImageSelectedColor) {
                comp->setColorShift(mImageSelectedColorGradientHorizontal ? mImageSelectedColor :
                                                                            mImageSelectedColorEnd);
                if (mImageSelectedColorEnd != mImageSelectedColor)
                    comp->setColorShiftEnd(mImageSelectedColorGradientHorizontal ?
                                               mImageSelectedColorEnd :
                                               mImageSelectedColor);
                if (mImageSelectedColorGradientHorizontal != mImageColorGradientHorizontal)
                    comp->setColorGradientHorizontal(mImageSelectedColorGradientHorizontal);
                comp->render(reflectionTrans);
                if (mImageSelectedColorGradientHorizontal != mImageColorGradientHorizontal)
                    comp->setColorGradientHorizontal(mImageColorGradientHorizontal);
                comp->setColorShift(mImageColorShift);
                if (mImageColorShiftEnd != mImageColorShift)
                    comp->setColorShiftEnd(mImageColorShiftEnd);
            }
            else {
                if ((mImageColorShift != 0xFFFFFFFF || mImageColorShiftEnd != 0xFFFFFFFF) &&
                    !mImageColorGradientHorizontal) {
                    // We need to reverse the color shift if a vertical gradient is applied.
                    comp->setColorShift(mImageColorShiftEnd);
                    comp->setColorShiftEnd(mImageColorShift);
                    comp->render(reflectionTrans);
                    comp->setColorShift(mImageColorShift);
                    comp->setColorShiftEnd(mImageColorShiftEnd);
                }
                else {
                    comp->render(reflectionTrans);
                }
            }
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
    mSize.x = Renderer::getScreenWidth();
    mSize.y = Renderer::getScreenHeight() * 0.23240f;
    GuiComponent::mPosition.x = 0.0f;
    GuiComponent::mPosition.y = Renderer::getScreenHeight() * 0.38378f;
    mCarouselColor = 0xFFFFFFD8;
    mCarouselColorEnd = 0xFFFFFFD8;
    mZIndex = mDefaultZIndex;

    using namespace ThemeFlags;
    const ThemeData::ThemeElement* elem {theme->getElement(view, element, "carousel")};

    if (!elem)
        return;

    mLegacyMode = theme->isLegacyTheme();

    if (elem->has("type")) {
        std::string type {elem->get<std::string>("type")};
        if (mLegacyMode && type == "horizontal_wheel")
            type = "horizontalWheel";
        else if (mLegacyMode && type == "vertical_wheel")
            type = "verticalWheel";

        if (type == "horizontal") {
            mType = CarouselType::HORIZONTAL;
        }
        else if (type == "horizontalWheel") {
            mType = CarouselType::HORIZONTAL_WHEEL;
        }
        else if (type == "vertical") {
            mType = CarouselType::VERTICAL;
        }
        else if (type == "verticalWheel") {
            mType = CarouselType::VERTICAL_WHEEL;
        }
        else {
            LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property \"type\" "
                               "for element \""
                            << element.substr(9) << "\" defined as \"" << type << "\"";
            mType = CarouselType::HORIZONTAL;
        }
    }

    if (mGamelistView && properties && elem->has("imageType")) {
        const std::vector<std::string> supportedImageTypes {
            "marquee",    "cover",       "backcover", "3dbox",  "physicalmedia",
            "screenshot", "titlescreen", "miximage",  "fanart", "none"};
        std::string imageTypesString {elem->get<std::string>("imageType")};

        for (auto& character : imageTypesString) {
            if (std::isspace(character))
                character = ',';
        }
        imageTypesString = Utils::String::replace(imageTypesString, ",,", ",");
        mImageTypes = Utils::String::delimitedStringToVector(imageTypesString, ",");

        // Only allow two imageType entries due to performance reasons.
        if (mImageTypes.size() > 2)
            mImageTypes.erase(mImageTypes.begin() + 2, mImageTypes.end());

        if (mImageTypes.empty()) {
            LOG(LogWarning)
                << "CarouselComponent: Invalid theme configuration, property \"imageType\" "
                   "for element \""
                << element.substr(9) << "\" contains no values";
        }

        for (std::string& type : mImageTypes) {
            if (std::find(supportedImageTypes.cbegin(), supportedImageTypes.cend(), type) ==
                supportedImageTypes.cend()) {
                LOG(LogWarning)
                    << "CarouselComponent: Invalid theme configuration, property \"imageType\" "
                       "for element \""
                    << element.substr(9) << "\" defined as \"" << type << "\"";
                mImageTypes.clear();
                break;
            }
        }

        if (mImageTypes.size() == 2 && mImageTypes.front() == mImageTypes.back()) {
            LOG(LogError)
                << "CarouselComponent: Invalid theme configuration, property \"imageType\" "
                   "for element \""
                << element.substr(9) << "\" contains duplicate values";
            mImageTypes.clear();
        }
    }

    if (elem->has("color")) {
        mCarouselColor = elem->get<unsigned int>("color");
        mCarouselColorEnd = mCarouselColor;
    }
    if (elem->has("colorEnd"))
        mCarouselColorEnd = elem->get<unsigned int>("colorEnd");

    if (elem->has("gradientType")) {
        const std::string& gradientType {elem->get<std::string>("gradientType")};
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
                                        "\"horizontalWheel\"" :
                                        "\"verticalWheel\"")
                                << " type";
            }
        }

        if (elem->has("itemsBeforeCenter"))
            mItemsBeforeCenter =
                glm::clamp(static_cast<int>(elem->get<unsigned int>("itemsBeforeCenter")), 0, 20);

        if (elem->has("itemsAfterCenter"))
            mItemsAfterCenter =
                glm::clamp(static_cast<int>(elem->get<unsigned int>("itemsAfterCenter")), 0, 20);

        if (mType == CarouselType::HORIZONTAL || mType == CarouselType::VERTICAL) {
            if (elem->has("selectedItemMargins")) {
                const glm::vec2 selectedItemMargins {
                    glm::clamp(elem->get<glm::vec2>("selectedItemMargins"), -1.0f, 1.0f)};
                if (mType == CarouselType::HORIZONTAL)
                    mSelectedItemMargins = selectedItemMargins * Renderer::getScreenWidth();
                else
                    mSelectedItemMargins = selectedItemMargins * Renderer::getScreenHeight();
            }
        }

        if (elem->has("itemSize")) {
            const glm::vec2 itemSize {glm::clamp(elem->get<glm::vec2>("itemSize"), 0.05f, 1.0f)};
            mItemSize =
                itemSize * glm::vec2(Renderer::getScreenWidth(), Renderer::getScreenHeight());
        }

        if (elem->has("itemStacking")) {
            const std::string& itemStacking {elem->get<std::string>("itemStacking")};
            if (itemStacking == "ascending") {
                mItemStacking = ItemStacking::ASCENDING;
            }
            else if (itemStacking == "ascendingRaised") {
                mItemStacking = ItemStacking::ASCENDING_RAISED;
            }
            else if (itemStacking == "descending") {
                mItemStacking = ItemStacking::DESCENDING;
            }
            else if (itemStacking == "descendingRaised") {
                mItemStacking = ItemStacking::DESCENDING_RAISED;
            }
            else if (itemStacking == "centered") {
                mItemStacking = ItemStacking::CENTERED;
            }
            else {
                mItemStacking = ItemStacking::CENTERED;
                LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                                   "\"itemStacking\" for element \""
                                << element.substr(9) << "\" defined as \"" << itemStacking << "\"";
            }
        }

        if (elem->has("itemScale"))
            mItemScale = glm::clamp(elem->get<float>("itemScale"), 0.2f, 3.0f);

        if (elem->has("imageFit")) {
            const std::string& imageFit {elem->get<std::string>("imageFit")};
            if (imageFit == "contain") {
                mImagefit = ImageFit::CONTAIN;
            }
            else if (imageFit == "fill") {
                mImagefit = ImageFit::FILL;
            }
            else if (imageFit == "cover") {
                mImagefit = ImageFit::COVER;
            }
            else {
                mImagefit = ImageFit::CONTAIN;
                LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                                   "\"imageFit\" for element \""
                                << element.substr(9) << "\" defined as \"" << imageFit << "\"";
            }
        }

        mImageSelectedColor = mImageColorShift;
        mImageSelectedColorEnd = mImageColorShiftEnd;

        if (elem->has("imageSelectedColor")) {
            mImageSelectedColor = elem->get<unsigned int>("imageSelectedColor");
            mImageSelectedColorEnd = mImageSelectedColor;
            mHasImageSelectedColor = true;
        }
        if (elem->has("imageSelectedColorEnd")) {
            mImageSelectedColorEnd = elem->get<unsigned int>("imageSelectedColorEnd");
            mHasImageSelectedColor = true;
        }
        if (elem->has("imageSelectedGradientType")) {
            const std::string& gradientType {elem->get<std::string>("imageSelectedGradientType")};
            if (gradientType == "horizontal") {
                mImageSelectedColorGradientHorizontal = true;
            }
            else if (gradientType == "vertical") {
                mImageSelectedColorGradientHorizontal = false;
            }
            else {
                mImageSelectedColorGradientHorizontal = true;
                LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                                   "\"imageSelectedGradientType\" for element \""
                                << element.substr(9) << "\" defined as \"" << gradientType << "\"";
            }
        }

        if (elem->has("imageBrightness"))
            mImageBrightness = glm::clamp(elem->get<float>("imageBrightness"), -2.0f, 2.0f);

        if (elem->has("imageSaturation"))
            mImageSaturation = glm::clamp(elem->get<float>("imageSaturation"), 0.0f, 1.0f);

        if (elem->has("itemDiagonalOffset") &&
            (mType == CarouselType::HORIZONTAL || mType == CarouselType::VERTICAL)) {
            const float diagonalOffset {
                glm::clamp(elem->get<float>("itemDiagonalOffset"), -0.5f, 0.5f)};
            if (mType == CarouselType::HORIZONTAL)
                mItemDiagonalOffset = diagonalOffset * Renderer::getScreenHeight();
            else
                mItemDiagonalOffset = diagonalOffset * Renderer::getScreenWidth();
        }

        if (elem->has("imageInterpolation")) {
            const std::string& imageInterpolation {elem->get<std::string>("imageInterpolation")};
            if (imageInterpolation == "linear") {
                mLinearInterpolation = true;
            }
            else if (imageInterpolation == "nearest") {
                mLinearInterpolation = false;
            }
            else {
                mLinearInterpolation = true;
                LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                                   "\"imageInterpolation\" for element \""
                                << element.substr(9) << "\" defined as \"" << imageInterpolation
                                << "\"";
            }
        }

        if (elem->has("itemTransitions")) {
            const std::string& itemTransitions {elem->get<std::string>("itemTransitions")};
            if (itemTransitions == "animate") {
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

        if (elem->has("itemRotation"))
            mItemRotation = elem->get<float>("itemRotation");

        if (elem->has("itemRotationOrigin"))
            mItemRotationOrigin = elem->get<glm::vec2>("itemRotationOrigin");

        mItemAxisHorizontal =
            (elem->has("itemAxisHorizontal") && elem->get<bool>("itemAxisHorizontal"));

        if (elem->has("itemAxisRotation"))
            mItemAxisRotation = elem->get<float>("itemAxisRotation");

        if (elem->has("imageColor")) {
            mImageColorShift = elem->get<unsigned int>("imageColor");
            mImageColorShiftEnd = mImageColorShift;
        }
        if (elem->has("imageColorEnd"))
            mImageColorShiftEnd = elem->get<unsigned int>("imageColorEnd");

        if (elem->has("imageGradientType")) {
            const std::string& gradientType {elem->get<std::string>("imageGradientType")};
            if (gradientType == "horizontal") {
                mImageColorGradientHorizontal = true;
            }
            else if (gradientType == "vertical") {
                mImageColorGradientHorizontal = false;
            }
            else {
                mImageColorGradientHorizontal = true;
                LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                                   "\"imageGradientType\" for element \""
                                << element.substr(9) << "\" defined as \"" << gradientType << "\"";
            }
        }

        if (elem->has("itemHorizontalAlignment") && mType != CarouselType::HORIZONTAL &&
            mType != CarouselType::HORIZONTAL_WHEEL) {
            const std::string& alignment {elem->get<std::string>("itemHorizontalAlignment")};
            if (alignment == "left") {
                mItemHorizontalAlignment = ALIGN_LEFT;
            }
            else if (alignment == "right") {
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

        if (elem->has("itemVerticalAlignment") && mType != CarouselType::VERTICAL) {
            const std::string& alignment {elem->get<std::string>("itemVerticalAlignment")};
            if (alignment == "top") {
                mItemVerticalAlignment = ALIGN_TOP;
            }
            else if (alignment == "bottom") {
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

        if (elem->has("wheelHorizontalAlignment") && mType == CarouselType::VERTICAL_WHEEL) {
            const std::string& alignment {elem->get<std::string>("wheelHorizontalAlignment")};
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

        if (elem->has("wheelVerticalAlignment") && mType == CarouselType::HORIZONTAL_WHEEL) {
            const std::string& alignment {elem->get<std::string>("wheelVerticalAlignment")};
            if (alignment == "top") {
                mWheelVerticalAlignment = ALIGN_TOP;
            }
            else if (alignment == "bottom") {
                mWheelVerticalAlignment = ALIGN_BOTTOM;
            }
            else if (alignment == "center") {
                mWheelVerticalAlignment = ALIGN_CENTER;
            }
            else {
                LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                                   "\"wheelVerticalAlignment\" for element \""
                                << element.substr(9) << "\" defined as \"" << alignment << "\"";
                mWheelVerticalAlignment = ALIGN_CENTER;
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

        if (elem->has("unfocusedItemSaturation")) {
            mUnfocusedItemSaturation =
                glm::clamp(elem->get<float>("unfocusedItemSaturation"), 0.0f, 1.0f);
            mHasUnfocusedItemSaturation = true;
        }

        if (elem->has("unfocusedItemDimming"))
            mUnfocusedItemDimming =
                glm::clamp(elem->get<float>("unfocusedItemDimming"), 0.0f, 1.0f);

        if (elem->has("fastScrolling") && elem->get<bool>("fastScrolling"))
            List::mTierList = IList<CarouselEntry, T>::LIST_SCROLL_STYLE_MEDIUM;
    }

    // Legacy themes.
    if (mLegacyMode) {
        // Don't allow logoScale below 1.0 for legacy themes as it introduces compatibility issues.
        // But we still need to load the value as-is for proper item sizing calculations, scaling
        // below 1.0 will be suppressed in render() instead.
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
            const std::string& alignment {elem->get<std::string>("logoAlignment")};
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
    mFont = Font::getFromTheme(elem, properties, mFont, 0.0f, false, mLegacyMode,
                               (mLegacyMode ? 1.0f : (mItemScale >= 1.0f ? mItemScale : 1.0f)));

    if (elem->has("textColor"))
        mTextColor = elem->get<unsigned int>("textColor");
    if (elem->has("textBackgroundColor"))
        mTextBackgroundColor = elem->get<unsigned int>("textBackgroundColor");

    mTextSelectedColor = mTextColor;
    mTextSelectedBackgroundColor = mTextBackgroundColor;

    if (elem->has("textSelectedColor")) {
        mTextSelectedColor = elem->get<unsigned int>("textSelectedColor");
        mHasTextSelectedColor = true;
    }
    if (elem->has("textSelectedBackgroundColor")) {
        mTextSelectedBackgroundColor = elem->get<unsigned int>("textSelectedBackgroundColor");
        mHasTextSelectedColor = true;
    }

    if (elem->has("lineSpacing"))
        mLineSpacing = glm::clamp(elem->get<float>("lineSpacing"), 0.5f, 3.0f);

    if (elem->has("letterCase")) {
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
            LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                               "\"letterCase\" for element \""
                            << element.substr(9) << "\" defined as \"" << letterCase << "\"";
        }
    }

    if (elem->has("letterCaseAutoCollections")) {
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
            LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                               "\"letterCaseAutoCollections\" for element \""
                            << element.substr(9) << "\" defined as \"" << letterCase << "\"";
        }
    }

    if (elem->has("letterCaseCustomCollections")) {
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
            LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                               "\"letterCaseCustomCollections\" for element \""
                            << element.substr(9) << "\" defined as \"" << letterCase << "\"";
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
            LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                               "\"letterCaseSystemNameSuffix\" for element \""
                            << element.substr(9) << "\" defined as \"" << letterCase << "\"";
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

            mEntryCamOffset = f;
        },
        static_cast<int>(animTime))};

    GuiComponent::setAnimation(anim, 0, nullptr, false, 0);

    if (mCursorChangedCallback)
        mCursorChangedCallback(state);
}

#endif // ES_CORE_COMPONENTS_PRIMARY_CAROUSEL_COMPONENT_H
