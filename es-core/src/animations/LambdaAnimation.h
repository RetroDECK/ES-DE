//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  LambdaAnimation.h
//
//  Custom animations, expressed as lambdas.
//

#ifndef ES_CORE_ANIMATIONS_LAMBDA_ANIMATION_H
#define ES_CORE_ANIMATIONS_LAMBDA_ANIMATION_H

#include "animations/Animation.h"

#include <functional>

class LambdaAnimation : public Animation
{
public:
    LambdaAnimation(const std::function<void(float t)>& func, int duration)
        : mFunction {func}
        , mDuration {duration}
    {
    }

    virtual ~LambdaAnimation() = default;

    int getDuration() const override { return mDuration; }
    void apply(float t) override { mFunction(t); }

private:
    std::function<void(float t)> mFunction;
    int mDuration;
};

#endif // ES_CORE_ANIMATIONS_LAMBDA_ANIMATION_H
