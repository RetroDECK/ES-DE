//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiTextEditKeyboardPopup.cpp
//
//  Text edit popup with a virtual keyboard.
//  Has a default mode and a complex mode, both with various options passed as arguments.
//

#define KEYBOARD_HEIGHT                                                                            \
    (Renderer::getIsVerticalOrientation() ? Renderer::getScreenWidth() * 0.60f :                   \
                                            Renderer::getScreenHeight() * 0.60f)

#define KEYBOARD_PADDINGX (Renderer::getScreenWidth() * 0.02f)
#define KEYBOARD_PADDINGY (Renderer::getScreenWidth() * 0.01f)

#define BUTTON_GRID_HORIZ_PADDING (10.0f * Renderer::getScreenResolutionModifier())

#define NAVIGATION_REPEAT_START_DELAY 400
#define NAVIGATION_REPEAT_SPEED 70 // Lower is faster.

#define DELETE_REPEAT_START_DELAY 600
#define DELETE_REPEAT_SPEED 90 // Lower is faster.

#if defined(_MSC_VER) // MSVC compiler.
#define DELETE_SYMBOL Utils::String::wideStringToString(L"\uf177")
#define OK_SYMBOL Utils::String::wideStringToString(L"\uf058")
#define SHIFT_SYMBOL Utils::String::wideStringToString(L"\uf176")
#define ALT_SYMBOL Utils::String::wideStringToString(L"\uf141")
#else
#define DELETE_SYMBOL "\uf177"
#define OK_SYMBOL "\uf058"
#define SHIFT_SYMBOL "\uf176"
#define ALT_SYMBOL "\uf141"
#endif

#include "guis/GuiTextEditKeyboardPopup.h"

#include "components/MenuComponent.h"
#include "guis/GuiMsgBox.h"
#include "utils/StringUtil.h"

