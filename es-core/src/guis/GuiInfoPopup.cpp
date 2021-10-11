//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiInfoPopup.cpp
//
//  Popup window used for user notifications.
//

#include "guis/GuiInfoPopup.h"

#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"
#include "components/TextComponent.h"

#include <SDL2/SDL_timer.h>

GuiInfoPopup::GuiInfoPopup(Window* window, std::string message, int duration)
    : GuiComponent(window)
    , mMessage(message)
    , mDuration(duration)
    , mRunning(true)
{
    mFrame = new NinePatchComponent(window);
    float maxWidth = Renderer::getScreenWidth() * 0.9f;
    float maxHeight = Renderer::getScreenHeight() * 0.2f;

    std::shared_ptr<TextComponent> s = std::make_shared<TextComponent>(
        mWindow, "", Font::get(FONT_SIZE_MINI), 0x444444FF, ALIGN_CENTER);

    // We do this to force the text container to resize and return the actual expected popup size.
    s->setSize(0.0f, 0.0f);
    s->setText(message);
    mSize = s->getSize();

    // Confirm that the size isn't larger than the screen width, otherwise cap it.
    if (mSize.x > maxWidth) {
        s->setSize(maxWidth, mSize.y);
        mSize.x = maxWidth;
    }
    if (mSize.y > maxHeight) {
        s->setSize(mSize.x, maxHeight);
        mSize.y = maxHeight;
    }

    // Add a padding to the box.
    int paddingX = static_cast<int>(Renderer::getScreenWidth() * 0.03f);
    int paddingY = static_cast<int>(Renderer::getScreenHeight() * 0.02f);
    mSize.x = mSize.x + paddingX;
    mSize.y = mSize.y + paddingY;

    float posX = Renderer::getScreenWidth() * 0.5f - mSize.x * 0.5f;
    float posY = Renderer::getScreenHeight() * 0.02f;

    setPosition(posX, posY, 0);

    mFrame->setImagePath(":/graphics/frame.svg");
    mFrame->fitTo(mSize, glm::vec3{}, glm::vec2{-32.0f, -32.0f});
    addChild(mFrame);

    // We only initialize the actual time when we first start to render.
    mStartTime = 0;

    mGrid = new ComponentGrid(window, glm::ivec2{1, 3});
    mGrid->setSize(mSize);
    mGrid->setEntry(s, glm::ivec2{0, 1}, false, true);
    addChild(mGrid);
}

GuiInfoPopup::~GuiInfoPopup()
{
    delete mGrid;
    delete mFrame;
}

void GuiInfoPopup::render(const glm::mat4& /*parentTrans*/)
{
    // We use getIdentity() as we want to render on a specific window position, not on the view.
    glm::mat4 trans{getTransform() * Renderer::getIdentity()};
    if (mRunning && updateState()) {
        // If we're still supposed to be rendering it.
        Renderer::setMatrix(trans);
        renderChildren(trans);
    }
}

bool GuiInfoPopup::updateState()
{
    int curTime = SDL_GetTicks();

    // We only initialize the actual time when we first start to render.
    if (mStartTime == 0)
        mStartTime = curTime;

    // Compute fade-in effect.
    if (curTime - mStartTime > mDuration) {
        // We're past the popup duration, no need to render.
        mRunning = false;
        return false;
    }
    else if (curTime < mStartTime) {
        // If SDL reset.
        mRunning = false;
        return false;
    }
    else if (curTime - mStartTime <= 500) {
        mAlpha = ((curTime - mStartTime) * 255 / 500);
    }
    else if (curTime - mStartTime < mDuration - 500) {
        mAlpha = 255;
    }
    else {
        mAlpha = ((-(curTime - mStartTime - mDuration) * 255) / 500);
    }
    mGrid->setOpacity(static_cast<unsigned char>(mAlpha));

    // Apply fade-in effect to popup frame.
    mFrame->setEdgeColor(0xFFFFFF00 | static_cast<unsigned char>(mAlpha));
    mFrame->setCenterColor(0xFFFFFF00 | static_cast<unsigned char>(mAlpha));
    return true;
}
