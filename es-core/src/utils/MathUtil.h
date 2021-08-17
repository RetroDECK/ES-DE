//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  MathUtil.h
//
//  Math utility functions.
//  The GLM library headers are also included from here.
//

#ifndef ES_CORE_UTILS_MATH_UTIL_H
#define ES_CORE_UTILS_MATH_UTIL_H

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

namespace Utils
{
    namespace Math
    {
        float smoothStep(const float left, const float right, const float value);
        float smootherStep(const float left, const float right, const float value);

        namespace Scroll
        {
            float bounce(const float delayTime,
                         const float scrollTime,
                         const float currentTime,
                         const float scrollLength);
            float loop(const float delayTime,
                       const float scrollTime,
                       const float currentTime,
                       const float scrollLength);
        } // namespace Scroll

    } // namespace Math

} // namespace Utils

#endif // ES_CORE_UTILS_MATH_UTIL_H