GuiTextEditKeyboardPopup::GuiTextEditKeyboardPopup(
    const HelpStyle& helpstyle,
    const float verticalPosition,
    const std::string& title,
    const std::string& initValue,
    const std::function<void(const std::string&)>& okCallback,
    bool multiLine,
    const std::string& acceptBtnHelpText,
    const std::string& saveConfirmationText,
    const std::string& infoString,
    const std::string& defaultValue,
    const std::string& loadBtnHelpText,
    const std::string& clearBtnHelpText,
    const std::string& cancelBtnHelpText)
    : mRenderer {Renderer::getInstance()}
    , mBackground {":/graphics/frame.svg"}
    , mGrid {glm::ivec2 {1, (infoString != "" && defaultValue != "" ? 8 : 6)}}
    , mHelpStyle {helpstyle}
    , mInitValue {initValue}
    , mAcceptBtnHelpText {acceptBtnHelpText}
    , mSaveConfirmationText {saveConfirmationText}
    , mLoadBtnHelpText {loadBtnHelpText}
    , mClearBtnHelpText {clearBtnHelpText}
    , mCancelBtnHelpText {cancelBtnHelpText}
    , mOkCallback {okCallback}
    , mMultiLine {multiLine}
    , mComplexMode {(infoString != "" && defaultValue != "")}
    , mDeleteRepeat {false}
    , mShift {false}
    , mAlt {false}
    , mVerticalPosition {verticalPosition}
    , mDeleteRepeatTimer {0}
    , mNavigationRepeatTimer {0}
    , mNavigationRepeatDirX {0}
    , mNavigationRepeatDirY {0}
{
    // clang-format off
    std::vector<std::vector<std::string>> kbBaseUS {
        {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "DEL"},
        {"!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+", "DEL"},
        {"¡", "²", "³", "¤", "€", "¼", "½", "¾", "‘", "’", "¥", "×", "DEL"},
        {"¹", "",  "",  "£", "",  "",  "",  "",  "",  "",  "",  "÷", "DEL"},

        {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "OK"},
        {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}", "OK"},
        {"ä", "å", "é", "®", "þ", "ü", "ú", "í", "ó", "ö", "«", "»", "OK"},
        {"Ä", "Å", "É", "",  "Þ", "Ü", "Ú", "Í", "Ó", "Ö", "",  "",  "OK"},

        {"a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "\\", "-rowspan-"},
        {"A", "S", "D", "F", "G", "H", "J", "K", "L", ":", "\"", "|", "-rowspan-"},
        {"á", "ß", "ð", "",  "",  "",  "",  "",  "ø", "¶", "´",  "¬", "-rowspan-"},
        {"Á", "§", "Ð", "",  "",  "",  "",  "",  "Ø", "°", "¨",  "¦", "-rowspan-"},

        {"`", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "ALT", "-colspan-"},
        {"~", "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?", "ALT", "-colspan-"},
        {"",  "æ", "",  "©", "",  "",  "ñ", "µ", "ç", "",  "¿", "ALT", "-colspan-"},
        {"",  "Æ", "",  "¢", "",  "",  "Ñ", "Μ", "Ç", "",  "",  "ALT", "-colspan-"}};

    std::vector<std::vector<std::string>> kbLastRowNormal {
        {"SHIFT", "-colspan-", _("SPACE"), "-colspan-", "-colspan-", "-colspan-", "-colspan-", "-colspan-", "-colspan-", _("CLEAR"), "-colspan-", _("CANCEL"), "-colspan-"},
        {"SHIFT", "-colspan-", _("SPACE"), "-colspan-", "-colspan-", "-colspan-", "-colspan-", "-colspan-", "-colspan-", _("CLEAR"), "-colspan-", _("CANCEL"), "-colspan-"},
        {"SHIFT", "-colspan-", _("SPACE"), "-colspan-", "-colspan-", "-colspan-", "-colspan-", "-colspan-", "-colspan-", _("CLEAR"), "-colspan-", _("CANCEL"), "-colspan-"},
        {"SHIFT", "-colspan-", _("SPACE"), "-colspan-", "-colspan-", "-colspan-", "-colspan-", "-colspan-", "-colspan-", _("CLEAR"), "-colspan-", _("CANCEL"), "-colspan-"}};

    std::vector<std::vector<std::string>> kbLastRowLoad {
        {"SHIFT", "-colspan-", _("SPACE"), "-colspan-", "-colspan-", "-colspan-", "-colspan-", _("LOAD"), "-colspan-", _("CLEAR"), "-colspan-", _("CANCEL"), "-colspan-"},
        {"SHIFT", "-colspan-", _("SPACE"), "-colspan-", "-colspan-", "-colspan-", "-colspan-", _("LOAD"), "-colspan-", _("CLEAR"), "-colspan-", _("CANCEL"), "-colspan-"},
        {"SHIFT", "-colspan-", _("SPACE"), "-colspan-", "-colspan-", "-colspan-", "-colspan-", _("LOAD"), "-colspan-", _("CLEAR"), "-colspan-", _("CANCEL"), "-colspan-"},
        {"SHIFT", "-colspan-", _("SPACE"), "-colspan-", "-colspan-", "-colspan-", "-colspan-", _("LOAD"), "-colspan-", _("CLEAR"), "-colspan-", _("CANCEL"), "-colspan-"}};
    // clang-format on

    addChild(&mBackground);
    addChild(&mGrid);

    mTitle = std::make_shared<TextComponent>(
        title, Font::get(FONT_SIZE_LARGE * Utils::Localization::sMenuTitleScaleFactor),
        mMenuColorTitle, ALIGN_CENTER);

    std::vector<std::vector<std::string>> kbLayout;

    // At the moment there is only the US keyboard layout available.
    kbLayout.insert(kbLayout.cend(), kbBaseUS.cbegin(), kbBaseUS.cend());

    // In complex mode, the last row of the keyboard contains an additional "LOAD" button.
    if (mComplexMode)
        kbLayout.insert(kbLayout.cend(), kbLastRowLoad.cbegin(), kbLastRowLoad.cend());
    else
        kbLayout.insert(kbLayout.cend(), kbLastRowNormal.cbegin(), kbLastRowNormal.cend());

    mHorizontalKeyCount = static_cast<int>(kbLayout[0].size());

    mKeyboardGrid = std::make_shared<ComponentGrid>(
        glm::ivec2 {mHorizontalKeyCount, static_cast<int>(kbLayout.size()) / 3});

    mText = std::make_shared<TextEditComponent>(mMultiLine);
    mText->setText(initValue, false);

    // Header.
    mGrid.setEntry(mTitle, glm::ivec2 {0, 0}, false, true);

    int yPos {1};

    if (mComplexMode) {
        mInfoString = std::make_shared<TextComponent>(infoString, Font::get(FONT_SIZE_MEDIUM),
                                                      mMenuColorTitle, ALIGN_CENTER);
        mGrid.setEntry(mInfoString, glm::ivec2 {0, yPos}, false, true);

        mDefaultValue = std::make_shared<TextComponent>(defaultValue, Font::get(FONT_SIZE_SMALL),
                                                        mMenuColorTitle, ALIGN_CENTER);
        mGrid.setEntry(mDefaultValue, glm::ivec2 {0, yPos + 1}, false, true);
        yPos += 2;
    }

    // Text edit field.
    mGrid.setEntry(mText, glm::ivec2 {0, yPos}, true, false, glm::ivec2 {1, 1},
                   GridFlags::BORDER_TOP);

    std::vector<std::vector<std::shared_ptr<ButtonComponent>>> buttonList;

    // Create keyboard.
    for (int i {0}; i < static_cast<int>(kbLayout.size()) / 4; ++i) {
        std::vector<std::shared_ptr<ButtonComponent>> buttons;

        for (int j {0}; j < static_cast<int>(kbLayout[i].size()); ++j) {
            std::string lower {kbLayout[4 * i][j]};
            if (lower.empty() || lower == "-rowspan-" || lower == "-colspan-")
                continue;

            std::string upper {kbLayout[4 * i + 1][j]};
            std::string alted {kbLayout[4 * i + 2][j]};
            std::string altshifted {kbLayout[4 * i + 3][j]};

            std::shared_ptr<ButtonComponent> button;

            if (lower == "DEL") {
                lower = DELETE_SYMBOL;
                upper = DELETE_SYMBOL;
                alted = DELETE_SYMBOL;
                altshifted = DELETE_SYMBOL;
            }
            else if (lower == "OK") {
                lower = OK_SYMBOL;
                upper = OK_SYMBOL;
                alted = OK_SYMBOL;
                altshifted = OK_SYMBOL;
            }
            else if (lower == _("SPACE")) {
                lower = " ";
                upper = " ";
                alted = " ";
                altshifted = " ";
            }
            else if (lower != "SHIFT" && lower.length() > 1) {
                lower = (lower.c_str());
                upper = (upper.c_str());
                alted = (alted.c_str());
                altshifted = (altshifted.c_str());
            }

            if (lower == "SHIFT") {
                mShiftButton = std::make_shared<ButtonComponent>(
                    (SHIFT_SYMBOL), ("SHIFT"), [this] { shiftKeys(); }, false, true);
                button = mShiftButton;
            }
            else if (lower == "ALT") {
                mAltButton = std::make_shared<ButtonComponent>((ALT_SYMBOL), ("ALT"),
                                                               [this] { altKeys(); }, false, true);
                button = mAltButton;
            }
            else {
                button = makeButton(lower, upper, alted, altshifted);
            }

            button->setPadding(
                glm::vec4 {BUTTON_GRID_HORIZ_PADDING / 4.0f, BUTTON_GRID_HORIZ_PADDING / 4.0f,
                           BUTTON_GRID_HORIZ_PADDING / 4.0f, BUTTON_GRID_HORIZ_PADDING / 4.0f});
            buttons.push_back(button);

            int colSpan {1};
            for (int cs {j + 1}; cs < static_cast<int>(kbLayout[i].size()); ++cs) {
                if (std::string(kbLayout[4 * i][cs]) == "-colspan-")
                    ++colSpan;
                else
                    break;
            }

            int rowSpan = 1;
            for (int cs {(4 * i) + 4}; cs < static_cast<int>(kbLayout.size()); cs += 4) {
                if (std::string(kbLayout[cs][j]) == "-rowspan-")
                    ++rowSpan;
                else
                    break;
            }

            mKeyboardGrid->setEntry(button, glm::ivec2 {j, i}, true, true,
                                    glm::ivec2 {colSpan, rowSpan});

            buttonList.push_back(buttons);
        }
    }

    mGrid.setEntry(mKeyboardGrid, glm::ivec2 {0, yPos + 1}, true, true, glm::ivec2 {1, 4});

    float textHeight {mText->getFont()->getHeight()};
    // If the multiLine option has been set, then include three lines of text on screen.
    if (multiLine) {
        textHeight *= 3.0f;
        textHeight += 2.0f * mRenderer->getScreenResolutionModifier();
    }

    mText->setSize(0.0f, textHeight);

    // If attempting to navigate beyond the edge of the keyboard grid, then wrap around.
    mGrid.setPastBoundaryCallback([this, kbLayout](InputConfig* config, Input input) -> bool {
        if (config->isMappedLike("left", input)) {
            if (mGrid.getSelectedComponent() == mKeyboardGrid) {
                mKeyboardGrid->moveCursorTo(mHorizontalKeyCount - 1, -1, true);
                return true;
            }
        }
        else if (config->isMappedLike("right", input)) {
            if (mGrid.getSelectedComponent() == mKeyboardGrid) {
                mKeyboardGrid->moveCursorTo(0, -1);
                return true;
            }
        }
        return false;
    });

    // Adapt width to the geometry of the display. The 1.778 aspect ratio is the 16:9 reference.
    float aspectValue {1.778f / mRenderer->getScreenAspectRatio()};
    const float maxWidthMultiplier {mRenderer->getIsVerticalOrientation() ? 0.95f : 0.90f};
    float width {glm::clamp(0.78f * aspectValue, 0.35f, maxWidthMultiplier) *
                 mRenderer->getScreenWidth()};

    // The combination of multiLine and complex mode is not supported as there is currently
    // no need for that.
    if (mMultiLine) {
        setSize(width, KEYBOARD_HEIGHT + textHeight - mText->getFont()->getHeight());

        if (mVerticalPosition == 0.0f)
            setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f,
                        (mRenderer->getScreenHeight() - mSize.y) / 2.0f);
        else
            setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f, mVerticalPosition);
    }
    else {
        if (mComplexMode)
            setSize(width, KEYBOARD_HEIGHT + mDefaultValue->getSize().y * 3.0f);
        else
            setSize(width, KEYBOARD_HEIGHT);

        if (mVerticalPosition == 0.0f)
            setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f,
                        (mRenderer->getScreenHeight() - mSize.y) / 2.0f);
        else
            setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f, mVerticalPosition);
    }

    if (!multiLine)
        mText->setCursor(initValue.size());
}

