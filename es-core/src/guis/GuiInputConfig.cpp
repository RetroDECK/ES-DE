//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiInputConfig.cpp
//
//  Input device configuration GUI (for keyboards, joysticks and gamepads).
//

#include "guis/GuiInputConfig.h"

#include "InputManager.h"
#include "Log.h"
#include "components/ButtonComponent.h"
#include "components/MenuComponent.h"
#include "guis/GuiMsgBox.h"

#define HOLD_TO_SKIP_MS 1000

struct InputConfigStructure {
    std::string name;
    bool skippable;
    std::string dispName;
    std::string icon;
};

static const int inputCount = 24;
static InputConfigStructure sGuiInputConfigList[inputCount];

GuiInputConfig::GuiInputConfig(InputConfig* target,
                               bool reconfigureAll,
                               const std::function<void()>& okCallback)
    : mBackground {":/graphics/frame.svg"}
    , mGrid {glm::ivec2 {1, 7}}
    , mTargetConfig {target}
    , mHoldingInput {false}
{
    // Populate the configuration list with the text and icons applicable to the
    // configured controller type.
    populateConfigList();

    LOG(LogInfo) << "Configuring device " << target->getDeviceId() << " ("
                 << target->getDeviceName() << ").";

    if (reconfigureAll)
        target->clear();

    mConfiguringAll = reconfigureAll;
    mConfiguringRow = mConfiguringAll;

    addChild(&mBackground);
    addChild(&mGrid);

    // 0 is a spacer row.
    mGrid.setEntry(std::make_shared<GuiComponent>(), glm::ivec2 {0, 0}, false);

    mTitle = std::make_shared<TextComponent>("CONFIGURING", Font::get(FONT_SIZE_LARGE), 0x555555FF,
                                             ALIGN_CENTER);
    mGrid.setEntry(mTitle, glm::ivec2 {0, 1}, false, true);

    std::stringstream ss;
    if (target->getDeviceId() == DEVICE_KEYBOARD)
        ss << "KEYBOARD";
    else if (target->getDeviceId() == DEVICE_CEC)
        ss << "CEC";
    else
        ss << "GAMEPAD " << (target->getDeviceId() + 1) << " (" << target->getDeviceName() << ")";
    mSubtitle1 = std::make_shared<TextComponent>(
        Utils::String::toUpper(ss.str()), Font::get(FONT_SIZE_MEDIUM), 0x555555FF, ALIGN_CENTER);
    mGrid.setEntry(mSubtitle1, glm::ivec2 {0, 2}, false, true);

    mSubtitle2 = std::make_shared<TextComponent>(
        "HOLD ANY BUTTON 1 SECOND TO SKIP", Font::get(FONT_SIZE_SMALL), 0x999999FF, ALIGN_CENTER);
    // The opacity will be set to visible for any row that is skippable.
    mSubtitle2->setOpacity(0.0f);

    mGrid.setEntry(mSubtitle2, glm::ivec2 {0, 3}, false, true);

    // 4 is a spacer row.
    mList = std::make_shared<ComponentList>();
    mGrid.setEntry(mList, glm::ivec2 {0, 5}, true, true);

    for (int i {0}; i < inputCount; ++i) {
        ComponentListRow row;

        // Icon.
        auto icon = std::make_shared<ImageComponent>();
        icon->setResize(0, Font::get(FONT_SIZE_MEDIUM)->getLetterHeight() * 1.25f);
        icon->setImage(sGuiInputConfigList[i].icon);
        icon->setColorShift(0x777777FF);
        row.addElement(icon, false);

        // Spacer between icon and text.
        auto spacer = std::make_shared<GuiComponent>();
        spacer->setSize(16, 0);
        row.addElement(spacer, false);

        auto text = std::make_shared<TextComponent>(sGuiInputConfigList[i].dispName,
                                                    Font::get(FONT_SIZE_MEDIUM), 0x777777FF);
        row.addElement(text, true);

        auto mapping = std::make_shared<TextComponent>(
            "-NOT DEFINED-", Font::get(FONT_SIZE_MEDIUM, FONT_PATH_LIGHT), 0x999999FF, ALIGN_RIGHT);
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
                if (!mHoldingInput || mHeldInput.device != input.device ||
                    mHeldInput.id != input.id || mHeldInput.type != input.type) {
                    return true;
                }

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
        if (sGuiInputConfigList[mList->getCursorId()].skippable)
            mSubtitle2->setOpacity(1.0f);
        else
            mSubtitle2->setOpacity(0.0f);
    });

    // Make the first one say "PRESS ANYTHING" if we're re-configuring everything.
    if (mConfiguringAll)
        setPress(mMappings.front());

    // GUI buttons.
    std::vector<std::shared_ptr<ButtonComponent>> buttons;
    std::function<void()> okFunction {[this, okCallback] {
        InputManager::getInstance().writeDeviceConfig(mTargetConfig); // Save.
        if (okCallback)
            okCallback();
        delete this;
    }};

    buttons.push_back(
        std::make_shared<ButtonComponent>("OK", "ok", [okFunction] { okFunction(); }));

    mButtonGrid = makeButtonGrid(buttons);
    mGrid.setEntry(mButtonGrid, glm::ivec2 {0, 6}, true, false);

    // Adjust the width relative to the aspect ratio of the screen to make the GUI look coherent
    // regardless of screen type. The 1.778 aspect ratio value is the 16:9 reference.
    float aspectValue {1.778f / Renderer::getScreenAspectRatio()};
    float width {glm::clamp(0.60f * aspectValue, 0.50f, 0.80f) * Renderer::getScreenWidth()};

    setSize(width, (Renderer::getIsVerticalOrientation() ? Renderer::getScreenWidth() * 0.75f :
                                                           Renderer::getScreenHeight() * 0.75f));
    setPosition((Renderer::getScreenWidth() - mSize.x) / 2.0f,
                (Renderer::getScreenHeight() - mSize.y) / 2.0f);
}

