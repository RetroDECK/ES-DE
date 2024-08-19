//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  ComponentGrid.cpp
//
//  Provides basic layout of components in an X*Y grid.
//

#include "components/ComponentGrid.h"

#include "Settings.h"
#include "components/TextComponent.h"
#include "utils/LocalizationUtil.h"

using namespace GridFlags;

ComponentGrid::ComponentGrid(const glm::ivec2& gridDimensions)
    : mRenderer {Renderer::getInstance()}
    , mGridSize {gridDimensions}
    , mCursor {0, 0}
{
    assert(gridDimensions.x > 0 && gridDimensions.y > 0);

    mCells.reserve(gridDimensions.x * gridDimensions.y);

    mColWidths = new float[gridDimensions.x];
    mRowHeights = new float[gridDimensions.y];

    for (int x = 0; x < gridDimensions.x; ++x)
        mColWidths[x] = 0;
    for (int y = 0; y < gridDimensions.y; ++y)
        mRowHeights[y] = 0;
}

ComponentGrid::~ComponentGrid()
{
    delete[] mRowHeights;
    delete[] mColWidths;
}

float ComponentGrid::getColWidth(int col)
{
    assert(col >= 0 && col < mGridSize.x);

    if (mColWidths[col] != 0)
        return mColWidths[col] * mSize.x;

    // Calculate automatic width.
    float freeWidthPerc {1.0};
    int between {0};
    for (int x = 0; x < mGridSize.x; ++x) {
        freeWidthPerc -= mColWidths[x]; // If it's 0 it won't do anything.
        if (mColWidths[x] == 0)
            ++between;
    }

    return (freeWidthPerc * mSize.x) / static_cast<float>(between);
}

float ComponentGrid::getRowHeight(int row)
{
    assert(row >= 0 && row < mGridSize.y);

    if (mRowHeights[row] != 0)
        return mRowHeights[row] * mSize.y;

    // Calculate automatic height.
    float freeHeightPerc {1.0f};
    int between {0};
    for (int y = 0; y < mGridSize.y; ++y) {
        freeHeightPerc -= mRowHeights[y]; // If it's 0 it won't do anything.
        if (mRowHeights[y] == 0)
            ++between;
    }

    return (freeHeightPerc * mSize.y) / static_cast<float>(between);
}

void ComponentGrid::setColWidthPerc(int col, float width, bool update)
{
    assert(col >= 0 && col < mGridSize.x);
    mColWidths[col] = width;

    if (update)
        onSizeChanged();
}

void ComponentGrid::setRowHeightPerc(int row, float height, bool update)
{
    assert(height >= 0 && height <= 1);
    assert(row >= 0 && row < mGridSize.y);
    mRowHeights[row] = height;

    if (update)
        onSizeChanged();
}

void ComponentGrid::setEntry(const std::shared_ptr<GuiComponent>& comp,
                             const glm::ivec2& pos,
                             bool canFocus,
                             bool resize,
                             const glm::ivec2& size,
                             unsigned int border,
                             GridFlags::UpdateType updateType,
                             glm::ivec2 autoCalcExtent)
{
    assert(pos.x >= 0 && pos.x < mGridSize.x && pos.y >= 0 && pos.y < mGridSize.y);
    assert(comp != nullptr);
    assert(comp->getParent() == nullptr);
    comp->setAutoCalcExtent(autoCalcExtent);

    GridEntry entry {pos, size, comp, canFocus, resize, updateType, border};
    mCells.push_back(entry);

    addChild(comp.get());

    if (!cursorValid() && canFocus) {
        auto origCursor = mCursor;
        mCursor = pos;
        onCursorMoved(origCursor, mCursor);
    }

    updateCellComponent(mCells.back());
    updateSeparators();
}

bool ComponentGrid::removeEntry(const std::shared_ptr<GuiComponent>& comp)
{
    for (auto it = mCells.cbegin(); it != mCells.cend(); ++it) {
        if (it->component == comp) {
            removeChild(comp.get());
            mCells.erase(it);
            return true;
        }
    }

    return false;
}

void ComponentGrid::updateCellComponent(const GridEntry& cell)
{
    // Size.
    glm::vec2 size {0.0f};
    for (int x = cell.pos.x; x < cell.pos.x + cell.dim.x; ++x)
        size.x += getColWidth(x);
    for (int y = cell.pos.y; y < cell.pos.y + cell.dim.y; ++y)
        size.y += getRowHeight(y);

    if (cell.resize && size != glm::vec2 {} && cell.component->getSize() != size)
        cell.component->setSize(size);

    // Find top left corner.
    glm::vec3 pos {};
    for (int x = 0; x < cell.pos.x; ++x)
        pos.x += getColWidth(x);
    for (int y = 0; y < cell.pos.y; ++y)
        pos.y += getRowHeight(y);

    // Center component.
    pos.x = pos.x + (size.x - cell.component->getSize().x) / 2.0f;
    pos.y = pos.y + (size.y - cell.component->getSize().y) / 2.0f;

    cell.component->setPosition(pos);
}

