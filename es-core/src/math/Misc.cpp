//
//  Misc.cpp
//
//  Miscellaneous math functions.
//

#include "math/Misc.h"

#include <math.h>

namespace Math
{
    // Added here to avoid including math.h whenever these are used.
    float cosf(const float _num)
    {
        return ::cosf(_num);
    }

    float sinf(const float _num)
    {
        return ::sinf(_num);
    }

    float floorf(const float _num)
    {
        return ::floorf(_num);
    }

    float ceilf(const float _num)
    {
        return ::ceilf(_num);
    }

    int min(const int _num1, const int _num2)
    {
        return (_num1 < _num2) ? _num1 : _num2;
    }

    int max(const int _num1, const int _num2)
    {
        return (_num1 > _num2) ? _num1 : _num2;
    }

    float min(const float _num1, const float _num2)
    {
        return (_num1 < _num2) ? _num1 : _num2;
    }

    float max(const float _num1, const float _num2)
    {
        return (_num1 > _num2) ? _num1 : _num2;
    }

    float clamp(const float _min, const float _max, const float _num)
    {
        return max(min(_num, _max), _min);
    }

    float round(const float _num)
    {
        return (float)(int)(_num + 0.5);
    }

    float lerp(const float _start, const float _end, const float _fraction)
    {
        return (_start + ((_end - _start) * clamp(0, 1, _fraction)));
    }

    float smoothStep(const float _left, const float _right, const float _x)
    {
        const float x = clamp(0, 1, (_x - _left)/(_right - _left));
        return x * x * (3 - (2 * x));
    }

    float smootherStep(const float _left, const float _right, const float _x)
    {
        const float x = clamp(0, 1, (_x - _left)/(_right - _left));
        return x * x * x * (x * ((x * 6) - 15) + 10);
    }

    namespace Scroll
    {
        float bounce(const float _delayTime, const float _scrollTime,
                const float _currentTime, const float _scrollLength)
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
                const float fraction = (_currentTime - _delayTime - _scrollTime -
                        _delayTime) / _scrollTime;
                return lerp(_scrollLength, 0.0f, smootherStep(0, 1, fraction));
            }
            // And back to waiting.
            return 0;
        }

        float loop(const float _delayTime, const float _scrollTime,
                const float _currentTime, const float _scrollLength)
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
    } // Math::Scroll::
} // Math::
