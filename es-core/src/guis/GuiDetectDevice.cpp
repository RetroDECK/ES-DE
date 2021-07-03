//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiDetectDevice.cpp
//
//  Detect input devices (keyboards, joysticks and gamepads).
//

#include "guis/GuiDetectDevice.h"

#include "components/TextComponent.h"
#include "guis/GuiInputConfig.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "InputManager.h"
#include "Window.h"

#define HOLD_TIME 1000

GuiDetectDevice::GuiDetectDevice(
        Window* window,
        bool firstRun,
        bool forcedConfig,
        const std::function<void()>& doneCallback)
        : GuiComponent(window),
        mFirstRun(firstRun),
        mForcedConfig(forcedConfig),
        mBackground(window, ":/graphics/frame.svg"),
        mGrid(window, Vector2i(1, 5))
{
    mHoldingConfig = nullptr;
    mHoldTime = 0;
    mDoneCallback = doneCallback;

    addChild(&mBackground);
    addChild(&mGrid);

    // Title.
    mTitle = std::make_shared<TextComponent>(mWindow, firstRun ? "WELCOME" :
        "CONFIGURE INPUT DEVICE", Font::get(FONT_SIZE_LARGE), 0x555555FF, ALIGN_CENTER);
    mGrid.setEntry(mTitle, Vector2i(0, 0), false, true, Vector2i(1, 1), GridFlags::BORDER_BOTTOM);

    // Device info.
    std::stringstream deviceInfo;
    int numDevices = InputManager::getInstance()->getNumJoysticks();

    if (numDevices > 0)
        deviceInfo << numDevices << " GAMEPAD" << (numDevices > 1 ? "S" : "") << " DETECTED";
    else
        deviceInfo << "NO GAMEPADS DETECTED";

    if (numDevices > 1 && Settings::getInstance()->getBool("InputOnlyFirstController"))
        deviceInfo << " (ONLY ACCEPTING INPUT FROM FIRST CONTROLLER)";

    mDeviceInfo = std::make_shared<TextComponent>(mWindow, deviceInfo.str(),
            Font::get(FONT_SIZE_SMALL), 0x999999FF, ALIGN_CENTER);
    mGrid.setEntry(mDeviceInfo, Vector2i(0, 1), false, true);

    // Message.
    if (numDevices > 0) {
        mMsg1 = std::make_shared<TextComponent>(mWindow,
                "HOLD A BUTTON ON YOUR GAMEPAD OR KEYBOARD TO CONFIGURE IT",
                Font::get(FONT_SIZE_SMALL), 0x777777FF, ALIGN_CENTER);
    }
    else {
        mMsg1 = std::make_shared<TextComponent>(mWindow,
                "HOLD A BUTTON ON YOUR KEYBOARD TO CONFIGURE IT",
                Font::get(FONT_SIZE_SMALL), 0x777777FF, ALIGN_CENTER);
    }

    mGrid.setEntry(mMsg1, Vector2i(0, 2), false, true);

    const std::string msg2str = firstRun ?
            "PRESS ESC TO SKIP (OR F4 TO QUIT AT ANY TIME)" : "PRESS ESC TO CANCEL";
    mMsg2 = std::make_shared<TextComponent>(mWindow, msg2str,
            Font::get(FONT_SIZE_SMALL), 0x777777FF, ALIGN_CENTER);
    mGrid.setEntry(mMsg2, Vector2i(0, 3), false, true);

    // Currently held device.
    mDeviceHeld = std::make_shared<TextComponent>(mWindow, "",
            Font::get(FONT_SIZE_MEDIUM), 0xFFFFFFFF, ALIGN_CENTER);
    mGrid.setEntry(mDeviceHeld, Vector2i(0, 4), false, true);

    // Adjust the width relative to the aspect ratio of the screen to make the GUI look coherent
    // regardless of screen type. The 1.778 aspect ratio value is the 16:9 reference.
    float aspectValue = 1.778f / Renderer::getScreenAspectRatio();
    float width = Math::clamp(0.60f * aspectValue, 0.50f, 0.80f) * Renderer::getScreenWidth();

    setSize(width, Renderer::getScreenHeight() * 0.5f);
    setPosition((Renderer::getScreenWidth() - mSize.x()) / 2,
            (Renderer::getScreenHeight() - mSize.y()) / 2);
}

void GuiDetectDevice::onSizeChanged()
{
    mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

    // Grid.
    mGrid.setSize(mSize);
    mGrid.setRowHeightPerc(0, mTitle->getFont()->getHeight() / mSize.y());
    //mGrid.setRowHeightPerc(1, mDeviceInfo->getFont()->getHeight() / mSize.y());
    mGrid.setRowHeightPerc(2, mMsg1->getFont()->getHeight() / mSize.y());
    mGrid.setRowHeightPerc(3, mMsg2->getFont()->getHeight() / mSize.y());
    //mGrid.setRowHeightPerc(4, mDeviceHeld->getFont()->getHeight() / mSize.y());
}

bool GuiDetectDevice::input(InputConfig* config, Input input)
{
    if (!mFirstRun && input.device == DEVICE_KEYBOARD && input.type == TYPE_KEY &&
            input.value && input.id == SDLK_ESCAPE) {
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
            mHoldTime = HOLD_TIME;
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
        // If ES starts and if a known device is connected after startup skip controller
        // configuration unless the flag to force the configuration was passed on the
        // command line.
        if (!mForcedConfig && mFirstRun &&
                Utils::FileSystem::exists(InputManager::getConfigPath()) &&
                InputManager::getInstance()->getNumConfiguredDevices() > 0) {
            if (mDoneCallback)
                mDoneCallback();
            delete this; // Delete GUI element.
        }
        else {
            mHoldTime -= deltaTime;
            const float t = static_cast<float>(mHoldTime) / HOLD_TIME;
            unsigned int c = static_cast<unsigned char>(t * 255);
            mDeviceHeld->setColor((c << 24) | (c << 16) | (c << 8) | 0xFF);
            if (mHoldTime <= 0) {
                // Picked one!
                mWindow->pushGui(new GuiInputConfig(mWindow, mHoldingConfig, true, mDoneCallback));
                delete this;
            }
        }
    }
}
