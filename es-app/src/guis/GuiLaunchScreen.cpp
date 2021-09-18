//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiLaunchScreen.cpp
//
//  Screen shown when launching a game.
//

#include "guis/GuiLaunchScreen.h"

#include "FileData.h"
#include "SystemData.h"
#include "components/ComponentGrid.h"
#include "components/TextComponent.h"
#include "utils/StringUtil.h"

GuiLaunchScreen::GuiLaunchScreen(Window* window)
    : GuiComponent(window)
    , mWindow(window)
    , mBackground(window, ":/graphics/frame.svg")
    , mGrid(nullptr)
    , mMarquee(nullptr)
{
    addChild(&mBackground);
    mWindow->setLaunchScreen(this);
}

GuiLaunchScreen::~GuiLaunchScreen()
{
    // This only executes when exiting the application.
    closeLaunchScreen();
}

void GuiLaunchScreen::displayLaunchScreen(FileData* game)
{
    mGrid = new ComponentGrid(mWindow, glm::ivec2{3, 8});
    addChild(mGrid);

    mImagePath = game->getMarqueePath();

    // We need to unload the image first as it may be cached at a modified resolution
    // which would lead to the wrong size when using the image here.
    if (mImagePath != "") {
        TextureResource::manualUnload(mImagePath, false);
        mMarquee = new ImageComponent(mWindow);
    }

    mScaleUp = 0.5f;
    const float titleFontSize = 0.060f;
    const float gameNameFontSize = 0.073f;

    // Spacer row.
    mGrid->setEntry(std::make_shared<GuiComponent>(mWindow), glm::ivec2{1, 0}, false, false,
                    glm::ivec2{1, 1});

    // Title.
    mTitle = std::make_shared<TextComponent>(
        mWindow, "LAUNCHING GAME",
        Font::get(static_cast<int>(
            titleFontSize * std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth()))),
        0x666666FF, ALIGN_CENTER);
    mGrid->setEntry(mTitle, glm::ivec2{1, 1}, false, true, glm::ivec2{1, 1});

    // Spacer row.
    mGrid->setEntry(std::make_shared<GuiComponent>(mWindow), glm::ivec2{1, 2}, false, false,
                    glm::ivec2{1, 1});

    // Row for the marquee.
    mGrid->setEntry(std::make_shared<GuiComponent>(mWindow), glm::ivec2{1, 3}, false, false,
                    glm::ivec2{1, 1});

    // Spacer row.
    mGrid->setEntry(std::make_shared<GuiComponent>(mWindow), glm::ivec2{1, 4}, false, false,
                    glm::ivec2{1, 1});

    // Game name.
    mGameName = std::make_shared<TextComponent>(
        mWindow, "GAME NAME",
        Font::get(static_cast<int>(
            gameNameFontSize * std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth()))),
        0x444444FF, ALIGN_CENTER);
    mGrid->setEntry(mGameName, glm::ivec2{1, 5}, false, true, glm::ivec2{1, 1});

    // System name.
    mSystemName = std::make_shared<TextComponent>(
        mWindow, "SYSTEM NAME", Font::get(FONT_SIZE_MEDIUM), 0x666666FF, ALIGN_CENTER);
    mGrid->setEntry(mSystemName, glm::ivec2{1, 6}, false, true, glm::ivec2{1, 1});

    // Spacer row.
    mGrid->setEntry(std::make_shared<GuiComponent>(mWindow), glm::ivec2{1, 7}, false, false,
                    glm::ivec2{1, 1});

    // Left spacer.
    mGrid->setEntry(std::make_shared<GuiComponent>(mWindow), glm::ivec2{0, 0}, false, false,
                    glm::ivec2{1, 8});

    // Right spacer.
    mGrid->setEntry(std::make_shared<GuiComponent>(mWindow), glm::ivec2{2, 0}, false, false,
                    glm::ivec2{1, 8});

    // Adjust the width depending on the aspect ratio of the screen, to make the screen look
    // somewhat coherent regardless of screen type. The 1.778 aspect ratio value is the 16:9
    // reference.
    float aspectValue = 1.778f / Renderer::getScreenAspectRatio();

    float maxWidthModifier = glm::clamp(0.78f * aspectValue, 0.78f, 0.90f);
    float minWidthModifier = glm::clamp(0.50f * aspectValue, 0.50f, 0.65f);

    float maxWidth = static_cast<float>(Renderer::getScreenWidth()) * maxWidthModifier;
    float minWidth = static_cast<float>(Renderer::getScreenWidth()) * minWidthModifier;

    float fontWidth =
        Font::get(static_cast<int>(gameNameFontSize * std::min(Renderer::getScreenHeight(),
                                                               Renderer::getScreenWidth())))
            ->sizeText(Utils::String::toUpper(game->getName()))
            .x;

    // Add a bit of width to compensate for the left and right spacers.
    fontWidth += static_cast<float>(Renderer::getScreenWidth()) * 0.05f;

    float width = glm::clamp(fontWidth, minWidth, maxWidth);

    if (mImagePath != "")
        setSize(width, static_cast<float>(Renderer::getScreenHeight()) * 0.60f);
    else
        setSize(width, static_cast<float>(Renderer::getScreenHeight()) * 0.38f);

    // Set row heights.
    if (mImagePath != "")
        mGrid->setRowHeightPerc(0, 0.09f, false);
    else
        mGrid->setRowHeightPerc(0, 0.15f, false);
    mGrid->setRowHeightPerc(1, mTitle->getFont()->getLetterHeight() * 1.70f / mSize.y, false);
    mGrid->setRowHeightPerc(2, 0.05f, false);
    if (mImagePath != "")
        mGrid->setRowHeightPerc(3, 0.35f, false);
    else
        mGrid->setRowHeightPerc(3, 0.01f, false);
    mGrid->setRowHeightPerc(4, 0.05f, false);
    mGrid->setRowHeightPerc(5, mGameName->getFont()->getHeight() * 0.80f / mSize.y, false);
    mGrid->setRowHeightPerc(6, mSystemName->getFont()->getHeight() * 0.90f / mSize.y, false);

    // Set left and right spacers column widths.
    mGrid->setColWidthPerc(0, 0.025f);
    mGrid->setColWidthPerc(2, 0.025f);

    mGrid->setSize(mSize);

    float totalRowHeight = 0.0f;

    // Hack to adjust the window height to the row boundary.
    for (int i = 0; i < 7; i++)
        totalRowHeight += mGrid->getRowHeight(i);

    setSize(mSize.x, totalRowHeight);

    mGameName->setText(Utils::String::toUpper(game->getName()));
    mSystemName->setText(Utils::String::toUpper(game->getSystem()->getFullName()));

    // For the marquee we strip away any transparent padding around the actual image.
    // When doing this, we restrict the scale-up to a certain percentage of the screen
    // width so that the sizes look somewhat consistent regardless of the aspect ratio
    // of the images.
    if (mImagePath != "") {
        mMarquee->setImage(game->getMarqueePath(), false, true);
        mMarquee->cropTransparentPadding(static_cast<float>(Renderer::getScreenWidth()) *
                                             (0.25f * (1.778f / Renderer::getScreenAspectRatio())),
                                         mGrid->getRowHeight(3));

        mMarquee->setOrigin(0.5f, 0.5f);
        glm::vec3 currentPos{mMarquee->getPosition()};

        // Position the image in the middle of row four.
        currentPos.x = mSize.x / 2.0f;
        currentPos.y = mGrid->getRowHeight(0) + mGrid->getRowHeight(1) + mGrid->getRowHeight(2) +
                       mGrid->getRowHeight(3) / 2.0f;
        mMarquee->setPosition(currentPos);
    }

    setOrigin({0.5f, 0.5f});

    // Center on the X axis and keep slightly off-center on the Y axis.
    setPosition(static_cast<float>(Renderer::getScreenWidth()) / 2.0f,
                static_cast<float>(Renderer::getScreenHeight()) / 2.25f);

    mBackground.fitTo(mSize, glm::vec3{}, glm::vec2{-32.0f, -32.0f});
    mBackground.setEdgeColor(0xEEEEEEFF);
}

void GuiLaunchScreen::closeLaunchScreen()
{
    if (mGrid) {
        delete mGrid;
        mGrid = nullptr;
    }

    if (mMarquee) {
        delete mMarquee;
        mMarquee = nullptr;
    }

    // An extra precaution.
    if (mImagePath != "") {
        TextureResource::manualUnload(mImagePath, false);
        mImagePath = "";
    }
}

void GuiLaunchScreen::onSizeChanged()
{
    // Update mGrid size.
    mGrid->setSize(mSize);
}

void GuiLaunchScreen::update(int deltaTime)
{
    if (Settings::getInstance()->getString("MenuOpeningEffect") == "none")
        mScaleUp = 1.0f;
    else if (mScaleUp < 1.0f)
        mScaleUp = glm::clamp(mScaleUp + 0.07f, 0.0f, 1.0f);
}

void GuiLaunchScreen::render()
{
    // Scale up animation.
    if (mScaleUp < 1.0f)
        setScale(mScaleUp);

    glm::mat4 trans{Renderer::getIdentity() * getTransform()};
    Renderer::setMatrix(trans);

    GuiComponent::renderChildren(trans);

    if (mMarquee)
        mMarquee->render(trans);
}
