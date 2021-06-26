//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  InputManager.h
//
//  Low-level input handling.
//  Initiates and maps the keyboard and controllers.
//  Reads and writes the es_input.xml configuration file.
//

#ifndef ES_CORE_INPUT_MANAGER_H
#define ES_CORE_INPUT_MANAGER_H

#include <SDL2/SDL.h>

#if defined(__linux__) || defined(_WIN64)
#include <SDL2/SDL_joystick.h>
#else
#include "SDL_joystick.h"
#endif

#include <map>
#include <memory>
#include <string>

class InputConfig;
class Window;
union SDL_Event;

class InputManager
{
public:
    InputManager();
    virtual ~InputManager();
    static InputManager* getInstance();

    void init();
    void deinit();

    void writeDeviceConfig(InputConfig* config);
    void doOnFinish();

    static std::string getConfigPath();
    static std::string getTemporaryConfigPath();

    int getNumJoysticks();
    int getNumConfiguredDevices();
    int getAxisCountByDevice(int deviceId);
    int getButtonCountByDevice(int deviceId);

    std::string getDeviceGUIDString(int deviceId);
    InputConfig* getInputConfigByDevice(int deviceId);

    bool parseEvent(const SDL_Event& event, Window* window);

private:
    bool initialized() const;

    bool loadInputConfig(InputConfig* config);
    void loadDefaultKBConfig();
    void loadDefaultControllerConfig(SDL_JoystickID deviceIndex);

    void addControllerByDeviceIndex(int deviceIndex);
    void removeControllerByJoystickID(SDL_JoystickID joyID);

    static InputManager* sInstance;
    static const int DEADZONE = 23000;
    bool mConfigFileExists;

    std::map<SDL_JoystickID, SDL_Joystick*> mJoysticks;
    std::map<SDL_JoystickID, SDL_GameController*> mControllers;
    std::map<SDL_JoystickID, std::unique_ptr<InputConfig>> mInputConfigs;

    std::unique_ptr<InputConfig> mKeyboardInputConfig;
    std::unique_ptr<InputConfig> mCECInputConfig;

    std::map<std::pair<SDL_JoystickID, int>, int> mPrevAxisValues;
    std::map<std::pair<SDL_JoystickID, int>, int> mPrevButtonValues;
};

#endif // ES_CORE_INPUT_MANAGER_H
