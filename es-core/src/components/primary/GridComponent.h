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
    std::string itemPath;
    std::string defaultItemPath;
};

template <typename T>
class GridComponent : public PrimaryComponent<T>, protected IList<GridEntry, T>
{
    using List = IList<GridEntry, T>;

protected:
    using List::mCursor;
    using List::mEntries;
    using List::mLastCursor;
    using List::mScrollVelocity;
    using List::mSize;

public:
    using Entry = typename IList<GridEntry, T>::Entry;

    GridComponent();

    void addEntry(Entry& entry, const std::shared_ptr<ThemeData>& theme);
    void updateEntry(Entry& entry, const std::shared_ptr<ThemeData>& theme);
    void onDemandTextureLoad() override;

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
    const LetterCase getLetterCaseCollections() const override { return mLetterCaseCollections; }
    const LetterCase getLetterCaseGroupedCollections() const override
    {
        return mLetterCaseGroupedCollections;
    }
    const std::string& getItemType() { return mItemType; }
    void setItemType(std::string itemType) { mItemType = itemType; }
    const std::string& getDefaultItem() { return mDefaultItem; }
    void setDefaultItem(std::string defaultItem) { mDefaultItem = defaultItem; }
    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;
    void applyTheme(const std::shared_ptr<ThemeData>& theme,
                    const std::string& view,
                    const std::string& element,
                    unsigned int properties) override;

private:
    void calculateLayout();

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

    Renderer* mRenderer;
    std::function<void()> mCancelTransitionsCallback;
    std::function<void(CursorState state)> mCursorChangedCallback;

    std::string mItemType;
    std::string mDefaultItem;
    float mEntryOffset;
    float mEntryCamTarget;
    float mTransitionFactor;
    std::shared_ptr<Font> mFont;

    unsigned int mColumns;
    glm::vec2 mItemSize;
    float mItemScale;
    glm::vec2 mItemSpacing;
    bool mInstantItemTransitions;
    float mUnfocusedItemOpacity;
    unsigned int mTextColor;
    unsigned int mTextBackgroundColor;
    LetterCase mLetterCase;
    LetterCase mLetterCaseCollections;
    LetterCase mLetterCaseGroupedCollections;
    float mLineSpacing;
    bool mFadeAbovePrimary;
    int mPreviousScrollVelocity;
    bool mPositiveDirection;
    bool mGamelistView;
    bool mLayoutValid;
    bool mRowJump;
    bool mWasScrolling;
};

template <typename T>
GridComponent<T>::GridComponent()
    : IList<GridEntry, T> {LIST_SCROLL_STYLE_SLOW, ListLoopType::LIST_PAUSE_AT_END}
    , mRenderer {Renderer::getInstance()}
    , mEntryOffset {0.0f}
    , mEntryCamTarget {0.0f}
    , mTransitionFactor {1.0f}
    , mFont {Font::get(FONT_SIZE_LARGE)}
    , mColumns {5}
    , mItemSize {glm::vec2 {Renderer::getScreenWidth() * 0.15f,
                            Renderer::getScreenHeight() * 0.25f}}
    , mItemScale {1.05f}
    , mItemSpacing {glm::vec2 {Renderer::getScreenWidth() * 0.02f,
                               Renderer::getScreenHeight() * 0.02f}}
    , mInstantItemTransitions {false}
    , mUnfocusedItemOpacity {1.0f}
    , mTextColor {0x000000FF}
    , mTextBackgroundColor {0xFFFFFF00}
    , mLetterCase {LetterCase::NONE}
    , mLetterCaseCollections {LetterCase::NONE}
    , mLetterCaseGroupedCollections {LetterCase::NONE}
    , mLineSpacing {1.5f}
    , mFadeAbovePrimary {false}
    , mPreviousScrollVelocity {0}
    , mPositiveDirection {false}
    , mGamelistView {std::is_same_v<T, FileData*> ? true : false}
    , mLayoutValid {false}
    , mRowJump {false}
    , mWasScrolling {false}
{
}

