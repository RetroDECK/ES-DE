//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Vector4f.cpp
//
//  4-dimensional floating point vector functions.
//

#include "math/Vector4f.h"

Vector4f& Vector4f::round()
{
    mX = static_cast<float>(static_cast<int>(mX + 0.5f));
    mY = static_cast<float>(static_cast<int>(mY + 0.5f));
    mZ = static_cast<float>(static_cast<int>(mZ + 0.5f));
    mW = static_cast<float>(static_cast<int>(mW + 0.5f));

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
