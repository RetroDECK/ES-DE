//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  InputManager.cpp
//
//  Low-level input handling.
//  Initiates and maps the keyboard and controllers.
//  Reads and writes the es_input.xml configuration file.
//

#include "InputManager.h"

#include "Log.h"
#include "Scripting.h"
#include "Window.h"
#include "resources/ResourceManager.h"
#include "utils/FileSystemUtil.h"
#include "utils/LocalizationUtil.h"
#include "utils/PlatformUtil.h"
#include "utils/StringUtil.h"

#include <iostream>
#include <pugixml.hpp>

#define KEYBOARD_GUID_STRING "-1"
#define CEC_GUID_STRING "-2"

#if defined(__ANDROID__)
#define TOUCH_GUID_STRING "-3"
#include "utils/PlatformUtilAndroid.h"
#endif

namespace
{
    int SDL_USER_CECBUTTONDOWN {-1};
    int SDL_USER_CECBUTTONUP {-1};
} // namespace

InputManager::InputManager() noexcept
    : mWindow {Window::getInstance()}
#if defined(__ANDROID__)
    , mInputOverlay {InputOverlay::getInstance()}
#endif
    , mKeyboardInputConfig {nullptr}
    , mTouchInputConfig {nullptr}
    , mCECInputConfig {nullptr}
{
}

InputManager::~InputManager()
{
    // Deinit when destroyed.
    deinit();
}

InputManager& InputManager::getInstance()
{
    static InputManager instance;
    return instance;
}

void InputManager::init()
{
    if (initialized())
        deinit();

    mConfigFileExists = false;

    LOG(LogInfo) << "Setting up InputManager...";

    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    SDL_GameControllerEventState(SDL_ENABLE);
    SDL_StopTextInput();

    if (!Utils::FileSystem::exists(getConfigPath())) {
        LOG(LogInfo) << "No input configuration file found, default mappings will be applied";
    }
    else {
        mConfigFileExists = true;
    }

    mKeyboardInputConfig =
        std::make_unique<InputConfig>(DEVICE_KEYBOARD, "Keyboard", KEYBOARD_GUID_STRING);

    bool customConfig {loadInputConfig(mKeyboardInputConfig.get())};

    if (customConfig) {
        LOG(LogInfo) << "Added keyboard with custom configuration";
    }
    else {
        loadDefaultKBConfig();
        LOG(LogInfo) << "Added keyboard with default configuration";
    }

#if defined(__ANDROID__)
    mTouchInputConfig = std::make_unique<InputConfig>(DEVICE_TOUCH, "Touch", TOUCH_GUID_STRING);
    loadTouchConfig();
#endif

    // Load optional controller mappings. Normally the supported controllers should be compiled
    // into SDL as a header file, but if a user has a very rare controller that is not supported,
    // the bundled mapping is incorrect, or the SDL version is a bit older, it makes sense to be
    // able to customize this. If a controller GUID is present in the mappings file that is
    // already present inside SDL, the custom mapping will overwrite the bundled one.
    std::string mappingsFile;

    if (Settings::getInstance()->getBool("LegacyAppDataDirectory")) {
        mappingsFile = Utils::FileSystem::getAppDataDirectory() + "/es_controller_mappings.cfg";
    }
    else {
        mappingsFile =
            Utils::FileSystem::getAppDataDirectory() + "/controllers/es_controller_mappings.cfg";
    }

    if (!Utils::FileSystem::exists(mappingsFile))
        mappingsFile = ResourceManager::getInstance().getResourcePath(
            ":/controllers/es_controller_mappings.cfg");

    int controllerMappings {SDL_GameControllerAddMappingsFromFile(mappingsFile.c_str())};

    if (controllerMappings != -1 && controllerMappings != 0) {
        LOG(LogInfo) << "Loaded " << controllerMappings << " controller "
                     << (controllerMappings == 1 ? "mapping" : "mappings");
    }

    int numJoysticks {SDL_NumJoysticks()};

    // Make sure that every joystick is actually supported by the GameController API.
    for (int i {0}; i < numJoysticks; ++i) {
        if (SDL_IsGameController(i))
            addControllerByDeviceIndex(nullptr, i);
    }

    SDL_USER_CECBUTTONDOWN = SDL_RegisterEvents(2);
    SDL_USER_CECBUTTONUP = SDL_USER_CECBUTTONDOWN + 1;
    mCECInputConfig = std::make_unique<InputConfig>(DEVICE_CEC, "CEC", CEC_GUID_STRING);
    loadInputConfig(mCECInputConfig.get());
}

