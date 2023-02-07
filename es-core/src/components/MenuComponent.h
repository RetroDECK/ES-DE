//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  MenuComponent.h
//
//  Basic component for building a menu.
//

#ifndef ES_CORE_COMPONENTS_MENU_COMPONENT_H
#define ES_CORE_COMPONENTS_MENU_COMPONENT_H

#include "components/ComponentGrid.h"
#include "components/ComponentList.h"
#include "components/NinePatchComponent.h"
#include "components/ScrollIndicatorComponent.h"
#include "components/TextComponent.h"
#include "utils/StringUtil.h"

#include <cmath>

class ButtonComponent;
class ImageComponent;

std::shared_ptr<ComponentGrid> makeButtonGrid(
    const std::vector<std::shared_ptr<ButtonComponent>>& buttons);
std::shared_ptr<ImageComponent> makeArrow();

class MenuComponent : public GuiComponent
{
public:
    MenuComponent(std::string title,
                  const std::shared_ptr<Font>& titleFont = Font::get(FONT_SIZE_LARGE));
    virtual ~MenuComponent();

    void save();
    void onSizeChanged() override;

    void setNeedsSaving() { mNeedsSaving = true; }

    void addRow(const ComponentListRow& row, bool setCursorHere = false, bool updateRowSize = true)
    {
        mList->addRow(row, setCursorHere);
        if (updateRowSize)
            updateSize();
    }

    void addWithLabel(const std::string& label,
                      const std::shared_ptr<GuiComponent>& comp,
                      bool setCursorHere = false,
                      bool invert_when_selected = true)
    {
        ComponentListRow row;
        row.addElement(std::make_shared<TextComponent>(Utils::String::toUpper(label),
                                                       Font::get(FONT_SIZE_MEDIUM), 0x777777FF),
                       true);
        row.addElement(comp, false, invert_when_selected);
        addRow(row, setCursorHere);
    }

    void addSaveFunc(const std::function<void()>& func) { mSaveFuncs.push_back(func); }

    void addButton(const std::string& label,
                   const std::string& helpText,
                   const std::function<void()>& callback);

    void setTitle(std::string title, const std::shared_ptr<Font>& font);
    std::shared_ptr<ComponentList> getList() { return mList; }

    void setCursorToFirstListEntry() { mList->moveCursor(-mList->getCursorId()); }
    void setCursorToList() { mGrid.setCursorTo(mList); }
    void setCursorToButtons()
    {
        assert(mButtonGrid);
        mGrid.setCursorTo(mButtonGrid);
    }

    std::vector<HelpPrompt> getHelpPrompts() override { return mGrid.getHelpPrompts(); }

private:
    void updateSize();
    void updateGrid();
    float getButtonGridHeight() const;

    Renderer* mRenderer;
    NinePatchComponent mBackground;
    ComponentGrid mGrid;

    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<ImageComponent> mScrollUp;
    std::shared_ptr<ImageComponent> mScrollDown;
    std::shared_ptr<ScrollIndicatorComponent> mScrollIndicator;
    std::shared_ptr<ComponentList> mList;
    std::shared_ptr<ComponentGrid> mButtonGrid;
    std::vector<std::shared_ptr<ButtonComponent>> mButtons;
    std::vector<std::function<void()>> mSaveFuncs;

    bool mNeedsSaving;
};

#endif // ES_CORE_COMPONENTS_MENU_COMPONENT_H