void GuiTextEditKeyboardPopup::onSizeChanged()
{
    mBackground.fitTo(mSize);
    mText->setSize(mSize.x - KEYBOARD_PADDINGX - KEYBOARD_PADDINGX, mText->getSize().y);

    // Update grid.
    mGrid.setRowHeightPerc(0, mTitle->getFont()->getHeight() / mSize.y);

    if (mInfoString && mDefaultValue) {
        mGrid.setRowHeightPerc(1, (mInfoString->getSize().y * 0.6f) / mSize.y);
        mGrid.setRowHeightPerc(2, (mDefaultValue->getSize().y * 1.6f) / mSize.y);
        mGrid.setRowHeightPerc(1, (mText->getSize().y * 1.0f) / mSize.y);
    }
    else if (mMultiLine) {
        mGrid.setRowHeightPerc(1, (mText->getSize().y * 1.15f) / mSize.y);
    }

    mGrid.setSize(mSize);

    auto pos = mKeyboardGrid->getPosition();
    auto sz = mKeyboardGrid->getSize();

    // Add a small margin between buttons.
    mKeyboardGrid->setSize(mSize.x - KEYBOARD_PADDINGX - KEYBOARD_PADDINGX,
                           sz.y - KEYBOARD_PADDINGY +
                               70.0f * mRenderer->getScreenResolutionModifier());
    mKeyboardGrid->setPosition(KEYBOARD_PADDINGX, pos.y);
}

