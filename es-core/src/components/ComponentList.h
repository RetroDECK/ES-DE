//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ComponentList.h
//
//  Used to lay out and navigate lists in GUI menus.
//

#ifndef ES_CORE_COMPONENTS_COMPONENT_LIST_H
#define ES_CORE_COMPONENTS_COMPONENT_LIST_H

#include "IList.h"

struct ComponentListElement {
    ComponentListElement(const std::shared_ptr<GuiComponent>& cmp = nullptr,
                         bool resize_w = true,
                         bool inv = true)
        : component(cmp)
        , resize_width(resize_w)
        , invert_when_selected(inv)
    {
    }

    std::shared_ptr<GuiComponent> component;
    bool resize_width;
    bool invert_when_selected;
};

struct ComponentListRow {
    std::vector<ComponentListElement> elements;

    // The input handler is called when the user enters any input while this row is
    // highlighted (including up/down).
    // Return false to let the list try to use it or true if the input has been consumed.
    // If no input handler is supplied (input_handler == nullptr), the default behavior is
    // to forward the input to the rightmost element in the currently selected row.
    std::function<bool(InputConfig*, Input)> input_handler;

    void addElement(const std::shared_ptr<GuiComponent>& component,
                    bool resize_width,
                    bool invert_when_selected = true)
    {
        elements.push_back(ComponentListElement(component, resize_width, invert_when_selected));
    }

    // Utility function for making an input handler for "when the users presses A on this, do func".
    void makeAcceptInputHandler(const std::function<void()>& func)
    {
        input_handler = [func](InputConfig* config, Input input) -> bool {
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
    ComponentList(Window* window);

    void addRow(const ComponentListRow& row, bool setCursorHere = false);

    void textInput(const std::string& text) override;
    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;
    virtual std::vector<HelpPrompt> getHelpPrompts() override;

    void onSizeChanged() override;
    void onFocusGained() override { mFocused = true; }
    void onFocusLost() override { mFocused = false; }

    bool moveCursor(int amt);
    int getCursorId() const { return mCursor; }

    float getTotalRowHeight() const;
    float getRowHeight(int row) const { return getRowHeight(mEntries.at(row).data); }

    void setCursorChangedCallback(const std::function<void(CursorState state)>& callback)
    {
        mCursorChangedCallback = callback;
    }
    const std::function<void(CursorState state)>& getCursorChangedCallback() const
    {
        return mCursorChangedCallback;
    }

protected:
    void onCursorChanged(const CursorState& state) override;

private:
    bool mFocused;

    void updateCameraOffset();
    void updateElementPosition(const ComponentListRow& row);
    void updateElementSize(const ComponentListRow& row);

    float getRowHeight(const ComponentListRow& row) const;

    float mHorizontalPadding;
    float mSelectorBarOffset;
    float mCameraOffset;

    std::function<void(CursorState state)> mCursorChangedCallback;
};

#endif // ES_CORE_COMPONENTS_COMPONENT_LIST_H