void InputManager::deinit()
{
    if (!initialized())
        return;

    for (auto it = mControllers.cbegin(); it != mControllers.cend(); ++it)
        SDL_GameControllerClose(it->second);

    mControllers.clear();
    mJoysticks.clear();
    mPrevAxisValues.clear();
    mPrevButtonValues.clear();
    mInputConfigs.clear();

    mKeyboardInputConfig.reset();
    mTouchInputConfig.reset();
    mCECInputConfig.reset();

    SDL_GameControllerEventState(SDL_DISABLE);
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}

void InputManager::writeDeviceConfig(InputConfig* config)
{
    assert(initialized());

    std::string path {getConfigPath()};

    LOG(LogDebug) << "InputManager::writeDeviceConfig(): "
                     "Saving input configuration file to \""
                  << path << "\"";

    pugi::xml_document doc;

    if (Utils::FileSystem::exists(path)) {
        // Merge files.
#if defined(_WIN64)
        pugi::xml_parse_result result {
            doc.load_file(Utils::String::stringToWideString(path).c_str())};
#else
        pugi::xml_parse_result result {doc.load_file(path.c_str())};
#endif
        if (!result) {
            LOG(LogError) << "Couldn't parse input configuration file: " << result.description();
        }
        else {
            // Successfully loaded, delete the old entry if it exists.
            pugi::xml_node root {doc.child("inputList")};
            if (root) {
                // If inputAction @type=onfinish is set, let doOnFinish command take care of
                // creating input configuration. We just put the input configuration into a
                // temporary input config file.
                pugi::xml_node actionnode {
                    root.find_child_by_attribute("inputAction", "type", "onfinish")};
                if (actionnode) {
                    path = getTemporaryConfigPath();
                    doc.reset();
                    root = doc.append_child("inputList");
                    root.append_copy(actionnode);
                }
                else {
                    pugi::xml_node oldEntry {root.find_child_by_attribute(
                        "inputConfig", "deviceGUID", config->getDeviceGUIDString().c_str())};
                    if (oldEntry)
                        root.remove_child(oldEntry);
                    oldEntry = root.find_child_by_attribute("inputConfig", "deviceName",
                                                            config->getDeviceName().c_str());
                    if (oldEntry)
                        root.remove_child(oldEntry);
                }
            }
        }
    }

    pugi::xml_node root {doc.child("inputList")};
    if (!root)
        root = doc.append_child("inputList");

    config->writeToXML(root);

#if defined(_WIN64)
    doc.save_file(Utils::String::stringToWideString(path).c_str());
#else
    doc.save_file(path.c_str());
#endif

    Scripting::fireEvent("config-changed");
    Scripting::fireEvent("controls-changed");

    // Execute any doOnFinish commands and reload the config for changes.
    doOnFinish();
    mConfigFileExists = true;
    loadInputConfig(config);
}

void InputManager::doOnFinish()
{
    assert(initialized());
    std::string path {getConfigPath()};
    pugi::xml_document doc;

    if (Utils::FileSystem::exists(path)) {
#if defined(_WIN64)
        pugi::xml_parse_result result {
            doc.load_file(Utils::String::stringToWideString(path).c_str())};
#else
        pugi::xml_parse_result result {doc.load_file(path.c_str())};
#endif

        if (!result) {
            LOG(LogError) << "Couldn't parse input configuration file: " << result.description();
        }
        else {
            pugi::xml_node root {doc.child("inputList")};
            if (root) {
                root = root.find_child_by_attribute("inputAction", "type", "onfinish");
                if (root) {
                    for (pugi::xml_node command {root.child("command")}; command;
                         command = command.next_sibling("command")) {
                        std::string tocall {command.text().get()};

                        LOG(LogInfo) << "	" << tocall;
                        std::cout << "==============================================\n"
                                     "input config finish command:\n";
                        int exitCode = Utils::Platform::runSystemCommand(tocall);
                        std::cout << "==============================================\n";

                        if (exitCode != 0) {
                            LOG(LogWarning) << "...launch terminated with nonzero exit code "
                                            << exitCode << "!";
                        }
                    }
                }
            }
        }
    }
}

std::string InputManager::getConfigPath()
{
    if (Settings::getInstance()->getBool("LegacyAppDataDirectory"))
        return Utils::FileSystem::getAppDataDirectory() + "/es_input.xml";
    else
        return Utils::FileSystem::getAppDataDirectory() + "/settings/es_input.xml";
}

std::string InputManager::getTemporaryConfigPath()
{
    if (Settings::getInstance()->getBool("LegacyAppDataDirectory"))
        return Utils::FileSystem::getAppDataDirectory() + "/es_temporaryinput.xml";
    else
        return Utils::FileSystem::getAppDataDirectory() + "/settings/es_temporaryinput.xml";
}

