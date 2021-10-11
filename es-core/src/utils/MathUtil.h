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

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/trigonometric.hpp"

#include <string>

namespace Utils
{
    namespace Math
    {
        float smoothStep(const float left, const float right, const float value);
        float smootherStep(const float left, const float right, const float value);

        // Used for horizontal scrolling, e.g. long names in TextListComponent.
        float loop(const float delayTime,
                   const float scrollTime,
                   const float currentTime,
                   const float scrollLength);
        // Variation of the loop, with an acceleration and deceleration at the start and ending.
        float bounce(const float delayTime,
                     const float scrollTime,
                     const float currentTime,
                     const float scrollLength);

        // The MD5 functions are derived from the RSA Data Security, Inc. MD5 Message-Digest
        // Algorithm.
        std::string md5Hash(const std::string& data);
        void md5Update(const unsigned char* buf,
                       unsigned int length,
                       unsigned int (&state)[4],
                       unsigned int (&count)[2],
                       unsigned char (&buffer)[64]);
        void md5Transform(const unsigned char block[64], unsigned int (&state)[4]);

    } // namespace Math

} // namespace Utils

#endif // ES_CORE_UTILS_MATH_UTIL_H
