//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiSettings.h
//
//  User interface template for a settings GUI.
//  The saving of es_settings.xml, the reload of gamelists and some other actions are
//  also triggered to be executed here via flags set by the menu entries' lambda functions.
//

#ifndef ES_APP_GUIS_GUI_SETTINGS_H
#define ES_APP_GUIS_GUI_SETTINGS_H

#include "SystemData.h"
#include "components/MenuComponent.h"
#include "views/ViewController.h"

// This is just a really simple template for a GUI that calls some save functions when closed.
class GuiSettings : public GuiComponent
{
public:
    GuiSettings(std::string title);
    virtual ~GuiSettings();

    void save();
    void addRow(const ComponentListRow& row, bool setCursorHere = false)
    {
        mMenu.addRow(row, setCursorHere);
    }
    void addWithLabel(const std::string& label, const std::shared_ptr<GuiComponent>& comp)
    {
        mMenu.addWithLabel(label, comp);
    }
    void addEditableTextComponent(const std::string label,
                                  std::shared_ptr<GuiComponent> ed,
                                  std::string value,
                                  std::string defaultValue = "",
                                  bool isPassword = false);
    void addSaveFunc(const std::function<void()>& func) { mSaveFuncs.push_back(func); }

    MenuComponent& getMenu() { return mMenu; }
    glm::vec2 getMenuSize() { return mMenu.getSize(); }
    void setMenuSize(glm::vec2 size) { mMenu.setSize(size); }
    glm::vec3 getMenuPosition() { return mMenu.getPosition(); }
    void setMenuPosition(glm::vec3 position) { mMenu.setPosition(position); }

    void setNeedsSaving(bool state = true) { mNeedsSaving = state; }
    void setNeedsCollectionsUpdate() { mNeedsCollectionsUpdate = true; }
    void setNeedsSorting() { mNeedsSorting = true; }
    void setNeedsSortingCollections() { mNeedsSortingCollections = true; }
    void setNeedsResetFilters() { mNeedsResetFilters = true; }
    void setNeedsReloading() { mNeedsReloading = true; }
    void setNeedsGoToStart() { mNeedsGoToStart = true; }
    void setNeedsGoToSystem(SystemData* goToSystem)
    {
        mNeedsGoToSystem = true;
        mGoToSystem = goToSystem;
    };
    void setNeedsGoToGroupedCollections() { mNeedsGoToGroupedCollections = true; }
    void setInvalidateCachedBackground() { mInvalidateCachedBackground = true; }

    bool input(InputConfig* config, Input input) override;
    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override { return ViewController::getInstance()->getViewHelpStyle(); }

private:
    Renderer* mRenderer;
    MenuComponent mMenu;
    std::vector<std::function<void()>> mSaveFuncs;
    SystemData* mGoToSystem;

    bool mNeedsSaving;
    bool mNeedsCollectionsUpdate;
    bool mNeedsSorting;
    bool mNeedsSortingCollections;
    bool mNeedsResetFilters;
    bool mNeedsReloading;
    bool mNeedsGoToStart;
    bool mNeedsGoToSystem;
    bool mNeedsGoToGroupedCollections;
    bool mInvalidateCachedBackground;
};

#endif // ES_APP_GUIS_GUI_SETTINGS_H