void GuiInputConfig::populateConfigList()
{
    std::string controllerType {Settings::getInstance()->getString("InputControllerType")};

    // clang-format off
    sGuiInputConfigList[0] = {"Up",    false, "D-PAD UP",    ":/graphics/help/dpad_up.svg"};
    sGuiInputConfigList[1] = {"Down",  false, "D-PAD DOWN",  ":/graphics/help/dpad_down.svg"};
    sGuiInputConfigList[2] = {"Left",  false, "D-PAD LEFT",  ":/graphics/help/dpad_left.svg"};
    sGuiInputConfigList[3] = {"Right", false, "D-PAD RIGHT", ":/graphics/help/dpad_right.svg"};

    if (controllerType == "snes") {
        sGuiInputConfigList[4] = {"Back",  false, "SELECT",    ":/graphics/help/button_back_SNES.svg"};
        sGuiInputConfigList[5] = {"Start", false, "START",     ":/graphics/help/button_start_SNES.svg"};
        sGuiInputConfigList[6] = {"A",     false, "B",         ":/graphics/help/mbuttons_b_SNES.svg"};
        sGuiInputConfigList[7] = {"B",     false, "A",         ":/graphics/help/mbuttons_a_SNES.svg"};
        sGuiInputConfigList[8] = {"X",     true,  "Y",         ":/graphics/help/mbuttons_y_SNES.svg"};
        sGuiInputConfigList[9] = {"Y",     true,  "X",         ":/graphics/help/mbuttons_x_SNES.svg"};
    }
    else if (controllerType == "switchpro") {
        sGuiInputConfigList[4] = {"Back",  false, "MINUS",     ":/graphics/help/button_back_switch.svg"};
        sGuiInputConfigList[5] = {"Start", false, "PLUS",      ":/graphics/help/button_start_switch.svg"};
        sGuiInputConfigList[7] = {"A",     false, "A",         ":/graphics/help/mbuttons_a_switch.svg"};
        sGuiInputConfigList[6] = {"B",     false, "B",         ":/graphics/help/mbuttons_b_switch.svg"};
        sGuiInputConfigList[9] = {"X",     true,  "X",         ":/graphics/help/mbuttons_x_switch.svg"};
        sGuiInputConfigList[8] = {"Y",     true,  "Y",         ":/graphics/help/mbuttons_y_switch.svg"};
    }
    else if (controllerType == "ps123") {
        sGuiInputConfigList[4] = {"Back",  false, "SELECT",    ":/graphics/help/button_back_PS123.svg"};
        sGuiInputConfigList[5] = {"Start", false, "START",     ":/graphics/help/button_start_PS123.svg"};
        sGuiInputConfigList[6] = {"A",     false, "CROSS",     ":/graphics/help/mbuttons_a_PS.svg"};
        sGuiInputConfigList[7] = {"B",     false, "CIRCLE",    ":/graphics/help/mbuttons_b_PS.svg"};
        sGuiInputConfigList[8] = {"X",     true,  "SQUARE",    ":/graphics/help/mbuttons_x_PS.svg"};
        sGuiInputConfigList[9] = {"Y",     true,  "TRIANGLE",  ":/graphics/help/mbuttons_y_PS.svg"};
    }
    else if (controllerType == "ps4") {
        sGuiInputConfigList[4] = {"Back",  false, "SHARE",     ":/graphics/help/button_back_PS4.svg"};
        sGuiInputConfigList[5] = {"Start", false, "OPTIONS",   ":/graphics/help/button_start_PS4.svg"};
        sGuiInputConfigList[6] = {"A",     false, "CROSS",     ":/graphics/help/mbuttons_a_PS.svg"};
        sGuiInputConfigList[7] = {"B",     false, "CIRCLE",    ":/graphics/help/mbuttons_b_PS.svg"};
        sGuiInputConfigList[8] = {"X",     true,  "SQUARE",    ":/graphics/help/mbuttons_x_PS.svg"};
        sGuiInputConfigList[9] = {"Y",     true,  "TRIANGLE",  ":/graphics/help/mbuttons_y_PS.svg"};
    }
    else if (controllerType == "ps5") {
        sGuiInputConfigList[4] = {"Back",  false, "CREATE",    ":/graphics/help/button_back_PS5.svg"};
        sGuiInputConfigList[5] = {"Start", false, "OPTIONS",   ":/graphics/help/button_start_PS5.svg"};
        sGuiInputConfigList[6] = {"A",     false, "CROSS",     ":/graphics/help/mbuttons_a_PS.svg"};
        sGuiInputConfigList[7] = {"B",     false, "CIRCLE",    ":/graphics/help/mbuttons_b_PS.svg"};
        sGuiInputConfigList[8] = {"X",     true,  "SQUARE",    ":/graphics/help/mbuttons_x_PS.svg"};
        sGuiInputConfigList[9] = {"Y",     true,  "TRIANGLE",  ":/graphics/help/mbuttons_y_PS.svg"};
    }
    else if (controllerType == "xbox360") {
        sGuiInputConfigList[4] = {"Back",  false, "BACK",      ":/graphics/help/button_back_XBOX360.svg"};
        sGuiInputConfigList[5] = {"Start", false, "START",     ":/graphics/help/button_start_XBOX360.svg"};
        sGuiInputConfigList[6] = {"A",     false, "A",         ":/graphics/help/mbuttons_a_XBOX.svg"};
        sGuiInputConfigList[7] = {"B",     false, "B",         ":/graphics/help/mbuttons_b_XBOX.svg"};
        sGuiInputConfigList[8] = {"X",     true,  "X",         ":/graphics/help/mbuttons_x_XBOX.svg"};
        sGuiInputConfigList[9] = {"Y",     true,  "Y",         ":/graphics/help/mbuttons_y_XBOX.svg"};
    }
    else {
        // Xbox One and later.
        sGuiInputConfigList[4] = {"Back",  false, "VIEW", ":/graphics/help/button_back_XBOX.svg"};
        sGuiInputConfigList[5] = {"Start", false, "MENU", ":/graphics/help/button_start_XBOX.svg"};
        sGuiInputConfigList[6] = {"A",     false, "A",    ":/graphics/help/mbuttons_a_XBOX.svg"};
        sGuiInputConfigList[7] = {"B",     false, "B",    ":/graphics/help/mbuttons_b_XBOX.svg"};
        sGuiInputConfigList[8] = {"X",     true,  "X",    ":/graphics/help/mbuttons_x_XBOX.svg"};
        sGuiInputConfigList[9] = {"Y",     true,  "Y",    ":/graphics/help/mbuttons_y_XBOX.svg"};
    }

    sGuiInputConfigList[10] = {"LeftShoulder",         true, "LEFT SHOULDER",          ":/graphics/help/button_l.svg"};
    sGuiInputConfigList[11] = {"RightShoulder",        true, "RIGHT SHOULDER",         ":/graphics/help/button_r.svg"};
    sGuiInputConfigList[12] = {"LeftTrigger",          true, "LEFT TRIGGER",           ":/graphics/help/button_lt.svg"};
    sGuiInputConfigList[13] = {"RightTrigger",         true, "RIGHT TRIGGER",          ":/graphics/help/button_rt.svg"};
    sGuiInputConfigList[14] = {"LeftThumbstickUp",     true, "LEFT THUMBSTICK UP",     ":/graphics/help/thumbstick_up.svg"};
    sGuiInputConfigList[15] = {"LeftThumbstickDown",   true, "LEFT THUMBSTICK DOWN",   ":/graphics/help/thumbstick_down.svg"};
    sGuiInputConfigList[16] = {"LeftThumbstickLeft",   true, "LEFT THUMBSTICK LEFT",   ":/graphics/help/thumbstick_left.svg"};
    sGuiInputConfigList[17] = {"LeftThumbstickRight",  true, "LEFT THUMBSTICK RIGHT",  ":/graphics/help/thumbstick_right.svg"};
    sGuiInputConfigList[18] = {"LeftThumbstickClick",  true, "LEFT THUMBSTICK CLICK",  ":/graphics/help/thumbstick_click.svg"};
    sGuiInputConfigList[19] = {"RightThumbstickUp",    true, "RIGHT THUMBSTICK UP",    ":/graphics/help/thumbstick_up.svg"};
    sGuiInputConfigList[20] = {"RightThumbstickDown",  true, "RIGHT THUMBSTICK DOWN",  ":/graphics/help/thumbstick_down.svg"};
    sGuiInputConfigList[21] = {"RightThumbstickLeft",  true, "RIGHT THUMBSTICK LEFT",  ":/graphics/help/thumbstick_left.svg"};
    sGuiInputConfigList[22] = {"RightThumbstickRight", true, "RIGHT THUMBSTICK RIGHT", ":/graphics/help/thumbstick_right.svg"};
    sGuiInputConfigList[23] = {"RightThumbstickClick", true, "RIGHT THUMBSTICK CLICK", ":/graphics/help/thumbstick_click.svg"};
    // clang-format on
}

