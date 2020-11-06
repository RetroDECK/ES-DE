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
#include "components/TextComponent.h"
#include "utils/StringUtil.h"

class ButtonComponent;
class ImageComponent;

std::shared_ptr<ComponentGrid> makeButtonGrid(Window* window,
        const std::vector< std::shared_ptr<ButtonComponent> >& buttons);
std::shared_ptr<ImageComponent> makeArrow(Window* window);

#define TITLE_VERT_PADDING (Renderer::getScreenHeight()*0.0637f)

class MenuComponent : public GuiComponent
{
public:
    MenuComponent(Window* window, std::string title,
            const std::shared_ptr<Font>& titleFont = Font::get(FONT_SIZE_LARGE));
    virtual ~MenuComponent(); // just calls save();

    void save();
    void onSizeChanged() override;

    void setNeedsSaving() { mNeedsSaving = true; };

    inline void addRow(const ComponentListRow& row, bool setCursorHere = false)
            { mList->addRow(row, setCursorHere); updateSize(); }

    inline void addWithLabel(const std::string& label, const std::shared_ptr<GuiComponent>& comp,
            bool setCursorHere = false, bool invert_when_selected = true)
    {
        ComponentListRow row;
        row.addElement(std::make_shared<TextComponent>(mWindow,
                Utils::String::toUpper(label), Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
        row.addElement(comp, false, invert_when_selected);
        addRow(row, setCursorHere);
    }

    inline void addSaveFunc(const std::function<void()>& func) { mSaveFuncs.push_back(func); };

    void addButton(const std::string& label, const std::string& helpText,
            const std::function<void()>& callback);

    void setTitle(std::string title, const std::shared_ptr<Font>& font);

    inline void setCursorToList() { mGrid.setCursorTo(mList); }
    inline void setCursorToButtons() { assert(mButtonGrid); mGrid.setCursorTo(mButtonGrid); }

    virtual std::vector<HelpPrompt> getHelpPrompts() override;

private:
    void updateSize();
    void updateGrid();
    float getButtonGridHeight() const;

    NinePatchComponent mBackground;
    ComponentGrid mGrid;

    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<ComponentList> mList;
    std::shared_ptr<ComponentGrid> mButtonGrid;
    std::vector<std::shared_ptr<ButtonComponent>> mButtons;
    std::vector<std::function<void()>> mSaveFuncs;

    bool mNeedsSaving;
};

#endif // ES_CORE_COMPONENTS_MENU_COMPONENT_H