bool GuiTextEditKeyboardPopup::input(InputConfig* config, Input input)
{
    // Enter/return key or numpad enter key accepts the changes.
    if (config->getDeviceId() == DEVICE_KEYBOARD && mText->isEditing() && !mMultiLine &&
        input.value && (input.id == SDLK_RETURN || input.id == SDLK_KP_ENTER)) {
        this->mOkCallback(mText->getValue());
        delete this;
        return true;
    }
    // Dito for the A button if using a controller.
    else if (config->getDeviceId() != DEVICE_KEYBOARD && mText->isEditing() &&
             config->isMappedTo("a", input) && input.value) {
        this->mOkCallback(mText->getValue());
        delete this;
        return true;
    }

    // Pressing a key stops the navigation repeat, i.e. the cursor stops at the selected key.
    if (config->isMappedTo("a", input) && input.value && !mText->isEditing()) {
        mNavigationRepeatDirX = 0;
        mNavigationRepeatDirY = 0;
    }

    // Ignore whatever key is mapped to the back button so it can be used for text input.
    const bool keyboardBack {config->getDeviceId() == DEVICE_KEYBOARD && mText->isEditing() &&
                             config->isMappedLike("b", input)};

    // Pressing back (or the escape key if using keyboard input) closes us.
    if ((config->getDeviceId() == DEVICE_KEYBOARD && input.value && input.id == SDLK_ESCAPE) ||
        (!keyboardBack && input.value && config->isMappedTo("b", input))) {
        if (mText->getValue() != mInitValue) {
            // Changes were made, ask if the user wants to save them.
            mWindow->pushGui(new GuiMsgBox(
                mHelpStyle, mSaveConfirmationText, _("YES"),
                [this] {
                    this->mOkCallback(mText->getValue());
                    delete this;
                    return true;
                },
                _("NO"),
                [this] {
                    delete this;
                    return true;
                },
                "", nullptr, nullptr, true));
        }
        else {
            if (mText->isEditing())
                mText->stopEditing();
            delete this;
            return true;
        }
    }

    if (mText->isEditing() && config->isMappedLike("down", input) && input.value)
        mGrid.setCursorTo(mGrid.getSelectedComponent());

    // Left trigger button outside text editing field toggles Shift key.
    if (!mText->isEditing() && config->isMappedLike("lefttrigger", input) && input.value)
        shiftKeys();

    // Right trigger button outside text editing field toggles Alt key.
    if (!mText->isEditing() && config->isMappedLike("righttrigger", input) && input.value)
        altKeys();

    // Left shoulder button deletes a character (backspace).
    if (config->isMappedTo("leftshoulder", input)) {
        if (input.value) {
            mDeleteRepeat = true;
            mDeleteRepeatTimer = -(DELETE_REPEAT_START_DELAY - DELETE_REPEAT_SPEED);

            const bool editing {mText->isEditing()};
            if (!editing)
                mText->startEditing();

            mText->setMaskInput(false);
            mText->textInput("\b");
            mText->setMaskInput(true);

            if (!editing)
                mText->stopEditing();
        }
        else {
            mDeleteRepeat = false;
        }
        return true;
    }

    // Right shoulder button inserts a blank space.
    if (config->isMappedTo("rightshoulder", input) && input.value) {
        const bool editing {mText->isEditing()};
        if (!editing)
            mText->startEditing();

        mText->setMaskInput(false);
        mText->textInput(" ");
        mText->setMaskInput(true);

        if (!editing)
            mText->stopEditing();

        return true;
    }

    // Actual navigation of the keyboard grid is done in ComponentGrid, this code only handles
    // key repeat while holding the left/right/up/down buttons.
    if (!mText->isEditing() && config->isMappedLike("left", input)) {
        if (input.value) {
            mNavigationRepeatDirX = -1;
            mNavigationRepeatTimer = -(NAVIGATION_REPEAT_START_DELAY - NAVIGATION_REPEAT_SPEED);
        }
        else {
            mNavigationRepeatDirX = 0;
        }
    }

    if (!mText->isEditing() && config->isMappedLike("right", input)) {
        if (input.value) {
            mNavigationRepeatDirX = 1;
            mNavigationRepeatTimer = -(NAVIGATION_REPEAT_START_DELAY - NAVIGATION_REPEAT_SPEED);
        }
        else {
            mNavigationRepeatDirX = 0;
        }
    }

    if (!mText->isEditing() && config->isMappedLike("up", input)) {
        if (input.value) {
            mNavigationRepeatDirY = -1;
            mNavigationRepeatTimer = -(NAVIGATION_REPEAT_START_DELAY - NAVIGATION_REPEAT_SPEED);
        }
        else {
            mNavigationRepeatDirY = 0;
        }
    }

    if (!mText->isEditing() && config->isMappedLike("down", input)) {
        if (input.value) {
            mNavigationRepeatDirY = 1;
            mNavigationRepeatTimer = -(NAVIGATION_REPEAT_START_DELAY - NAVIGATION_REPEAT_SPEED);
        }
        else {
            mNavigationRepeatDirY = 0;
        }
    }

    if (GuiComponent::input(config, input))
        return true;

    return false;
}

