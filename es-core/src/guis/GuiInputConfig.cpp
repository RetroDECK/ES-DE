//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiInputConfig.cpp
//
//  Input device configuration GUI (for keyboards, joysticks and gamepads).
//

#include "guis/GuiInputConfig.h"

#include "components/ButtonComponent.h"
#include "components/MenuComponent.h"
#include "guis/GuiMsgBox.h"
#include "InputManager.h"
#include "Log.h"
#include "Window.h"

#define HOLD_TO_SKIP_MS 1000

struct InputConfigStructure {
    std::string name;
    const bool skippable;
    std::string dispName;
    std::string icon;
};

static const int inputCount = 24;

static const InputConfigStructure GUI_INPUT_CONFIG_LIST[inputCount] =
{
    { "Up",                   false, "D-PAD UP",               ":/help/dpad_up.svg" },
    { "Down",                 false, "D-PAD DOWN",             ":/help/dpad_down.svg" },
    { "Left",                 false, "D-PAD LEFT",             ":/help/dpad_left.svg" },
    { "Right",                false, "D-PAD RIGHT",            ":/help/dpad_right.svg" },
    { "Start",                false, "START",                  ":/help/button_start.svg" },
    { "Select",               false, "SELECT",                 ":/help/button_select.svg" },
    { "A",                    false, "BUTTON A",               ":/help/buttons_south_XBOX.svg" },
    { "B",                    false, "BUTTON B",               ":/help/buttons_east_XBOX.svg" },
    { "X",                    true,  "BUTTON X",               ":/help/buttons_west_XBOX.svg" },
    { "Y",                    true,  "BUTTON Y",               ":/help/buttons_north_XBOX.svg" },
    { "LeftShoulder",         true,  "LEFT SHOULDER",          ":/help/button_l.svg" },
    { "RightShoulder",        true,  "RIGHT SHOULDER",         ":/help/button_r.svg" },
    { "LeftTrigger",          true,  "LEFT TRIGGER",           ":/help/button_lt.svg" },
    { "RightTrigger",         true,  "RIGHT TRIGGER",          ":/help/button_rt.svg" },
    { "LeftThumbstickUp",     true,  "LEFT THUMBSTICK UP",     ":/help/thumbstick_up.svg" },
    { "LeftThumbstickDown",   true,  "LEFT THUMBSTICK DOWN",   ":/help/thumbstick_down.svg" },
    { "LeftThumbstickLeft",   true,  "LEFT THUMBSTICK LEFT",   ":/help/thumbstick_left.svg" },
    { "LeftThumbstickRight",  true,  "LEFT THUMBSTICK RIGHT",  ":/help/thumbstick_right.svg" },
    { "LeftThumbstickClick",  true,  "LEFT THUMBSTICK CLICK",  ":/help/thumbstick_click.svg" },
    { "RightThumbstickUp",    true,  "RIGHT THUMBSTICK UP",    ":/help/thumbstick_up.svg" },
    { "RightThumbstickDown",  true,  "RIGHT THUMBSTICK DOWN",  ":/help/thumbstick_down.svg" },
    { "RightThumbstickLeft",  true,  "RIGHT THUMBSTICK LEFT",  ":/help/thumbstick_left.svg" },
    { "RightThumbstickRight", true,  "RIGHT THUMBSTICK RIGHT", ":/help/thumbstick_right.svg" },
    { "RightThumbstickClick", true,  "RIGHT THUMBSTICK CLICK", ":/help/thumbstick_click.svg" },
//    { "HotKeyEnable",         true,  "HOTKEY ENABLE",          ":/help/button_hotkey.svg" }
};

