//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiSettings.h
//
//  User interface template for a settings GUI.
//  The saving of es_settings.cfg and the reload of the gamelists are triggered from here
//  based on the flags set by the actual menu entries' lambda functions.
//

#ifndef ES_APP_GUIS_GUI_SETTINGS_H
#define ES_APP_GUIS_GUI_SETTINGS_H

#include "components/MenuComponent.h"
#include "SystemData.h"

// This is just a really simple template for a GUI that calls some save functions when closed.
class GuiSettings : public GuiComponent
{
public:
    GuiSettings(Window* window, std::string title);
    virtual ~GuiSettings();

    void save();
    inline void addRow(const ComponentListRow& row) { mMenu.addRow(row); };
    inline void addWithLabel(const std::string& label,
            const std::shared_ptr<GuiComponent>& comp) { mMenu.addWithLabel(label, comp); };
    void addEditableTextComponent(
            const std::string label,
            std::shared_ptr<GuiComponent> ed,
            std::string value,
            std::string defaultValue = "",
            bool isPassword = false);
    inline void addSaveFunc(const std::function<void()>& func) { mSaveFuncs.push_back(func); };

    void setNeedsSaving() { mNeedsSaving = true; };
    void setNeedsCollectionsUpdate() { mNeedsCollectionsUpdate = true; };
    void setNeedsReloading() { mNeedsReloading = true; };
    void setNeedsSorting() { mNeedsSorting = true; };
    void setNeedsSortingCollections() { mNeedsSortingCollections = true; };
    void setNeedsGoToStart() { mNeedsGoToStart = true; };
    void setNeedsGoToSystemView(SystemData* goToSystem)
            { mNeedsGoToSystemView = true; mGoToSystem = goToSystem; };
    void setNeedsDestroyAllWindows() { mNeedsDestroyAllWindows = true; };

    bool input(InputConfig* config, Input input) override;
    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override;

private:
    MenuComponent mMenu;
    std::vector<std::function<void()>> mSaveFuncs;
    bool mNeedsSaving;
    bool mNeedsCollectionsUpdate;
    bool mNeedsReloading;
    bool mNeedsSorting;
    bool mNeedsSortingCollections;
    bool mNeedsGoToStart;
    bool mNeedsGoToSystemView;
    bool mNeedsDestroyAllWindows;

    SystemData* mGoToSystem;
};

#endif // ES_APP_GUIS_GUI_SETTINGS_H
