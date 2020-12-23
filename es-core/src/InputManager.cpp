//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  InputManager.cpp
//
//  Low-level input handling.
//  Initiates and maps the keyboard and controllers.
//  Reads and writes the es_input.cfg configuration file.
//

#include "InputManager.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "CECInput.h"
#include "Log.h"
#include "Platform.h"
#include "Scripting.h"
#include "Window.h"

#include <SDL2/SDL.h>
#include <assert.h>
#include <iostream>
#include <pugixml.hpp>

#define KEYBOARD_GUID_STRING "-1"
#define CEC_GUID_STRING "-2"

// There are four distinct IDs used for joysticks:
// 1. Device index - this is the "lowest level" identifier, and is just the Nth joystick plugged
//    in to the system (like /dev/js#). It can change even if the device is the same, and is
//    only used to open joysticks (required to receive SDL events).
// 2. SDL_JoystickID - this is an ID for each joystick that is supposed to remain consistent
//    between plugging and unplugging. ES doesn't care if it does, though.
// 3. "Device ID" - this is something I made up and is what InputConfig's getDeviceID() returns.
//    This is actually just an SDL_JoystickID (also called instance ID), but -1 means "keyboard"
//    instead of "error."
// 4. Joystick GUID - this is some squashed version of joystick vendor, version, and a bunch of
//    other device-specific things. It should remain the same across runs of the program/system
//    restarts/device reordering and is what I use to identify which joystick to load.

// Hack for CEC support.
int SDL_USER_CECBUTTONDOWN = -1;
int SDL_USER_CECBUTTONUP   = -1;

InputManager* InputManager::mInstance = nullptr;

InputManager::InputManager() : mKeyboardInputConfig(nullptr)
{
}

InputManager::~InputManager()
{
    deinit();
}

InputManager* InputManager::getInstance()
{
    if (!mInstance)
        mInstance = new InputManager();

    return mInstance;
}

void InputManager::init()
{
    if (initialized())
        deinit();

    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,
        Settings::getInstance()->getBool("BackgroundJoystickInput") ? "1" : "0");
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    SDL_JoystickEventState(SDL_ENABLE);

    // First, open all currently present joysticks.
    int numJoysticks = SDL_NumJoysticks();
    for (int i = 0; i < numJoysticks; i++)
        addJoystickByDeviceIndex(i);

    mKeyboardInputConfig = new InputConfig(DEVICE_KEYBOARD, "Keyboard", KEYBOARD_GUID_STRING);
    loadInputConfig(mKeyboardInputConfig);

    SDL_USER_CECBUTTONDOWN = SDL_RegisterEvents(2);
    SDL_USER_CECBUTTONUP = SDL_USER_CECBUTTONDOWN + 1;
    CECInput::init();
    mCECInputConfig = new InputConfig(DEVICE_CEC, "CEC", CEC_GUID_STRING);
    loadInputConfig(mCECInputConfig);
}

void InputManager::addJoystickByDeviceIndex(int id)
{
    assert(id >= 0 && id < SDL_NumJoysticks());

    // Open joystick & add to our list.
    SDL_Joystick* joy = SDL_JoystickOpen(id);
    assert(joy);

    // Add it to our list so we can close it again later.
    SDL_JoystickID joyId = SDL_JoystickInstanceID(joy);
    mJoysticks[joyId] = joy;

    char guid[65];
    SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, 65);

    // Create the InputConfig.
    mInputConfigs[joyId] = new InputConfig(joyId, SDL_JoystickName(joy), guid);
    if (!loadInputConfig(mInputConfigs[joyId]))
        LOG(LogInfo) << "Added unconfigured joystick " << SDL_JoystickName(joy) << " (GUID: " <<
                guid << ", instance ID: " << joyId << ", device index: " << id << ").";
    else
        LOG(LogInfo) << "Added known joystick " << SDL_JoystickName(joy) << " (instance ID: " <<
                joyId << ", device index: " << id << ")";

    // Set up the prevAxisValues.
    int numAxes = SDL_JoystickNumAxes(joy);
    mPrevAxisValues[joyId] = new int[numAxes];
    // Initialize array to 0.
    std::fill(mPrevAxisValues[joyId], mPrevAxisValues[joyId] + numAxes, 0);
}

void InputManager::removeJoystickByJoystickID(SDL_JoystickID joyId)
{
    assert(joyId != -1);

    // Delete old prevAxisValues.
    auto axisIt = mPrevAxisValues.find(joyId);
    delete[] axisIt->second;
    mPrevAxisValues.erase(axisIt);

    // Delete old InputConfig.
    auto it = mInputConfigs.find(joyId);
    delete it->second;
    mInputConfigs.erase(it);

    // Close the joystick.
    auto joyIt = mJoysticks.find(joyId);
    if (joyIt != mJoysticks.cend()) {
        SDL_JoystickClose(joyIt->second);
        mJoysticks.erase(joyIt);
    }
    else {
        LOG(LogError) << "Could not find joystick to close (instance ID: " << joyId << ")";
    }
}

