//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Misc.h
//
//  Miscellaneous math functions.
//

#ifndef ES_CORE_MATH_MISC_H
#define ES_CORE_MATH_MISC_H

#include <algorithm>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include <glm/gtc/round.hpp>

namespace Math
{
    // When moving to the C++20 standard these functions are no longer required.
    template <typename T> T const& clamp(const T& _num, const T& _min, const T& _max)
    {
        return std::max(std::min(_num, _max), _min);
    }
    float lerp(const float _start, const float _end, const float _fraction);

    float smoothStep(const float _left, const float _right, const float _x);
    float smootherStep(const float _left, const float _right, const float _x);

    namespace Scroll
    {
        float bounce(const float _delayTime,
                     const float _scrollTime,
                     const float _currentTime,
                     const float _scrollLength);
        float loop(const float _delayTime,
                   const float _scrollTime,
                   const float _currentTime,
                   const float _scrollLength);
    } // namespace Scroll

} // namespace Math

#endif // ES_CORE_MATH_MISC_H