int InputManager::getNumConfiguredDevices()
{
    int num {0};
    for (auto it = mInputConfigs.cbegin(); it != mInputConfigs.cend(); ++it)
        if (it->second->isConfigured())
            ++num;

    if (mKeyboardInputConfig->isConfigured())
        ++num;

#if defined(__ANDROID__)
    if (mTouchInputConfig->isConfigured())
        ++num;
#endif

    if (mCECInputConfig->isConfigured())
        ++num;

    return num;
}

int InputManager::getAxisCountByDevice(SDL_JoystickID id)
{
    return SDL_JoystickNumAxes(mJoysticks[id]);
}

int InputManager::getButtonCountByDevice(SDL_JoystickID id)
{
    if (id == DEVICE_KEYBOARD)
        return -1;
    else if (id == DEVICE_CEC)
#if defined(HAVE_CECLIB)
        return CEC::CEC_USER_CONTROL_CODE_MAX;
#else
        return 0;
#endif
    else
        return SDL_JoystickNumButtons(mJoysticks[id]);
}

std::string InputManager::getDeviceGUIDString(int deviceId)
{
    if (deviceId == DEVICE_KEYBOARD)
        return KEYBOARD_GUID_STRING;
#if defined(__ANDROID__)
    else if (deviceId == DEVICE_TOUCH)
        return TOUCH_GUID_STRING;
#endif
    else if (deviceId == DEVICE_CEC)
        return CEC_GUID_STRING;

    auto it = mJoysticks.find(deviceId);
    if (it == mJoysticks.cend()) {
        LOG(LogError) << "getDeviceGUIDString - deviceId " << deviceId << " not found!";
        return "Something went horribly wrong";
    }

    std::string guid(65, '\0');
    SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(it->second), &guid[0], 64);
    guid.erase(guid.find('\0'));
    return guid;
}

InputConfig* InputManager::getInputConfigByDevice(int device)
{
    if (device == DEVICE_KEYBOARD)
        return mKeyboardInputConfig.get();
#if defined(__ANDROID__)
    else if (device == DEVICE_TOUCH)
        return mTouchInputConfig.get();
#endif
    else if (device == DEVICE_CEC)
        return mCECInputConfig.get();
    else
        return mInputConfigs[device].get();
}

