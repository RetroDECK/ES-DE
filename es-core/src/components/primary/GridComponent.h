//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GridComponent.h
//
//  Grid, usable in both the system and gamelist views.
//

#ifndef ES_CORE_COMPONENTS_PRIMARY_GRID_COMPONENT_H
#define ES_CORE_COMPONENTS_PRIMARY_GRID_COMPONENT_H

#include "components/IList.h"
#include "components/primary/PrimaryComponent.h"

struct GridEntry {
    std::shared_ptr<GuiComponent> item;
    std::string imagePath;
    std::string defaultImagePath;
};

template <typename T>
class GridComponent : public PrimaryComponent<T>, protected IList<GridEntry, T>
{
protected:
    using List = IList<GridEntry, T>;
    using List::mColumns;
    using List::mCursor;
    using List::mEntries;
    using List::mLastCursor;
    using List::mRows;
    using List::mScrollVelocity;
    using List::mSize;

public:
    using Entry = typename IList<GridEntry, T>::Entry;

    GridComponent();
    ~GridComponent();

    void addEntry(Entry& entry, const std::shared_ptr<ThemeData>& theme);
    void updateEntry(Entry& entry, const std::shared_ptr<ThemeData>& theme);
    void onDemandTextureLoad() override;
    void calculateLayout();
    const int getColumnCount() const { return mColumns; }
    const int getRowCount() const { return mRows; }
    void setScrollVelocity(int velocity) { mScrollVelocity = velocity; }
    void setSuppressTransitions(bool state) { mSuppressTransitions = state; }

    void setCancelTransitionsCallback(const std::function<void()>& func) override
    {
        mCancelTransitionsCallback = func;
    }
    void setCursorChangedCallback(const std::function<void(CursorState state)>& func) override
    {
        mCursorChangedCallback = func;
    }
    int getCursor() override { return mCursor; }
    const size_t getNumEntries() override { return mEntries.size(); }
    const bool getFadeAbovePrimary() const override { return mFadeAbovePrimary; }
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
    const std::string& getDefaultGridImage() const { return mDefaultImagePath; }
    const std::string& getDefaultGridFolderImage() const { return mDefaultFolderImagePath; }
    void setDefaultImage(std::string defaultImage) { mDefaultImagePath = defaultImage; }
    void setDefaultFolderImage(std::string defaultImage) { mDefaultFolderImagePath = defaultImage; }
    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;
    void applyTheme(const std::shared_ptr<ThemeData>& theme,
                    const std::string& view,
                    const std::string& element,
                    unsigned int properties) override;

private:
    void onScroll() override
    {
        if (mGamelistView)
            NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
        else
            NavigationSounds::getInstance().playThemeNavigationSound(SYSTEMBROWSESOUND);
    }
    void onCursorChanged(const CursorState& state) override;
    bool isScrolling() const override { return List::isScrolling(); }
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
    bool setCursor(const T& obj) override
    {
        mLastCursor = mCursor;
        return List::setCursor(obj);
    }
    bool remove(const T& obj) override { return List::remove(obj); }
    int size() const override { return List::size(); }

    enum class ImageFit {
        CONTAIN,
        FILL,
        COVER
    };

    enum class SelectorLayer {
        TOP,
        MIDDLE,
        BOTTOM
    };

    Renderer* mRenderer;
    std::function<void()> mCancelTransitionsCallback;
    std::function<void(CursorState state)> mCursorChangedCallback;
    float mEntryOffset;
    float mScrollPos;
    float mTransitionFactor;
    float mVisibleRows;
    int mPreviousScrollVelocity;
    bool mPositiveDirection;
    bool mGamelistView;
    bool mLayoutValid;
    bool mWasScrolling;
    bool mJustCalculatedLayout;
    bool mSuppressTransitions;
    float mHorizontalMargin;
    float mVerticalMargin;

