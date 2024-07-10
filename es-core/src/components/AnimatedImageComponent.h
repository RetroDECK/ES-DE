//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  AnimatedImageComponent.h
//
//  Creates animation from multiple images files.
//

#ifndef ES_CORE_COMPONENTS_ANIMATED_IMAGE_COMPONENT_H
#define ES_CORE_COMPONENTS_ANIMATED_IMAGE_COMPONENT_H

#include "GuiComponent.h"

class ImageComponent;

struct AnimationFrame {
    std::string path;
    int time;
};

struct AnimationDef {
    AnimationFrame* frames;
    size_t frameCount;
    unsigned int color;
    bool loop;
};

class AnimatedImageComponent : public GuiComponent
{
public:
    AnimatedImageComponent();

    void load(const AnimationDef* def); // No reference to def is kept after loading is complete.

    void reset(); // Set to frame 0.

    void update(int deltaTime) override;
    void render(const glm::mat4& trans) override;

    void onSizeChanged() override;

private:
    using ImageFrame = std::pair<std::unique_ptr<ImageComponent>, int>;

    std::vector<ImageFrame> mFrames;

    bool mLoop;
    bool mEnabled;
    int mFrameAccumulator;
    int mCurrentFrame;
};

#endif // ES_CORE_COMPONENTS_ANIMATED_IMAGE_COMPONENT_H