bool InputManager::parseEvent(const SDL_Event& event)
{
    bool causedEvent {false};
    int32_t axisValue {0};

    switch (event.type) {
        case SDL_CONTROLLERAXISMOTION: {
            // Whether to only accept input from the first controller.
            if (Settings::getInstance()->getBool("InputOnlyFirstController"))
                if (mInputConfigs.begin()->first != event.cdevice.which)
                    return false;

            // This is needed for a situation which sometimes occur when a game is launched,
            // some axis input is generated and then the controller is disconnected before
            // leaving the game. In this case, events for the old device index could be received
            // when returning from the game. If this happens we simply delete the configuration
            // map entry.
            if (!mInputConfigs[event.caxis.which]) {
                auto it = mInputConfigs.find(event.cdevice.which);
                mInputConfigs.erase(it);
                return false;
            }

            axisValue = event.caxis.value;
            int deadzone {0};

            if (event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT ||
                event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
                deadzone = DEADZONE_TRIGGERS;
            }
            else {
                deadzone = DEADZONE_THUMBSTICKS;
            }

            // Check if the input value switched boundaries.
            if ((abs(axisValue) > deadzone) !=
                (abs(mPrevAxisValues[std::make_pair(event.caxis.which, event.caxis.axis)]) >
                 deadzone)) {
                int normValue {0};
                if (abs(axisValue) <= deadzone) {
                    normValue = 0;
                }
                else {
                    if (axisValue > 0)
                        normValue = 1;
                    else
                        normValue = -1;
                }

                mWindow->input(
                    getInputConfigByDevice(event.caxis.which),
                    Input(event.caxis.which, TYPE_AXIS, event.caxis.axis, normValue, false));
                causedEvent = true;
            }

            mPrevAxisValues[std::make_pair(event.caxis.which, event.caxis.axis)] = axisValue;
            return causedEvent;
        }
        case SDL_CONTROLLERBUTTONDOWN: {
        }
        case SDL_CONTROLLERBUTTONUP: {
            // Whether to only accept input from the first controller.
            if (Settings::getInstance()->getBool("InputOnlyFirstController"))
                if (mInputConfigs.begin()->first != event.cdevice.which)
                    return false;

            // The event filtering below is required as some controllers send button presses
            // starting with the state 0 when using the D-pad. I consider this invalid behavior
            // and the more popular controllers such as those from Microsoft and Sony do not show
            // this strange behavior.
            int buttonState {
                mPrevButtonValues[std::make_pair(event.cbutton.which, event.cbutton.button)]};

            if ((buttonState == -1 || buttonState == 0) && event.cbutton.state == 0)
                return false;

            mPrevButtonValues[std::make_pair(event.cbutton.which, event.cbutton.button)] =
                event.cbutton.state;

            mWindow->input(getInputConfigByDevice(event.cbutton.which),
                           Input(event.cbutton.which, TYPE_BUTTON, event.cbutton.button,
                                 event.cbutton.state == SDL_PRESSED, false));

            return true;
        }
        case SDL_KEYDOWN: {
            if (SDL_IsTextInputActive()) {
                // Paste from clipboard.
#if defined(__APPLE__)
                if (event.key.keysym.mod & KMOD_GUI && event.key.keysym.sym == SDLK_v) {
#else
                if ((event.key.keysym.mod & KMOD_CTRL && event.key.keysym.sym == SDLK_v) ||
                    (event.key.keysym.mod & KMOD_SHIFT && event.key.keysym.sym == SDLK_INSERT)) {
#endif
                    if (SDL_HasClipboardText()) {
                        char* clipboardText {SDL_GetClipboardText()};
                        mWindow->textInput(clipboardText, true);
                        SDL_free(clipboardText);
                        return true;
                    }
                }

                // Handle backspace presses.
                if (event.key.keysym.sym == SDLK_BACKSPACE)
                    mWindow->textInput("\b");
            }

            if (event.key.repeat)
                return false;

#if defined(__ANDROID__)
            // Quit application if the back button is pressed or if the back gesture is used,
            // unless we're set as the Android home app.
            if (event.key.keysym.sym == SDLK_AC_BACK &&
                Settings::getInstance()->getBool("BackEventAppExit") &&
                !AndroidVariables::sIsHomeApp) {
                SDL_Event quit {};
                quit.type = SDL_QUIT;
                SDL_PushEvent(&quit);
                return false;
            }
#endif
            // There is no need to handle the OS-default quit shortcut (Alt + F4 on Windows and
            // Linux and Command + Q on macOS) as that's taken care of by the window manager.
            // The exception is Android as there are are no default quit shortcuts on this OS.
            std::string quitShortcut {Settings::getInstance()->getString("KeyboardQuitShortcut")};
#if defined(__APPLE__)
            if (quitShortcut != "CmdQ") {
#elif defined(__ANDROID__)
            if (!AndroidVariables::sIsHomeApp) {
#else
            if (quitShortcut != "AltF4") {
#endif
                bool quitES {false};
#if defined(__ANDROID__)
                if (quitShortcut == "AltF4" && event.key.keysym.sym == SDLK_F4 &&
                    (event.key.keysym.mod & KMOD_LALT))
                    quitES = true;
                else if (quitShortcut == "F4" && event.key.keysym.sym == SDLK_F4 &&
#else
                if (quitShortcut == "F4" && event.key.keysym.sym == SDLK_F4 &&
#endif
                         !(event.key.keysym.mod & KMOD_LALT))
                    quitES = true;
                else if (quitShortcut == "CtrlQ" && event.key.keysym.sym == SDLK_q &&
                         event.key.keysym.mod & KMOD_CTRL)
                    quitES = true;
                else if (quitShortcut == "AltQ" && event.key.keysym.sym == SDLK_q &&
                         event.key.keysym.mod & KMOD_LALT)
                    quitES = true;

                if (quitES) {
                    SDL_Event quit {};
                    quit.type = SDL_QUIT;
                    SDL_PushEvent(&quit);
                    return false;
                }
            }

            if (Settings::getInstance()->getBool("InputIgnoreKeyboard"))
                return true;

            mWindow->input(getInputConfigByDevice(DEVICE_KEYBOARD),
                           Input(DEVICE_KEYBOARD, TYPE_KEY, event.key.keysym.sym, 1, false));
            return true;
        }
        case SDL_KEYUP: {
            if (Settings::getInstance()->getBool("InputIgnoreKeyboard"))
                return true;

            mWindow->input(getInputConfigByDevice(DEVICE_KEYBOARD),
                           Input(DEVICE_KEYBOARD, TYPE_KEY, event.key.keysym.sym, 0, false));
            return true;
        }
#if defined(__ANDROID__)
        case SDL_FINGERDOWN: {
            if (!Settings::getInstance()->getBool("InputTouchOverlay"))
                return false;

            const int buttonID {mInputOverlay.getButtonId(
                SDL_FINGERDOWN, event.tfinger.fingerId + 1, event.tfinger.x, event.tfinger.y)};
            if (buttonID != -2) {
                mWindow->input(getInputConfigByDevice(DEVICE_TOUCH),
                               Input(DEVICE_TOUCH, TYPE_TOUCH, buttonID, 1, false));
                return true;
            }
            else {
                return false;
            }
        }
        case SDL_FINGERUP: {
            if (!Settings::getInstance()->getBool("InputTouchOverlay"))
                return false;

            const int buttonID {mInputOverlay.getButtonId(SDL_FINGERUP, event.tfinger.fingerId + 1,
                                                          event.tfinger.x, event.tfinger.y)};
            if (buttonID != -2) {
                mWindow->input(getInputConfigByDevice(DEVICE_TOUCH),
                               Input(DEVICE_TOUCH, TYPE_TOUCH, buttonID, 0, false));

                return true;
            }
            else {
                return false;
            }
        }
        case SDL_FINGERMOTION: {
            if (!Settings::getInstance()->getBool("InputTouchOverlay"))
                return false;

            bool releasedButton {false};
            const int buttonID {
                mInputOverlay.getButtonId(SDL_FINGERMOTION, event.tfinger.fingerId + 1,
                                          event.tfinger.x, event.tfinger.y, &releasedButton)};

            if (buttonID == -2)
                return false;

            if (releasedButton) {
                mWindow->input(getInputConfigByDevice(DEVICE_TOUCH),
                               Input(DEVICE_TOUCH, TYPE_TOUCH, buttonID, 0, false));
                return true;
            }
            else {
                mWindow->input(getInputConfigByDevice(DEVICE_TOUCH),
                               Input(DEVICE_TOUCH, TYPE_TOUCH, buttonID, 1, false));
                return true;
            }
        }
#endif
        case SDL_TEXTINPUT: {
            mWindow->textInput(event.text.text);
            break;
        }
        case SDL_CONTROLLERDEVICEADDED: {
            addControllerByDeviceIndex(mWindow, event.cdevice.which);
            return true;
        }
        case SDL_CONTROLLERDEVICEREMOVED: {
            removeControllerByJoystickID(mWindow, event.cdevice.which);
            return false;
        }
    }

    if ((event.type == static_cast<unsigned int>(SDL_USER_CECBUTTONDOWN)) ||
        (event.type == static_cast<unsigned int>(SDL_USER_CECBUTTONUP))) {
        mWindow->input(getInputConfigByDevice(DEVICE_CEC),
                       Input(DEVICE_CEC, TYPE_CEC_BUTTON, event.user.code,
                             event.type == static_cast<unsigned int>(SDL_USER_CECBUTTONDOWN),
                             false));
        return true;
    }

    return false;
}

bool InputManager::loadInputConfig(InputConfig* config)
{
    if (!mConfigFileExists)
        return false;

    std::string path {getConfigPath()};

    pugi::xml_document doc;
#if defined(_WIN64)
    pugi::xml_parse_result res {doc.load_file(Utils::String::stringToWideString(path).c_str())};
#else
    pugi::xml_parse_result res {doc.load_file(path.c_str())};
#endif

    if (!res) {
        LOG(LogError) << "Couldn't parse the input configuration file: " << res.description();
        return false;
    }

    pugi::xml_node root {doc.child("inputList")};
    if (!root)
        return false;

    pugi::xml_node configNode {root.find_child_by_attribute("inputConfig", "deviceGUID",
                                                            config->getDeviceGUIDString().c_str())};

    // With the move to the SDL GameController API the button layout changed quite a lot, so
    // es_input.xml files generated using the old API will end up with a completely unusable
    // controller configuration. These older files had the configuration entry type set to
    // "joystick", so it's easy to ignore such entries by only accepting entries with the
    // type set to "controller" (which is now applied when saving the es_input.xml file).
    if (configNode && config->getDeviceName() != "Keyboard")
        if (!root.find_child_by_attribute("inputConfig", "type", "controller"))
            return false;

    if (!configNode)
        return false;

    config->loadFromXML(configNode);
    return true;
}

void InputManager::loadDefaultKBConfig()
{
    InputConfig* cfg {getInputConfigByDevice(DEVICE_KEYBOARD)};

    if (cfg->isConfigured())
        return;

    cfg->clear();
    cfg->mapInput("Up", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_UP, 1, true));
    cfg->mapInput("Down", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_DOWN, 1, true));
    cfg->mapInput("Left", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_LEFT, 1, true));
    cfg->mapInput("Right", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_RIGHT, 1, true));

    cfg->mapInput("A", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_RETURN, 1, true));
    cfg->mapInput("B", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_BACKSPACE, 1, true));
    cfg->mapInput("X", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_DELETE, 1, true));