    std::vector<std::string> mImageTypes;
    std::string mDefaultImagePath;
    std::string mDefaultFolderImagePath;
    std::shared_ptr<ImageComponent> mDefaultImage;
    glm::vec2 mItemSize;
    float mItemScale;
    glm::vec2 mItemSpacing;
    bool mFractionalRows;
    bool mInstantItemTransitions;
    bool mInstantRowTransitions;
    float mUnfocusedItemOpacity;
    float mUnfocusedItemSaturation;
    bool mHasUnfocusedItemSaturation;
    float mUnfocusedItemDimming;
    ImageFit mImagefit;
    float mImageRelativeScale;
    unsigned int mImageColor;
    unsigned int mImageColorEnd;
    bool mImageColorGradientHorizontal;
    unsigned int mImageSelectedColor;
    unsigned int mImageSelectedColorEnd;
    bool mImageSelectedColorGradientHorizontal;
    bool mHasImageSelectedColor;
    float mImageBrightness;
    float mImageSaturation;
    std::unique_ptr<ImageComponent> mBackgroundImage;
    std::string mBackgroundImagePath;
    float mBackgroundRelativeScale;
    unsigned int mBackgroundColor;
    unsigned int mBackgroundColorEnd;
    bool mBackgroundColorGradientHorizontal;
    bool mHasBackgroundColor;
    std::unique_ptr<ImageComponent> mSelectorImage;
    std::string mSelectorImagePath;
    float mSelectorRelativeScale;
    SelectorLayer mSelectorLayer;
    unsigned int mSelectorColor;
    unsigned int mSelectorColorEnd;
    bool mSelectorColorGradientHorizontal;
    bool mHasSelectorColor;
    float mTextRelativeScale;
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
GridComponent<T>::GridComponent()
    : IList<GridEntry, T> {IList<GridEntry, T>::LIST_SCROLL_STYLE_SLOW,
                           ListLoopType::LIST_PAUSE_AT_END}
    , mRenderer {Renderer::getInstance()}
    , mEntryOffset {0.0f}
    , mScrollPos {0.0f}
    , mTransitionFactor {1.0f}
    , mVisibleRows {1.0f}
    , mPreviousScrollVelocity {0}
    , mPositiveDirection {false}
    , mGamelistView {std::is_same_v<T, FileData*> ? true : false}
    , mLayoutValid {false}
    , mWasScrolling {false}
    , mJustCalculatedLayout {false}
    , mSuppressTransitions {false}
    , mHorizontalMargin {0.0f}
    , mVerticalMargin {0.0f}
    , mItemSize {glm::vec2 {mRenderer->getScreenWidth() * 0.15f,
                            mRenderer->getScreenHeight() * 0.25f}}
    , mItemScale {1.05f}
    , mItemSpacing {0.0f, 0.0f}
    , mFractionalRows {false}
    , mInstantItemTransitions {false}
    , mInstantRowTransitions {false}
    , mUnfocusedItemOpacity {1.0f}
    , mUnfocusedItemSaturation {1.0f}
    , mHasUnfocusedItemSaturation {false}
    , mUnfocusedItemDimming {1.0f}
    , mImagefit {ImageFit::CONTAIN}
    , mImageRelativeScale {1.0f}
    , mImageColor {0xFFFFFFFF}
    , mImageColorEnd {0xFFFFFFFF}
    , mImageColorGradientHorizontal {true}
    , mImageSelectedColor {0xFFFFFFFF}
    , mImageSelectedColorEnd {0xFFFFFFFF}
    , mImageSelectedColorGradientHorizontal {true}
    , mHasImageSelectedColor {false}
    , mImageBrightness {0.0f}
    , mImageSaturation {1.0f}
    , mBackgroundRelativeScale {1.0f}
    , mBackgroundColor {0xFFFFFFFF}
    , mBackgroundColorEnd {0xFFFFFFFF}
    , mBackgroundColorGradientHorizontal {true}
    , mHasBackgroundColor {false}
    , mSelectorRelativeScale {1.0f}
    , mSelectorLayer {SelectorLayer::TOP}
    , mSelectorColor {0xFFFFFFFF}
    , mSelectorColorEnd {0xFFFFFFFF}
    , mSelectorColorGradientHorizontal {true}
    , mHasSelectorColor {false}
    , mTextRelativeScale {1.0f}
    , mTextColor {0x000000FF}
    , mTextBackgroundColor {0xFFFFFF00}
    , mTextSelectedColor {0x000000FF}
    , mTextSelectedBackgroundColor {0xFFFFFF00}
    , mHasTextSelectedColor {false}
    , mLetterCase {LetterCase::NONE}
    , mLetterCaseAutoCollections {LetterCase::UNDEFINED}
    , mLetterCaseCustomCollections {LetterCase::UNDEFINED}
    , mLineSpacing {1.5f}
    , mSystemNameSuffix {true}
    , mLetterCaseSystemNameSuffix {LetterCase::UPPERCASE}
    , mFadeAbovePrimary {false}
{
}

template <typename T> GridComponent<T>::~GridComponent()
{
    // Manually flush the background and selector images from the texture cache on destruction
    // when running in debug mode, otherwise a complete system view reload would be needed to
    // get these images updated. This is useful during theme development when using the Ctrl-r
    // keyboard combination to reload the theme configuration.
    if (Settings::getInstance()->getBool("Debug")) {
        TextureResource::manualUnload(mBackgroundImagePath, false);
        TextureResource::manualUnload(mSelectorImagePath, false);
    }
}

template <typename T>
void GridComponent<T>::addEntry(Entry& entry, const std::shared_ptr<ThemeData>& theme)
{
    const bool dynamic {mGamelistView};

    if (entry.data.imagePath != "" &&
        ResourceManager::getInstance().fileExists(entry.data.imagePath)) {
        auto item = std::make_shared<ImageComponent>(false, dynamic);
        item->setLinearInterpolation(true);
        item->setMipmapping(true);
        if (mImagefit == ImageFit::CONTAIN)
            item->setMaxSize(mItemSize * mImageRelativeScale);
        else if (mImagefit == ImageFit::FILL)
            item->setResize(mItemSize * mImageRelativeScale);
        else if (mImagefit == ImageFit::COVER)
            item->setCroppedSize(mItemSize * mImageRelativeScale);
        item->setImage(entry.data.imagePath);
        item->applyTheme(theme, "system", "", ThemeFlags::ALL);
        if (mImageBrightness != 0.0)
            item->setBrightness(mImageBrightness);
        if (mImageSaturation != 1.0)
            item->setSaturation(mImageSaturation);
        if (mImageColor != 0xFFFFFFFF)
            item->setColorShift(mImageColor);
        if (mImageColorEnd != mImageColor) {
            item->setColorShiftEnd(mImageColorEnd);
            if (!mImageColorGradientHorizontal)
                item->setColorGradientHorizontal(false);
        }
        item->setOrigin(0.5f, 0.5f);
        item->setRotateByTargetSize(true);
        entry.data.item = item;
    }
    else if (entry.data.defaultImagePath != "" &&
             ResourceManager::getInstance().fileExists(entry.data.defaultImagePath)) {
        if (!mGamelistView)
            entry.data.imagePath = "";

        if (mDefaultImage.get() == nullptr || !mGamelistView) {
            mDefaultImage = std::make_shared<ImageComponent>(false, dynamic);
            mDefaultImage->setLinearInterpolation(true);
            mDefaultImage->setMipmapping(true);
            if (mImagefit == ImageFit::CONTAIN)
                mDefaultImage->setMaxSize(mItemSize * mImageRelativeScale);
            else if (mImagefit == ImageFit::FILL)
                mDefaultImage->setResize(mItemSize * mImageRelativeScale);
            else if (mImagefit == ImageFit::COVER)
                mDefaultImage->setCroppedSize(mItemSize * mImageRelativeScale);
            mDefaultImage->setImage(entry.data.defaultImagePath);
            mDefaultImage->applyTheme(theme, "system", "", ThemeFlags::ALL);
            if (mImageBrightness != 0.0)
                mDefaultImage->setBrightness(mImageBrightness);
            if (mImageSaturation != 1.0)
                mDefaultImage->setSaturation(mImageSaturation);
            if (mImageColor != 0xFFFFFFFF)
                mDefaultImage->setColorShift(mImageColor);
            if (mImageColorEnd != mImageColor) {
                mDefaultImage->setColorShiftEnd(mImageColorEnd);
                if (!mImageColorGradientHorizontal)
                    mDefaultImage->setColorGradientHorizontal(false);
            }
            mDefaultImage->setOrigin(0.5f, 0.5f);
            mDefaultImage->setRotateByTargetSize(true);
        }
        // For the gamelist view the default image is applied in onDemandTextureLoad().
        if (!mGamelistView)
            entry.data.item = mDefaultImage;
    }
    else if (!mGamelistView) {
        entry.data.imagePath = "";
    }

    if (!entry.data.item) {
        // If no item image is present, add item text as fallback.
        auto text = std::make_shared<TextComponent>(
            entry.name, mFont, 0x000000FF, Alignment::ALIGN_CENTER, Alignment::ALIGN_CENTER,
            glm::vec3 {0.0f, 0.0f, 0.0f}, mItemSize * mTextRelativeScale, 0x00000000);
        text->setOrigin(0.5f, 0.5f);
        text->setLineSpacing(mLineSpacing);
        if (!mGamelistView)
            text->setValue(entry.name);
        text->setColor(mTextColor);
        text->setBackgroundColor(mTextBackgroundColor);
        text->setRenderBackground(true);

        entry.data.item = text;
    }

    List::add(entry);
}

template <typename T>
void GridComponent<T>::updateEntry(Entry& entry, const std::shared_ptr<ThemeData>& theme)
{
    if (entry.data.imagePath != "") {
        const glm::vec3& calculatedItemPos {entry.data.item->getPosition()};
        auto item = std::make_shared<ImageComponent>(false, true);
        item->setLinearInterpolation(true);
        item->setMipmapping(true);
        if (mImagefit == ImageFit::CONTAIN)
            item->setMaxSize(mItemSize * mImageRelativeScale);
        else if (mImagefit == ImageFit::FILL)
            item->setResize(mItemSize * mImageRelativeScale);
        else if (mImagefit == ImageFit::COVER)
            item->setCroppedSize(mItemSize * mImageRelativeScale);
        item->setImage(entry.data.imagePath);
        item->applyTheme(theme, "system", "", ThemeFlags::ALL);
        if (mImageBrightness != 0.0)
            item->setBrightness(mImageBrightness);
        if (mImageSaturation != 1.0)
            item->setSaturation(mImageSaturation);
        if (mImageColor != 0xFFFFFFFF)
            item->setColorShift(mImageColor);
        if (mImageColorEnd != mImageColor) {
            item->setColorShiftEnd(mImageColorEnd);
            if (!mImageColorGradientHorizontal)
                item->setColorGradientHorizontal(false);
        }
        item->setOrigin(0.5f, 0.5f);
        item->setRotateByTargetSize(true);
        entry.data.item = item;
        entry.data.item->setPosition(calculatedItemPos);
    }
    else {
        return;
    }
}

template <typename T> void GridComponent<T>::onDemandTextureLoad()
{
    if constexpr (std::is_same_v<T, FileData*>) {
        if (size() == 0)
            return;

        if (mImageTypes.empty())
            mImageTypes.emplace_back("marquee");

        const int visibleRows {static_cast<int>(std::ceil(mVisibleRows))};
        const int columnPos {mCursor % mColumns};
        int loadItems {mColumns * visibleRows};

        const int numEntries {size()};
        int startPos {mCursor};
        int loadedItems {0};

        if (mCursor / mColumns <= visibleRows - 1)
            startPos = 0;
        else
            startPos = mCursor - (mColumns * (visibleRows - 1)) - columnPos;

        if (mItemSpacing.y < mVerticalMargin) {
            loadItems += mColumns;
            if (!mFractionalRows) {
                loadItems += mColumns;
                startPos -= mColumns;
                if (startPos < 0)
                    startPos = 0;
            }
        }

        for (int i {startPos}; i < size(); ++i) {
            if (loadedItems == loadItems)
                break;
            ++loadedItems;
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

template <typename T> void GridComponent<T>::calculateLayout()
{
    assert(!mEntries.empty());

    if (mItemScale < 1.0f) {
        mHorizontalMargin = 0.0f;
        mVerticalMargin = 0.0f;
    }
    else {
        mHorizontalMargin = ((mItemSize.x * mItemScale) - mItemSize.x) / 2.0f;
        mVerticalMargin = ((mItemSize.y * mItemScale) - mItemSize.y) / 2.0f;
    }

    int columnCount {0};
    mColumns = 0;
    mRows = 0;

    float width {mHorizontalMargin * 2.0f};

    while (1) {
        width += mItemSize.x;
        if (mColumns != 0)
            width += mItemSpacing.x;
        if (width > mSize.x)
            break;
        ++mColumns;
    }

    if (mColumns == 0)
        ++mColumns;

    for (auto& entry : mEntries) {
        entry.data.item->setPosition(glm::vec3 {
            mHorizontalMargin + (mItemSize.x * columnCount) + (mItemSize.x * 0.5f) +
                mItemSpacing.x * columnCount,
            mVerticalMargin + (mItemSize.y * mRows) + (mItemSize.y * 0.5f) + mItemSpacing.y * mRows,
            0.0f});
        if (columnCount == mColumns - 1) {
            ++mRows;
            columnCount = 0;
            continue;
        }

        ++columnCount;
    }

    mVisibleRows = mSize.y / (mItemSize.y + mItemSpacing.y);
    mVisibleRows -= (mVerticalMargin / mSize.y) * mVisibleRows * 2.0f;
    mVisibleRows += (mItemSpacing.y / mSize.y) * mVisibleRows;

    if (!mFractionalRows)
        mVisibleRows = std::floor(mVisibleRows);

    if (mVisibleRows == 0.0f)
        ++mVisibleRows;

    mLayoutValid = true;
    mJustCalculatedLayout = true;
}

template <typename T> bool GridComponent<T>::input(InputConfig* config, Input input)
{
    if (size() > 0) {
        if (input.value != 0) {
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
            if (config->isMappedLike("up", input)) {
                if (mCursor >= mColumns) {
                    if (mCancelTransitionsCallback)
                        mCancelTransitionsCallback();
                    List::listInput(-mColumns);
                }
                return true;
            }
            if (config->isMappedLike("down", input)) {
                const int columnModulus {size() % mColumns};
                if (mCursor < size() - (columnModulus == 0 ? mColumns : columnModulus)) {
                    if (mCancelTransitionsCallback)
                        mCancelTransitionsCallback();
                    List::listInput(mColumns);
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
            if (config->isMappedLike("left", input) || config->isMappedLike("right", input) ||
                config->isMappedLike("up", input) || config->isMappedLike("down", input) ||
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

template <typename T> void GridComponent<T>::update(int deltaTime)
{
    List::listUpdate(deltaTime);
    GuiComponent::update(deltaTime);
}

template <typename T> void GridComponent<T>::render(const glm::mat4& parentTrans)
{
    if (mEntries.empty())
        return;

    glm::mat4 trans {parentTrans * List::getTransform()};
    mRenderer->setMatrix(trans);

    // In image debug mode, draw a green rectangle covering the entire grid area.
    if (Settings::getInstance()->getBool("DebugImage"))
        mRenderer->drawRect(0.0f, 0.0f, mSize.x, mSize.y, 0x00FF0033, 0x00FF0033);

    // Clip to element boundaries.
    glm::vec3 dim {mSize.x, mSize.y, 0.0f};

    if (!mFractionalRows && mSize.y > mItemSize.y)
        dim.y = mVisibleRows * (mItemSize.y + mItemSpacing.y) + (mVerticalMargin * 2.0f) -
                mItemSpacing.y;

    dim.x = (trans[0].x * dim.x + trans[3].x) - trans[3].x;
    dim.y = (trans[1].y * dim.y + trans[3].y) - trans[3].y;

    mRenderer->pushClipRect(glm::ivec2 {static_cast<int>(trans[3].x), static_cast<int>(trans[3].y)},
                            glm::ivec2 {static_cast<int>(dim.x), static_cast<int>(dim.y)});

    // We want to render the currently selected item last and before that the last selected
    // item to avoid incorrect overlapping in case the element has been configured with for
    // example large scaling or small or no margins between items.
    std::vector<size_t> renderEntries;

    const int currRow {static_cast<int>(std::ceil(mScrollPos))};
    const int visibleRows {static_cast<int>(std::ceil(mVisibleRows))};
    int startPos {0};
    int loadItems {mColumns * visibleRows};
    int loadedItems {0};

    if (currRow > 0) {
        if (GuiComponent::isAnimationPlaying(0) || mItemSpacing.y < mVerticalMargin) {
            loadItems += mColumns;
            startPos = (currRow - 1) * mColumns;
        }
        else {
            if (mFractionalRows)
                startPos = (currRow - 1) * mColumns;
            else
                startPos = currRow * mColumns;
        }

        if (mItemSpacing.y < mVerticalMargin) {
            if (GuiComponent::isAnimationPlaying(0)) {
                loadItems += mColumns;
                startPos -= mColumns;
                if (startPos < 0)
                    startPos = 0;
            }
        }
    }

    if (!mFractionalRows && mItemSpacing.y < mVerticalMargin)
        loadItems += mColumns;

    for (int i {startPos}; i < size(); ++i) {
        if (loadedItems == loadItems)
            break;
        ++loadedItems;
        if (i == mCursor || i == mLastCursor)
            continue;
        renderEntries.emplace_back(i);
    }

    if (mLastCursor >= startPos && mLastCursor < startPos + loadItems)
        renderEntries.emplace_back(mLastCursor);
    if (mLastCursor != mCursor)
        renderEntries.emplace_back(mCursor);

    float scale {1.0f};
    float opacity {1.0f};
    float saturation {1.0f};
    float dimming {1.0f};

    trans[3].y -= (mItemSize.y + mItemSpacing.y) * mScrollPos;

    auto selectorRenderFunc = [this, &trans](std::vector<size_t>::const_iterator it,
                                             const float scale, const float opacity,
                                             const bool cursorEntry, const bool lastCursorEntry) {
        if (mSelectorImage != nullptr) {
            mSelectorImage->setPosition(mEntries.at(*it).data.item->getPosition());
            mSelectorImage->setScale(scale);
            mSelectorImage->setOpacity(opacity);
            mSelectorImage->render(trans);
        }
        else if (mHasSelectorColor) {
            // If a selector color is set but no selector image, then render a rectangle.
            const float sizeX {mItemSize.x * (cursorEntry || lastCursorEntry ? scale : 1.0f) *
                               mSelectorRelativeScale};
            const float sizeY {mItemSize.y * (cursorEntry || lastCursorEntry ? scale : 1.0f) *
                               mSelectorRelativeScale};
            float posX {mEntries.at(*it).data.item->getPosition().x - mItemSize.x * 0.5f};
            float posY {mEntries.at(*it).data.item->getPosition().y - mItemSize.y * 0.5f};

            if (cursorEntry || lastCursorEntry) {
                posX -= ((mItemSize.x * scale * mSelectorRelativeScale) - mItemSize.x) / 2.0f;
                posY -= ((mItemSize.y * scale * mSelectorRelativeScale) - mItemSize.y) / 2.0f;
            }
            else {
                posX -= ((mItemSize.x * mSelectorRelativeScale) - mItemSize.x) / 2.0f;
                posY -= ((mItemSize.y * mSelectorRelativeScale) - mItemSize.y) / 2.0f;
            }

            mRenderer->setMatrix(trans);
            mRenderer->drawRect(posX, posY, sizeX, sizeY, mSelectorColor, mSelectorColorEnd,
                                mSelectorColorGradientHorizontal, opacity);
        }
    };

    for (auto it = renderEntries.cbegin(); it != renderEntries.cend(); ++it) {
        float metadataOpacity {1.0f};
        bool cursorEntry {false};
        bool lastCursorEntry {false};

        if constexpr (std::is_same_v<T, FileData*>) {
            // If a game is marked as hidden, lower the opacity a lot.
            // If a game is marked to not be counted, lower the opacity a moderate amount.
            if (mEntries.at(*it).object->getHidden())
                metadataOpacity = 0.4f;
            else if (!mEntries.at(*it).object->getCountAsGame())
                metadataOpacity = 0.7f;
        }

        opacity = mUnfocusedItemOpacity * metadataOpacity;
        if (mHasUnfocusedItemSaturation)
            saturation = mUnfocusedItemSaturation;
        dimming = mUnfocusedItemDimming;

        if (*it == static_cast<size_t>(mCursor)) {
            cursorEntry = true;
            scale = glm::mix(1.0f, mItemScale, mTransitionFactor);
            opacity = glm::mix(mUnfocusedItemOpacity * metadataOpacity, 1.0f * metadataOpacity,
                               mTransitionFactor);
            if (mHasUnfocusedItemSaturation)
                saturation =
                    glm::mix(mUnfocusedItemSaturation, mImageSaturation, mTransitionFactor);
            dimming = glm::mix(mUnfocusedItemDimming, 1.0f, mTransitionFactor);
        }
        else if (*it == static_cast<size_t>(mLastCursor)) {
            lastCursorEntry = true;
            scale = glm::mix(mItemScale, 1.0f, mTransitionFactor);
            opacity = glm::mix(1.0f * metadataOpacity, mUnfocusedItemOpacity * metadataOpacity,
                               mTransitionFactor);
            if (mHasUnfocusedItemSaturation)
                saturation =
                    glm::mix(mImageSaturation, mUnfocusedItemSaturation, mTransitionFactor);
            dimming = glm::mix(1.0f, mUnfocusedItemDimming, mTransitionFactor);
        }

        if (cursorEntry && mSelectorLayer == SelectorLayer::BOTTOM)
            selectorRenderFunc(it, scale, opacity, cursorEntry, lastCursorEntry);

        if (mBackgroundImage != nullptr) {
            mBackgroundImage->setPosition(mEntries.at(*it).data.item->getPosition());
            mBackgroundImage->setScale(scale);
            mBackgroundImage->setOpacity(opacity);
            if (mHasUnfocusedItemSaturation)
                mBackgroundImage->setSaturation(saturation);
            if (mUnfocusedItemDimming != 1.0f)
                mBackgroundImage->setDimming(dimming);
            mBackgroundImage->render(trans);
        }
        else if (mHasBackgroundColor) {
            // If a background color is set but no background image, then render a rectangle.
            const float sizeX {mItemSize.x * (cursorEntry || lastCursorEntry ? scale : 1.0f) *
                               mBackgroundRelativeScale};
            const float sizeY {mItemSize.y * (cursorEntry || lastCursorEntry ? scale : 1.0f) *
                               mBackgroundRelativeScale};
            float posX {mEntries.at(*it).data.item->getPosition().x - mItemSize.x * 0.5f};
            float posY {mEntries.at(*it).data.item->getPosition().y - mItemSize.y * 0.5f};

            if (cursorEntry || lastCursorEntry) {
                posX -= ((mItemSize.x * scale * mBackgroundRelativeScale) - mItemSize.x) / 2.0f;
                posY -= ((mItemSize.y * scale * mBackgroundRelativeScale) - mItemSize.y) / 2.0f;
            }
            else {
                posX -= ((mItemSize.x * mBackgroundRelativeScale) - mItemSize.x) / 2.0f;
                posY -= ((mItemSize.y * mBackgroundRelativeScale) - mItemSize.y) / 2.0f;
            }

            mRenderer->setMatrix(trans);
            mRenderer->drawRect(posX, posY, sizeX, sizeY, mBackgroundColor, mBackgroundColorEnd,
                                mBackgroundColorGradientHorizontal, opacity);
        }

        if (cursorEntry && mSelectorLayer == SelectorLayer::MIDDLE)
            selectorRenderFunc(it, scale, opacity, cursorEntry, lastCursorEntry);

        mEntries.at(*it).data.item->setScale(scale);
        mEntries.at(*it).data.item->setOpacity(opacity);
        if (mHasUnfocusedItemSaturation)
            mEntries.at(*it).data.item->setSaturation(saturation);
        if (mUnfocusedItemDimming != 1.0f)
            mEntries.at(*it).data.item->setDimming(dimming);
        if (cursorEntry && (mHasTextSelectedColor || mHasImageSelectedColor)) {
            if (mHasTextSelectedColor && mEntries.at(*it).data.imagePath == "" &&
                mEntries.at(*it).data.defaultImagePath == "") {
                mEntries.at(*it).data.item->setColor(mTextSelectedColor);
                if (mTextSelectedBackgroundColor != mTextBackgroundColor)
                    mEntries.at(*it).data.item->setBackgroundColor(mTextSelectedBackgroundColor);
                mEntries.at(*it).data.item->render(trans);
                mEntries.at(*it).data.item->setColor(mTextColor);
                if (mTextSelectedBackgroundColor != mTextBackgroundColor)
                    mEntries.at(*it).data.item->setBackgroundColor(mTextBackgroundColor);
            }
            else if (mHasImageSelectedColor) {
                mEntries.at(*it).data.item->setColorShift(mImageSelectedColor);
                if (mImageSelectedColorEnd != mImageSelectedColor)
                    mEntries.at(*it).data.item->setColorShiftEnd(mImageSelectedColorEnd);
                if (mImageSelectedColorGradientHorizontal != mImageColorGradientHorizontal)
                    mEntries.at(*it).data.item->setColorGradientHorizontal(
                        mImageSelectedColorGradientHorizontal);
                mEntries.at(*it).data.item->render(trans);
                if (mImageSelectedColorGradientHorizontal != mImageColorGradientHorizontal)
                    mEntries.at(*it).data.item->setColorGradientHorizontal(
                        mImageColorGradientHorizontal);
                mEntries.at(*it).data.item->setColorShift(mImageColor);
                if (mImageColorEnd != mImageColor)
                    mEntries.at(*it).data.item->setColorShiftEnd(mImageColorEnd);
            }
            else {
                mEntries.at(*it).data.item->render(trans);
            }
        }
        else {
            mEntries.at(*it).data.item->render(trans);
        }
        mEntries.at(*it).data.item->setScale(1.0f);
        mEntries.at(*it).data.item->setOpacity(1.0f);

        if (cursorEntry && mSelectorLayer == SelectorLayer::TOP)
            selectorRenderFunc(it, scale, opacity, cursorEntry, lastCursorEntry);
    }

    mRenderer->popClipRect();
    GuiComponent::renderChildren(trans);
}

template <typename T>
void GridComponent<T>::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                  const std::string& view,
                                  const std::string& element,
                                  unsigned int properties)
{
    mSize.x = Renderer::getScreenWidth();
    mSize.y = Renderer::getScreenHeight() * 0.8f;
    GuiComponent::mPosition.x = 0.0f;
    GuiComponent::mPosition.y = Renderer::getScreenHeight() * 0.1f;
    mItemSpacing.x = ((mItemSize.x * mItemScale) - mItemSize.x) / 2.0f;
    mItemSpacing.y = ((mItemSize.y * mItemScale) - mItemSize.y) / 2.0f;
    mHorizontalMargin = ((mItemSize.x * mItemScale) - mItemSize.x) / 2.0f;
    mVerticalMargin = ((mItemSize.y * mItemScale) - mItemSize.y) / 2.0f;

    GuiComponent::applyTheme(theme, view, element, properties);

    using namespace ThemeFlags;
    const ThemeData::ThemeElement* elem {theme->getElement(view, element, "grid")};

    if (!elem)
        return;

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
            LOG(LogWarning) << "GridComponent: Invalid theme configuration, property \"imageType\" "
                               "for element \""
                            << element.substr(5) << "\" contains no values";
        }

        for (std::string& type : mImageTypes) {
            if (std::find(supportedImageTypes.cbegin(), supportedImageTypes.cend(), type) ==
                supportedImageTypes.cend()) {
                LOG(LogWarning)
                    << "GridComponent: Invalid theme configuration, property \"imageType\" "
                       "for element \""
                    << element.substr(5) << "\" defined as \"" << type << "\"";
                mImageTypes.clear();
                break;
            }
        }

        if (mImageTypes.size() == 2 && mImageTypes.front() == mImageTypes.back()) {
            LOG(LogError) << "GridComponent: Invalid theme configuration, property \"imageType\" "
                             "for element \""
                          << element.substr(5) << "\" contains duplicate values";
            mImageTypes.clear();
        }
    }

    mFractionalRows = (elem->has("fractionalRows") && elem->get<bool>("fractionalRows"));

    if (elem->has("itemSize")) {
        const glm::vec2& itemSize {elem->get<glm::vec2>("itemSize")};
        if (!(itemSize.x == -1 && itemSize.y == -1)) {
            if (itemSize.x == -1) {
                mItemSize.y =
                    glm::clamp(itemSize.y, 0.05f, 1.0f) * (mRenderer->getIsVerticalOrientation() ?
                                                               mRenderer->getScreenWidth() :
                                                               mRenderer->getScreenHeight());
                mItemSize.x = mItemSize.y;
            }
            else if (itemSize.y == -1) {
                mItemSize.x = glm::clamp(itemSize.x, 0.05f, 1.0f) * mRenderer->getScreenWidth();
                mItemSize.y = mItemSize.x;
            }
            else {
                mItemSize = glm::clamp(itemSize, 0.05f, 1.0f) *
                            glm::vec2(mRenderer->getScreenWidth(), mRenderer->getScreenHeight());
            }
        }
    }

    if (elem->has("itemScale"))
        mItemScale = glm::clamp(elem->get<float>("itemScale"), 0.5f, 2.0f);

    if (elem->has("imageRelativeScale"))
        mImageRelativeScale = glm::clamp(elem->get<float>("imageRelativeScale"), 0.2f, 1.0f);

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
            LOG(LogWarning) << "GridComponent: Invalid theme configuration, property "
                               "\"imageFit\" for element \""
                            << element.substr(5) << "\" defined as \"" << imageFit << "\"";
        }
    }

    if (elem->has("backgroundRelativeScale"))
        mBackgroundRelativeScale =
            glm::clamp(elem->get<float>("backgroundRelativeScale"), 0.2f, 1.0f);

    mHasBackgroundColor = false;

    if (elem->has("backgroundColor")) {
        mHasBackgroundColor = true;
        mBackgroundColor = elem->get<unsigned int>("backgroundColor");
        mBackgroundColorEnd = mBackgroundColor;
    }
    if (elem->has("backgroundColorEnd"))
        mBackgroundColorEnd = elem->get<unsigned int>("backgroundColorEnd");

    if (elem->has("backgroundGradientType")) {
        const std::string& gradientType {elem->get<std::string>("backgroundGradientType")};
        if (gradientType == "horizontal") {
            mBackgroundColorGradientHorizontal = true;
        }
        else if (gradientType == "vertical") {
            mBackgroundColorGradientHorizontal = false;
        }
        else {
            mBackgroundColorGradientHorizontal = true;
            LOG(LogWarning) << "GridComponent: Invalid theme configuration, property "
                               "\"backgroundGradientType\" for element \""
                            << element.substr(5) << "\" defined as \"" << gradientType << "\"";
        }
    }

    if (elem->has("selectorRelativeScale"))
        mSelectorRelativeScale = glm::clamp(elem->get<float>("selectorRelativeScale"), 0.2f, 1.0f);

    mHasSelectorColor = false;

    if (elem->has("selectorColor")) {
        mHasSelectorColor = true;
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
            LOG(LogWarning) << "GridComponent: Invalid theme configuration, property "
                               "\"selectorGradientType\" for element \""
                            << element.substr(5) << "\" defined as \"" << gradientType << "\"";
        }
    }

    if (elem->has("backgroundImage")) {
        const std::string& path {elem->get<std::string>("backgroundImage")};
        if (Utils::FileSystem::exists(path) && !Utils::FileSystem::isDirectory(path)) {
            mBackgroundImage = std::make_unique<ImageComponent>(false, false);
            mBackgroundImage->setLinearInterpolation(true);
            mBackgroundImage->setResize(mItemSize * mBackgroundRelativeScale);
            mBackgroundImage->setOrigin(0.5f, 0.5f);
            if (mHasBackgroundColor) {
                mBackgroundImage->setColorShift(mBackgroundColor);
                if (mBackgroundColor != mBackgroundColorEnd) {
                    mBackgroundImage->setColorShiftEnd(mBackgroundColorEnd);
                    if (!mBackgroundColorGradientHorizontal)
                        mBackgroundImage->setColorGradientHorizontal(false);
                }
            }
            mBackgroundImage->setImage(elem->get<std::string>("backgroundImage"));
            mBackgroundImagePath = path;
        }
        else {
            LOG(LogWarning) << "GridComponent: Invalid theme configuration, property "
                               "\"backgroundImage\" for element \""
                            << element.substr(5) << "\", image does not exist: \"" << path << "\"";
        }
    }

    if (elem->has("selectorImage")) {
        const std::string& path {elem->get<std::string>("selectorImage")};
        if (Utils::FileSystem::exists(path) && !Utils::FileSystem::isDirectory(path)) {
            mSelectorImage = std::make_unique<ImageComponent>(false, false);
            mSelectorImage->setLinearInterpolation(true);
            mSelectorImage->setResize(mItemSize * mSelectorRelativeScale);
            mSelectorImage->setOrigin(0.5f, 0.5f);
            if (mHasSelectorColor) {
                mSelectorImage->setColorShift(mSelectorColor);
                if (mBackgroundColor != mBackgroundColorEnd) {
                    mSelectorImage->setColorShiftEnd(mSelectorColorEnd);
                    if (!mSelectorColorGradientHorizontal)
                        mSelectorImage->setColorGradientHorizontal(false);
                }
            }
            mSelectorImage->setImage(elem->get<std::string>("selectorImage"));
            mSelectorImagePath = path;
        }
        else {
            LOG(LogWarning) << "GridComponent: Invalid theme configuration, property "
                               "\"selectorImage\" for element \""
                            << element.substr(5) << "\", image does not exist: \"" << path << "\"";
        }
    }

    if (elem->has("selectorLayer")) {
        const std::string& selectorLayer {elem->get<std::string>("selectorLayer")};
        if (selectorLayer == "top") {
            mSelectorLayer = SelectorLayer::TOP;
        }
        else if (selectorLayer == "middle") {
            mSelectorLayer = SelectorLayer::MIDDLE;
        }
        else if (selectorLayer == "bottom") {
            mSelectorLayer = SelectorLayer::BOTTOM;
        }
        else {
            mSelectorLayer = SelectorLayer::TOP;
            LOG(LogWarning) << "GridComponent: Invalid theme configuration, property "
                               "\"selectorLayer\" for element \""
                            << element.substr(5) << "\" defined as \"" << selectorLayer << "\"";
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
            LOG(LogWarning) << "GridComponent: Invalid theme configuration, property "
                               "\"itemTransitions\" for element \""
                            << element.substr(5) << "\" defined as \"" << itemTransitions << "\"";
        }
    }

    if (elem->has("rowTransitions")) {
        const std::string& rowTransitions {elem->get<std::string>("rowTransitions")};
        if (rowTransitions == "animate") {
            mInstantRowTransitions = false;
        }
        else if (rowTransitions == "instant") {
            mInstantRowTransitions = true;
        }
        else {
            mInstantRowTransitions = false;
            LOG(LogWarning) << "GridComponent: Invalid theme configuration, property "
                               "\"rowTransitions\" for element \""
                            << element.substr(5) << "\" defined as \"" << rowTransitions << "\"";
        }
    }

    // If itemSpacing is not defined, then it's automatically calculated so that scaled items
    // don't overlap. If the property is present but one axis is defined as -1 then set this
    // axis to the same pixel value as the other axis.
    if (elem->has("itemSpacing")) {
        const glm::vec2& itemSpacing {elem->get<glm::vec2>("itemSpacing")};
        if (itemSpacing.x == -1 && itemSpacing.y == -1) {
            mItemSpacing = {0.0f, 0.0f};
        }
        else if (itemSpacing.x == -1) {
            mItemSpacing.y = glm::clamp(itemSpacing.y, 0.0f, 0.1f) * mRenderer->getScreenHeight();
            mItemSpacing.x = mItemSpacing.y;
        }
        else if (itemSpacing.y == -1) {
            mItemSpacing.x = glm::clamp(itemSpacing.x, 0.0f, 0.1f) * mRenderer->getScreenWidth();
            mItemSpacing.y = mItemSpacing.x;
        }
        else {
            mItemSpacing = glm::clamp(itemSpacing, 0.0f, 0.1f) *
                           glm::vec2(Renderer::getScreenWidth(), Renderer::getScreenHeight());
        }
    }
    else if (mItemScale < 1.0f) {
        mItemSpacing = glm::vec2 {0.0f, 0.0f};
    }
    else {
        mItemSpacing.x = ((mItemSize.x * mItemScale) - mItemSize.x) / 2.0f;
        mItemSpacing.y = ((mItemSize.y * mItemScale) - mItemSize.y) / 2.0f;
    }

    if (elem->has("imageColor")) {
        mImageColor = elem->get<unsigned int>("imageColor");
        mImageColorEnd = mImageColor;
    }
    if (elem->has("imageColorEnd"))
        mImageColorEnd = elem->get<unsigned int>("imageColorEnd");

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
            LOG(LogWarning) << "GridComponent: Invalid theme configuration, property "
                               "\"imageGradientType\" for element \""
                            << element.substr(5) << "\" defined as \"" << gradientType << "\"";
        }
    }

    mImageSelectedColor = mImageColor;
    mImageSelectedColorEnd = mImageColorEnd;

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
            LOG(LogWarning) << "GridComponent: Invalid theme configuration, property "
                               "\"imageSelectedGradientType\" for element \""
                            << element.substr(5) << "\" defined as \"" << gradientType << "\"";
        }
    }

