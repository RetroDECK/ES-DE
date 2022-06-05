//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ComponentGrid.h
//
//  Provides basic layout of components in an X*Y grid.
//

#ifndef ES_CORE_COMPONENTS_COMPONENT_GRID_H
#define ES_CORE_COMPONENTS_COMPONENT_GRID_H

#include "GuiComponent.h"
#include "renderers/Renderer.h"

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
} // namespace GridFlags

// Provides basic layout of components in an X*Y grid.
class ComponentGrid : public GuiComponent
{
public:
    ComponentGrid(const glm::ivec2& gridDimensions);
    virtual ~ComponentGrid();

    bool removeEntry(const std::shared_ptr<GuiComponent>& comp);

    void setEntry(const std::shared_ptr<GuiComponent>& comp,
                  const glm::ivec2& pos,
                  bool canFocus,
                  bool resize = true,
                  const glm::ivec2& size = glm::ivec2 {1, 1},
                  unsigned int border = GridFlags::BORDER_NONE,
                  GridFlags::UpdateType updateType = GridFlags::UPDATE_ALWAYS);

    void setPastBoundaryCallback(const std::function<bool(InputConfig* config, Input input)>& func)
    {
        mPastBoundaryCallback = func;
    }

    void textInput(const std::string& text) override;
    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;
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

    bool moveCursor(glm::ivec2 dir);
    // Pass -1 for xPos or yPos to keep its axis cursor position.
    void moveCursorTo(int xPos, int yPos, bool selectLeftCell = false);
    void setCursorTo(const std::shared_ptr<GuiComponent>& comp);

    std::shared_ptr<GuiComponent> getSelectedComponent()
    {
        const GridEntry* e = getCellAt(mCursor);
        if (e)
            return e->component;
        else
            return nullptr;
    }

    void onFocusLost() override;
    void onFocusGained() override;

    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    class GridEntry
    {
    public:
        glm::ivec2 pos;
        glm::ivec2 dim;
        std::shared_ptr<GuiComponent> component;
        bool canFocus;
        bool resize;
        GridFlags::UpdateType updateType;
        unsigned int border;

        GridEntry(const glm::ivec2& p = glm::ivec2 {},
                  const glm::ivec2& d = glm::ivec2 {},
                  const std::shared_ptr<GuiComponent>& cmp = nullptr,
                  bool f = false,
                  bool r = true,
                  GridFlags::UpdateType u = GridFlags::UPDATE_ALWAYS,
                  unsigned int b = GridFlags::BORDER_NONE)
            : pos(p)
            , dim(d)
            , component(cmp)
            , canFocus(f)
            , resize(r)
            , updateType(u)
            , border(b)
        {
        }

        operator bool() const { return component != nullptr; }
    };

    // Update position and size.
    void updateCellComponent(const GridEntry& cell);
    void updateSeparators();

    void onCursorMoved(glm::ivec2 from, glm::ivec2 to);
    const GridEntry* getCellAt(int x, int y) const;
    const GridEntry* getCellAt(const glm::ivec2& pos) const { return getCellAt(pos.x, pos.y); }

    Renderer* mRenderer;
    std::vector<std::vector<float>> mSeparators;
    glm::ivec2 mGridSize;
    std::vector<GridEntry> mCells;
    glm::ivec2 mCursor;

    std::function<bool(InputConfig* config, Input input)> mPastBoundaryCallback;

    float* mRowHeights;
    float* mColWidths;
};

#endif // ES_CORE_COMPONENTS_COMPONENT_GRID_H
