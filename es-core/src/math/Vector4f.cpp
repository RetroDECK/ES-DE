//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Vector4f.cpp
//
//  4-dimensional floating point vector functions.
//

#include "math/Vector4f.h"

#include <cmath>

Vector4f& Vector4f::round()
{
    mX = std::round(mX);
    mY = std::round(mY);
    mZ = std::round(mZ);
    mW = std::round(mW);

    return *this;
}

Vector4f& Vector4f::lerp(const Vector4f& _start, const Vector4f& _end, const float _fraction)
{
    mX = Math::lerp(_start.x(), _end.x(), _fraction);
    mY = Math::lerp(_start.y(), _end.y(), _fraction);
    mZ = Math::lerp(_start.z(), _end.z(), _fraction);
    mW = Math::lerp(_start.w(), _end.w(), _fraction);

    return *this;
}
