//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Misc.cpp
//
//  Miscellaneous math functions.
//

#include "math/Misc.h"

namespace Math
{
    float lerp(const float _start, const float _end, const float _fraction)
    {
        return (_start + ((_end - _start) * clamp(_fraction, 0.0f, 1.0f)));
    }

    float smoothStep(const float _left, const float _right, const float _x)
    {
        const float x = clamp((_x - _left) / (_right - _left), 0.0f, 1.0f);
        return x * x * (3 - (2 * x));
    }

    float smootherStep(const float _left, const float _right, const float _x)
    {
        const float x = clamp((_x - _left) / (_right - _left), 0.0f, 1.0f);
        return x * x * x * (x * ((x * 6) - 15) + 10);
    }

    namespace Scroll
    {
        float bounce(const float _delayTime,
                     const float _scrollTime,
                     const float _currentTime,
                     const float _scrollLength)
        {
            if (_currentTime < _delayTime) {
                // Wait.
                return 0;
            }
            else if (_currentTime < (_delayTime + _scrollTime)) {
                // Lerp from 0 to scrollLength.
                const float fraction = (_currentTime - _delayTime) / _scrollTime;
                return lerp(0.0f, _scrollLength, smootherStep(0, 1, fraction));
            }
            else if (_currentTime < (_delayTime + _scrollTime + _delayTime)) {
                // Wait some more.
                return _scrollLength;
            }
            else if (_currentTime < (_delayTime + _scrollTime + _delayTime + _scrollTime)) {
                // Lerp back from scrollLength to 0.
                const float fraction =
                    (_currentTime - _delayTime - _scrollTime - _delayTime) / _scrollTime;
                return lerp(_scrollLength, 0.0f, smootherStep(0, 1, fraction));
            }
            // And back to waiting.
            return 0;
        }

        float loop(const float _delayTime,
                   const float _scrollTime,
                   const float _currentTime,
                   const float _scrollLength)
        {
            if (_currentTime < _delayTime) {
                // Wait.
                return 0;
            }
            else if (_currentTime < (_delayTime + _scrollTime)) {
                // Lerp from 0 to scrollLength.
                const float fraction = (_currentTime - _delayTime) / _scrollTime;
                return lerp(0.0f, _scrollLength, fraction);
            }

            // And back to waiting.
            return 0;

        } // Math::Scroll::loop

    } // namespace Scroll

} // namespace Math
