//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiDetectDevice.cpp
//
//  Detect input devices (keyboards, joysticks and gamepads).
//

#include "guis/GuiDetectDevice.h"

#include "InputManager.h"
#include "Window.h"
#include "components/TextComponent.h"
#include "guis/GuiInputConfig.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

#define HOLD_TIME 1000.0f

GuiDetectDevice::GuiDetectDevice(bool firstRun,
                                 bool forcedConfig,
                                 const std::function<void()>& doneCallback)
    : mFirstRun {firstRun}
    , mForcedConfig {forcedConfig}
    , mRenderer {Renderer::getInstance()}
    , mBackground {":/graphics/frame.svg"}
    , mGrid {glm::ivec2 {1, 5}}
{
    mHoldingConfig = nullptr;
    mHoldTime = 0;
    mDoneCallback = doneCallback;

    addChild(&mBackground);
    addChild(&mGrid);

    // Title.
    mTitle = std::make_shared<TextComponent>(firstRun ? "WELCOME" : "CONFIGURE INPUT DEVICE",
                                             Font::get(FONT_SIZE_LARGE), 0x555555FF, ALIGN_CENTER);
    mGrid.setEntry(mTitle, glm::ivec2 {0, 0}, false, true, glm::ivec2 {1, 1},
                   GridFlags::BORDER_BOTTOM);

    // Device info.
    std::stringstream deviceInfo;
    int numDevices {InputManager::getInstance().getNumJoysticks()};

    if (numDevices > 0)
        deviceInfo << numDevices << " GAMEPAD" << (numDevices > 1 ? "S" : "") << " DETECTED";
    else
        deviceInfo << "NO GAMEPADS DETECTED";

    if (numDevices > 1 && Settings::getInstance()->getBool("InputOnlyFirstController"))
        deviceInfo << " (ONLY ACCEPTING INPUT FROM FIRST CONTROLLER)";

    mDeviceInfo = std::make_shared<TextComponent>(deviceInfo.str(), Font::get(FONT_SIZE_SMALL),
                                                  0x999999FF, ALIGN_CENTER);
    mGrid.setEntry(mDeviceInfo, glm::ivec2 {0, 1}, false, true);

    // Message.
    if (numDevices > 0) {
        mMsg1 = std::make_shared<TextComponent>(
            "HOLD A BUTTON ON YOUR GAMEPAD OR KEYBOARD TO CONFIGURE IT", Font::get(FONT_SIZE_SMALL),
            0x777777FF, ALIGN_CENTER);
    }
    else {
        mMsg1 =
            std::make_shared<TextComponent>("HOLD A BUTTON ON YOUR KEYBOARD TO CONFIGURE IT",
                                            Font::get(FONT_SIZE_SMALL), 0x777777FF, ALIGN_CENTER);
    }

    mGrid.setEntry(mMsg1, glm::ivec2 {0, 2}, false, true);

    const std::string msg2str {firstRun ? "PRESS ESC TO SKIP (OR F4 TO QUIT AT ANY TIME)" :
                                          "PRESS ESC TO CANCEL"};
    mMsg2 = std::make_shared<TextComponent>(msg2str, Font::get(FONT_SIZE_SMALL), 0x777777FF,
                                            ALIGN_CENTER);
    mGrid.setEntry(mMsg2, glm::ivec2 {0, 3}, false, true);

    // Currently held device.
    mDeviceHeld =
        std::make_shared<TextComponent>("", Font::get(FONT_SIZE_MEDIUM), 0xFFFFFFFF, ALIGN_CENTER);
    mGrid.setEntry(mDeviceHeld, glm::ivec2 {0, 4}, false, true);

    // Adjust the width relative to the aspect ratio of the screen to make the GUI look coherent
    // regardless of screen type. The 1.778 aspect ratio value is the 16:9 reference.
    const float aspectValue {1.778f / mRenderer->getScreenAspectRatio()};
    const float width {glm::clamp(0.60f * aspectValue, 0.50f,
                                  (mRenderer->getIsVerticalOrientation() ? 0.85f : 0.80f)) *
                       mRenderer->getScreenWidth()};

    setSize(width, mRenderer->getScreenHeight() * 0.5f);
    setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f,
                (mRenderer->getScreenHeight() - mSize.y) / 2.0f);
}

void GuiDetectDevice::onSizeChanged()
{
    mBackground.fitTo(mSize, glm::vec3 {}, glm::vec2 {-32.0f, -32.0f});

    // Grid.
    mGrid.setSize(mSize);
    mGrid.setRowHeightPerc(0, mTitle->getFont()->getHeight() / mSize.y);
    mGrid.setRowHeightPerc(2, mMsg1->getFont()->getHeight() / mSize.y);
    mGrid.setRowHeightPerc(3, mMsg2->getFont()->getHeight() / mSize.y);
}

bool GuiDetectDevice::input(InputConfig* config, Input input)
{
    if (!mFirstRun && input.device == DEVICE_KEYBOARD && input.type == TYPE_KEY && input.value &&
        input.id == SDLK_ESCAPE) {
        if (mDoneCallback)
            mDoneCallback();
        // Cancel the configuration.
        delete this; // Delete GUI element.
        return true;
    }
    // First run, but the user chooses to skip the configuration. This will default to the
    // built-in keyboard mappings.
    else if (mFirstRun && input.device == DEVICE_KEYBOARD && input.type == TYPE_KEY &&
             input.value && input.id == SDLK_ESCAPE) {
        if (mDoneCallback)
            mDoneCallback();
        delete this; // Delete GUI element.
        return true;
    }

    if (input.type == TYPE_BUTTON || input.type == TYPE_AXIS || input.type == TYPE_KEY ||
        input.type == TYPE_CEC_BUTTON) {
        if (input.value && mHoldingConfig == nullptr) {
            // Started holding.
            mHoldingConfig = config;
            mHoldTime = static_cast<int>(HOLD_TIME);
            mDeviceHeld->setText(Utils::String::toUpper(config->getDeviceName()));
        }
        else if (!input.value && mHoldingConfig == config) {
            // Cancel.
            mHoldingConfig = nullptr;
            mDeviceHeld->setText("");
        }
    }
    return true;
}

void GuiDetectDevice::update(int deltaTime)
{
    if (mHoldingConfig) {
        // If ES-DE starts and if a known device is connected after startup, then skip
        // controller configuration unless the flag to force the configuration was passed
        // on the command line.
        if (!mForcedConfig && mFirstRun &&
            Utils::FileSystem::exists(InputManager::getConfigPath()) &&
            InputManager::getInstance().getNumConfiguredDevices() > 0) {
            if (mDoneCallback)
                mDoneCallback();
            delete this; // Delete GUI element.
        }
        else {
            mHoldTime -= deltaTime;
            // Fade in device name.
            const float t {std::fabs((static_cast<float>(mHoldTime) / HOLD_TIME) - 1.0f)};
            mDeviceHeld->setColor(0x44444400 | static_cast<unsigned char>(t * 255.0f));
            if (mHoldTime <= 0) {
                // A device was selected.
                mWindow->pushGui(new GuiInputConfig(mHoldingConfig, true, mDoneCallback));
                delete this;
            }
        }
    }
}