#if defined(__APPLE__)
    cfg->mapInput("Y", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_PRINTSCREEN, 1, true));
#else
    cfg->mapInput("Y", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_INSERT, 1, true));
#endif
    cfg->mapInput("Back", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_F1, 1, true));
    cfg->mapInput("Start", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_ESCAPE, 1, true));

    cfg->mapInput("LeftShoulder", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_PAGEUP, 1, true));
    cfg->mapInput("RightShoulder", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_PAGEDOWN, 1, true));
    cfg->mapInput("LeftTrigger", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_HOME, 1, true));
    cfg->mapInput("RightTrigger", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_END, 1, true));

    cfg->mapInput("LeftThumbstickClick", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_F2, 1, true));
    cfg->mapInput("RightThumbstickClick", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_F3, 1, true));
}

void InputManager::loadDefaultControllerConfig(SDL_JoystickID deviceIndex)
{
    InputConfig* cfg {getInputConfigByDevice(deviceIndex)};

    if (cfg->isConfigured())
        return;

    // clang-format off
    cfg->mapInput("Up", Input(deviceIndex, TYPE_BUTTON, SDL_CONTROLLER_BUTTON_DPAD_UP, 1, true));
    cfg->mapInput("Down", Input(deviceIndex, TYPE_BUTTON, SDL_CONTROLLER_BUTTON_DPAD_DOWN, 1, true));
    cfg->mapInput("Left", Input(deviceIndex, TYPE_BUTTON, SDL_CONTROLLER_BUTTON_DPAD_LEFT, 1, true));
    cfg->mapInput("Right", Input(deviceIndex, TYPE_BUTTON, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, 1, true));
    cfg->mapInput("Start", Input(deviceIndex, TYPE_BUTTON, SDL_CONTROLLER_BUTTON_START, 1, true));
    cfg->mapInput("Back", Input(deviceIndex, TYPE_BUTTON, SDL_CONTROLLER_BUTTON_BACK, 1, true));
    cfg->mapInput("A", Input(deviceIndex, TYPE_BUTTON, SDL_CONTROLLER_BUTTON_A, 1, true));
    cfg->mapInput("B", Input(deviceIndex, TYPE_BUTTON, SDL_CONTROLLER_BUTTON_B, 1, true));
    cfg->mapInput("X", Input(deviceIndex, TYPE_BUTTON, SDL_CONTROLLER_BUTTON_X, 1, true));
    cfg->mapInput("Y", Input(deviceIndex, TYPE_BUTTON, SDL_CONTROLLER_BUTTON_Y, 1, true));
    cfg->mapInput("LeftShoulder", Input(deviceIndex, TYPE_BUTTON, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, 1, true));
    cfg->mapInput("RightShoulder", Input(deviceIndex, TYPE_BUTTON, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, 1, true));
    cfg->mapInput("LeftTrigger", Input(deviceIndex, TYPE_AXIS, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 1, true));
    cfg->mapInput("RightTrigger", Input(deviceIndex, TYPE_AXIS, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 1, true));
    cfg->mapInput("LeftThumbstickUp", Input(deviceIndex, TYPE_AXIS, SDL_CONTROLLER_AXIS_LEFTY, -1, true));
    cfg->mapInput("LeftThumbstickDown", Input(deviceIndex, TYPE_AXIS, SDL_CONTROLLER_AXIS_LEFTY, 1, true));
    cfg->mapInput("LeftThumbstickLeft", Input(deviceIndex, TYPE_AXIS, SDL_CONTROLLER_AXIS_LEFTX, -1, true));
    cfg->mapInput("LeftThumbstickRight", Input(deviceIndex, TYPE_AXIS, SDL_CONTROLLER_AXIS_LEFTX, 1, true));
    cfg->mapInput("LeftThumbstickClick", Input(deviceIndex, TYPE_BUTTON, SDL_CONTROLLER_BUTTON_LEFTSTICK, 1, true));
    cfg->mapInput("RightThumbstickUp", Input(deviceIndex, TYPE_AXIS, SDL_CONTROLLER_AXIS_RIGHTY, -1, true));
    cfg->mapInput("RightThumbstickDown", Input(deviceIndex, TYPE_AXIS, SDL_CONTROLLER_AXIS_RIGHTY, 1, true));
    cfg->mapInput("RightThumbstickLeft", Input(deviceIndex, TYPE_AXIS, SDL_CONTROLLER_AXIS_RIGHTX, -1, true));
    cfg->mapInput("RightThumbstickRight", Input(deviceIndex, TYPE_AXIS, SDL_CONTROLLER_AXIS_RIGHTX, 1, true));
    cfg->mapInput("RightThumbstickClick", Input(deviceIndex, TYPE_BUTTON, SDL_CONTROLLER_BUTTON_RIGHTSTICK, 1, true));
    // clang-format on
}