void GuiTextEditKeyboardPopup::update(int deltaTime)
{
    if (mText->isEditing())
        mNavigationRepeatDirX = 0;

    updateNavigationRepeat(deltaTime);
    updateDeleteRepeat(deltaTime);
    GuiComponent::update(deltaTime);
}

std::vector<HelpPrompt> GuiTextEditKeyboardPopup::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts {mGrid.getHelpPrompts()};

    if (!mText->isEditing()) {
        prompts.push_back(HelpPrompt("lt", _("shift")));
        prompts.push_back(HelpPrompt("rt", _("alt")));
    }
    else if (mMultiLine) {
        prompts.push_back(HelpPrompt("a", _("newline")));
    }
    else {
        prompts.push_back(HelpPrompt("a", mAcceptBtnHelpText));
    }

    prompts.push_back(HelpPrompt("l", _("backspace")));
    prompts.push_back(HelpPrompt("r", _("space")));
    prompts.push_back(HelpPrompt("b", _("back")));

    if (prompts.size() > 0 && prompts.front().second == OK_SYMBOL)
        prompts.front().second = mAcceptBtnHelpText;
    else if (prompts.size() > 0 && prompts.front().second == " ")
        prompts.front().second = _("SPACE");
    else if (prompts.size() > 0 && prompts.front().second == _("CLEAR"))
        prompts.front().second = mClearBtnHelpText;
    else if (prompts.size() > 0 && prompts.front().second == _("LOAD"))
        prompts.front().second = mLoadBtnHelpText;
    else if (prompts.size() > 0 && prompts.front().second == _("CANCEL"))
        prompts.front().second = mCancelBtnHelpText;

    // If a prompt has no value set, then remove it.
    if (prompts.size() > 0 && prompts.front().second == "")
        prompts.erase(prompts.begin(), prompts.begin() + 1);

    return prompts;
}

