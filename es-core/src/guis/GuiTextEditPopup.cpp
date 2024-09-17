//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiTextEditPopup.cpp
//
//  Text edit popup.
//  Has a default mode and a complex mode, both with various options passed as arguments.
//

#define DELETE_REPEAT_START_DELAY 600
#define DELETE_REPEAT_SPEED 90 // Lower is faster.

#include "guis/GuiTextEditPopup.h"

#include "components/MenuComponent.h"
#include "guis/GuiMsgBox.h"

GuiTextEditPopup::GuiTextEditPopup(const HelpStyle& helpstyle,
                                   const std::string& title,
                                   const std::string& initValue,
                                   const std::function<void(const std::string&)>& okCallback,
                                   bool multiLine,
                                   const std::string& acceptBtnText,
                                   const std::string& saveConfirmationText,
                                   const std::string& infoString,
                                   const std::string& defaultValue,
                                   const std::string& loadBtnHelpText,
                                   const std::string& clearBtnHelpText,
                                   const std::string& cancelBtnHelpText)
    : mBackground {":/graphics/frame.svg"}
    , mGrid {glm::ivec2 {1, (infoString != "" && defaultValue != "" ? 5 : 3)}}
    , mHelpStyle {helpstyle}
    , mInitValue {initValue}
    , mAcceptBtnText {acceptBtnText}
    , mSaveConfirmationText {saveConfirmationText}
    , mLoadBtnHelpText {loadBtnHelpText}
    , mClearBtnHelpText {clearBtnHelpText}
    , mCancelBtnHelpText {cancelBtnHelpText}
    , mOkCallback {okCallback}
    , mMultiLine {multiLine}
    , mComplexMode {(infoString != "" && defaultValue != "")}
    , mDeleteRepeat {false}
    , mDeleteRepeatTimer {0}
{
    addChild(&mBackground);
    addChild(&mGrid);

    mTitle = std::make_shared<TextComponent>(title, Font::get(FONT_SIZE_MEDIUM), mMenuColorTitle,
                                             ALIGN_CENTER);

    if (mComplexMode) {
        mInfoString = std::make_shared<TextComponent>(infoString, Font::get(FONT_SIZE_SMALL),
                                                      mMenuColorTitle, ALIGN_CENTER);
        mDefaultValue = std::make_shared<TextComponent>(defaultValue, Font::get(FONT_SIZE_SMALL),
                                                        mMenuColorTitle, ALIGN_CENTER);
    }

    mText = std::make_shared<TextEditComponent>(mMultiLine);
    mText->setText(initValue, false);

    std::vector<std::shared_ptr<ButtonComponent>> buttons;
    buttons.push_back(
        std::make_shared<ButtonComponent>(acceptBtnText, acceptBtnText, [this, okCallback] {
            okCallback(mText->getValue());
            delete this;
        }));
    if (mComplexMode) {
        buttons.push_back(
            std::make_shared<ButtonComponent>(_("LOAD"), loadBtnHelpText, [this, defaultValue] {
                mText->setText(defaultValue);
                mText->setCursor(0);
                mText->setCursor(defaultValue.size());
            }));
    }

    buttons.push_back(std::make_shared<ButtonComponent>(_("CLEAR"), clearBtnHelpText,
                                                        [this] { mText->setText(""); }));

    buttons.push_back(std::make_shared<ButtonComponent>(_("CANCEL"), _("discard changes"),
                                                        [this] { delete this; }));

    mButtonGrid = MenuComponent::makeButtonGrid(buttons);

    mGrid.setEntry(mTitle, glm::ivec2 {0, 0}, false, true);

    int yPos {1};

    if (mComplexMode) {
        mGrid.setEntry(mInfoString, glm::ivec2 {0, yPos}, false, true);
        mGrid.setEntry(mDefaultValue, glm::ivec2 {0, yPos + 1}, false, false);
        yPos += 2;
    }

    mGrid.setEntry(mText, glm::ivec2 {0, yPos}, true, false, glm::ivec2 {1, 1},
                   GridFlags::BORDER_TOP | GridFlags::BORDER_BOTTOM);
    mGrid.setEntry(mButtonGrid, glm::ivec2 {0, yPos + 1}, true, false);

    float textHeight = mText->getFont()->getHeight();

#if defined(__ANDROID__)
    if (multiLine)
        textHeight *= 2.0f;
#else
    if (multiLine)
        textHeight *= 6.0f;
#endif

    mText->setSize(0.0f, textHeight);

    // Adapt width to the geometry of the display. The 1.778 aspect ratio is the 16:9 reference.
    float aspectValue = 1.778f / Renderer::getScreenAspectRatio();

    if (mComplexMode) {
        float infoWidth =
            glm::clamp(0.70f * aspectValue, 0.34f, 0.85f) * Renderer::getScreenWidth();
        float windowWidth =
            glm::clamp(0.75f * aspectValue, 0.40f, 0.90f) * Renderer::getScreenWidth();

        mDefaultValue->setSize(infoWidth, mDefaultValue->getFont()->getHeight());

        setSize(windowWidth, mTitle->getFont()->getHeight() + textHeight +
                                 mButtonGrid->getSize().y + mButtonGrid->getSize().y * 1.85f);
#if defined(__ANDROID__)
        setPosition((Renderer::getScreenWidth() - mSize.x) / 2.0f,
                    Font::get(FONT_SIZE_LARGE_FIXED)->getLetterHeight());
#else
        setPosition((Renderer::getScreenWidth() - mSize.x) / 2.0f,
                    (Renderer::getScreenHeight() - mSize.y) / 2.0f);
#endif
    }
    else {
        float width = glm::clamp(0.54f * aspectValue, 0.20f, 0.70f) * Renderer::getScreenWidth();

        setSize(width, mTitle->getFont()->getHeight() + textHeight + mButtonGrid->getSize().y +
                           mButtonGrid->getSize().y / 2.0f);

#if defined(__ANDROID__)
        setPosition((Renderer::getScreenWidth() - mSize.x) / 2.0f,
                    Font::get(FONT_SIZE_LARGE_FIXED)->getLetterHeight());
#else
        setPosition((Renderer::getScreenWidth() - mSize.x) / 2.0f,
                    (Renderer::getScreenHeight() - mSize.y) / 2.0f);
#endif
    }

    if (!multiLine)
        mText->setCursor(initValue.size());

    mText->startEditing();
}