void InputManager::loadTouchConfig()
{
#if defined(__ANDROID__)
    InputConfig* cfg {mTouchInputConfig.get()};

    if (cfg->isConfigured())
        return;

    // clang-format off
    cfg->mapInput("Up", Input(DEVICE_TOUCH, TYPE_TOUCH, SDL_CONTROLLER_BUTTON_DPAD_UP, 1, true));
    cfg->mapInput("Down", Input(DEVICE_TOUCH, TYPE_TOUCH, SDL_CONTROLLER_BUTTON_DPAD_DOWN, 1, true));
    cfg->mapInput("Left", Input(DEVICE_TOUCH, TYPE_TOUCH, SDL_CONTROLLER_BUTTON_DPAD_LEFT, 1, true));
    cfg->mapInput("Right", Input(DEVICE_TOUCH, TYPE_TOUCH, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, 1, true));
    cfg->mapInput("Start", Input(DEVICE_TOUCH, TYPE_TOUCH, SDL_CONTROLLER_BUTTON_START, 1, true));
    cfg->mapInput("Back", Input(DEVICE_TOUCH, TYPE_TOUCH, SDL_CONTROLLER_BUTTON_BACK, 1, true));
    cfg->mapInput("A", Input(DEVICE_TOUCH, TYPE_TOUCH, SDL_CONTROLLER_BUTTON_A, 1, true));
    cfg->mapInput("B", Input(DEVICE_TOUCH, TYPE_TOUCH, SDL_CONTROLLER_BUTTON_B, 1, true));
    cfg->mapInput("X", Input(DEVICE_TOUCH, TYPE_TOUCH, SDL_CONTROLLER_BUTTON_X, 1, true));
    cfg->mapInput("Y", Input(DEVICE_TOUCH, TYPE_TOUCH, SDL_CONTROLLER_BUTTON_Y, 1, true));
    cfg->mapInput("LeftShoulder", Input(DEVICE_TOUCH, TYPE_TOUCH, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, 1, true));
    cfg->mapInput("RightShoulder", Input(DEVICE_TOUCH, TYPE_TOUCH, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, 1, true));
    cfg->mapInput("LeftTrigger", Input(DEVICE_TOUCH, TYPE_TOUCH, InputOverlay::TriggerButtons::TRIGGER_LEFT, 1, true));
    cfg->mapInput("RightTrigger", Input(DEVICE_TOUCH, TYPE_TOUCH, InputOverlay::TriggerButtons::TRIGGER_RIGHT, 1, true));
    // clang-format on
#endif
}