GuiInputConfig::GuiInputConfig(
        Window* window,
        InputConfig* target,
        bool reconfigureAll,
        const std::function<void()>& okCallback)
        : GuiComponent(window),
        mBackground(window, ":/graphics/frame.svg"),
        mGrid(window, Vector2i(1, 7)),
        mTargetConfig(target),
        mHoldingInput(false)
{
    LOG(LogInfo) << "Configuring device " << target->getDeviceId() << " (" <<
            target->getDeviceName() << ").";

    if (reconfigureAll)
        target->clear();

    mConfiguringAll = reconfigureAll;
    mConfiguringRow = mConfiguringAll;

    addChild(&mBackground);
    addChild(&mGrid);

    // 0 is a spacer row.
    mGrid.setEntry(std::make_shared<GuiComponent>(mWindow), Vector2i(0, 0), false);

    mTitle = std::make_shared<TextComponent>(mWindow, "CONFIGURING",
            Font::get(FONT_SIZE_LARGE), 0x555555FF, ALIGN_CENTER);
    mGrid.setEntry(mTitle, Vector2i(0, 1), false, true);

    std::stringstream ss;
    if (target->getDeviceId() == DEVICE_KEYBOARD)
        ss << "KEYBOARD";
    else if (target->getDeviceId() == DEVICE_CEC)
        ss << "CEC";
    else
        ss << "GAMEPAD " << (target->getDeviceId() + 1);
    mSubtitle1 = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(ss.str()),
            Font::get(FONT_SIZE_MEDIUM), 0x555555FF, ALIGN_CENTER);
    mGrid.setEntry(mSubtitle1, Vector2i(0, 2), false, true);

    mSubtitle2 = std::make_shared<TextComponent>(mWindow, "HOLD ANY BUTTON 1 SECOND TO SKIP",
            Font::get(FONT_SIZE_SMALL), 0x999999FF, ALIGN_CENTER);
    // The opacity will be set to visible for any row that is skippable.
    mSubtitle2->setOpacity(0);

    mGrid.setEntry(mSubtitle2, Vector2i(0, 3), false, true);

    // 4 is a spacer row.
    mList = std::make_shared<ComponentList>(mWindow);
    mGrid.setEntry(mList, Vector2i(0, 5), true, true);

    for (int i = 0; i < inputCount; i++) {
        ComponentListRow row;

        // Icon.
        auto icon = std::make_shared<ImageComponent>(mWindow);
        icon->setImage(GUI_INPUT_CONFIG_LIST[i].icon);
        icon->setColorShift(0x777777FF);
        icon->setResize(0, Font::get(FONT_SIZE_MEDIUM)->getLetterHeight() * 1.25f);
        row.addElement(icon, false);

        // Spacer between icon and text.
        auto spacer = std::make_shared<GuiComponent>(mWindow);
        spacer->setSize(16, 0);
        row.addElement(spacer, false);

        auto text = std::make_shared<TextComponent>(mWindow,
                GUI_INPUT_CONFIG_LIST[i].dispName, Font::get(FONT_SIZE_MEDIUM), 0x777777FF);
        row.addElement(text, true);

        auto mapping = std::make_shared<TextComponent>(mWindow, "-NOT DEFINED-",
                Font::get(FONT_SIZE_MEDIUM, FONT_PATH_LIGHT), 0x999999FF, ALIGN_RIGHT);
        setNotDefined(mapping); // Overrides the text and color set above.
        row.addElement(mapping, true);
        mMappings.push_back(mapping);

        row.input_handler = [this, i, mapping](InputConfig* config, Input input) -> bool {
            // Ignore input not from our target device.
            if (config != mTargetConfig)
                return false;

            // If we're not configuring, start configuring when A is pressed.
            if (!mConfiguringRow) {
                if (config->isMappedTo("a", input) && input.value) {
                    mList->stopScrolling();
                    mConfiguringRow = true;
                    setPress(mapping);
                    return true;
                }
                // We're not configuring and A wasn't pressed to start, so ignore this input.
                return false;
            }

            // We are configuring.
            if (input.value != 0) {
                // Button pressed. If we're already holding something, ignore this,
                // otherwise plan to map this input.
                if (mHoldingInput)
                    return true;

                mHoldingInput = true;
                mHeldInput = input;
                mHeldTime = 0;
                mHeldInputId = i;

                return true;
            }
            else {
                // Button released. Make sure we were holding something and we let go of
                // what we were previously holding.
                if (!mHoldingInput || mHeldInput.device != input.device || mHeldInput.id !=
                        input.id || mHeldInput.type != input.type)
                    return true;

                mHoldingInput = false;

                if (assign(mHeldInput, i))
                    // If successful, move cursor/stop configuring - if not, we'll just try again.
                    rowDone();

                return true;
            }
        };
        mList->addRow(row);
    }

    // Only show "HOLD TO SKIP" if this input is skippable.
    mList->setCursorChangedCallback([this](CursorState) {
        bool skippable = GUI_INPUT_CONFIG_LIST[mList->getCursorId()].skippable;
        mSubtitle2->setOpacity(skippable * 255);
    });

    // Make the first one say "PRESS ANYTHING" if we're re-configuring everything.
    if (mConfiguringAll)
        setPress(mMappings.front());

    // GUI buttons.
    std::vector<std::shared_ptr<ButtonComponent>> buttons;
    std::function<void()> okFunction = [this, okCallback] {
        InputManager::getInstance()->writeDeviceConfig(mTargetConfig); // Save.
        if (okCallback)
            okCallback();
        delete this;
    };

    buttons.push_back(std::make_shared<ButtonComponent>
            (mWindow, "OK", "ok", [this, okFunction] { okFunction(); }));