void GuiTextEditKeyboardPopup::updateDeleteRepeat(int deltaTime)
{
    if (!mDeleteRepeat)
        return;

    mDeleteRepeatTimer += deltaTime;

    while (mDeleteRepeatTimer >= DELETE_REPEAT_SPEED) {
        bool editing {mText->isEditing()};
        if (!editing)
            mText->startEditing();

        mText->setMaskInput(false);
        mText->textInput("\b");
        mText->setMaskInput(true);

        if (!editing)
            mText->stopEditing();

        mDeleteRepeatTimer -= DELETE_REPEAT_SPEED;
    }
}

void GuiTextEditKeyboardPopup::updateNavigationRepeat(int deltaTime)
{
    if (mNavigationRepeatDirX == 0 && mNavigationRepeatDirY == 0)
        return;

    mNavigationRepeatTimer += deltaTime;

    while (mNavigationRepeatTimer >= NAVIGATION_REPEAT_SPEED) {

        if (mNavigationRepeatDirX != 0) {
            mKeyboardGrid.get()->moveCursor({mNavigationRepeatDirX, 0});
            // If replacing the line above with this code, the keyboard will wrap around the
            // edges also when key repeat is active.
            //            if (!mKeyboardGrid.get()->moveCursor({mNavigationRepeatDirX, 0})) {
            //                if (mNavigationRepeatDirX < 0)
            //                    mKeyboardGrid->moveCursorTo(mHorizontalKeyCount - 1, -1);
            //                else
            //                    mKeyboardGrid->moveCursorTo(0, -1);
            //            }
        }

        if (mNavigationRepeatDirY != 0)
            mKeyboardGrid.get()->moveCursor({0, mNavigationRepeatDirY});

        mNavigationRepeatTimer -= NAVIGATION_REPEAT_SPEED;
    }
}