void GuiTextEditPopup::onSizeChanged()
{
    mBackground.fitTo(mSize);
    mText->setSize(mSize.x - 40.0f * Renderer::getScreenHeightModifier(), mText->getSize().y);

    // Update grid.
    mGrid.setRowHeightPerc(0, mTitle->getFont()->getHeight() / mSize.y);

    if (mComplexMode)
        mGrid.setRowHeightPerc(1, 0.15f);

    mGrid.setRowHeightPerc(2, mButtonGrid->getSize().y / mSize.y);
    mGrid.setSize(mSize);
}

bool GuiTextEditPopup::input(InputConfig* config, Input input)
{
    // Enter key (main key or via numpad) accepts the changes.
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

    // Ignore whatever key is mapped to the back button so it can be used for text input.
    bool keyboardBack {config->getDeviceId() == DEVICE_KEYBOARD && mText->isEditing() &&
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

    // Left shoulder button deletes a character (backspace).
    if (config->isMappedTo("leftshoulder", input)) {
        if (input.value) {
            mDeleteRepeat = true;
            mDeleteRepeatTimer = -(DELETE_REPEAT_START_DELAY - DELETE_REPEAT_SPEED);

            bool editing = mText->isEditing();
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
        bool editing = mText->isEditing();
        if (!editing)
            mText->startEditing();

        mText->setMaskInput(false);
        mText->textInput(" ");
        mText->setMaskInput(true);

        if (!editing)
            mText->stopEditing();

        return true;
    }

    if (GuiComponent::input(config, input))
        return true;

    return false;
}

void GuiTextEditPopup::update(int deltaTime)
{
    updateDeleteRepeat(deltaTime);
    GuiComponent::update(deltaTime);
}

std::vector<HelpPrompt> GuiTextEditPopup::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts {mGrid.getHelpPrompts()};

    if (mText->isEditing()) {
        if (mMultiLine)
            prompts.push_back(HelpPrompt("a", _("newline")));
        else
            prompts.push_back(HelpPrompt("a", mAcceptBtnText));
    }

    prompts.push_back(HelpPrompt("l", _("backspace")));
    prompts.push_back(HelpPrompt("r", _("space")));
    prompts.push_back(HelpPrompt("b", _("back")));
    return prompts;
}

void GuiTextEditPopup::updateDeleteRepeat(int deltaTime)
{
    if (!mDeleteRepeat)
        return;

    mDeleteRepeatTimer += deltaTime;

    while (mDeleteRepeatTimer >= DELETE_REPEAT_SPEED) {
        bool editing = mText->isEditing();
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
