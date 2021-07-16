//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  AnimationController.h
//
//  Basic animation controls.
//

#ifndef ES_CORE_ANIMATIONS_ANIMATION_CONTROLLER_H
#define ES_CORE_ANIMATIONS_ANIMATION_CONTROLLER_H

#include <functional>

class Animation;

class AnimationController
{
public:
    // Takes ownership of anim (will delete in destructor).
    AnimationController(Animation* anim,
                        int delay = 0,
                        std::function<void()> finishedCallback = nullptr,
                        bool reverse = false);
    virtual ~AnimationController();

    // Returns true if the animation is complete.
    bool update(int deltaTime);

    bool isReversed() const { return mReverse; }
    int getTime() const { return mTime; }
    int getDelay() const { return mDelay; }
    const std::function<void()>& getFinishedCallback() const { return mFinishedCallback; }
    Animation* getAnimation() const { return mAnimation; }

    void removeFinishedCallback() { mFinishedCallback = nullptr; }

private:
    Animation* mAnimation;
    std::function<void()> mFinishedCallback;
    bool mReverse;
    int mTime;
    int mDelay;
};

#endif // ES_CORE_ANIMATIONS_ANIMATION_CONTROLLER_H
