//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  ComponentList.h
//
//  Used to lay out and navigate lists in GUI menus.
//

#ifndef ES_CORE_COMPONENTS_COMPONENT_LIST_H
#define ES_CORE_COMPONENTS_COMPONENT_LIST_H

#include "IList.h"

struct ComponentListElement {
    ComponentListElement(const std::shared_ptr<GuiComponent>& componentArg = nullptr,
                         bool resizeWidthArg = true,
                         bool invertWhenSelectedArg = true)
        : component {componentArg}
        , resizeWidth {resizeWidthArg}
        , invertWhenSelected {invertWhenSelectedArg}
    {
    }

    std::shared_ptr<GuiComponent> component;
    bool resizeWidth;
    bool invertWhenSelected;
};

struct ComponentListRow {
    std::vector<ComponentListElement> elements;

    // The input handler is called when the user enters any input while this row is
    // highlighted (including up/down navigation).
    // Return false to let the list try to use it or true if the input has been consumed.
    // If no input handler is supplied (inputHandler == nullptr), then the default behavior
    // is to forward the input to the rightmost element in the currently selected row.
    std::function<bool(InputConfig*, Input)> inputHandler;

    void addElement(const std::shared_ptr<GuiComponent>& comp,
                    bool resizeWidth,
                    bool invertWhenSelected = true,
                    glm::ivec2 autoCalcExtent = {0, 0})
    {
        comp->setAutoCalcExtent(autoCalcExtent);
        elements.push_back(ComponentListElement(comp, resizeWidth, invertWhenSelected));
    }

    // Utility function for making an input handler for an input event.
    void makeAcceptInputHandler(const std::function<void()>& func)
    {
        inputHandler = [func](InputConfig* config, Input input) -> bool {
            if (config->isMappedTo("a", input) && input.value != 0) {
                func();
                return true;
            }
            return false;
        };
    }
};

class ComponentList : public IList<ComponentListRow, void*>
{
public:
    ComponentList();

    enum ScrollIndicator {
        SCROLL_NONE,
        SCROLL_UP,
        SCROLL_UP_DOWN,
        SCROLL_DOWN
    };

    void addRow(const ComponentListRow& row, bool setCursorHere = false);

    void textInput(const std::string& text, const bool pasting = false) override;
    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;
    std::vector<HelpPrompt> getHelpPrompts() override;

    void onSizeChanged() override;
    void onFocusGained() override { mFocused = true; }
    void onFocusLost() override
    {
        mFocused = false;
        resetSelectedRow();
    }

    bool moveCursor(int amount);
    int getCursorId() const { return mCursor; }

    const float getRowHeight() const { return mRowHeight; }
    void setRowHeight(float height) { mRowHeight = height; }
    const float getTotalRowHeight() const { return mRowHeight * mEntries.size(); }

    void resetSelectedRow()
    {
        if (mEntries.size() > static_cast<size_t>(mCursor)) {
            for (auto& comp : mEntries.at(mCursor).data.elements)
                comp.component->resetComponent();
        }
    }

    void setHorizontalScrolling(bool state) override
    {
        for (auto& entry : mEntries) {
            for (auto& element : entry.data.elements)
                element.component->setHorizontalScrolling(state);
        }
    }

    void resetScrollIndicatorStatus()
    {
        mScrollIndicatorStatus = SCROLL_NONE;
        if (mScrollIndicatorChangedCallback != nullptr)
            mScrollIndicatorChangedCallback(mScrollIndicatorStatus, false);
    }

    void setCursorChangedCallback(const std::function<void(CursorState state)>& callback)
    {
        mCursorChangedCallback = callback;
    }
    const std::function<void(CursorState state)>& getCursorChangedCallback() const
    {
        return mCursorChangedCallback;
    }
    void setScrollIndicatorChangedCallback(
        const std::function<void(ScrollIndicator state, bool singleRowScroll)>& callback)
    {
        mScrollIndicatorChangedCallback = callback;
    }

protected:
    void onCursorChanged(const CursorState& state) override;

private:
    Renderer* mRenderer;
    bool mFocused;
    bool mSetupCompleted;
    bool mBottomCameraOffset;
    bool mSingleRowScroll;

    void updateCameraOffset();
    void updateElementPosition(const ComponentListRow& row);
    void updateElementSize(const ComponentListRow& row);

    float mRowHeight;
    float mHorizontalPadding;
    float mSelectorBarOffset;
    float mCameraOffset;

    std::function<void(CursorState state)> mCursorChangedCallback;
    std::function<void(ScrollIndicator state, bool singleRowScroll)>
        mScrollIndicatorChangedCallback;

    ScrollIndicator mScrollIndicatorStatus;
};

#endif // ES_CORE_COMPONENTS_COMPONENT_LIST_H
