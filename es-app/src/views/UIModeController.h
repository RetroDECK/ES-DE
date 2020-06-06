//
//	UIModeController.h
//
//	Handling of application user interface modes (full, kiosk and kid).
//	This includes switching the mode when the UI mode passkey is used.
//

#pragma once
#ifndef ES_APP_VIEWS_UI_MODE_CONTROLLER_H
#define ES_APP_VIEWS_UI_MODE_CONTROLLER_H

#include <vector>
#include <string>

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
	inline std::vector<std::string> getUIModes() { return mUIModes; };

private:
	UIModeController();
	bool inputIsMatch(InputConfig * config, Input input);
	bool isValidInput(InputConfig * config, Input input);
	void logInput(InputConfig * config, Input input);

	// Return UI mode to 'full'.
	void unlockUIMode();

	static UIModeController * sInstance;
	const std::vector<std::string> mUIModes = { "Full", "Kiosk", "Kid" };

	// Default passkeyseq = "uuddlrlrba", as defined in the setting 'UIMode_passkey'.
	std::string mPassKeySequence;
	int mPassKeyCounter;
	const std::vector<std::string> mInputVals =
			{ "up", "down", "left", "right", "a", "b", "x", "y" };
	std::string mCurrentUIMode;
};

#endif // ES_APP_VIEWS_UI_MODE_CONTROLLER_H
