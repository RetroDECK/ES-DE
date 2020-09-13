//
//  LambdaAnimation.h
//
//  Basic animation controls, to be used in lambda expressions.
//

#pragma once
#ifndef ES_CORE_ANIMATIONS_LAMBDA_ANIMATION_H
#define ES_CORE_ANIMATIONS_LAMBDA_ANIMATION_H

#include "animations/Animation.h"

#include <functional>

// Useful for simple one-off animations, you can supply the animation's apply(t)
// function directly in the constructor as a lambda.
class LambdaAnimation : public Animation
{
public:
    LambdaAnimation(const std::function<void(float t)>& func, int duration)
            : mFunction(func), mDuration(duration) {}

    virtual ~LambdaAnimation() = default;

    int getDuration() const override { return mDuration; }

    void apply(float t) override
    {
        mFunction(t);
    }

private:
    std::function<void(float t)> mFunction;
    int mDuration;
};

#endif // ES_CORE_ANIMATIONS_LAMBDA_ANIMATION_H
