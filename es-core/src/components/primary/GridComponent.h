//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GridComponent.h
//
//  Grid, usable in both the system and gamelist views.
//

#ifndef ES_CORE_COMPONENTS_GRID_COMPONENT_H
#define ES_CORE_COMPONENTS_GRID_COMPONENT_H

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

public:
    GridComponent();

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
    virtual const LetterCase getLetterCaseCollections() const = 0;
    virtual const LetterCase getLetterCaseGroupedCollections() const = 0;

private:
    Renderer* mRenderer;
    std::function<void()> mCancelTransitionsCallback;
    std::function<void(CursorState state)> mCursorChangedCallback;

    bool mFadeAbovePrimary;
    LetterCase mLetterCase;
    LetterCase mLetterCaseCollections;
    LetterCase mLetterCaseGroupedCollections;
};

template <typename T>
GridComponent<T>::GridComponent()
    : IList<GridEntry, T> {}
    , mRenderer {Renderer::getInstance()}
    , mFadeAbovePrimary {false}
    , mLetterCase {LetterCase::NONE}
    , mLetterCaseCollections {LetterCase::NONE}
    , mLetterCaseGroupedCollections {LetterCase::NONE}
{
}

#endif // ES_CORE_COMPONENTS_GRID_COMPONENT_H
