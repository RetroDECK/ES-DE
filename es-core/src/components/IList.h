//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  IList.h
//
//  List base class, used by both the gamelist views and the menu.
//

#ifndef ES_CORE_COMPONENTS_ILIST_H
#define ES_CORE_COMPONENTS_ILIST_H

#include "Window.h"
#include "components/ImageComponent.h"
#include "utils/StringUtil.h"

enum CursorState {
    CURSOR_STOPPED, // Replace with AllowShortEnumsOnASingleLine: false (clang-format >=11.0).
    CURSOR_SCROLLING
};

enum ListLoopType {
    LIST_ALWAYS_LOOP, // Replace with AllowShortEnumsOnASingleLine: false (clang-format >=11.0).
    LIST_PAUSE_AT_END,
    LIST_NEVER_LOOP
};

struct ScrollTier {
    int length; // How long we stay on this tier before going to the next.
    int scrollDelay; // How long between scrolls.
};

struct ScrollTierList {
    const int count;
    const ScrollTier* tiers;
};

// Default scroll tiers.
// clang-format off
const ScrollTier QUICK_SCROLL_TIERS[] = {
    {500, 500},
    {1200, 114},
    {0, 16}
};
const ScrollTierList LIST_SCROLL_STYLE_QUICK = {
    3,
    QUICK_SCROLL_TIERS
};

const ScrollTier SLOW_SCROLL_TIERS[] = {
    {500, 500},
    {0, 200}
};

const ScrollTierList LIST_SCROLL_STYLE_SLOW = {
    2,
    SLOW_SCROLL_TIERS
};
// clang-format on

template <typename EntryData, typename UserData> class IList : public GuiComponent
{
public:
    struct Entry {
        std::string name;
        UserData object;
        EntryData data;
    };

protected:
    int mCursor;
    int mScrollTier;
    int mScrollVelocity;
    int mScrollTierAccumulator;
    int mScrollCursorAccumulator;

    unsigned char mTitleOverlayOpacity;
    unsigned int mTitleOverlayColor;

    const ScrollTierList& mTierList;
    const ListLoopType mLoopType;

    std::vector<Entry> mEntries;
    Window* mWindow;

public:
    IList(Window* window,
          const ScrollTierList& tierList = LIST_SCROLL_STYLE_QUICK,
          const ListLoopType& loopType = LIST_PAUSE_AT_END)
        : GuiComponent(window)
        , mTierList(tierList)
        , mLoopType(loopType)
        , mWindow(window)
    {
        mCursor = 0;
        mScrollTier = 0;
        mScrollVelocity = 0;
        mScrollTierAccumulator = 0;
        mScrollCursorAccumulator = 0;

        mTitleOverlayOpacity = 0x00;
        mTitleOverlayColor = 0xFFFFFF00;
    }

    bool isScrolling() const { return (mScrollVelocity != 0 && mScrollTier > 0); }

    int getScrollingVelocity() { return mScrollVelocity; }

    void stopScrolling()
    {
        mTitleOverlayOpacity = 0;

        listInput(0);
        if (mScrollVelocity == 0)
            onCursorChanged(CURSOR_STOPPED);
    }

    void clear()
    {
        mEntries.clear();
        mCursor = 0;
        listInput(0);
        onCursorChanged(CURSOR_STOPPED);
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
        onCursorChanged(CURSOR_STOPPED);
    }

    // Returns true if successful (select is in our list), false if not.
    bool setCursor(const UserData& obj)
    {
        for (auto it = mEntries.cbegin(); it != mEntries.cend(); it++) {
            if ((*it).object == obj) {
                mCursor = static_cast<int>(it - mEntries.cbegin());
                onCursorChanged(CURSOR_STOPPED);
                return true;
            }
        }

        return false;
    }

    // Entry management.
    void add(const Entry& e) { mEntries.push_back(e); }

    bool remove(const UserData& obj)
    {
        for (auto it = mEntries.cbegin(); it != mEntries.cend(); it++) {
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
            mCursor--;
            onCursorChanged(CURSOR_STOPPED);
        }

        mEntries.erase(it);
    }

    bool listFirstRow()
    {
        mCursor = 0;
        onCursorChanged(CURSOR_STOPPED);
        onScroll();
        return true;
    }

    bool listLastRow()
    {
        mCursor = static_cast<int>(mEntries.size()) - 1;
        onCursorChanged(CURSOR_STOPPED);
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
        const int dir = (mScrollTier >= mTierList.count - 1) ? 1 : -1;
        // We simply translate the time directly to opacity, i.e. no scaling is performed.
        int op = mTitleOverlayOpacity + deltaTime * dir;
        if (op >= 255)
            mTitleOverlayOpacity = 255;
        else if (op <= 0)
            mTitleOverlayOpacity = 0;
        else
            mTitleOverlayOpacity = static_cast<unsigned char>(op);

        if (mScrollVelocity == 0 || size() < 2)
            return;

        mScrollCursorAccumulator += deltaTime;
        mScrollTierAccumulator += deltaTime;

        // We delay scrolling until after scroll tier has updated so isScrolling() returns
        // accurately during onCursorChanged callbacks. We don't just do scroll tier first
        // because it would not catch the scrollDelay == tier length case.
        int scrollCount = 0;
        while (mScrollCursorAccumulator >= mTierList.tiers[mScrollTier].scrollDelay) {
            mScrollCursorAccumulator -= mTierList.tiers[mScrollTier].scrollDelay;
            scrollCount++;
        }

        // Should we go to the next scrolling tier?
        while (mScrollTier < mTierList.count - 1 &&
               mScrollTierAccumulator >= mTierList.tiers[mScrollTier].length) {
            mScrollTierAccumulator -= mTierList.tiers[mScrollTier].length;
            mScrollTier++;
        }

        // Actually perform the scrolling.
        for (int i = 0; i < scrollCount; i++)
            scroll(mScrollVelocity);
    }

    void listRenderTitleOverlay(const glm::mat4& /*trans*/)
    {
        if (!Settings::getInstance()->getBool("ListScrollOverlay"))
            return;

        if (size() == 0 || mTitleOverlayOpacity == 0) {
            mWindow->renderListScrollOverlay(0, "");
            return;
        }

        std::string titleIndex;
        bool favoritesSorting;

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

    void scroll(int amt)
    {
        if (mScrollVelocity == 0 || size() < 2)
            return;

        int cursor = mCursor + amt;
        int absAmt = amt < 0 ? -amt : amt;

        // Stop at the end if we've been holding down the button for a long time or
        // we're scrolling faster than one item at a time (e.g. page up/down).
        // Otherwise, loop around.
        if ((mLoopType == LIST_PAUSE_AT_END && (mScrollTier > 0 || absAmt > 1)) ||
            mLoopType == LIST_NEVER_LOOP) {
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
        onCursorChanged((mScrollTier > 0) ? CURSOR_SCROLLING : CURSOR_STOPPED);
    }

    virtual void onCursorChanged(const CursorState& /*state*/) {}
    virtual void onScroll() {}
};

#endif // ES_CORE_COMPONENTS_ILIST_H