void ComponentGrid::updateSeparators()
{
    mSeparators.clear();

    bool drawAll {Settings::getInstance()->getBool("DebugGrid")};

    glm::vec2 pos;
    glm::vec2 size;

    for (auto it = mCells.cbegin(); it != mCells.cend(); ++it) {
        if (!it->border && !drawAll)
            continue;

        // Find component position + size.
        pos = glm::vec2 {0.0f, 0.0f};
        size = glm::vec2 {0.0f, 0.0f};
        for (int x = 0; x < it->pos.x; ++x)
            pos[0] += getColWidth(x);
        for (int y = 0; y < it->pos.y; ++y)
            pos[1] += getRowHeight(y);
        for (int x = it->pos.x; x < it->pos.x + it->dim.x; ++x)
            size[0] += getColWidth(x);
        for (int y = it->pos.y; y < it->pos.y + it->dim.y; ++y)
            size[1] += getRowHeight(y);

        if (size == glm::vec2 {0.0f, 0.0f})
            return;

        if (it->border & BORDER_TOP || drawAll) {
            std::vector<float> coordVector;
            coordVector.push_back(pos.x);
            coordVector.push_back(pos.y);
            coordVector.push_back(size.x);
            coordVector.push_back(1.0f * Renderer::getScreenHeightModifier());
            mSeparators.push_back(coordVector);
        }
        if (it->border & BORDER_BOTTOM || drawAll) {
            std::vector<float> coordVector;
            coordVector.push_back(pos.x);
            coordVector.push_back(pos.y + size.y);
            coordVector.push_back(size.x);
            coordVector.push_back(1.0f * Renderer::getScreenHeightModifier());
            mSeparators.push_back(coordVector);
        }
        if (it->border & BORDER_LEFT || drawAll) {
            std::vector<float> coordVector;
            coordVector.push_back(pos.x);
            coordVector.push_back(pos.y);
            coordVector.push_back(1.0f * Renderer::getScreenWidthModifier());
            coordVector.push_back(size.y);
            mSeparators.push_back(coordVector);
        }
        if (it->border & BORDER_RIGHT || drawAll) {
            std::vector<float> coordVector;
            coordVector.push_back(pos.x + size.x);
            coordVector.push_back(pos.y);
            coordVector.push_back(1.0f * Renderer::getScreenWidthModifier());
            coordVector.push_back(size.y);
            mSeparators.push_back(coordVector);
        }
    }
}

void ComponentGrid::onSizeChanged()
{
    for (auto it = mCells.cbegin(); it != mCells.cend(); ++it)
        updateCellComponent(*it);

    updateSeparators();
}

const ComponentGrid::GridEntry* ComponentGrid::getCellAt(int x, int y) const
{
    assert(x >= 0 && x < mGridSize.x && y >= 0 && y < mGridSize.y);

    for (auto it = mCells.cbegin(); it != mCells.cend(); ++it) {
        int xmin {it->pos.x};
        int xmax {xmin + it->dim.x};
        int ymin {it->pos.y};
        int ymax {ymin + it->dim.y};

        if (x >= xmin && y >= ymin && x < xmax && y < ymax)
            return &(*it);
    }

    return nullptr;
}

bool ComponentGrid::input(InputConfig* config, Input input)
{
    const GridEntry* cursorEntry {getCellAt(mCursor)};
    if (cursorEntry && cursorEntry->component->input(config, input))
        return true;

    if (!input.value)
        return false;

    bool withinBoundary {false};

    if (config->isMappedLike("down", input))
        withinBoundary = moveCursor(glm::ivec2 {0, 1});

    if (config->isMappedLike("up", input))
        withinBoundary = moveCursor(glm::ivec2 {0, -1});

    if (config->isMappedLike("left", input))
        withinBoundary = moveCursor(glm::ivec2 {-1, 0});

    if (config->isMappedLike("right", input))
        withinBoundary = moveCursor(glm::ivec2 {1, 0});

    if (!withinBoundary && mPastBoundaryCallback)
        return mPastBoundaryCallback(config, input);

    return withinBoundary;
}

void ComponentGrid::resetCursor()
{
    if (!mCells.size())
        return;

    for (auto it = mCells.cbegin(); it != mCells.cend(); ++it) {
        if (it->canFocus) {
            glm::ivec2 origCursor {mCursor};
            mCursor = it->pos;
            onCursorMoved(origCursor, mCursor);
            break;
        }
    }
}

