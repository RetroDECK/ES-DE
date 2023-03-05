//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  IList.h
//
//  List base class, used by the system view, gamelist view and menu system.
//

#ifndef ES_CORE_COMPONENTS_ILIST_H
#define ES_CORE_COMPONENTS_ILIST_H

#include "Window.h"
#include "components/ImageComponent.h"
#include "utils/StringUtil.h"

enum class CursorState {
    CURSOR_STOPPED,
    CURSOR_SCROLLING
};

enum class ListLoopType {
    LIST_ALWAYS_LOOP,
    LIST_PAUSE_AT_END,
    LIST_PAUSE_AT_END_ON_JUMP,
    LIST_NEVER_LOOP
};

template <typename EntryData, typename UserData> class IList : public virtual GuiComponent
{
public:
    struct Entry {
        std::string name;
        UserData object;
        EntryData data;
    };

protected:
    struct ScrollTier {
        int length; // How long we stay on this tier before going to the next.
        int scrollDelay; // How long between scrolls.
    };

    struct ScrollTierList {
        int count;
        const ScrollTier* tiers;
    };

    // Default scroll tiers.
    // clang-format off
    static inline const ScrollTier QUICK_SCROLL_TIERS[3] {
        {500, 500},
        {1200, 114},
        {0, 16}
    };

    static inline const ScrollTierList LIST_SCROLL_STYLE_QUICK {
        3,
        QUICK_SCROLL_TIERS
    };

    static inline const ScrollTier MEDIUM_SCROLL_TIERS[3] {
        {500, 500},
        {1100, 180},
        {0, 80}
    };

    static inline const ScrollTierList LIST_SCROLL_STYLE_MEDIUM {
        3,
        MEDIUM_SCROLL_TIERS
    };

    static inline const ScrollTier SLOW_SCROLL_TIERS[2] {
        {500, 500},
        {0, 200}
    };

    static inline const ScrollTierList LIST_SCROLL_STYLE_SLOW {
        2,
        SLOW_SCROLL_TIERS
    };
    // clang-format on

    Window* mWindow;
    std::vector<Entry> mEntries;
    ScrollTierList mTierList;
    const ListLoopType mLoopType;
    int mCursor;
    int mLastCursor;
    int mColumns;
    int mRows;
    int mScrollTier;
    int mScrollVelocity;
    int mScrollTierAccumulator;
    int mScrollCursorAccumulator;

    float mTitleOverlayOpacity;
    unsigned int mTitleOverlayColor;

public:
    IList(const ScrollTierList& tierList = LIST_SCROLL_STYLE_QUICK,
          const ListLoopType& loopType = ListLoopType::LIST_PAUSE_AT_END)
        : mWindow {Window::getInstance()}
        , mTierList {tierList}
        , mLoopType {loopType}
        , mCursor {0}
        , mLastCursor {0}
        , mColumns {0}
        , mRows {0}
        , mScrollTier {0}
        , mScrollVelocity {0}
        , mScrollTierAccumulator {0}
        , mScrollCursorAccumulator {0}
        , mTitleOverlayOpacity {0.0f}
        , mTitleOverlayColor {0xFFFFFF00}
    {
    }

    const bool isScrolling() const { return (mScrollVelocity != 0 && mScrollTier > 0); }

    void stopScrolling()
    {
        mTitleOverlayOpacity = 0.0f;

        if (mScrollVelocity == 0)
            return;

        listInput(0);
        if (mScrollVelocity == 0)
            onCursorChanged(CursorState::CURSOR_STOPPED);
    }

    const int getScrollingVelocity() const { return mScrollVelocity; }

    void clear()
    {
        mEntries.clear();
        mCursor = 0;
        mLastCursor = 0;
        listInput(0);
        onCursorChanged(CursorState::CURSOR_STOPPED);
    }

    const std::string& getSelectedName()
    {
        assert(size() > 0);
        return mEntries.at(mCursor).name;
    }

    const UserData& getSelected() const
    {
        assert(size() > 0);
        return mEntries.at(mCursor).object;
    }

    const UserData& getNext() const
    {
        // If there is a next entry, then return it, otherwise return the current entry.
        if (mCursor + 1 < static_cast<int>(mEntries.size()))
            return mEntries.at(mCursor + 1).object;
        else
            return mEntries.at(mCursor).object;
    }

    const UserData& getPrevious() const
    {
        // If there is a previous entry, then return it, otherwise return the current entry.
        if (mCursor != 0)
            return mEntries.at(mCursor - 1).object;
        else
            return mEntries.at(mCursor).object;
    }

    const UserData& getFirst() const
    {
        assert(size() > 0);
        return mEntries.front().object;
    }

    const UserData& getLast() const
    {
        assert(size() > 0);
        return mEntries.back().object;
    }

    void setCursor(typename std::vector<Entry>::const_iterator& it)
    {
        assert(it != mEntries.cend());
        mCursor = it - mEntries.cbegin();
        onCursorChanged(CursorState::CURSOR_STOPPED);
    }

    bool setCursor(const UserData& obj)
    {
        for (auto it = mEntries.cbegin(); it != mEntries.cend(); ++it) {
            if ((*it).object == obj) {
                mCursor = static_cast<int>(it - mEntries.cbegin());
                onCursorChanged(CursorState::CURSOR_STOPPED);
                return true;
            }
        }

        return false;
    }

    // Entry management.
    void add(const Entry& e) { mEntries.push_back(e); }

    bool remove(const UserData& obj)
    {
        for (auto it = mEntries.cbegin(); it != mEntries.cend(); ++it) {
            if ((*it).object == obj) {
                remove(it);
                return true;
            }
        }

        return false;
    }

    int size() const { return static_cast<int>(mEntries.size()); }

protected:
    void remove(typename std::vector<Entry>::const_iterator& it)
    {
        if (mCursor > 0 && it - mEntries.cbegin() <= mCursor) {
            --mCursor;
            onCursorChanged(CursorState::CURSOR_STOPPED);
        }

        mEntries.erase(it);
    }

    bool listFirstRow()
    {
        mLastCursor = mCursor;
        mCursor = 0;
        mScrollVelocity = 0;
        mScrollTier = 0;
        onCursorChanged(CursorState::CURSOR_STOPPED);
        onScroll();
        return true;
    }

    bool listLastRow()
    {
        mLastCursor = mCursor;
        mCursor = static_cast<int>(mEntries.size()) - 1;
        mScrollVelocity = 0;
        mScrollTier = 0;
        onCursorChanged(CursorState::CURSOR_STOPPED);
        onScroll();
        return true;
    }

    bool listInput(int velocity) // A velocity of 0 = stop scrolling.
    {
        mScrollVelocity = velocity;
        mScrollTier = 0;
        mScrollTierAccumulator = 0;
        mScrollCursorAccumulator = 0;

        int prevCursor = mCursor;
        scroll(mScrollVelocity);
        return (prevCursor != mCursor);
    }

    void listUpdate(int deltaTime)
    {
        // Update the title overlay opacity.
        // Fade in if scroll tier is >= 1, otherwise fade out.
        const float dir {(mScrollTier >= mTierList.count - 1) ? 1.0f : -1.0f};
        // We simply translate the time directly to opacity, i.e. no scaling is performed.
        mTitleOverlayOpacity = glm::clamp(
            mTitleOverlayOpacity + (static_cast<float>(deltaTime) / 255.0f) * dir, 0.0f, 1.0f);

        if (mScrollVelocity == 0 || size() < 2)
            return;

        mScrollCursorAccumulator += deltaTime;
        mScrollTierAccumulator += deltaTime;

        // We delay scrolling until after scroll tier has updated so isScrolling() returns
        // accurately during onCursorChanged callbacks. We don't just do scroll tier first
        // because it would not catch the scrollDelay == tier length case.
        int scrollCount {0};
        while (mScrollCursorAccumulator >= mTierList.tiers[mScrollTier].scrollDelay) {
            mScrollCursorAccumulator -= mTierList.tiers[mScrollTier].scrollDelay;
            ++scrollCount;
        }

        // Should we go to the next scrolling tier?
        while (mScrollTier < mTierList.count - 1 &&
               mScrollTierAccumulator >= mTierList.tiers[mScrollTier].length) {
            mScrollTierAccumulator -= mTierList.tiers[mScrollTier].length;
            // This is required for the carousel as the highest tier introduces weird behavior
            // if there are only two entries.
            if (!(mScrollTier > 0 && mEntries.size() < 3))
                ++mScrollTier;
        }

        // Actually perform the scrolling.
        for (int i {0}; i < scrollCount; ++i)
            scroll(mScrollVelocity);
    }

    void listRenderTitleOverlay(const glm::mat4&)
    {
        if constexpr (std::is_same_v<UserData, FileData*>) {
            if (!Settings::getInstance()->getBool("ListScrollOverlay"))
                return;

            if (size() == 0 || mTitleOverlayOpacity == 0.0f) {
                mWindow->renderListScrollOverlay(0.0f, "");
                return;
            }

            std::string titleIndex;
            bool favoritesSorting {true};

            if (getSelected()->getSystem()->isCustomCollection())
                favoritesSorting = Settings::getInstance()->getBool("FavFirstCustom");
            else
                favoritesSorting = Settings::getInstance()->getBool("FavoritesFirst");

            if (favoritesSorting && getSelected()->getFavorite()) {
#if defined(_MSC_VER) // MSVC compiler.
                titleIndex = Utils::String::wideStringToString(L"\uF005");
#else
                titleIndex = "\uF005";
#endif
            }
            else {
                titleIndex = getSelected()->getName();
                if (titleIndex.size()) {
                    titleIndex[0] = toupper(titleIndex[0]);
                    if (titleIndex.size() > 1) {
                        titleIndex = titleIndex.substr(0, 2);
                        titleIndex[1] = tolower(titleIndex[1]);
                    }
                }
            }

            // The actual rendering takes place in Window to make sure that the overlay is placed on
            // top of all GUI elements but below the info popups and GPU statistics overlay.
            mWindow->renderListScrollOverlay(mTitleOverlayOpacity, titleIndex);
        }
    }

    void scroll(int amt)
    {
        if (mScrollVelocity == 0 || size() < 2)
            return;

        bool doScroll {true};

        // This is only needed for GridComponent.
        if (mColumns > 1 && mScrollVelocity == -mColumns && mCursor < mColumns) {
            doScroll = false;
        }
        else if (mColumns != 0 && mScrollVelocity == mColumns) {
            if (size() - mCursor <= size() % mColumns)
                doScroll = false;
            else if (mColumns != 1 && mCursor >= (mColumns * mRows) - mColumns &&
                     size() - mCursor <= mColumns && size() % mColumns == 0)
                doScroll = false;
            else if (size() < mColumns)
                doScroll = false;
        }

        mLastCursor = mCursor;

        if (!doScroll) {
            onCursorChanged(CursorState::CURSOR_STOPPED);
            return;
        }

        int cursor {mCursor + amt};
        int absAmt {amt < 0 ? -amt : amt};

        bool stopScroll {false};

        // Depending on the loop type we'll either pause at the ends if holding a navigation
        // button, or we'll only stop if it's a quick jump key (should or trigger button) that
        // is held, or we never loop.
        if (mLoopType == ListLoopType::LIST_PAUSE_AT_END && (mScrollTier > 0 || absAmt > 1))
            stopScroll = true;
        else if (mLoopType == ListLoopType::LIST_PAUSE_AT_END_ON_JUMP && abs(mScrollVelocity) > 1 &&
                 (mScrollTier > 0 || absAmt > 1))
            stopScroll = true;
        else if (mLoopType == ListLoopType::LIST_NEVER_LOOP)
            stopScroll = true;

        if (stopScroll) {
            if (cursor < 0) {
                cursor = 0;
                mScrollVelocity = 0;
                mScrollTier = 0;
            }
            else if (cursor >= size()) {
                cursor = size() - 1;
                mScrollVelocity = 0;
                mScrollTier = 0;
            }
        }
        else {
            while (cursor < 0)
                cursor += size();
            while (cursor >= size())
                cursor -= size();
        }

        if (cursor != mCursor)
            onScroll();

        mCursor = cursor;
        onCursorChanged((mScrollTier > 0) ? CursorState::CURSOR_SCROLLING :
                                            CursorState::CURSOR_STOPPED);
    }

    virtual void onCursorChanged(const CursorState&) {}
    virtual void onScroll() {}
};

#endif // ES_CORE_COMPONENTS_ILIST_H