template <typename T>
void GridComponent<T>::addEntry(Entry& entry, const std::shared_ptr<ThemeData>& theme)
{
    bool dynamic {true};

    if (!mGamelistView)
        dynamic = false;

    if (entry.data.itemPath != "" &&
        ResourceManager::getInstance().fileExists(entry.data.itemPath)) {
        auto item = std::make_shared<ImageComponent>(false, dynamic);
        item->setLinearInterpolation(true);
        item->setMipmapping(true);
        item->setMaxSize(mItemSize);
        item->setImage(entry.data.itemPath);
        item->applyTheme(theme, "system", "", ThemeFlags::ALL);
        item->setOrigin(0.5f, 0.5f);
        item->setRotateByTargetSize(true);
        entry.data.item = item;
    }
    else if (entry.data.defaultItemPath != "" &&
             ResourceManager::getInstance().fileExists(entry.data.defaultItemPath)) {
        auto defaultItem = std::make_shared<ImageComponent>(false, dynamic);
        defaultItem->setLinearInterpolation(true);
        defaultItem->setMipmapping(true);
        defaultItem->setMaxSize(mItemSize);
        defaultItem->setImage(entry.data.defaultItemPath);
        defaultItem->applyTheme(theme, "system", "", ThemeFlags::ALL);
        defaultItem->setOrigin(0.5f, 0.5f);
        defaultItem->setRotateByTargetSize(true);
        entry.data.item = defaultItem;
    }

    if (!entry.data.item) {
        // If no item image is present, add item text as fallback.
        auto text = std::make_shared<TextComponent>(
            entry.name, mFont, 0x000000FF, Alignment::ALIGN_CENTER, Alignment::ALIGN_CENTER,
            glm::vec3 {0.0f, 0.0f, 0.0f}, mItemSize, 0x00000000);
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
    if (entry.data.itemPath != "") {
        auto item = std::make_shared<ImageComponent>(false, true);
        item->setLinearInterpolation(true);
        item->setMipmapping(true);
        item->setMaxSize(mItemSize);
        item->setImage(entry.data.itemPath);
        item->applyTheme(theme, "system", "", ThemeFlags::ALL);
        item->setOrigin(0.5f, 0.5f);
        item->setRotateByTargetSize(true);
        entry.data.item = item;
    }
    else {
        return;
    }
}

template <typename T> void GridComponent<T>::onDemandTextureLoad()
{
    if constexpr (std::is_same_v<T, FileData*>) {
        const int numEntries {static_cast<int>(mEntries.size())};

        // TODO: Currently loads every item every time.
        for (int i {0}; i < size(); ++i) {
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

template <typename T> bool GridComponent<T>::input(InputConfig* config, Input input)
{
    if (size() > 0) {
        if (input.value != 0) {
            mRowJump = false;

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
                if (mCancelTransitionsCallback)
                    mCancelTransitionsCallback();
                mRowJump = true;
                List::listInput(-mColumns);
                return true;
            }
            if (config->isMappedLike("down", input)) {
                if (mCancelTransitionsCallback)
                    mCancelTransitionsCallback();
                mRowJump = true;
                List::listInput(mColumns);
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
                if constexpr (std::is_same_v<T, SystemData*>) {
                    if (isScrolling())
                        onCursorChanged(CursorState::CURSOR_STOPPED);
                    List::listInput(0);
                }
                else {
                    if (isScrolling())
                        onCursorChanged(CursorState::CURSOR_STOPPED);
                    List::listInput(0);
                }
            }
        }
    }

    return GuiComponent::input(config, input);
}

template <typename T> void GridComponent<T>::update(int deltaTime)
{
    if (!mLayoutValid)
        calculateLayout();

    List::listUpdate(deltaTime);
    GuiComponent::update(deltaTime);
}

template <typename T> void GridComponent<T>::render(const glm::mat4& parentTrans)
{
    int numEntries {static_cast<int>(mEntries.size())};
    if (numEntries == 0)
        return;

    glm::mat4 trans {parentTrans * List::getTransform()};
    mRenderer->setMatrix(trans);

    // In image debug mode, draw a green rectangle covering the entire grid area.
    if (Settings::getInstance()->getBool("DebugImage"))
        mRenderer->drawRect(0.0f, 0.0f, mSize.x, mSize.y, 0x00FF0033, 0x00FF0033);

    // We want to render the currently selected item last and before that the last selected
    // item to avoid incorrect overlapping in case the element has been configured with for
    // example large scaling or small or no margins between items.
    std::vector<size_t> renderEntries;

    for (size_t i {0}; i < mEntries.size(); ++i) {
        if (i == static_cast<size_t>(mCursor) || i == static_cast<size_t>(mLastCursor))
            continue;
        renderEntries.emplace_back(i);
    }

    renderEntries.emplace_back(mLastCursor);
    if (mLastCursor != mCursor)
        renderEntries.emplace_back(mCursor);

    float opacity {mUnfocusedItemOpacity};
    float scale {1.0f};

    for (auto it = renderEntries.cbegin(); it != renderEntries.cend(); ++it) {
        if (*it == static_cast<size_t>(mCursor)) {
            scale = glm::mix(1.0f, mItemScale, mTransitionFactor);
            opacity = glm::mix(mUnfocusedItemOpacity, 1.0f, mTransitionFactor);
        }
        else if (*it == static_cast<size_t>(mLastCursor)) {
            scale = glm::mix(mItemScale, 1.0f, mTransitionFactor);
            opacity = glm::mix(1.0f, mUnfocusedItemOpacity, mTransitionFactor);
        }

        mEntries.at(*it).data.item->setScale(scale);
        mEntries.at(*it).data.item->setOpacity(opacity);
        mEntries.at(*it).data.item->render(trans);
        mEntries.at(*it).data.item->setScale(1.0f);
        mEntries.at(*it).data.item->setOpacity(1.0f);
    }

    GuiComponent::renderChildren(trans);
}

template <typename T>
void GridComponent<T>::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                  const std::string& view,
                                  const std::string& element,
                                  unsigned int properties)
{
    mSize.x = Renderer::getScreenWidth();
    mSize.y = Renderer::getScreenHeight() * 0.8;
    GuiComponent::mPosition.x = 0.0f;
    GuiComponent::mPosition.y = Renderer::getScreenHeight() * 0.1;

    GuiComponent::applyTheme(theme, view, element, properties);

    using namespace ThemeFlags;
    const ThemeData::ThemeElement* elem {theme->getElement(view, element, "grid")};

    if (!elem)
        return;

    if (elem->has("columns"))
        mColumns = glm::clamp(elem->get<unsigned int>("columns"), 0u, 100u);

    if (elem->has("itemSize")) {
        const glm::vec2 itemSize {glm::clamp(elem->get<glm::vec2>("itemSize"), 0.05f, 1.0f)};
        mItemSize = itemSize * glm::vec2(Renderer::getScreenWidth(), Renderer::getScreenHeight());
    }

    if (elem->has("itemScale"))
        mItemScale = glm::clamp(elem->get<float>("itemScale"), 0.5f, 2.0f);

    if (elem->has("itemTransitions")) {
        const std::string& itemTransitions {elem->get<std::string>("itemTransitions")};
        if (itemTransitions == "scale") {
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

    if (elem->has("itemSpacing")) {
        const glm::vec2 itemSpacing {glm::clamp(elem->get<glm::vec2>("itemSpacing"), 0.0f, 0.1f)};
        mItemSpacing =
            itemSpacing * glm::vec2(Renderer::getScreenWidth(), Renderer::getScreenHeight());
    }

    if (elem->has("unfocusedItemOpacity"))
        mUnfocusedItemOpacity = glm::clamp(elem->get<float>("unfocusedItemOpacity"), 0.1f, 1.0f);
}

template <typename T> void GridComponent<T>::onCursorChanged(const CursorState& state)
{
    if (mWasScrolling && state == CursorState::CURSOR_STOPPED) {
        if (mCursorChangedCallback)
            mCursorChangedCallback(state);
        mWasScrolling = false;
        return;
    }

    if (mCursor == mLastCursor)
        return;

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

    mEntryCamTarget = endPos;
    float animTime {250.0f};
    float timeDiff {1.0f};

    // If startPos is inbetween two positions then reduce the time slightly as the distance will
    // be shorter meaning the animation would play for too long if not compensated for.
    if (mScrollVelocity == 1)
        timeDiff = endPos - startPos;
    else if (mScrollVelocity == -1)
        timeDiff = startPos - endPos;

    if (timeDiff != 1.0f)
        animTime =
            glm::clamp(std::fabs(glm::mix(0.0f, animTime, timeDiff * 1.5f)), 180.0f, animTime);

    Animation* anim {new LambdaAnimation(
        [this, startPos, endPos, posMax](float t) {
            // Non-linear interpolation.
            t = 1.0f - (1.0f - t) * (1.0f - t);
            float f {(endPos * t) + (startPos * (1.0f - t))};
            if (f < 0)
                f += posMax;
            if (f >= posMax)
                f -= posMax;

            mEntryOffset = f;

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

template <typename T> void GridComponent<T>::calculateLayout()
{
    assert(!mEntries.empty());

    unsigned int columnCount {0};
    unsigned int rowCount {0};

    for (auto& entry : mEntries) {
        entry.data.item->setPosition(glm::vec3 {
            (mItemSize.x * columnCount) + (mItemSize.x * 0.5f) + mItemSpacing.x * columnCount,
            (mItemSize.y * rowCount) + (mItemSize.y * 0.5f) + mItemSpacing.y * rowCount, 0.0f});
        if (columnCount == mColumns - 1) {
            ++rowCount;
            columnCount = 0;
            continue;
        }

        ++columnCount;
    }

    mLayoutValid = true;
}

#endif // ES_CORE_COMPONENTS_PRIMARY_GRID_COMPONENT_H
