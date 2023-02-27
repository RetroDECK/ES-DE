//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiThemeDownloader.h
//
//  Theme downloader.
//  Currently only a skeleton with some JSON configuration parsing.
//

#ifndef ES_APP_GUIS_GUI_THEME_DOWNLOADER_H
#define ES_APP_GUIS_GUI_THEME_DOWNLOADER_H

#include "GuiComponent.h"
#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"
#include "components/TextComponent.h"
#include "renderers/Renderer.h"
#include "views/ViewController.h"

class GuiThemeDownloader : public GuiComponent
{
public:
    GuiThemeDownloader();

    void parseThemesList();

    void onSizeChanged() override;
    bool input(InputConfig* config, Input input) override;

    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override { return ViewController::getInstance()->getViewHelpStyle(); }

private:
    Renderer* mRenderer;
    NinePatchComponent mBackground;
    ComponentGrid mGrid;

    struct Screenshot {
        std::string image;
        std::string caption;
    };

    struct ThemeEntry {
        std::string name;
        std::string reponame;
        std::string url;
        bool bundled {false};
        std::vector<std::string> variants;
        std::vector<std::string> colorSchemes;
        std::vector<std::string> aspectRatios;
        std::vector<std::string> transitions;
        std::vector<Screenshot> screenshots;
    };

    std::shared_ptr<TextComponent> mTitle;
    std::vector<ThemeEntry> mThemeSets;
};

#endif // ES_APP_GUIS_GUI_THEME_DOWNLOADER_H
