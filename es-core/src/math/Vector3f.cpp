//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Vector3f.cpp
//
//  3-dimensional floating point vector functions.
//

#include "math/Vector3f.h"

#include <cmath>

Vector3f& Vector3f::round()
{
    mX = std::round(mX);
    mY = std::round(mY);
    mZ = std::round(mZ);

    return *this;
}

Vector3f& Vector3f::lerp(const Vector3f& _start, const Vector3f& _end, const float _fraction)
{
    mX = Math::lerp(_start.x(), _end.x(), _fraction);
    mY = Math::lerp(_start.y(), _end.y(), _fraction);
    mZ = Math::lerp(_start.z(), _end.z(), _fraction);

    return *this;
}