void InputManager::deinit()
{
    if (!initialized())
        return;

    for (auto it = mJoysticks.cbegin(); it != mJoysticks.cend(); it++)
        SDL_JoystickClose(it->second);
    mJoysticks.clear();

    for (auto it = mInputConfigs.cbegin(); it != mInputConfigs.cend(); it++)
        delete it->second;
    mInputConfigs.clear();

    for (auto it = mPrevAxisValues.cbegin(); it != mPrevAxisValues.cend(); it++)
        delete[] it->second;
    mPrevAxisValues.clear();

    if (mKeyboardInputConfig != nullptr) {
        delete mKeyboardInputConfig;
        mKeyboardInputConfig = nullptr;
    }

    if (mCECInputConfig != nullptr) {
        delete mCECInputConfig;
        mCECInputConfig = nullptr;
    }

    CECInput::deinit();

    SDL_JoystickEventState(SDL_DISABLE);
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

int InputManager::getNumJoysticks()
{
    int numJoysticks = 0;

    // This is a workaround to exclude the keyboard (ID -1) from the total joystick count.
    // It's incorrectly added when configuring the keyboard in GuiInputConfig, but I've
    // been unable to find a proper fix to not having it added to mJoysticks.
    for (auto it = mJoysticks.cbegin(); it != mJoysticks.cend(); it++) {
        if ((*it).first >= 0)
            numJoysticks += 1;
    }
    return numJoysticks;
}

int InputManager::getAxisCountByDevice(SDL_JoystickID id)
{
    return SDL_JoystickNumAxes(mJoysticks[id]);
}

int InputManager::getButtonCountByDevice(SDL_JoystickID id)
{
    if (id == DEVICE_KEYBOARD)
        return 120; // It's a lot, okay.
    else if (id == DEVICE_CEC)
    #if defined(HAVE_CECLIB)
        return CEC::CEC_USER_CONTROL_CODE_MAX;
    #else // HAVE_LIBCEF
        return 0;
    #endif // HAVE_CECLIB
    else
        return SDL_JoystickNumButtons(mJoysticks[id]);
}

InputConfig* InputManager::getInputConfigByDevice(int device)
{
    if (device == DEVICE_KEYBOARD)
        return mKeyboardInputConfig;
    else if (device == DEVICE_CEC)
        return mCECInputConfig;
    else
        return mInputConfigs[device];
}

bool InputManager::parseEvent(const SDL_Event& ev, Window* window)
{
    bool causedEvent = false;
    int32_t axisValue;

    switch (ev.type) {
    case SDL_JOYAXISMOTION:
        // This should hopefully prevent a potential crash if the controller was unplugged
        // while a game was running.
        if (!mInputConfigs[ev.jaxis.which])
            return false;

        axisValue = ev.jaxis.value;
        // For the analog trigger buttons, convert the negative<->positive axis values to only
        // positive values in order to avoid registering double inputs. This is only a
        // temporary solution until ES has been updated to use the SDL GameController API.
        if (ev.jaxis.axis == mInputConfigs[ev.jaxis.which]->getInputIDByName("lefttrigger") ||
                ev.jaxis.axis == mInputConfigs[ev.jaxis.which]->getInputIDByName("righttrigger")) {
            axisValue += 32768;
            axisValue /= 2;
        }

        // Check if the input value switched boundaries.
        if ((abs(axisValue) > DEADZONE) !=
                (abs(mPrevAxisValues[ev.jaxis.which][ev.jaxis.axis]) > DEADZONE)) {
            int normValue;
            if (abs(axisValue) <= DEADZONE) {
                normValue = 0;
            }
            else {
                if (axisValue > 0)
                    normValue = 1;
                else
                    normValue = -1;
            }

            window->input(getInputConfigByDevice(ev.jaxis.which), Input(ev.jaxis.which,
                    TYPE_AXIS, ev.jaxis.axis, normValue, false));
            causedEvent = true;
        }

        mPrevAxisValues[ev.jaxis.which][ev.jaxis.axis] = axisValue;
        return causedEvent;

    case SDL_JOYBUTTONDOWN:

    case SDL_JOYBUTTONUP:
        if (!mInputConfigs[ev.jaxis.which])
            return false;

        window->input(getInputConfigByDevice(ev.jbutton.which), Input(ev.jbutton.which,
                TYPE_BUTTON, ev.jbutton.button, ev.jbutton.state == SDL_PRESSED, false));
        return true;

    case SDL_JOYHATMOTION:
        if (!mInputConfigs[ev.jaxis.which])
            return false;

        window->input(getInputConfigByDevice(ev.jhat.which), Input(ev.jhat.which,
                TYPE_HAT, ev.jhat.hat, ev.jhat.value, false));
        return true;

    case SDL_KEYDOWN:
        if (ev.key.keysym.sym == SDLK_BACKSPACE && SDL_IsTextInputActive())
            window->textInput("\b");

        if (ev.key.repeat)
            return false;

        if (ev.key.keysym.sym == SDLK_F4) {
            SDL_Event quit;
            quit.type = SDL_QUIT;
            SDL_PushEvent(&quit);
            return false;
        }

        window->input(getInputConfigByDevice(DEVICE_KEYBOARD), Input(DEVICE_KEYBOARD,
                TYPE_KEY, ev.key.keysym.sym, 1, false));
        return true;

    case SDL_KEYUP:
        window->input(getInputConfigByDevice(DEVICE_KEYBOARD), Input(DEVICE_KEYBOARD,
                TYPE_KEY, ev.key.keysym.sym, 0, false));
        return true;

    case SDL_TEXTINPUT:
        window->textInput(ev.text.text);
        break;

    case SDL_JOYDEVICEADDED:
        // ev.jdevice.which is a device index.
        addJoystickByDeviceIndex(ev.jdevice.which);
        return true;

    case SDL_JOYDEVICEREMOVED:
        // ev.jdevice.which is an SDL_JoystickID (instance ID).
        removeJoystickByJoystickID(ev.jdevice.which);
        return false;
    }

    if ((ev.type == static_cast<unsigned int>(SDL_USER_CECBUTTONDOWN)) ||
            (ev.type == static_cast<unsigned int>(SDL_USER_CECBUTTONUP))) {
        window->input(getInputConfigByDevice(DEVICE_CEC), Input(DEVICE_CEC,
                TYPE_CEC_BUTTON, ev.user.code, ev.type ==
                static_cast<unsigned int>(SDL_USER_CECBUTTONDOWN), false));
        return true;
    }

    return false;
}

bool InputManager::loadInputConfig(InputConfig* config)
{
    std::string path = getConfigPath();
    if (!Utils::FileSystem::exists(path)) {
        if (config->getDeviceName() == "Keyboard") {
            LOG(LogDebug) << "InputManager::loadInputConfig(): Assigning default keyboard "
                    "mappings as there is no es_input.cfg configuration file";
            loadDefaultKBConfig();
            config->setDefaultConfigFlag();
        }
        return false;
    }

    pugi::xml_document doc;
    #if defined(_WIN64)
    pugi::xml_parse_result res = doc.load_file(Utils::String::stringToWideString(path).c_str());
    #else
    pugi::xml_parse_result res = doc.load_file(path.c_str());
    #endif

    if (!res) {
        LOG(LogError) << "Error parsing input config: " << res.description();
        return false;
    }

    pugi::xml_node root = doc.child("inputList");
    if (!root)
        return false;

    pugi::xml_node configNode = root.find_child_by_attribute("inputConfig",
            "deviceGUID", config->getDeviceGUIDString().c_str());
    if (!configNode)
        configNode = root.find_child_by_attribute("inputConfig",
                "deviceName", config->getDeviceName().c_str());
    if (!configNode) {
        if (config->getDeviceName() == "Keyboard") {
            LOG(LogDebug) << "InputManager::loadInputConfig(): Assigning default keyboard "
                    "mappings as there is no keyboard configuration in es_input.cfg.";
            loadDefaultKBConfig();
            config->setDefaultConfigFlag();
            return true;
        }
        else {
            return false;
        }
    }

    config->loadFromXML(configNode);
    return true;
}

// If there is no es_input.cfg file or if the user has not yet configured the keyboard
// mappings, then load these defaults.
void InputManager::loadDefaultKBConfig()
{
    InputConfig* cfg = getInputConfigByDevice(DEVICE_KEYBOARD);

    cfg->clear();
    cfg->mapInput("up", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_UP, 1, true));
    cfg->mapInput("down", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_DOWN, 1, true));
    cfg->mapInput("left", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_LEFT, 1, true));
    cfg->mapInput("right", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_RIGHT, 1, true));

    cfg->mapInput("a", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_RETURN, 1, true));
    cfg->mapInput("b", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_BACKSPACE, 1, true));
    cfg->mapInput("x", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_DELETE, 1, true));
    #if defined(__APPLE__)
    cfg->mapInput("y", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_PRINTSCREEN, 1, true));
    #else
    cfg->mapInput("y", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_INSERT, 1, true));
    #endif
    cfg->mapInput("start", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_ESCAPE, 1, true));
    cfg->mapInput("select", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_F1, 1, true));

    cfg->mapInput("leftshoulder", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_PAGEUP, 1, true));
    cfg->mapInput("rightshoulder", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_PAGEDOWN, 1, true));
    cfg->mapInput("lefttrigger", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_HOME, 1, true));
    cfg->mapInput("righttrigger", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_END, 1, true));
}

