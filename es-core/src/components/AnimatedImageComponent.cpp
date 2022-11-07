//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  AnimatedImageComponent.cpp
//
//  Creates animation from multiple images files.
//

#include "components/AnimatedImageComponent.h"

#include "Log.h"
#include "components/ImageComponent.h"
#include "resources/ResourceManager.h"

AnimatedImageComponent::AnimatedImageComponent()
    : mEnabled {false}
{
}

void AnimatedImageComponent::load(const AnimationDef* def)
{
    mFrames.clear();

    assert(def->frameCount >= 1);

    for (size_t i = 0; i < def->frameCount; ++i) {
        if (def->frames[i].path != "" &&
            !ResourceManager::getInstance().fileExists(def->frames[i].path)) {
            LOG(LogError) << "Missing animation frame " << i << " (\"" << def->frames[i].path
                          << "\")";
            continue;
        }

        auto img = std::unique_ptr<ImageComponent>(new ImageComponent);
        img->setResize(mSize);
        img->setImage(std::string(def->frames[i].path), false);

        mFrames.push_back(ImageFrame(std::move(img), def->frames[i].time));
    }

    mLoop = def->loop;

    mCurrentFrame = 0;
    mFrameAccumulator = 0;
    mEnabled = true;
}

void AnimatedImageComponent::reset()
{
    mCurrentFrame = 0;
    mFrameAccumulator = 0;
}

void AnimatedImageComponent::onSizeChanged()
{
    for (auto it = mFrames.cbegin(); it != mFrames.cend(); ++it)
        it->first->setResize(mSize);
}

void AnimatedImageComponent::update(int deltaTime)
{
    if (!mEnabled || mFrames.size() == 0)
        return;

    mFrameAccumulator += deltaTime;

    while (mFrames.at(mCurrentFrame).second <= mFrameAccumulator) {
        ++mCurrentFrame;

        if (mCurrentFrame == static_cast<int>(mFrames.size())) {
            if (mLoop) {
                // Restart.
                mCurrentFrame = 0;
            }
            else {
                // Done, stop at last frame.
                --mCurrentFrame;
                mEnabled = false;
                break;
            }
        }

        mFrameAccumulator -= mFrames.at(mCurrentFrame).second;
    }
}

void AnimatedImageComponent::render(const glm::mat4& trans)
{
    if (mFrames.size())
        mFrames.at(mCurrentFrame).first->render(getTransform() * trans);
}
