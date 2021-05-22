//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiMenu.h
//
//  Main menu.
//  Some submenus are covered in separate source files.
//

#ifndef ES_APP_GUIS_GUI_MENU_H
#define ES_APP_GUIS_GUI_MENU_H

#include "components/MenuComponent.h"
#include "GuiComponent.h"

class GuiMenu : public GuiComponent
{
public:
    GuiMenu(Window* window);
    ~GuiMenu();

    bool input(InputConfig* config, Input input) override;
    void onSizeChanged() override;
    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override;

private:
    void close(bool closeAllWindows);
    void addEntry(const std::string& name, unsigned int color,
            bool add_arrow, const std::function<void()>& func);
    void addVersionInfo();

    void openScraperOptions();
    void openUIOptions();
    void openMediaViewerOptions();
    void openScreensaverOptions();
    void openSoundOptions();
    void openCollectionSystemOptions();
    void openOtherOptions();
    void openConfigInput();
    void openQuitMenu();

    MenuComponent mMenu;
    TextComponent mVersion;
};

#endif // ES_APP_GUIS_GUI_MENU_H