    if (elem->has("imageBrightness"))
        mImageBrightness = glm::clamp(elem->get<float>("imageBrightness"), -2.0f, 2.0f);

    if (elem->has("imageSaturation"))
        mImageSaturation = glm::clamp(elem->get<float>("imageSaturation"), 0.0f, 1.0f);

    if (elem->has("unfocusedItemOpacity"))
        mUnfocusedItemOpacity = glm::clamp(elem->get<float>("unfocusedItemOpacity"), 0.1f, 1.0f);

    if (elem->has("unfocusedItemSaturation")) {
        mUnfocusedItemSaturation =
            glm::clamp(elem->get<float>("unfocusedItemSaturation"), 0.0f, 1.0f);
        mHasUnfocusedItemSaturation = true;
    }

    if (elem->has("unfocusedItemDimming"))
        mUnfocusedItemDimming = glm::clamp(elem->get<float>("unfocusedItemDimming"), 0.0f, 1.0f);

    mFont = Font::getFromTheme(elem, properties, mFont, 0.0f, (mItemScale > 1.0f));

    if (elem->has("textRelativeScale"))
        mTextRelativeScale = glm::clamp(elem->get<float>("textRelativeScale"), 0.2f, 1.0f);

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
            LOG(LogWarning) << "GridComponent: Invalid theme configuration, property "
                               "\"letterCase\" for element \""
                            << element.substr(5) << "\" defined as \"" << letterCase << "\"";
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
            LOG(LogWarning) << "GridComponent: Invalid theme configuration, property "
                               "\"letterCaseAutoCollections\" for element \""
                            << element.substr(5) << "\" defined as \"" << letterCase << "\"";
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
            LOG(LogWarning) << "GridComponent: Invalid theme configuration, property "
                               "\"letterCaseCustomCollections\" for element \""
                            << element.substr(5) << "\" defined as \"" << letterCase << "\"";
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
            LOG(LogWarning) << "GridComponent: Invalid theme configuration, property "
                               "\"letterCaseSystemNameSuffix\" for element \""
                            << element.substr(5) << "\" defined as \"" << letterCase << "\"";
        }
    }

    if (elem->has("fadeAbovePrimary"))
        mFadeAbovePrimary = elem->get<bool>("fadeAbovePrimary");

    mSize.x = glm::clamp(mSize.x, mRenderer->getScreenWidth() * 0.05f,
                         mRenderer->getScreenWidth() * 1.0f);
    mSize.y = glm::clamp(mSize.y, mRenderer->getScreenHeight() * 0.05f,
                         mRenderer->getScreenHeight() * 1.0f);
}