void GuiInputConfig::update(int deltaTime)
{
    if (mConfiguringRow && mHoldingInput && sGuiInputConfigList[mHeldInputId].skippable) {
        int prevSec {mHeldTime / 1000};
        mHeldTime += deltaTime;
        int curSec {mHeldTime / 1000};

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
    mBackground.fitTo(mSize, glm::vec3 {}, glm::vec2 {-32.0f, -32.0f});

    // Update grid.
    mGrid.setSize(mSize);

    mGrid.setRowHeightPerc(1, mTitle->getFont()->getHeight() * 0.75f / mSize.y);
    mGrid.setRowHeightPerc(2, mSubtitle1->getFont()->getHeight() / mSize.y);
    mGrid.setRowHeightPerc(3, mSubtitle2->getFont()->getHeight() / mSize.y);
    mGrid.setRowHeightPerc(5, (mList->getRowHeight(0) * 5.0f + 2.0f) / mSize.y);
    mGrid.setRowHeightPerc(6, mButtonGrid->getSize().y / mSize.y);
}

void GuiInputConfig::rowDone()
{
    if (mConfiguringAll) {
        // Try to move to the next row.
        if (!mList->moveCursor(1)) {
            // At bottom of list, we're done.
            mConfiguringAll = false;
            mConfiguringRow = false;
            mGrid.moveCursor(glm::ivec2 {0, 1});
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
        !mTargetConfig->isMappedTo(sGuiInputConfigList[inputId].name, input) &&
        sGuiInputConfigList[inputId].name != "HotKeyEnable") {
        error(mMappings.at(inputId), "Already mapped!");
        return false;
    }

    setAssignedTo(mMappings.at(inputId), input);

    input.configured = true;
    mTargetConfig->mapInput(sGuiInputConfigList[inputId].name, input);

    LOG(LogInfo) << "Mapping [" << input.string() << "] to [" << sGuiInputConfigList[inputId].name
                 << "]";

    return true;
}

void GuiInputConfig::clearAssignment(int inputId)
{
    mTargetConfig->unmapInput(sGuiInputConfigList[inputId].name);
}
