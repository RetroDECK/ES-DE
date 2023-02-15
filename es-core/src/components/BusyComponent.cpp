//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  BusyComponent.cpp
//
//  Animated busy indicator.
//

#include "BusyComponent.h"

#include "components/AnimatedImageComponent.h"
#include "components/ImageComponent.h"
#include "components/TextComponent.h"

// Animation definition.
AnimationFrame BUSY_ANIMATION_FRAMES[] {
    {":/graphics/busy_0.svg", 300},
    {":/graphics/busy_1.svg", 300},
    {":/graphics/busy_2.svg", 300},
    {":/graphics/busy_3.svg", 300},
};

const AnimationDef BUSY_ANIMATION_DEF = {BUSY_ANIMATION_FRAMES, 4, true};

BusyComponent::BusyComponent()
    : mBackground {":/graphics/frame.png"}
    , mGrid {glm::ivec2 {5, 3}}
{
    mAnimation = std::make_shared<AnimatedImageComponent>();
    mText = std::make_shared<TextComponent>("WORKING...", Font::get(FONT_SIZE_MEDIUM), 0x777777FF);

    // Col 0 = animation, col 1 = spacer, col 2 = text.
    mGrid.setEntry(mAnimation, glm::ivec2 {1, 1}, false, true);
    mGrid.setEntry(mText, glm::ivec2 {3, 1}, false, true);

    addChild(&mBackground);
    addChild(&mGrid);
}

void BusyComponent::onSizeChanged()
{
    mGrid.setSize(mSize);

    if (mSize.x == 0.0f || mSize.y == 0.0f)
        return;

    const float middleSpacerWidth {0.01f * Renderer::getScreenWidth()};
    const float textHeight {mText->getFont()->getLetterHeight()};
    mText->setSize(0, textHeight);
    const float textWidth {mText->getSize().x + (4.0f * Renderer::getScreenWidthModifier())};

    mGrid.setColWidthPerc(1, textHeight / mSize.x); // Animation is square.
    mGrid.setColWidthPerc(2, middleSpacerWidth / mSize.x);
    mGrid.setColWidthPerc(3, textWidth / mSize.x);

    mGrid.setRowHeightPerc(1, textHeight / std::round(mSize.y));

    mBackground.setCornerSize({16.0f * Renderer::getScreenResolutionModifier(),
                               16.0f * Renderer::getScreenResolutionModifier()});
    mBackground.fitTo(glm::vec2 {mGrid.getColWidth(1) + mGrid.getColWidth(2) + mGrid.getColWidth(3),
                                 textHeight + (2.0f * Renderer::getScreenResolutionModifier())},
                      mAnimation->getPosition(), glm::vec2 {});

    mAnimation->load(&BUSY_ANIMATION_DEF);
}

void BusyComponent::reset()
{
    // mAnimation->reset();
}