template <typename T> void GridComponent<T>::onCursorChanged(const CursorState& state)
{
    if (mColumns == 0)
        return;

    if (mWasScrolling && state == CursorState::CURSOR_STOPPED && mScrollVelocity != 0) {
        mWasScrolling = false;
        if (mCursorChangedCallback)
            mCursorChangedCallback(state);
        return;
    }

    if (mCursor == mLastCursor && !mJustCalculatedLayout) {
        mWasScrolling = false;
        if (mCursorChangedCallback)
            mCursorChangedCallback(state);
        return;
    }
    else {
        mJustCalculatedLayout = false;
    }

    float startPos {mEntryOffset};
    float posMax {static_cast<float>(mEntries.size())};
    float target {static_cast<float>(mCursor)};

    // Find the shortest path to the target.
    float endPos {target}; // Directly.

    if (mPreviousScrollVelocity > 0 && mScrollVelocity == 0 && mEntryOffset > posMax - 1.0f)
        startPos = 0.0f;

    float dist {std::fabs(endPos - startPos)};

    if (std::fabs(target + posMax - startPos - mScrollVelocity) < dist)
        endPos = target + posMax; // Loop around the end (0 -> max).
    if (std::fabs(target - posMax - startPos - mScrollVelocity) < dist)
        endPos = target - posMax; // Loop around the start (max - 1 -> -1).

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

    float animTime {250.0f};

    // If startPos is inbetween two positions then reduce the time slightly as the distance will
    // be shorter meaning the animation would play for too long if not compensated for.
    //    float timeDiff {1.0f};
    //    if (mScrollVelocity == 1)
    //        timeDiff = endPos - startPos;
    //    else if (mScrollVelocity == -1)
    //        timeDiff = startPos - endPos;
    //    if (timeDiff != 1.0f)
    //        animTime =
    //            glm::clamp(std::fabs(glm::mix(0.0f, animTime, timeDiff * 1.5f)), 180.0f,
    //            animTime);

    if (mSuppressTransitions)
        animTime = 0.0f;

    const float visibleRows {mVisibleRows - 1.0f};
    const float startRow {static_cast<float>(mScrollPos)};
    float endRow {static_cast<float>(mCursor / mColumns)};

    if (endRow <= visibleRows)
        endRow = 0.0f;
    else
        endRow -= visibleRows;

    Animation* anim {new LambdaAnimation(
        [this, startPos, endPos, posMax, startRow, endRow](float t) {
            // Non-linear interpolation.
            t = 1.0f - (1.0f - t) * (1.0f - t);
            float f {(endPos * t) + (startPos * (1.0f - t))};

            if (f < 0)
                f += posMax;
            if (f >= posMax)
                f -= posMax;

            mEntryOffset = f;

            if (mInstantRowTransitions)
                mScrollPos = endRow;
            else
                mScrollPos = {(endRow * t) + (startRow * (1.0f - t))};

            if (mInstantItemTransitions) {
                mTransitionFactor = 1.0f;
            }
            else {
                // Linear interpolation.
                mTransitionFactor = t;
                // Non-linear interpolation doesn't seem to be a good match for this component.
                // mTransitionFactor = {(1.0f * t) + (0.0f * (1.0f - t))};
            }
        },
        static_cast<int>(animTime))};

    GuiComponent::setAnimation(anim, 0, nullptr, false, 0);

    if (mCursorChangedCallback)
        mCursorChangedCallback(state);

    mWasScrolling = (state == CursorState::CURSOR_SCROLLING);
}

#endif // ES_CORE_COMPONENTS_PRIMARY_GRID_COMPONENT_H
