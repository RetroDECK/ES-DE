//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ComponentGrid.h
//
//  Provides basic layout of components in an X*Y grid.
//

#ifndef ES_CORE_COMPONENTS_COMPONENT_GRID_H
#define ES_CORE_COMPONENTS_COMPONENT_GRID_H

#include "math/Vector2i.h"
#include "renderers/Renderer.h"
#include "GuiComponent.h"

namespace GridFlags
{
    enum UpdateType {
        UPDATE_ALWAYS,
        UPDATE_WHEN_SELECTED,
        UPDATE_NEVER
    };

    enum Border : unsigned int {
        BORDER_NONE = 0,
        BORDER_TOP = 1,
        BORDER_BOTTOM = 2,
        BORDER_LEFT = 4,
        BORDER_RIGHT = 8
    };
};

// Provides basic layout of components in an X*Y grid.
class ComponentGrid : public GuiComponent
{
public:
    ComponentGrid(Window* window, const Vector2i& gridDimensions);
    virtual ~ComponentGrid();

    bool removeEntry(const std::shared_ptr<GuiComponent>& comp);

    void setEntry(
            const std::shared_ptr<GuiComponent>& comp,
            const Vector2i& pos,
            bool canFocus,
            bool resize = true,
            const Vector2i& size = Vector2i(1, 1),
            unsigned int border = GridFlags::BORDER_NONE,
            GridFlags::UpdateType updateType = GridFlags::UPDATE_ALWAYS);

    void textInput(const std::string& text) override;
    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const Transform4x4f& parentTrans) override;
    void onSizeChanged() override;

    void resetCursor();
    bool cursorValid();

    float getColWidth(int col);
    float getRowHeight(int row);

    // If update is false, will not call an onSizeChanged() which triggers
    // a (potentially costly) repositioning + resizing of every element.
    void setColWidthPerc(int col, float width, bool update = true);
    // Dito.
    void setRowHeightPerc(int row, float height, bool update = true);

    bool moveCursor(Vector2i dir);
    void setCursorTo(const std::shared_ptr<GuiComponent>& comp);

    inline std::shared_ptr<GuiComponent> getSelectedComponent()
    {
        const GridEntry* e = getCellAt(mCursor);
        if (e)
            return e->component;
        else
            return nullptr;
    }

    void onFocusLost() override;
    void onFocusGained() override;

    virtual std::vector<HelpPrompt> getHelpPrompts() override;

private:
    class GridEntry
    {
    public:
        Vector2i pos;
        Vector2i dim;
        std::shared_ptr<GuiComponent> component;
        bool canFocus;
        bool resize;
        GridFlags::UpdateType updateType;
        unsigned int border;

        GridEntry(
                const Vector2i& p = Vector2i::Zero(),
                const Vector2i& d = Vector2i::Zero(),
                const std::shared_ptr<GuiComponent>& cmp = nullptr,
                bool f = false,
                bool r = true,
                GridFlags::UpdateType u = GridFlags::UPDATE_ALWAYS,
                unsigned int b =
                GridFlags::BORDER_NONE)
                : pos(p),
                dim(d),
                component(cmp),
                canFocus(f),
                resize(r),
                updateType(u),
                border(b)
        {};

        operator bool() const
        {
            return component != nullptr;
        }
    };

    // Update position and size.
    void updateCellComponent(const GridEntry& cell);
    void updateSeparators();

    void onCursorMoved(Vector2i from, Vector2i to);
    const GridEntry* getCellAt(int x, int y) const;

    inline const GridEntry* getCellAt(const Vector2i& pos) const
            { return getCellAt(pos.x(), pos.y()); }

    std::vector<std::vector<float>> mSeparators;
    Vector2i mGridSize;
    std::vector<GridEntry> mCells;
    Vector2i mCursor;

    float* mRowHeights;
    float* mColWidths;
};

#endif // ES_CORE_COMPONENTS_COMPONENT_GRID_H
