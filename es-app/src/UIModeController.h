//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  UIModeController.h
//
//  Handling of application user interface modes (full, kiosk and kid).
//  This includes switching the mode when the UI mode passkey is used.
//

#ifndef ES_APP_UI_MODE_CONTROLLER_H
#define ES_APP_UI_MODE_CONTROLLER_H

#include <string>
#include <vector>

class FileData;
class InputConfig;
class ViewController;

struct Input;

class UIModeController
{
public:
    static UIModeController* getInstance();

    // Monitor input for UI mode change, returns true (consumes input) when a UI mode
    // change is triggered.
    bool listen(InputConfig* config, Input input);

    // Get the current Passphrase as a (unicode) formatted, comma-separated, string.
    std::string getFormattedPassKeyStr();

    // Check for change in UI mode.
    void monitorUIMode();

    bool isUIModeFull();
    bool isUIModeKid();
    bool isUIModeKiosk();

    void setCurrentUIMode(const std::string& mode) { mCurrentUIMode = mode; }

private:
    UIModeController() noexcept;

    bool inputIsMatch(InputConfig* config, Input input);
    bool isValidInput(InputConfig* config, Input input);

    // Return UI mode to 'full'.
    void unlockUIMode();

    // Default passkeyseq = "uuddlrlrba", as defined in the setting 'UIMode_passkey'.
    std::string mPassKeySequence;

    std::string mCurrentUIMode;
    int mPassKeyCounter;

    // These are Xbox button names, so they may be different in pracise on non-Xbox controllers.
    const std::vector<std::string> mInputVals = {"up", "down", "left", "right", "a", "b", "x", "y"};
};

#endif // ES_APP_UI_MODE_CONTROLLER_H