void InputManager::writeDeviceConfig(InputConfig* config)
{
    assert(initialized());

    std::string path = getConfigPath();

    LOG(LogDebug) << "InputManager::writeDeviceConfig(): "
            "Saving input configuration file to " << path;

    pugi::xml_document doc;

    if (Utils::FileSystem::exists(path)) {
        // Merge files.
        #if defined(_WIN64)
        pugi::xml_parse_result result =
                doc.load_file(Utils::String::stringToWideString(path).c_str());
        #else
        pugi::xml_parse_result result = doc.load_file(path.c_str());
        #endif
        if (!result) {
            LOG(LogError) << "Error parsing input config: " << result.description();
        }
        else {
            // Successfully loaded, delete the old entry if it exists.
            pugi::xml_node root = doc.child("inputList");
            if (root) {
                // If inputAction @type=onfinish is set, let doOnFinish command take care of
                // creating input configuration. We just put the input configuration into a
                // temporary input config file.
                pugi::xml_node actionnode = root.find_child_by_attribute("inputAction",
                        "type", "onfinish");
                if (actionnode) {
                    path = getTemporaryConfigPath();
                    doc.reset();
                    root = doc.append_child("inputList");
                    root.append_copy(actionnode);
                }
                else {
                    pugi::xml_node oldEntry = root.find_child_by_attribute("inputConfig",
                            "deviceGUID", config->getDeviceGUIDString().c_str());
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

    pugi::xml_node root = doc.child("inputList");
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
    loadInputConfig(config);
}

void InputManager::doOnFinish()
{
    assert(initialized());
    std::string path = getConfigPath();
    pugi::xml_document doc;

    if (Utils::FileSystem::exists(path)) {
        #if defined(_WIN64)
        pugi::xml_parse_result result =
                doc.load_file(Utils::String::stringToWideString(path).c_str());
        #else
        pugi::xml_parse_result result = doc.load_file(path.c_str());
        #endif

        if (!result) {
            LOG(LogError) << "Couldn't parse input config: " << result.description();
        }
        else {
            pugi::xml_node root = doc.child("inputList");
            if (root) {
                root = root.find_child_by_attribute("inputAction", "type", "onfinish");
                if (root) {
                    for (pugi::xml_node command = root.child("command"); command;
                            command = command.next_sibling("command")) {
                        std::string tocall = command.text().get();

                        LOG(LogInfo) << "	" << tocall;
                        std::cout << "==============================================\n"
                                "input config finish command:\n";
                        int exitCode = runSystemCommand(tocall);
                        std::cout << "==============================================\n";

                        if (exitCode != 0) {
                            LOG(LogWarning) << "...launch terminated with nonzero exit code " <<
                                    exitCode << "!";
                        }
                    }
                }
            }
        }
    }
}

std::string InputManager::getConfigPath()
{
    std::string path = Utils::FileSystem::getHomePath();
    path += "/.emulationstation/es_input.cfg";
    return path;
}

std::string InputManager::getTemporaryConfigPath()
{
    std::string path = Utils::FileSystem::getHomePath();
    path += "/.emulationstation/es_temporaryinput.cfg";
    return path;
}

bool InputManager::initialized() const
{
    return mKeyboardInputConfig != nullptr;
}

int InputManager::getNumConfiguredDevices()
{
    int num = 0;
    for (auto it = mInputConfigs.cbegin(); it != mInputConfigs.cend(); it++) {
        if (it->second->isConfigured())
            num++;
    }

    if (mKeyboardInputConfig->isConfigured())
        num++;

    if (mCECInputConfig->isConfigured())
        num++;

    return num;
}

std::string InputManager::getDeviceGUIDString(int deviceId)
{
    if (deviceId == DEVICE_KEYBOARD)
        return KEYBOARD_GUID_STRING;

    if (deviceId == DEVICE_CEC)
        return CEC_GUID_STRING;

    auto it = mJoysticks.find(deviceId);
    if (it == mJoysticks.cend()) {
        LOG(LogError) << "getDeviceGUIDString - deviceId " << deviceId << " not found!";
        return "something went horribly wrong";
    }

    char guid[65];
    SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(it->second), guid, 65);
    return std::string(guid);
}
