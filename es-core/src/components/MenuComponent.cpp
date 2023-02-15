//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  MenuComponent.cpp
//
//  Basic component for building a menu.
//

#include "components/MenuComponent.h"

#include "Settings.h"
#include "components/ButtonComponent.h"

#define BUTTON_GRID_VERT_PADDING Font::get(FONT_SIZE_MEDIUM)->getLetterHeight() * 0.915f
#define BUTTON_GRID_HORIZ_PADDING Font::get(FONT_SIZE_MEDIUM)->getLetterHeight() * 0.283f

#define TITLE_HEIGHT                                                                               \
    (mTitle->getFont()->getLetterHeight() + (Renderer::getIsVerticalOrientation() ?                \
                                                 Renderer::getScreenWidth() * 0.0637f :            \
                                                 Renderer::getScreenHeight() * 0.0637f))

MenuComponent::MenuComponent(std::string title, const std::shared_ptr<Font>& titleFont)
    : mRenderer {Renderer::getInstance()}
    , mGrid {glm::ivec2 {2, 4}}
    , mNeedsSaving {false}
{
    addChild(&mBackground);
    addChild(&mGrid);

    mBackground.setImagePath(":/graphics/frame.svg");

    // Set up title.
    mTitle = std::make_shared<TextComponent>();
    mTitle->setHorizontalAlignment(ALIGN_CENTER);
    mTitle->setColor(0x555555FF);
    setTitle(title, titleFont);
    mGrid.setEntry(mTitle, glm::ivec2 {0, 0}, false, true, glm::ivec2 {2, 2});

    // Set up list which will never change (externally, anyway).
    mList = std::make_shared<ComponentList>();
    mGrid.setEntry(mList, glm::ivec2 {0, 2}, true, true, glm::ivec2 {2, 1});

    // Set up scroll indicators.
    mScrollUp = std::make_shared<ImageComponent>();
    mScrollDown = std::make_shared<ImageComponent>();

    mScrollUp->setResize(0.0f, mTitle->getFont()->getLetterHeight() / 2.0f);
    mScrollUp->setOrigin(0.0f, -0.35f);

    mScrollDown->setResize(0.0f, mTitle->getFont()->getLetterHeight() / 2.0f);
    mScrollDown->setOrigin(0.0f, 0.35f);

    mScrollIndicator = std::make_shared<ScrollIndicatorComponent>(mList, mScrollUp, mScrollDown);

    mGrid.setEntry(mScrollUp, glm::ivec2 {1, 0}, false, false, glm::ivec2 {1, 1});
    mGrid.setEntry(mScrollDown, glm::ivec2 {1, 1}, false, false, glm::ivec2 {1, 1});

    updateGrid();
    updateSize();

    mGrid.resetCursor();
}

MenuComponent::~MenuComponent()
{
    // Save when destroyed.
    save();
}

void MenuComponent::save()
{
    if (!mSaveFuncs.size())
        return;

    for (auto it = mSaveFuncs.cbegin(); it != mSaveFuncs.cend(); ++it)
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
    return (mButtonGrid ? mButtonGrid->getSize().y :
                          Font::get(FONT_SIZE_MEDIUM)->getSize() * 1.5f + BUTTON_GRID_VERT_PADDING);
}

void MenuComponent::updateSize()
{
    const float maxHeight {mRenderer->getScreenHeight() *
                           (mRenderer->getIsVerticalOrientation() ? 0.70f : 0.80f)};
    float height {TITLE_HEIGHT + mList->getTotalRowHeight() + getButtonGridHeight() +
                  (2.0f * mRenderer->getScreenResolutionModifier())};
    if (height > maxHeight) {
        height = TITLE_HEIGHT + getButtonGridHeight();
        int i {0};
        while (i < mList->size()) {
            // Add the separator height to the row height so that it also gets properly rendered.
            float rowHeight {mList->getRowHeight(i) + mRenderer->getScreenResolutionModifier()};
            if (height + rowHeight < maxHeight)
                height += rowHeight;
            else
                break;
            ++i;
        }
    }

    float width {std::min(mRenderer->getScreenHeight() * 1.05f,
                          mRenderer->getScreenWidth() *
                              (mRenderer->getIsVerticalOrientation() ? 0.94f : 0.90f))};
    setSize(width, height);
}

void MenuComponent::onSizeChanged()
{
    mBackground.fitTo(mSize, glm::vec3 {0.0f, 0.0f, 0.0f}, glm::vec2 {-32.0f, -32.0f});

    // Update grid row/column sizes.
    mGrid.setRowHeightPerc(0, TITLE_HEIGHT / mSize.y / 2.0f);
    mGrid.setRowHeightPerc(1, TITLE_HEIGHT / mSize.y / 2.0f);
    mGrid.setRowHeightPerc(3, getButtonGridHeight() / mSize.y);

    mGrid.setColWidthPerc(1, 0.055f);

    mGrid.setSize(mSize);

    // Limit the title size to reserve space for the scroll indicators.
    float indicatorsSize {mSize.x * 0.09f};

    glm::vec2 titleSize {mTitle->getSize()};
    mTitle->setSize(titleSize.x - indicatorsSize, titleSize.y);

    glm::vec3 titlePos {mTitle->getPosition()};
    mTitle->setPosition(titlePos.x + indicatorsSize / 2.0f, titlePos.y, titlePos.z);
}

void MenuComponent::addButton(const std::string& name,
                              const std::string& helpText,
                              const std::function<void()>& callback)
{
    mButtons.push_back(
        std::make_shared<ButtonComponent>(Utils::String::toUpper(name), helpText, callback));
    updateGrid();
    updateSize();
}

void MenuComponent::updateGrid()
{
    if (mButtonGrid)
        mGrid.removeEntry(mButtonGrid);

    mButtonGrid.reset();

    if (mButtons.size()) {
        mButtonGrid = makeButtonGrid(mButtons);
        mGrid.setEntry(mButtonGrid, glm::ivec2 {0, 3}, true, false, glm::ivec2 {2, 1});
    }
}

std::shared_ptr<ComponentGrid> makeButtonGrid(
    const std::vector<std::shared_ptr<ButtonComponent>>& buttons)
{
    std::shared_ptr<ComponentGrid> buttonGrid {
        std::make_shared<ComponentGrid>(glm::ivec2 {static_cast<int>(buttons.size()), 2})};

    // Initialize to padding.
    float buttonGridWidth {BUTTON_GRID_HORIZ_PADDING * buttons.size()};

    for (int i = 0; i < static_cast<int>(buttons.size()); ++i) {
        buttonGrid->setEntry(buttons.at(i), glm::ivec2 {i, 0}, true, false);
        buttonGridWidth += buttons.at(i)->getSize().x;
    }
    for (unsigned int i = 0; i < buttons.size(); ++i)
        buttonGrid->setColWidthPerc(i, (buttons.at(i)->getSize().x + BUTTON_GRID_HORIZ_PADDING) /
                                           buttonGridWidth);

    buttonGrid->setSize(buttonGridWidth,
                        buttons.at(0)->getSize().y + BUTTON_GRID_VERT_PADDING + 2.0f);
    // Spacer row to deal with dropshadow to make buttons look centered.
    buttonGrid->setRowHeightPerc(1, 2.0f / buttonGrid->getSize().y);

    return buttonGrid;
}

std::shared_ptr<ImageComponent> makeArrow()
{
    auto bracket = std::make_shared<ImageComponent>();
    bracket->setResize(0, Font::get(FONT_SIZE_MEDIUM)->getLetterHeight());
    bracket->setImage(":/graphics/arrow.svg");
    return bracket;
}
