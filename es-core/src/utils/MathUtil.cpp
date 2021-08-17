//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  MathUtil.cpp
//
//  Math utility functions.
//  The GLM library headers are also included from here.
//

#include "utils/MathUtil.h"

namespace Utils
{
    namespace Math
    {
        float smoothStep(const float left, const float right, const float value)
        {
            const float x{glm::clamp((value - left) / (right - left), 0.0f, 1.0f)};
            return x * x * (3.0f - (2.0f * x));
        }

        float smootherStep(const float left, const float right, const float value)
        {
            const float x{glm::clamp((value - left) / (right - left), 0.0f, 1.0f)};
            return x * x * x * (x * ((x * 6.0f) - 15.0f) + 10.0f);
        }

        namespace Scroll
        {
            float bounce(const float delayTime,
                         const float scrollTime,
                         const float currentTime,
                         const float scrollLength)
            {
                if (currentTime < delayTime) {
                    // Wait.
                    return 0.0f;
                }
                else if (currentTime < (delayTime + scrollTime)) {
                    // Interpolate from 0 to scrollLength.
                    const float fraction{(currentTime - delayTime) / scrollTime};
                    return glm::mix(0.0f, scrollLength, smootherStep(0.0f, 1.0f, fraction));
                }
                else if (currentTime < (delayTime + scrollTime + delayTime)) {
                    // Wait some more.
                    return scrollLength;
                }
                else if (currentTime < (delayTime + scrollTime + delayTime + scrollTime)) {
                    // Interpolate back from scrollLength to 0.
                    const float fraction{(currentTime - delayTime - scrollTime - delayTime) /
                                         scrollTime};
                    return glm::mix(scrollLength, 0.0f, smootherStep(0.0f, 1.0f, fraction));
                }
                // And back to waiting.
                return 0.0f;
            }

            float loop(const float delayTime,
                       const float scrollTime,
                       const float currentTime,
                       const float scrollLength)
            {
                if (currentTime < delayTime) {
                    // Wait.
                    return 0.0f;
                }
                else if (currentTime < (delayTime + scrollTime)) {
                    // Interpolate from 0 to scrollLength.
                    const float fraction{(currentTime - delayTime) / scrollTime};
                    return glm::mix(0.0f, scrollLength, fraction);
                }

                // And back to waiting.
                return 0.0f;
            }

        } // namespace Scroll

    } // namespace Math

} // namespace Utils