bool ComponentGrid::moveCursor(glm::ivec2 dir)
{
    assert(dir.x || dir.y);

    const glm::ivec2 origCursor {mCursor};
    const GridEntry* currentCursorEntry {getCellAt(mCursor)};
    glm::ivec2 searchAxis {dir.x == 0, dir.y == 0};

    // Logic to handle entries that span several cells.
    if (currentCursorEntry->dim.x > 1) {
        if (dir.x < 0 && currentCursorEntry->pos.x == 0 && mCursor.x > currentCursorEntry->pos.x) {
            onCursorMoved(mCursor, glm::ivec2 {0, mCursor.y});
            mCursor.x = 0;
            return false;
        }

        if (dir.x > 0 && currentCursorEntry->pos.x + currentCursorEntry->dim.x == mGridSize.x &&
            mCursor.x < currentCursorEntry->pos.x + currentCursorEntry->dim.x - 1) {
            onCursorMoved(mCursor, glm::ivec2 {mGridSize.x - 1, mCursor.y});
            mCursor.x = mGridSize.x - 1;
            return false;
        }

        if (dir.x > 0 && mCursor.x != currentCursorEntry->pos.x + currentCursorEntry->dim.x - 1)
            dir.x = currentCursorEntry->dim.x - (mCursor.x - currentCursorEntry->pos.x);
        else if (dir.x < 0 && mCursor.x != currentCursorEntry->pos.x)
            dir.x = -(mCursor.x - currentCursorEntry->pos.x + 1);
    }

    if (currentCursorEntry->dim.y > 1) {
        if (dir.y > 0 && mCursor.y != currentCursorEntry->pos.y + currentCursorEntry->dim.y - 1)
            dir.y = currentCursorEntry->dim.y - (mCursor.y - currentCursorEntry->pos.y);
        else if (dir.y < 0 && mCursor.y != currentCursorEntry->pos.y)
            dir.y = -(mCursor.y - currentCursorEntry->pos.y + 1);
    }

    while (mCursor.x >= 0 && mCursor.y >= 0 && mCursor.x < mGridSize.x && mCursor.y < mGridSize.y) {
        mCursor = mCursor + dir;
        glm::ivec2 curDirPos {mCursor};
        const GridEntry* cursorEntry;

        // Spread out on search axis+
        while (mCursor.x < mGridSize.x && mCursor.y < mGridSize.y && mCursor.x >= 0 &&
               mCursor.y >= 0) {
            cursorEntry = getCellAt(mCursor);

            // Multi-cell entries.
            if (cursorEntry != nullptr) {
                if (dir.x < 0 && cursorEntry->dim.x > 1)
                    mCursor.x = getCellAt(origCursor)->pos.x - cursorEntry->dim.x;
                if (dir.y < 0 && cursorEntry->dim.y > 1)
                    mCursor.y = getCellAt(origCursor)->pos.y - cursorEntry->dim.y;

                if (cursorEntry->canFocus && cursorEntry != currentCursorEntry) {
                    onCursorMoved(origCursor, mCursor);
                    return true;
                }
            }
            mCursor += searchAxis;
        }

        // Now again on search axis-
        mCursor = curDirPos;
        while (mCursor.x >= 0 && mCursor.y >= 0 && mCursor.x < mGridSize.x &&
               mCursor.y < mGridSize.y) {
            cursorEntry = getCellAt(mCursor);

            if (cursorEntry && cursorEntry->canFocus && cursorEntry != currentCursorEntry) {
                onCursorMoved(origCursor, mCursor);
                return true;
            }
            mCursor -= searchAxis;
        }
        mCursor = curDirPos;
    }

    // Failed to find another focusable element in this direction.
    mCursor = origCursor;
    return false;
}

void ComponentGrid::moveCursorTo(int xPos, int yPos, bool selectLeftCell)
{
    const glm::ivec2 origCursor {mCursor};

    if (xPos != -1)
        mCursor.x = xPos;
    if (yPos != -1)
        mCursor.y = yPos;

    const GridEntry* currentCursorEntry = getCellAt(mCursor);

    // If requested, select the leftmost cell of entries wider than 1 cell.
    if (selectLeftCell && mCursor.x > currentCursorEntry->pos.x)
        mCursor.x = currentCursorEntry->pos.x;

    onCursorMoved(origCursor, mCursor);
}

void ComponentGrid::onFocusLost()
{
    const GridEntry* cursorEntry {getCellAt(mCursor)};
    if (cursorEntry)
        cursorEntry->component->onFocusLost();
}

void ComponentGrid::onFocusGained()
{
    const GridEntry* cursorEntry {getCellAt(mCursor)};
    if (cursorEntry)
        cursorEntry->component->onFocusGained();
}

