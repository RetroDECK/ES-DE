//
//	GuiComplexTextEditPopup.cpp
//
//	Text edit popup with a title, two text strings, a text input box and buttons
//	to load the second text string and to clear the input field.
//	Intended for updating settings for configuration files and similar.
//

#include "guis/GuiComplexTextEditPopup.h"
#include "guis/GuiMsgBox.h"

#include "components/ButtonComponent.h"
#include "components/MenuComponent.h"
#include "components/TextEditComponent.h"
#include "Window.h"

GuiComplexTextEditPopup::GuiComplexTextEditPopup(
		Window* window,
		const HelpStyle& helpstyle,
		const std::string& title,
		const std::string& infoString1,
		const std::string& infoString2,
		const std::string& initValue,
		const std::function<void(const std::string&)>& okCallback,
		bool multiLine,
		const char* acceptBtnText,
		const char* saveConfirmationText)
		: GuiComponent(window),
		mHelpStyle(helpstyle),
		mBackground(window, ":/frame.png"),
		mGrid(window, Vector2i(1, 5)),
		mMultiLine(multiLine),
		mInitValue(initValue),
		mOkCallback(okCallback),
		mSaveConfirmationText(saveConfirmationText)
{
	addChild(&mBackground);
	addChild(&mGrid);

	mTitle = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(title),
			Font::get(FONT_SIZE_MEDIUM), 0x555555FF, ALIGN_CENTER);
	mInfoString1 = std::make_shared<TextComponent>(mWindow, infoString1,
			Font::get(FONT_SIZE_SMALL), 0x555555FF,	ALIGN_CENTER);
	mInfoString2 = std::make_shared<TextComponent>(mWindow, infoString2,
			Font::get(FONT_SIZE_SMALL), 0x555555FF, ALIGN_CENTER);

	mText = std::make_shared<TextEditComponent>(mWindow);
	mText->setValue(initValue);

	if (!multiLine)
		mText->setCursor(initValue.size());

	std::vector< std::shared_ptr<ButtonComponent> > buttons;
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, acceptBtnText, acceptBtnText,
			[this, okCallback] { okCallback(mText->getValue()); delete this; }));
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "LOAD", "load default string",
			[this, infoString2] { mText->setValue(infoString2); }));
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "CLEAR", "clear string",
			[this] { mText->setValue(""); }));
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "CANCEL", "discard changes",
			[this] { delete this; }));

	mButtonGrid = makeButtonGrid(mWindow, buttons);

	mGrid.setEntry(mTitle, Vector2i(0, 0), false, true);
	mGrid.setEntry(mInfoString1, Vector2i(0, 1), false, true);
	mGrid.setEntry(mInfoString2, Vector2i(0, 2), false, true);
	mGrid.setEntry(mText, Vector2i(0, 3), true, false, Vector2i(1, 1),
			GridFlags::BORDER_TOP | GridFlags::BORDER_BOTTOM);
	mGrid.setEntry(mButtonGrid, Vector2i(0, 4), true, false);
	mGrid.setRowHeightPerc(1, 0.15, true);

	float textHeight = mText->getFont()->getHeight();

	if (multiLine)
		textHeight *= 6;

	mText->setSize(0, textHeight);

	setSize(Renderer::getScreenWidth() * 0.75f,mTitle->getFont()->getHeight() +
			textHeight + mButtonGrid->getSize().y() + 220);
	setPosition((Renderer::getScreenWidth() - mSize.x()) / 2,
			(Renderer::getScreenHeight() - mSize.y()) / 2);
}

void GuiComplexTextEditPopup::onSizeChanged()
{
	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

	mText->setSize(mSize.x() - 40, mText->getSize().y());

	// Update grid.
	mGrid.setRowHeightPerc(0, mTitle->getFont()->getHeight() / mSize.y());
	mGrid.setRowHeightPerc(2, mButtonGrid->getSize().y() / mSize.y());
	mGrid.setSize(mSize);
}

bool GuiComplexTextEditPopup::input(InputConfig* config, Input input)
{
	if (GuiComponent::input(config, input))
		return true;

	// Pressing back when not text editing closes us.
	if (config->isMappedTo("b", input) && input.value) {
		if (mText->getValue() != mInitValue) {
			// Changes were made, ask if the user wants to save them.
			mWindow->pushGui(new GuiMsgBox(mWindow, mHelpStyle, mSaveConfirmationText, "YES",
					[this] { this->mOkCallback(mText->getValue()); delete this; return true; },
					"NO", [this] { delete this; return false; }));
		}
		else {
			delete this;
		}
	}
	return false;
}

std::vector<HelpPrompt> GuiComplexTextEditPopup::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mGrid.getHelpPrompts();
	prompts.push_back(HelpPrompt("b", "back"));
	return prompts;
}
