//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Vector3f.h
//
//  3-dimensional floating point vector functions.
//

#ifndef ES_CORE_MATH_VECTOR3F_H
#define ES_CORE_MATH_VECTOR3F_H

#include "math/Misc.h"

#include <assert.h>

class Vector2f;
class Vector4f;

class Vector3f
{
public:
    Vector3f() {}
    Vector3f(const float _f)
        : mX(_f)
        , mY(_f)
        , mZ(_f)
    {
    }
    Vector3f(const float _x, const float _y, const float _z)
        : mX(_x)
        , mY(_y)
        , mZ(_z)
    {
    }
    explicit Vector3f(const Vector2f& _v)
        : mX((reinterpret_cast<const Vector3f&>(_v)).mX)
        , mY((reinterpret_cast<const Vector3f&>(_v)).mY)
        , mZ(0)
    {
    }
    explicit Vector3f(const Vector2f& _v, const float _z)
        : mX((reinterpret_cast<const Vector3f&>(_v)).mX)
        , mY((reinterpret_cast<const Vector3f&>(_v)).mY)
        , mZ(_z)
    {
    }
    explicit Vector3f(const Vector4f& _v)
        : mX((reinterpret_cast<const Vector3f&>(_v)).mX)
        , mY((reinterpret_cast<const Vector3f&>(_v)).mY)
        , mZ((reinterpret_cast<const Vector3f&>(_v)).mZ)
    {
    }

    // clang-format off
    const bool operator==(const Vector3f& _other) const
        { return ((mX == _other.mX) && (mY == _other.mY) && (mZ == _other.mZ)); }
    const bool operator!=(const Vector3f& _other) const
        { return ((mX != _other.mX) || (mY != _other.mY) || (mZ != _other.mZ)); }

    const Vector3f operator+(const Vector3f& _other) const
        { return { mX + _other.mX, mY + _other.mY, mZ + _other.mZ }; }
    const Vector3f operator-(const Vector3f& _other) const
        { return { mX - _other.mX, mY - _other.mY, mZ - _other.mZ }; }
    const Vector3f operator*(const Vector3f& _other) const
        { return { mX * _other.mX, mY * _other.mY, mZ * _other.mZ }; }
    const Vector3f operator/(const Vector3f& _other) const
        { return { mX / _other.mX, mY / _other.mY, mZ / _other.mZ }; }

    const Vector3f operator+(const float& _other) const
        { return { mX + _other, mY + _other, mZ + _other }; }
    const Vector3f operator-(const float& _other) const
        { return { mX - _other, mY - _other, mZ - _other }; }
    const Vector3f operator*(const float& _other) const
        { return { mX * _other, mY * _other, mZ * _other }; }
    const Vector3f operator/(const float& _other) const
        { return { mX / _other, mY / _other, mZ / _other }; }

    const Vector3f operator-() const { return { -mX , -mY, -mZ }; }

    Vector3f& operator+=(const Vector3f& _other) { *this = *this + _other; return *this; }
    Vector3f& operator-=(const Vector3f& _other) { *this = *this - _other; return *this; }
    Vector3f& operator*=(const Vector3f& _other) { *this = *this * _other; return *this; }
    Vector3f& operator/=(const Vector3f& _other) { *this = *this / _other; return *this; }

    Vector3f& operator+=(const float& _other) { *this = *this + _other; return *this; }
    Vector3f& operator-=(const float& _other) { *this = *this - _other; return *this; }
    Vector3f& operator*=(const float& _other) { *this = *this * _other; return *this; }
    Vector3f& operator/=(const float& _other) { *this = *this / _other; return *this; }

    float& operator[](const int _index)
        { assert(_index < 3 && "index out of range"); return (&mX)[_index]; }
    const float& operator[](const int _index) const
        { assert(_index < 3 && "index out of range"); return (&mX)[_index]; }
    // clang-format on

    float& x() { return mX; }
    float& y() { return mY; }
    float& z() { return mZ; }
    const float& x() const { return mX; }
    const float& y() const { return mY; }
    const float& z() const { return mZ; }

    Vector2f& v2() { return *reinterpret_cast<Vector2f*>(this); }
    const Vector2f& v2() const { return *reinterpret_cast<const Vector2f*>(this); }

    Vector3f& round();
    Vector3f& lerp(const Vector3f& _start, const Vector3f& _end, const float _fraction);

    static const Vector3f Zero() { return {0.0f, 0.0f, 0.0f}; }
    static const Vector3f UnitX() { return {1.0f, 0.0f, 0.0f}; }
    static const Vector3f UnitY() { return {0.0f, 1.0f, 0.0f}; }
    static const Vector3f UnitZ() { return {0.0f, 0.0f, 1.0f}; }

private:
    float mX;
    float mY;
    float mZ;
};

#endif // ES_CORE_MATH_VECTOR3F_H
