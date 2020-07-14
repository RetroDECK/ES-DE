//
//  UIModeController.cpp
//
//  Handling of application user interface modes (full, kiosk and kid).
//  This includes switching the mode when the UI mode passkey is used.
//

#include "UIModeController.h"

#include "utils/StringUtil.h"
#include "views/ViewController.h"
#include "Log.h"
#include "Window.h"

UIModeController*  UIModeController::sInstance = nullptr;

UIModeController* UIModeController::getInstance()
{
    if (sInstance == nullptr)
        sInstance = new UIModeController();

    return sInstance;
}

UIModeController::UIModeController()
    :mPassKeyCounter(0)
{
    mPassKeySequence = Settings::getInstance()->getString("UIMode_passkey");
    mCurrentUIMode = Settings::getInstance()->getString("UIMode");
}

void UIModeController::monitorUIMode()
{
    std::string uimode = Settings::getInstance()->getString("UIMode");
    // UI mode was changed.
    if (uimode != mCurrentUIMode) {
        mCurrentUIMode = uimode;
        ViewController::get()->ReloadAndGoToStart();
    }
}

bool UIModeController::listen(InputConfig* config, Input input)
{
    // Reads the current input to listen for the passkey sequence to unlock
    // the UI mode. The progress is saved in mPassKeyCounter.
    if (Settings::getInstance()->getBool("Debug"))
        logInput(config, input);

    if ((Settings::getInstance()->getString("UIMode") == "Full") || !isValidInput(config, input))
        return false; // Already unlocked, or invalid input, nothing to do here.

    if (!inputIsMatch(config, input))
        mPassKeyCounter = 0; // Current input is incorrect, reset counter.

    if (mPassKeyCounter == (int)mPassKeySequence.length()) {
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
            mPassKeyCounter++;
            return true;
        }
    }
    return false;
}

// When we have reached the end of the list, trigger UI_mode unlock.
void UIModeController::unlockUIMode()
{
    LOG(LogDebug) <<
            " UIModeController::listen(): Passkey sequence completed, switching UIMode to full";
    Settings::getInstance()->setString("UIMode", "Full");
    Settings::getInstance()->saveFile();
    mPassKeyCounter = 0;
}

bool UIModeController::isUIModeFull()
{
    return ((mCurrentUIMode == "Full") && !Settings::getInstance()->getBool("ForceKiosk"));
}

bool UIModeController::isUIModeKid()
{
    return (Settings::getInstance()->getBool("ForceKid") ||
        ((mCurrentUIMode == "Kid") && !Settings::getInstance()->getBool("ForceKiosk")));
}

bool UIModeController::isUIModeKiosk()
{
    return (Settings::getInstance()->getBool("ForceKiosk") ||
        ((mCurrentUIMode == "Kiosk") && !Settings::getInstance()->getBool("ForceKid")));
}

std::string UIModeController::getFormattedPassKeyStr()
{
    // Supported sequence-inputs: u (up), d (down), l (left), r (right), a, b, x, y.

    std::string out = "";
    for (auto c : mPassKeySequence) {
        out += (out == "") ? "" : ", ";  // Add a comma after the first entry.

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
                out += "A";
                break;
            case 'b':
                out += "B";
                break;
            case 'x':
                out += "X";
                break;
            case 'y':
                out += "Y";
                break;
        }
    }
    return out;
}

void UIModeController::logInput(InputConfig* config, Input input)
{
    std::string mapname = "";
    std::vector<std::string> maps = config->getMappedTo(input);

    for (auto mn : maps) {
        mapname += mn;
        mapname += ", ";
    }

    LOG(LogDebug) << "UIModeController::logInput(" << config->getDeviceName() << "): " <<
            input.string() << ", isMappedTo=" << mapname << ", value=" << input.value;
}

bool UIModeController::isValidInput(InputConfig* config, Input input)
{
    if ((config->getMappedTo(input).size() == 0)  || // Not a mapped input, so ignore..
            (input.type == TYPE_HAT) ||  // Ignore all hat inputs.
            (!input.value))	// Not a key-down event.
        return false;
    else
        return true;
}
