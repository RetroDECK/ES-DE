//
//  GuiGeneralScreensaverOptions.cpp
//
//  User interface for the screensaver options.
//  Based on the GuiScreenSaverOptions template.
//  Submenu to the GuiMenu main menu.
//

#include "guis/GuiGeneralScreensaverOptions.h"

#include "components/OptionListComponent.h"
#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiSlideshowScreensaverOptions.h"
#include "guis/GuiVideoScreensaverOptions.h"
#include "Settings.h"

GuiGeneralScreensaverOptions::GuiGeneralScreensaverOptions(Window* window, const char* title)
        : GuiScreensaverOptions(window, title)
{
    // Screensaver time.
    auto screensaver_time = std::make_shared<SliderComponent>(mWindow, 0.f, 30.f, 1.f, "m");
    screensaver_time->setValue((float)(Settings::getInstance()->
            getInt("ScreenSaverTime") / (1000 * 60)));
    addWithLabel("SCREENSAVER AFTER", screensaver_time);
    addSaveFunc([screensaver_time] {
        Settings::getInstance()->setInt("ScreenSaverTime",
                (int)Math::round(screensaver_time->getValue()) * (1000 * 60));
        PowerSaver::updateTimeouts();
    });

    // Allow ScreenSaver Controls - ScreenSaverControls.
    auto ss_controls = std::make_shared<SwitchComponent>(mWindow);
    ss_controls->setState(Settings::getInstance()->getBool("ScreenSaverControls"));
    addWithLabel("SCREENSAVER CONTROLS", ss_controls);
    addSaveFunc([ss_controls] { Settings::getInstance()->setBool("ScreenSaverControls",
            ss_controls->getState()); });

    // Screensaver behavior.
    auto screensaver_behavior = std::make_shared<OptionListComponent<std::string>>
            (mWindow, getHelpStyle(), "SCREENSAVER BEHAVIOR", false);
    std::vector<std::string> screensavers;
    screensavers.push_back("dim");
    screensavers.push_back("black");
    screensavers.push_back("slideshow");
    screensavers.push_back("video");
    for (auto it = screensavers.cbegin(); it != screensavers.cend(); it++)
        screensaver_behavior->add(*it, *it, Settings::getInstance()->
                getString("ScreenSaverBehavior") == *it);
    addWithLabel("SCREENSAVER BEHAVIOR", screensaver_behavior);
    addSaveFunc([this, screensaver_behavior] {
        if (Settings::getInstance()->getString("ScreenSaverBehavior") !=
                "video" && screensaver_behavior->getSelected() == "video") {
            // If before it wasn't risky but now there's a risk of problems, show warning.
            mWindow->pushGui(new GuiMsgBox(mWindow, getHelpStyle(),
            "THE \"VIDEO\" SCREENSAVER SHOWS\nVIDEOS FROM YOUR GAMELISTS.\n\nIF YOU DO NOT "
            "HAVE ANY VIDEOS, THE\nSCREENSAVER WILL DEFAULT TO \"BLACK\"",
                "OK", [] { return; }, "", nullptr, "", nullptr));
        }
        Settings::getInstance()->setString("ScreenSaverBehavior",
                screensaver_behavior->getSelected());
        PowerSaver::updateTimeouts();
    });

    ComponentListRow row;

    // Show filtered menu.
    row.elements.clear();
    row.addElement(std::make_shared<TextComponent>(mWindow,
            "VIDEO SCREENSAVER SETTINGS", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
    row.addElement(makeArrow(mWindow), false);
    row.makeAcceptInputHandler(std::bind(
                &GuiGeneralScreensaverOptions::openVideoScreensaverOptions, this));
    addRow(row);

    row.elements.clear();
    row.addElement(std::make_shared<TextComponent>(mWindow,
            "SLIDESHOW SCREENSAVER SETTINGS", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
    row.addElement(makeArrow(mWindow), false);
    row.makeAcceptInputHandler(std::bind(
            &GuiGeneralScreensaverOptions::openSlideshowScreensaverOptions, this));
    addRow(row);
}

GuiGeneralScreensaverOptions::~GuiGeneralScreensaverOptions()
{
}

void GuiGeneralScreensaverOptions::openVideoScreensaverOptions() {
    mWindow->pushGui(new GuiVideoScreensaverOptions(mWindow, "VIDEO SCREENSAVER"));
}

void GuiGeneralScreensaverOptions::openSlideshowScreensaverOptions() {
    mWindow->pushGui(new GuiSlideshowScreensaverOptions(mWindow, "SLIDESHOW SCREENSAVER"));
}