void InputManager::addControllerByDeviceIndex(Window* window, int deviceIndex)
{
    // Open joystick and add it to our list.
    SDL_GameController* controller {SDL_GameControllerOpen(deviceIndex)};

    if (controller == nullptr) {
        LOG(LogError) << "Couldn't add controller with device index " << deviceIndex << " ("
                      << SDL_GetError() << ")";
        return;
    }

    SDL_Joystick* joy {SDL_GameControllerGetJoystick(controller)};

    // Add it to our list so we can close it again later.
    SDL_JoystickID joyID {SDL_JoystickInstanceID(joy)};
    mJoysticks[joyID] = joy;
    mControllers[joyID] = controller;

    std::string guid(65, '\0');
    SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), &guid[0], 64);
    guid.erase(guid.find('\0'));

    if (guid.substr(0, 32) == "00000000000000000000000000000000") {
        // This can occur if there are SDL bugs or controller driver bugs.
        LOG(LogWarning)
            << "Attempted to add an invalid controller entry with zero GUID, buggy drivers?";
        SDL_GameControllerClose(controller);
        mControllers.erase(mControllers.find(joyID));
        mJoysticks.erase(mJoysticks.find(joyID));
        return;
    }

    mInputConfigs[joyID] =
        std::make_unique<InputConfig>(joyID, SDL_GameControllerName(mControllers[joyID]), guid);

    bool customConfig {loadInputConfig(mInputConfigs[joyID].get())};

#if SDL_MAJOR_VERSION > 2 || (SDL_MAJOR_VERSION == 2 && SDL_MINOR_VERSION > 0) ||                  \
    (SDL_MAJOR_VERSION == 2 && SDL_MINOR_VERSION == 0 && SDL_PATCHLEVEL >= 14)
    const std::string serialNumber {SDL_GameControllerGetSerial(controller) == nullptr ?
                                        "" :
                                        SDL_GameControllerGetSerial(controller)};
#else
    const std::string serialNumber;
