//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  UIModeController.cpp
//
//  Handling of application user interface modes (full, kiosk and kid).
//  This includes switching the mode when the UI mode passkey is used.
//

#include "UIModeController.h"

#include "FileFilterIndex.h"
#include "Log.h"
#include "SystemData.h"
#include "Window.h"
#include "utils/StringUtil.h"
#include "views/ViewController.h"

UIModeController::UIModeController() noexcept
    : mPassKeyCounter {0}
{
    mPassKeySequence = Settings::getInstance()->getString("UIMode_passkey");
    mCurrentUIMode = Settings::getInstance()->getString("UIMode");
    // Handle a potentially invalid entry in the configuration file.
    if (mCurrentUIMode != "full" && mCurrentUIMode != "kid" && mCurrentUIMode != "kiosk") {
        mCurrentUIMode = "full";
        Settings::getInstance()->setString("UIMode", mCurrentUIMode);
        Settings::getInstance()->saveFile();
    }
}

UIModeController* UIModeController::getInstance()
{
    static UIModeController instance;
    return &instance;
}

void UIModeController::monitorUIMode()
{
    std::string uimode = Settings::getInstance()->getString("UIMode");
    // UI mode was changed.
    if (uimode != mCurrentUIMode && !ViewController::getInstance()->isCameraMoving()) {
        mCurrentUIMode = uimode;
        // Reset filters and sort gamelists (which will update the game counter).
        for (auto it = SystemData::sSystemVector.cbegin(); // Line break.
             it != SystemData::sSystemVector.cend(); ++it) {
            (*it)->sortSystem(true);
            (*it)->getIndex()->resetFilters();
            if ((*it)->getThemeFolder() == "custom-collections") {
                for (FileData* customSystem : (*it)->getRootFolder()->getChildrenListToDisplay())
                    customSystem->getSystem()->getIndex()->resetFilters();
            }
        }
        ViewController::getInstance()->ReloadAndGoToStart();
    }
}

bool UIModeController::listen(InputConfig* config, Input input)
{
    // Reads the current input to listen for the passkey sequence to unlock
    // the UI mode. The progress is saved in mPassKeyCounter.
    if ((Settings::getInstance()->getString("UIMode") == "full") || !isValidInput(config, input))
        return false; // Already unlocked, or invalid input, nothing to do here.

    if (!inputIsMatch(config, input))
        mPassKeyCounter = 0; // Current input is incorrect, reset counter.

    if (mPassKeyCounter == static_cast<int>(mPassKeySequence.length())) {
        unlockUIMode();
        return true;
    }

    return false;
}

bool UIModeController::inputIsMatch(InputConfig* config, Input input)
{
    for (auto valstring : mInputVals) {
        if (config->isMappedLike(valstring, input) &&
            (mPassKeySequence[mPassKeyCounter] == valstring[0])) {
            ++mPassKeyCounter;
            return true;
        }
    }
    return false;
}

// When we have reached the end of the list, trigger UI_mode unlock.
void UIModeController::unlockUIMode()
{
    LOG(LogInfo) << "Passkey sequence completed, switching UI mode to Full";
    Settings::getInstance()->setString("UIMode", "full");
    Settings::getInstance()->saveFile();
    mPassKeyCounter = 0;
}

bool UIModeController::isUIModeFull()
{
    return ((mCurrentUIMode == "full" ||
             (isUIModeKid() && Settings::getInstance()->getBool("EnableMenuKidMode"))) &&
            !Settings::getInstance()->getBool("ForceKiosk"));
}

bool UIModeController::isUIModeKid()
{
    return (Settings::getInstance()->getBool("ForceKid") ||
            ((mCurrentUIMode == "kid") && !Settings::getInstance()->getBool("ForceKiosk")));
}

bool UIModeController::isUIModeKiosk()
{
    return (Settings::getInstance()->getBool("ForceKiosk") ||
            ((mCurrentUIMode == "kiosk") && !Settings::getInstance()->getBool("ForceKid")));
}

std::string UIModeController::getFormattedPassKeyStr()
{
    // Supported sequence-inputs: u (up), d (down), l (left), r (right), a, b, x, y.

    std::string out = "";
    for (auto c : mPassKeySequence) {
        out += (out == "") ? "" : " , "; // Add commas between the entries.

        std::string controllerType = Settings::getInstance()->getString("InputControllerType");
        std::string symbolA;
        std::string symbolB;
        std::string symbolX;
        std::string symbolY;

        if (controllerType == "snes") {
            symbolA = "B";
            symbolB = "A";
            symbolX = "Y";
            symbolY = "X";
        }
        else if (controllerType == "ps4" || controllerType == "ps5") {
#if defined(_MSC_VER) // MSVC compiler.
            // These symbols are far from perfect but you can at least understand what
            // they are supposed to depict.
            symbolA = Utils::String::wideStringToString(L"\uF00D"); // Cross.
            symbolB = Utils::String::wideStringToString(L"\uF111"); // Circle
            symbolX = Utils::String::wideStringToString(L"\uF04D"); // Square.
            symbolY = Utils::String::wideStringToString(L"\uF0D8"); // Triangle.
#else
            symbolA = "\uF00D"; // Cross.
            symbolB = "\uF111"; // Circle
            symbolX = "\uF04D"; // Square.
            symbolY = "\uF0D8"; // Triangle.
#endif
        }
        else {
            // Xbox controller.
            symbolA = "A";
            symbolB = "B";
            symbolX = "X";
            symbolY = "Y";
        }

        switch (c) {
            case 'u':
                out += Utils::String::unicode2Chars(0x2191); // Arrow pointing up.
                break;
            case 'd':
                out += Utils::String::unicode2Chars(0x2193); // Arrow pointing down.
                break;
            case 'l':
                out += Utils::String::unicode2Chars(0x2190); // Arrow pointing left.
                break;
            case 'r':
                out += Utils::String::unicode2Chars(0x2192); // Arrow pointing right.
                break;
            case 'a':
                out += symbolA;
                break;
            case 'b':
                out += symbolB;
                break;
            case 'x':
                out += symbolX;
                break;
            case 'y':
                out += symbolY;
                break;
        }
    }
    return out;
}

bool UIModeController::isValidInput(InputConfig* config, Input input)
{
    if ((config->getMappedTo(input).size() == 0) || // Not a mapped input, so ignore it.
        (!input.value)) // Not a key-down event.
        return false;
    else
        return true;
}