//    This code is disabled as there is no intention to provide emulator configuration or
//    similar control via ES-DE. Let's keep the code for reference though.
//    buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "OK", "ok", [this, okFunction] {
//        // Check if the hotkey enable button is set. if not prompt the
//        // user to use select or nothing.
//        Input input;
//        okFunction();
//        if (!mTargetConfig->getInputByName("HotKeyEnable", &input)) {
//            mWindow->pushGui(new GuiMsgBox(mWindow, getHelpStyle(),
//                    "YOU DIDN'T CHOOSE A HOTKEY ENABLE BUTTON. THIS IS REQUIRED FOR EXITING "
//                    "GAMES WITH A CONTROLLER. DO YOU WANT TO USE THE SELECT BUTTON DEFAULT ? "
//                    "PLEASE ANSWER YES TO USE SELECT OR NO TO NOT SET A HOTKEY ENABLE BUTTON.",
//                    "YES", [this, okFunction] {
//                Input input;
//                mTargetConfig->getInputByName("Select", &input);
//                mTargetConfig->mapInput("HotKeyEnable", input);
//                okFunction();
//            },
//            "NO", [this, okFunction] {
//                // For a disabled hotkey enable button, set to a key with id 0,
//                // so the input configuration script can be backwards compatible.
//                mTargetConfig->mapInput("HotKeyEnable", Input(DEVICE_KEYBOARD,
//                    TYPE_KEY, 0, 1, true));
//                okFunction();
//            }));
//        }
//        else {
//            okFunction();
//        }
//    }));

    mButtonGrid = makeButtonGrid(mWindow, buttons);
    mGrid.setEntry(mButtonGrid, Vector2i(0, 6), true, false);

    // For narrower displays (e.g. in 4:3 ratio), allow the window to fill 80% of the screen
    // width rather than the 60% allowed for wider displays.
    float width = Renderer::getScreenWidth() *
            ((Renderer::getScreenAspectRatio() < 1.4f) ? 0.8f : 0.6f);

    setSize(width, Renderer::getScreenHeight() * 0.75f);
    setPosition((Renderer::getScreenWidth() - mSize.x()) / 2.0f,
            (Renderer::getScreenHeight() - mSize.y()) / 2.0f);
}

