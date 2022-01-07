//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  LottieComponent.h
//
//  Component to play Lottie animations using the rlottie library.
//

#ifndef ES_CORE_COMPONENTS_LOTTIE_COMPONENT_H
#define ES_CORE_COMPONENTS_LOTTIE_COMPONENT_H

#include "GuiComponent.h"
#include "renderers/Renderer.h"
#include "resources/TextureResource.h"

#include "rlottie.h"

#include <chrono>
#include <future>

class LottieComponent : public GuiComponent
{
public:
    LottieComponent(Window* window);

    virtual void applyTheme(const std::shared_ptr<ThemeData>& theme,
                            const std::string& view,
                            const std::string& element,
                            unsigned int properties) override;

private:
    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;

    std::shared_ptr<TextureResource> mTexture;
    std::vector<uint8_t> mPictureRGBA;

    Renderer::Vertex mVertices[4];

    std::unique_ptr<rlottie::Animation> mAnimation;
    std::unique_ptr<rlottie::Surface> mSurface;
    std::future<rlottie::Surface> mFuture;
    size_t mTotalFrames;
    size_t mFrameNum;
    double mFrameRate;
    int mTargetPacing;
    int mTimeAccumulator;
    bool mHoldFrame;

    std::chrono::time_point<std::chrono::system_clock> mAnimationStartTime;
    std::chrono::time_point<std::chrono::system_clock> mAnimationEndTime;
};

#endif // ES_CORE_COMPONENTS_LOTTIE_COMPONENT_H
