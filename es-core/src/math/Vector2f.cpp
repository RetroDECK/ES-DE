//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Vector2f.cpp
//
//  2-dimensional floating point vector functions.
//

#include "math/Vector2f.h"

#include <cmath>

Vector2f& Vector2f::round()
{
    mX = std::round(mX);
    mY = std::round(mY);

    return *this;
}

Vector2f& Vector2f::lerp(const Vector2f& _start, const Vector2f& _end, const float _fraction)
{
    mX = Math::lerp(_start.x(), _end.x(), _fraction);
    mY = Math::lerp(_start.y(), _end.y(), _fraction);

    return *this;
}