void GuiInputConfig::update(int deltaTime)
{
    if (mConfiguringRow && mHoldingInput && GUI_INPUT_CONFIG_LIST[mHeldInputId].skippable) {
        int prevSec = mHeldTime / 1000;
        mHeldTime += deltaTime;
        int curSec = mHeldTime / 1000;

        if (mHeldTime >= HOLD_TO_SKIP_MS) {
            setNotDefined(mMappings.at(mHeldInputId));
            clearAssignment(mHeldInputId);
            mHoldingInput = false;
            rowDone();
        }
        else {
            if (prevSec != curSec) {
                // Crossed the second boundary, update text.
                const auto& text = mMappings.at(mHeldInputId);
                std::stringstream ss;
                ss << "HOLD FOR " << HOLD_TO_SKIP_MS / 1000 - curSec << "S TO SKIP";
                text->setText(ss.str());
                text->setColor(0x777777FF);
            }
        }
    }
}

void GuiInputConfig::onSizeChanged()
{
    mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

    // Update grid.
    mGrid.setSize(mSize);

    mGrid.setRowHeightPerc(1, mTitle->getFont()->getHeight() * 0.75f / mSize.y());
    mGrid.setRowHeightPerc(2, mSubtitle1->getFont()->getHeight() / mSize.y());
    mGrid.setRowHeightPerc(3, mSubtitle2->getFont()->getHeight() / mSize.y());
    mGrid.setRowHeightPerc(5, (mList->getRowHeight(0) * 5 + 2) / mSize.y());
    mGrid.setRowHeightPerc(6, mButtonGrid->getSize().y() / mSize.y());
}

void GuiInputConfig::rowDone()
{
    if (mConfiguringAll) {
        // Try to move to the next row.
        if (!mList->moveCursor(1)) {
            // At bottom of list, we're done.
            mConfiguringAll = false;
            mConfiguringRow = false;
            mGrid.moveCursor(Vector2i(0, 1));
        }
        else {
            // On another row.
            setPress(mMappings.at(mList->getCursorId()));
        }
    }
    else {
        // Only configuring one row, so stop.
        mConfiguringRow = false;
    }
}

void GuiInputConfig::error(const std::shared_ptr<TextComponent>& text, const std::string& /*msg*/)
{
    text->setText("ALREADY TAKEN");
    text->setColor(0x656565FF);
}

void GuiInputConfig::setPress(const std::shared_ptr<TextComponent>& text)
{
    text->setText("PRESS ANYTHING");
    text->setColor(0x656565FF);
}

void GuiInputConfig::setNotDefined(const std::shared_ptr<TextComponent>& text)
{
    text->setText("-NOT DEFINED-");
    text->setColor(0x999999FF);
}

void GuiInputConfig::setAssignedTo(const std::shared_ptr<TextComponent>& text, Input input)
{
    text->setText(Utils::String::toUpper(input.string()));
    text->setColor(0x777777FF);
}

bool GuiInputConfig::assign(Input input, int inputId)
{
    // Input is from InputConfig* mTargetConfig.

    // If this input is mapped to something other than "nothing" or the current row,
    // generate an error. (If it's the same as what it was before, allow it.)
    if (mTargetConfig->getMappedTo(input).size() > 0 &&
            !mTargetConfig->isMappedTo(GUI_INPUT_CONFIG_LIST[inputId].name, input) &&
            GUI_INPUT_CONFIG_LIST[inputId].name != "HotKeyEnable") {
        error(mMappings.at(inputId), "Already mapped!");
        return false;
    }

    setAssignedTo(mMappings.at(inputId), input);

    input.configured = true;
    mTargetConfig->mapInput(GUI_INPUT_CONFIG_LIST[inputId].name, input);

    LOG(LogInfo) << "Mapping [" << input.string() << "] to [" <<
            GUI_INPUT_CONFIG_LIST[inputId].name << "]";

    return true;
}

void GuiInputConfig::clearAssignment(int inputId)
{
    mTargetConfig->unmapInput(GUI_INPUT_CONFIG_LIST[inputId].name);
}