bool ComponentGrid::cursorValid()
{
    const GridEntry* e {getCellAt(mCursor)};
    return (e != nullptr && e->canFocus);
}

void ComponentGrid::update(int deltaTime)
{
    // Update everything.
    const GridEntry* cursorEntry {getCellAt(mCursor)};
    for (auto it = mCells.cbegin(); it != mCells.cend(); ++it) {
        if (it->updateType == UPDATE_ALWAYS ||
            (it->updateType == UPDATE_WHEN_SELECTED && cursorEntry == &(*it))) {
            it->component->update(deltaTime);
        }
    }
}

void ComponentGrid::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans {parentTrans * getTransform()};

    renderChildren(trans);

    // Draw cell separators.
    for (size_t i = 0; i < mSeparators.size(); ++i) {
        mRenderer->setMatrix(trans);
        mRenderer->drawRect(mSeparators[i][0], mSeparators[i][1], mSeparators[i][2],
                            mSeparators[i][3], mMenuColorSeparators, mMenuColorSeparators);
    }
}

void ComponentGrid::textInput(const std::string& text, const bool pasting)
{
    const GridEntry* selectedEntry {getCellAt(mCursor)};
    if (selectedEntry != nullptr && selectedEntry->canFocus)
        selectedEntry->component->textInput(text, pasting);
}

void ComponentGrid::onCursorMoved(glm::ivec2 from, glm::ivec2 to)
{
    const GridEntry* cell {getCellAt(from)};
    if (cell)
        cell->component->onFocusLost();

    cell = getCellAt(to);
    if (cell)
        cell->component->onFocusGained();

    updateHelpPrompts();
}

void ComponentGrid::setCursorTo(const std::shared_ptr<GuiComponent>& comp)
{
    for (auto it = mCells.cbegin(); it != mCells.cend(); ++it) {
        if (it->component == comp) {
            glm::ivec2 oldCursor {mCursor};
            mCursor = it->pos;
            onCursorMoved(oldCursor, mCursor);
            return;
        }
    }

    // Component not found!
    assert(false);
}

std::vector<HelpPrompt> ComponentGrid::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    const GridEntry* e {getCellAt(mCursor)};
    if (e)
        prompts = e->component->getHelpPrompts();
    else
        return prompts;

    bool canScrollVert {false};

    // If the currently selected cell does not fill the entire Y axis, then check if the cells
    // above or below are actually focusable as otherwise they should not affect the help prompts.
    if (mGridSize.y > 1 && e->dim.y < mGridSize.y) {
        if (e->pos.y - e->dim.y >= 0) {
            const GridEntry* cell {getCellAt(glm::ivec2 {e->pos.x, e->pos.y - e->dim.y})};
            if (cell != nullptr && cell->canFocus)
                canScrollVert = true;
        }
        if (e->pos.y + e->dim.y < mGridSize.y) {
            const GridEntry* cell {getCellAt(glm::ivec2 {e->pos.x, e->pos.y + e->dim.y})};
            if (cell != nullptr && cell->canFocus)
                canScrollVert = true;
        }
    }

    bool canScrollHoriz {false};

    // Same as the above code section but for the X axis.
    if (mGridSize.x > 1 && e->dim.x < mGridSize.x) {
        if (e->pos.x - e->dim.x >= 0) {
            const GridEntry* cell {getCellAt(glm::ivec2 {e->pos.x - e->dim.x, e->pos.y})};
            if (cell != nullptr && cell->canFocus)
                canScrollHoriz = true;
        }
        if (e->pos.x + e->dim.x < mGridSize.x) {
            const GridEntry* cell {getCellAt(glm::ivec2 {e->pos.x + e->dim.x, e->pos.y})};
            if (cell != nullptr && cell->canFocus)
                canScrollHoriz = true;
        }
    }

    // Check existing capabilities as indicated by the help prompts, and if the prompts should
    // be combined into "up/down/left/right" then also remove the single-axis prompts.
    if (!prompts.empty() && prompts.back() == HelpPrompt("up/down", _("choose"))) {
        canScrollVert = true;
        if (canScrollHoriz && canScrollVert)
            prompts.pop_back();
    }
    else if (!prompts.empty() && prompts.back() == HelpPrompt("left/right", _("choose"))) {
        canScrollHoriz = true;
        if (canScrollHoriz && canScrollVert)
            prompts.pop_back();
    }

    // Any duplicates will be removed in Window::setHelpPrompts()
    if (canScrollHoriz && canScrollVert)
        prompts.push_back(HelpPrompt("up/down/left/right", _("choose")));
    else if (canScrollHoriz)
        prompts.push_back(HelpPrompt("left/right", _("choose")));
    else if (canScrollVert)
        prompts.push_back(HelpPrompt("up/down", _("choose")));

    return prompts;
}
