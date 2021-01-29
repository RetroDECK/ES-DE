//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  MenuComponent.cpp
//
//  Basic component for building a menu.
//

#include "components/MenuComponent.h"

#include "components/ButtonComponent.h"
#include "Settings.h"

#define BUTTON_GRID_VERT_PADDING 32.0f
#define BUTTON_GRID_HORIZ_PADDING 10.0f

#define TITLE_HEIGHT (mTitle->getFont()->getLetterHeight() + TITLE_VERT_PADDING)

MenuComponent::MenuComponent(
        Window* window,
        std::string title,
        const std::shared_ptr<Font>& titleFont)
        : GuiComponent(window),
        mBackground(window),
        mGrid(window, Vector2i(1, 3)),
        mNeedsSaving(false)
{
    addChild(&mBackground);
    addChild(&mGrid);

    mBackground.setImagePath(":/graphics/frame.svg");

    // Set up title.
    mTitle = std::make_shared<TextComponent>(mWindow);
    mTitle->setHorizontalAlignment(ALIGN_CENTER);
    mTitle->setColor(0x555555FF);
    setTitle(title, titleFont);
    mGrid.setEntry(mTitle, Vector2i(0, 0), false);

    // Set up list which will never change (externally, anyway).
    mList = std::make_shared<ComponentList>(mWindow);
    mGrid.setEntry(mList, Vector2i(0, 1), true);

    updateGrid();
    updateSize();

    mGrid.resetCursor();
}

MenuComponent::~MenuComponent()
{
    save();
}

void MenuComponent::save()
{
    if (!mSaveFuncs.size())
        return;

    for (auto it = mSaveFuncs.cbegin(); it != mSaveFuncs.cend(); it++)
        (*it)();

    if (mNeedsSaving) {
        Settings::getInstance()->saveFile();
        mNeedsSaving = false;
    }
}

void MenuComponent::setTitle(std::string title, const std::shared_ptr<Font>& font)
{
    mTitle->setText(Utils::String::toUpper(title));
    mTitle->setFont(font);
}

float MenuComponent::getButtonGridHeight() const
{
    return (mButtonGrid ? mButtonGrid->getSize().y() :
            Font::get(FONT_SIZE_MEDIUM)->getHeight() +
            (BUTTON_GRID_VERT_PADDING * Renderer::getScreenHeightModifier()));
}

void MenuComponent::updateSize()
{
    const float maxHeight = Renderer::getScreenHeight() * 0.75f;
    float height = TITLE_HEIGHT + mList->getTotalRowHeight() + getButtonGridHeight() +
            (2 * Renderer::getScreenHeightModifier());
    if (height > maxHeight) {
        height = TITLE_HEIGHT + getButtonGridHeight();
        int i = 0;
        while (i < mList->size()) {
            // Add the separator height to the row height so that it also gets properly rendered.
            float rowHeight = mList->getRowHeight(i) + (1 * Renderer::getScreenHeightModifier());
            if (height + rowHeight < maxHeight)
                height += rowHeight;
            else
                break;
            i++;
        }
    }

    float width = static_cast<float>(std::min(static_cast<int>(Renderer::getScreenHeight()),
            static_cast<int>(Renderer::getScreenWidth() * 0.90f)));
    setSize(width, height);
}

void MenuComponent::onSizeChanged()
{
    mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

    // Update grid row/col sizes.
    mGrid.setRowHeightPerc(0, TITLE_HEIGHT / mSize.y());
    mGrid.setRowHeightPerc(2, getButtonGridHeight() / mSize.y());

    mGrid.setSize(mSize);
}

void MenuComponent::addButton(const std::string& name,
        const std::string& helpText, const std::function<void()>& callback)
{
    mButtons.push_back(std::make_shared<ButtonComponent>
            (mWindow,Utils::String::toUpper(name), helpText, callback));
    updateGrid();
    updateSize();
}

void MenuComponent::updateGrid()
{
    if (mButtonGrid)
        mGrid.removeEntry(mButtonGrid);

    mButtonGrid.reset();

    if (mButtons.size()) {
        mButtonGrid = makeButtonGrid(mWindow, mButtons);
        mGrid.setEntry(mButtonGrid, Vector2i(0, 2), true, false);
    }
}

std::vector<HelpPrompt> MenuComponent::getHelpPrompts()
{
    return mGrid.getHelpPrompts();
}

std::shared_ptr<ComponentGrid> makeButtonGrid(Window* window,
        const std::vector< std::shared_ptr<ButtonComponent> >& buttons)
{
    std::shared_ptr<ComponentGrid> buttonGrid = std::make_shared<ComponentGrid>
            (window, Vector2i(static_cast<int>(buttons.size()), 2));

    // Initialize to padding.
    float buttonGridWidth = BUTTON_GRID_HORIZ_PADDING *
            Renderer::getScreenWidthModifier() * buttons.size();
    for (int i = 0; i < static_cast<int>(buttons.size()); i++) {
        buttonGrid->setEntry(buttons.at(i), Vector2i(i, 0), true, false);
        buttonGridWidth += buttons.at(i)->getSize().x();
    }
    for (unsigned int i = 0; i < buttons.size(); i++)
        buttonGrid->setColWidthPerc(i, (buttons.at(i)->getSize().x() +
                BUTTON_GRID_HORIZ_PADDING * Renderer::getScreenWidthModifier()) / buttonGridWidth);

    buttonGrid->setSize(buttonGridWidth, buttons.at(0)->getSize().y() +
            (BUTTON_GRID_VERT_PADDING * Renderer::getScreenHeightModifier()) + 2);
    // Spacer row to deal with dropshadow to make buttons look centered.
    buttonGrid->setRowHeightPerc(1, 2 / buttonGrid->getSize().y());

    return buttonGrid;
}

std::shared_ptr<ImageComponent> makeArrow(Window* window)
{
    auto bracket = std::make_shared<ImageComponent>(window);
    bracket->setImage(":/graphics/arrow.svg");
    bracket->setResize(0, std::round(Font::get(FONT_SIZE_MEDIUM)->getLetterHeight()));
    return bracket;
}