void GuiTextEditKeyboardPopup::shiftKeys()
{
    mShift = !mShift;

    if (mShift) {
        mShiftButton->setFlatColorFocused(mMenuColorKeyboardModifier);
        mShiftButton->setFlatColorUnfocused(mMenuColorKeyboardModifier);
    }
    else {
        mShiftButton->setFlatColorFocused(mMenuColorButtonFlatFocused);
        mShiftButton->setFlatColorUnfocused(mMenuColorButtonFlatUnfocused);
    }

    if (mAlt && mShift) {
        altShiftKeys();
        return;
    }

    // This only happens when Alt was deselected while both Shift and Alt were active.
    if (mAlt) {
        altKeys();
        altKeys();
    }
    else {
        for (auto& kb : mKeyboardButtons) {
            const std::string& text {mShift ? kb.shiftedKey : kb.key};
            auto sz = kb.button->getSize();
            kb.button->setText(text, text, false);
            kb.button->setSize(sz);
        }
    }
}

void GuiTextEditKeyboardPopup::altKeys()
{
    mAlt = !mAlt;

    if (mAlt) {
        mAltButton->setFlatColorFocused(mMenuColorKeyboardModifier);
        mAltButton->setFlatColorUnfocused(mMenuColorKeyboardModifier);
    }
    else {
        mAltButton->setFlatColorFocused(mMenuColorButtonFlatFocused);
        mAltButton->setFlatColorUnfocused(mMenuColorButtonFlatUnfocused);
    }

    if (mShift && mAlt) {
        altShiftKeys();
        return;
    }

    // This only happens when Shift was deselected while both Shift and Alt were active.
    if (mShift) {
        shiftKeys();
        shiftKeys();
    }
    else {
        for (auto& kb : mKeyboardButtons) {
            const std::string& text {mAlt ? kb.altedKey : kb.key};
            auto sz = kb.button->getSize();
            kb.button->setText(text, text, false);
            kb.button->setSize(sz);
        }
    }
}

void GuiTextEditKeyboardPopup::altShiftKeys()
{
    for (auto& kb : mKeyboardButtons) {
        const std::string& text {kb.altshiftedKey};
        auto sz = kb.button->getSize();
        kb.button->setText(text, text, false);
        kb.button->setSize(sz);
    }
}

std::shared_ptr<ButtonComponent> GuiTextEditKeyboardPopup::makeButton(
    const std::string& key,
    const std::string& shiftedKey,
    const std::string& altedKey,
    const std::string& altshiftedKey)
{
    std::shared_ptr<ButtonComponent> button {std::make_shared<ButtonComponent>(
        key, key,
        [this, key, shiftedKey, altedKey, altshiftedKey] {
            if (key == (OK_SYMBOL) || key.find("OK") != std::string::npos) {
                mOkCallback(mText->getValue());
                delete this;
                return;
            }
            else if (key == (DELETE_SYMBOL) || key == "DEL") {
                mText->startEditing();
                mText->textInput("\b");
                mText->stopEditing();
                return;
            }
            else if (key == _("SPACE") || key == " ") {
                mText->startEditing();
                mText->textInput(" ");
                mText->stopEditing();
                return;
            }
            else if (key == _("LOAD")) {
                mText->setText(mDefaultValue->getValue());
                mText->setCursor(mDefaultValue->getValue().size());
                return;
            }
            else if (key == _("CLEAR")) {
                mText->setText("");
                return;
            }
            else if (key == _("CANCEL")) {
                delete this;
                return;
            }

            if (mAlt && altedKey.empty())
                return;

            mText->startEditing();

            if (mShift && mAlt)
                mText->textInput(altshiftedKey.c_str());
            else if (mAlt)
                mText->textInput(altedKey.c_str());
            else if (mShift)
                mText->textInput(shiftedKey.c_str());
            else
                mText->textInput(key.c_str());

            mText->stopEditing();
        },
        false, true)};

    KeyboardButton kb {button, key, shiftedKey, altedKey, altshiftedKey};
    mKeyboardButtons.push_back(kb);
    return button;
}
