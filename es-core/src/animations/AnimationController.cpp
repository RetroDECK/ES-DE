//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  AnimationController.cpp
//
//  Basic animation controls.
//

#include "animations/AnimationController.h"

#include "animations/Animation.h"
#include "utils/MathUtil.h"

AnimationController::AnimationController(Animation* anim,
                                         int delay,
                                         std::function<void()> finishedCallback,
                                         bool reverse)
    : mAnimation {anim}
    , mFinishedCallback {finishedCallback}
    , mReverse {reverse}
    , mTime {-delay}
    , mDelay {delay}
{
}

AnimationController::~AnimationController()
{
    if (mFinishedCallback)
        mFinishedCallback();

    delete mAnimation;
}

bool AnimationController::update(int deltaTime)
{
    mTime += deltaTime;

    if (mTime < 0) // Are we still in delay?
        return false;

    float animTime {glm::clamp(static_cast<float>(mTime) / mAnimation->getDuration(), 0.0f, 1.0f)};
    mAnimation->apply(mReverse ? 1.0f - animTime : animTime);

    if (animTime == 1.0f)
        return true;

    return false;
}