#endif

    if (customConfig) {
        LOG(LogInfo) << "Added controller with custom configuration: \""
                     << SDL_GameControllerName(mControllers[joyID]) << "\" (GUID: " << guid
                     << ", serial number: " << (serialNumber == "" ? "n/a" : serialNumber)
                     << ", instance ID: " << joyID << ", device index: " << deviceIndex << ")";
    }
    else {
        loadDefaultControllerConfig(joyID);
        LOG(LogInfo) << "Added controller with default configuration: \""
                     << SDL_GameControllerName(mControllers[joyID]) << "\" (GUID: " << guid
                     << ", serial number: " << (serialNumber == "" ? "n/a" : serialNumber)
                     << ", instance ID: " << joyID << ", device index: " << deviceIndex << ")";
    }

    if (window != nullptr) {
        window->queueInfoPopup(
            Utils::String::format(
                _("ADDED INPUT DEVICE '%s'"),
                Utils::String::toUpper(std::string(SDL_GameControllerName(mControllers[joyID])))
                    .c_str()),
            4000);
    }

    int numAxes {SDL_JoystickNumAxes(joy)};
    int numButtons {SDL_JoystickNumButtons(joy)};

    for (int axis {0}; axis < numAxes; ++axis)
        mPrevAxisValues[std::make_pair(joyID, axis)] = 0;

    for (int button {0}; button < numButtons; ++button)
        mPrevButtonValues[std::make_pair(joyID, button)] = -1;
}

void InputManager::removeControllerByJoystickID(Window* window, SDL_JoystickID joyID)
{
    assert(joyID != -1);

    std::string guid(65, '\0');
    SDL_Joystick* joy {SDL_JoystickFromInstanceID(joyID)};
    SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), &guid[0], 64);
    guid.erase(guid.find('\0'));

    if (guid.substr(0, 32) == "00000000000000000000000000000000") {
        // This can occur if there are SDL bugs or controller driver bugs.
        LOG(LogWarning)
            << "Attempted to remove an invalid controller entry with zero GUID, buggy drivers?";
        return;
    }

#if SDL_MAJOR_VERSION > 2 || (SDL_MAJOR_VERSION == 2 && SDL_MINOR_VERSION > 0) ||                  \
    (SDL_MAJOR_VERSION == 2 && SDL_MINOR_VERSION == 0 && SDL_PATCHLEVEL >= 14)
    const std::string serialNumber {SDL_GameControllerGetSerial(mControllers[joyID]) == nullptr ?
                                        "" :
                                        SDL_GameControllerGetSerial(mControllers[joyID])};
#else
    const std::string serialNumber;
#endif

    LOG(LogInfo) << "Removed controller \"" << SDL_GameControllerName(mControllers[joyID])
                 << "\" (GUID: " << guid
                 << ", serial number: " << (serialNumber == "" ? "n/a" : serialNumber)
                 << ", instance ID: " << joyID << ")";

    if (window != nullptr) {
        window->queueInfoPopup(
            Utils::String::format(
                _("REMOVED INPUT DEVICE '%s'"),
                Utils::String::toUpper(std::string(SDL_GameControllerName(mControllers[joyID])))
                    .c_str()),
            4000);
    }

    // Delete mPrevAxisValues for the device.
    int axisEntries {static_cast<int>(mPrevAxisValues.size())};
    for (int i {0}; i < axisEntries; ++i) {
        auto entry = mPrevAxisValues.find(std::make_pair(joyID, i));
        if (entry != mPrevAxisValues.end()) {
            mPrevAxisValues.erase(entry);
        }
    }

    // Delete mPrevButtonValues for the device.
    int buttonEntries {static_cast<int>(mPrevButtonValues.size())};
    for (int i {0}; i < buttonEntries; ++i) {
        auto entry = mPrevButtonValues.find(std::make_pair(joyID, i));
        if (entry != mPrevButtonValues.end()) {
            mPrevButtonValues.erase(entry);
        }
    }

    auto it = mInputConfigs.find(joyID);
    mInputConfigs.erase(it);

    // Close the controller and remove its entry.
    auto controllerIt = mControllers.find(joyID);
    if (controllerIt != mControllers.cend()) {
        SDL_GameControllerClose(controllerIt->second);
        mControllers.erase(controllerIt);
    }
    else {
        LOG(LogError) << "Couldn't find controller to close (instance ID: " << joyID << ")";
    }

    // Remove the joystick entry.
    auto joystickIt = mJoysticks.find(joyID);
    if (joystickIt != mJoysticks.cend()) {
        mJoysticks.erase(joystickIt);
    }
    else {
        LOG(LogError) << "Couldn't find joystick entry to remove (instance ID: " << joyID << ")";
    }
}
