//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  MoveCameraAnimation.h
//
//  Animation to play when moving the camera, used by the slide transition style.
//

#ifndef ES_APP_ANIMATIONS_MOVE_CAMERA_ANIMATION_H
#define ES_APP_ANIMATIONS_MOVE_CAMERA_ANIMATION_H

#include "animations/Animation.h"
#include "math/Misc.h"

class MoveCameraAnimation : public Animation
{
public:
    MoveCameraAnimation(glm::mat4& camera, const glm::vec3& target)
        : mCameraStart(camera)
        , mTarget(target)
        , cameraPosition(camera)
    {
    }

    int getDuration() const override { return 400; }

    void apply(float t) override
    {
        t -= 1;
        cameraPosition[3].x = -glm::mix(-mCameraStart[3].x, mTarget.x, t * t * t + 1);
        cameraPosition[3].y = -glm::mix(-mCameraStart[3].y, mTarget.y, t * t * t + 1);
        cameraPosition[3].z = -glm::mix(-mCameraStart[3].z, mTarget.z, t * t * t + 1);
    }

private:
    glm::mat4 mCameraStart;
    glm::mat4& cameraPosition;
    glm::vec3 mTarget;
};

#endif // ES_APP_ANIMATIONS_MOVE_CAMERA_ANIMATION_H
