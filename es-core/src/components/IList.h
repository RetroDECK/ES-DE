//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  IList.h
//
//  Gamelist base class.
//

#ifndef ES_CORE_COMPONENTS_ILIST_H
#define ES_CORE_COMPONENTS_ILIST_H

#include "components/ImageComponent.h"
#include "resources/Font.h"
#include "PowerSaver.h"
#include "Window.h"

enum CursorState {
    CURSOR_STOPPED,
    CURSOR_SCROLLING
};

enum ListLoopType {
    LIST_ALWAYS_LOOP,
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
const ScrollTier QUICK_SCROLL_TIERS[] = {
    {500, 500},
    {2000, 114},
    {4000, 32},
    {0, 16}
};
const ScrollTierList LIST_SCROLL_STYLE_QUICK = {
    4,
    QUICK_SCROLL_TIERS
};

const ScrollTier SLOW_SCROLL_TIERS[] = {
    {500, 500},
    {0, 200}
};

const ScrollTierList LIST_SCROLL_STYLE_SLOW = { 2, SLOW_SCROLL_TIERS };

template <typename EntryData, typename UserData>
class IList : public GuiComponent
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
    ImageComponent mGradient;
    std::shared_ptr<Font> mTitleOverlayFont;

    const ScrollTierList& mTierList;
    const ListLoopType mLoopType;

    std::vector<Entry> mEntries;
    Window* mWindow;

public:
    IList(
            Window* window,
            const ScrollTierList& tierList = LIST_SCROLL_STYLE_QUICK,
            const ListLoopType& loopType = LIST_PAUSE_AT_END)
            : GuiComponent(window),
            mGradient(window),
            mTierList(tierList),
            mLoopType(loopType),
            mWindow(window)
    {
        mCursor = 0;
        mScrollTier = 0;
        mScrollVelocity = 0;
        mScrollTierAccumulator = 0;
        mScrollCursorAccumulator = 0;

        mTitleOverlayOpacity = 0x00;
        mTitleOverlayColor = 0xFFFFFF00;
        mGradient.setResize(static_cast<float>(Renderer::getScreenWidth()),
                static_cast<float>(Renderer::getScreenHeight()));
        mGradient.setImage(":/graphics/scroll_gradient.png");
        mTitleOverlayFont = Font::get(FONT_SIZE_LARGE);
    }

    bool isScrolling() const
    {
        return (mScrollVelocity != 0 && mScrollTier > 0);
    }

    int getScrollingVelocity()
    {
        return mScrollVelocity;
    }

    void stopScrolling()
    {
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

    inline const std::string& getSelectedName()
    {
        assert(size() > 0);
        return mEntries.at(mCursor).name;
    }

    inline const UserData& getSelected() const
    {
        assert(size() > 0);
        return mEntries.at(mCursor).object;
    }

    inline const UserData& getNext() const
    {
        // If there is a next entry, then return it, otherwise return the current entry.
        if (mCursor + 1 < mEntries.size())
            return mEntries.at(mCursor+1).object;
        else
            return mEntries.at(mCursor).object;
    }

    inline const UserData& getPrevious() const
    {
        // If there is a previous entry, then return it, otherwise return the current entry.
        if (mCursor != 0)
            return mEntries.at(mCursor-1).object;
        else
            return mEntries.at(mCursor).object;
    }

    inline const UserData& getFirst() const
    {
        assert(size() > 0);
        return mEntries.front().object;
    }

    inline const UserData& getLast() const
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
    void add(const Entry& e)
    {
        mEntries.push_back(e);
    }

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

    inline int size() const { return static_cast<int>(mEntries.size()); }

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
        mCursor = mEntries.size() - 1;
        onCursorChanged(CURSOR_STOPPED);
        onScroll();
        return true;
    }


    bool listInput(int velocity) // A velocity of 0 = stop scrolling.
    {
        PowerSaver::setState(velocity == 0);

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
        // We just do a 1-to-1 time -> opacity, no scaling.
        int op = mTitleOverlayOpacity + deltaTime*dir;
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

        // Are we ready to go even FASTER?
        while (mScrollTier < mTierList.count - 1 && mScrollTierAccumulator >=
                mTierList.tiers[mScrollTier].length) {
            mScrollTierAccumulator -= mTierList.tiers[mScrollTier].length;
            mScrollTier++;
        }

        // Actually perform the scrolling.
        for (int i = 0; i < scrollCount; i++)
            scroll(mScrollVelocity);
    }

    void listRenderTitleOverlay(const Transform4x4f& /*trans*/)
    {
        if (size() == 0 || !mTitleOverlayFont || mTitleOverlayOpacity == 0)
            return;

        // We don't bother caching this because it's only two letters and will change pretty
        // much every frame if we're scrolling.
        const std::string text = getSelectedName().size() >= 2 ?
                getSelectedName().substr(0, 2) : "??";

        Vector2f off = mTitleOverlayFont->sizeText(text);
        off[0] = (Renderer::getScreenWidth() - off.x()) * 0.5f;
        off[1] = (Renderer::getScreenHeight() - off.y()) * 0.5f;

        Transform4x4f identTrans = Transform4x4f::Identity();

        mGradient.setOpacity(mTitleOverlayOpacity);
        mGradient.render(identTrans);

        TextCache* cache = mTitleOverlayFont->buildTextCache(text, off.x(), off.y(),
                0xFFFFFF00 | mTitleOverlayOpacity);
        // Relies on mGradient's render for Renderer::setMatrix()
        mTitleOverlayFont->renderTextCache(cache);
        delete cache;
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
