//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  MathUtil.cpp
//
//  Math utility functions.
//  The GLM library headers are also included from here.
//

#if defined(_MSC_VER) // MSVC compiler.
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "utils/MathUtil.h"

#include <cstring>
#include <sstream>

namespace Utils
{
    namespace Math
    {
        float smoothStep(const float left, const float right, const float value)
        {
            const float x {glm::clamp((value - left) / (right - left), 0.0f, 1.0f)};
            return x * x * (3.0f - (2.0f * x));
        }

        float smootherStep(const float left, const float right, const float value)
        {
            const float x {glm::clamp((value - left) / (right - left), 0.0f, 1.0f)};
            return x * x * x * (x * ((x * 6.0f) - 15.0f) + 10.0f);
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
                const float fraction {(currentTime - delayTime) / scrollTime};
                return glm::mix(0.0f, scrollLength, fraction);
            }

            // And back to waiting.
            return 0.0f;
        }

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
                const float fraction {(currentTime - delayTime) / scrollTime};
                return glm::mix(0.0f, scrollLength, smootherStep(0.0f, 1.0f, fraction));
            }
            else if (currentTime < (delayTime + scrollTime + delayTime)) {
                // Wait some more.
                return scrollLength;
            }
            else if (currentTime < (delayTime + scrollTime + delayTime + scrollTime)) {
                // Interpolate back from scrollLength to 0.
                const float fraction {(currentTime - delayTime - scrollTime - delayTime) /
                                      scrollTime};
                return glm::mix(scrollLength, 0.0f, smootherStep(0.0f, 1.0f, fraction));
            }
            // And back to waiting.
            return 0.0f;
        }

        std::string md5Hash(const std::string& data)
        {
            // Data that didn't fit in last 64 byte chunk.
            unsigned char buffer[64] {};
            // 64 bit counter for the number of bits (low, high).
            unsigned int count[2] {};

            // Digest so far.
            unsigned int state[4];

            // Magic initialization constants.
            state[0] = 0x67452301;
            state[1] = 0xefcdab89;
            state[2] = 0x98badcfe;
            state[3] = 0x10325476;

            md5Update(reinterpret_cast<const unsigned char*>(data.c_str()),
                      static_cast<unsigned int>(data.length()), state, count, buffer);

            static unsigned char padding[64] {0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                              0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                              0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                              0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

            // Encodes unsigned int input into unsigned char output. Assumes len is a multiple of 4.
            auto encodeFunc = [](unsigned char output[], const unsigned int input[],
                                 unsigned int len) {
                for (unsigned int i = 0, j = 0; j < len; ++i, j += 4) {
                    output[j] = input[i] & 0xff;
                    output[j + 1] = (input[i] >> 8) & 0xff;
                    output[j + 2] = (input[i] >> 16) & 0xff;
                    output[j + 3] = (input[i] >> 24) & 0xff;
                }
            };

            // Save number of bits.
            unsigned char bits[8];
            encodeFunc(bits, count, 8);

            // Pad out to 56 mod 64.
            unsigned int index = count[0] / 8 % 64;
            unsigned int padLen = (index < 56) ? (56 - index) : (120 - index);
            md5Update(padding, padLen, state, count, buffer);

            // Append length (before padding).
            md5Update(bits, 8, state, count, buffer);

            // The result.
            unsigned char digest[16];

            // Store state in digest.
            encodeFunc(digest, state, 16);

            // Convert to hex string.
            char buf[33];
            for (int i = 0; i < 16; ++i)
                snprintf(buf + i * 2, 16, "%02x", digest[i]);
            buf[32] = 0;

            return std::string(buf);
        }

        void md5Update(const unsigned char input[],
                       unsigned int length,
                       unsigned int (&state)[4],
                       unsigned int (&count)[2],
                       unsigned char (&buffer)[64])
        {
            // Compute number of bytes (mod 64).
            unsigned int index = count[0] / 8 % 64;

            // Update number of bits.
            if ((count[0] += (length << 3)) < (length << 3))
                ++count[1];
            count[1] += (length >> 29);

            // Number of bytes we need to fill in buffer.
            unsigned int firstpart = 64 - index;

            unsigned int i;
            // Encodes unsigned int input into unsigned char output. Assumes len is a multiple of 4.
            // Transform as many times as possible.
            if (length >= firstpart) {
                // Fill buffer first, then transform.
                memcpy(&buffer[index], input, firstpart);
                md5Transform(buffer, state);

                // Transform chunks of 64 (64 bytes).
                for (i = firstpart; i + 64 <= length; i += 64)
                    md5Transform(&input[i], state);

                index = 0;
            }
            else
                i = 0;

            // Buffer remaining input.
            memcpy(&buffer[index], &input[i], length - i);
        }

        void md5Transform(const unsigned char block[64], unsigned int (&state)[4])
        {
            unsigned int a {state[0]};
            unsigned int b {state[1]};
            unsigned int c {state[2]};
            unsigned int d {state[3]};
            unsigned int x[16] {};

            // Encodes unsigned int input into unsigned char output. Assumes len is a multiple of 4.
            for (unsigned int i = 0, j = 0; j < 64; ++i, j += 4)
                x[i] = (static_cast<unsigned int>(block[j])) |
                       ((static_cast<unsigned int>(block[j + 1])) << 8) |
                       ((static_cast<unsigned int>(block[j + 2])) << 16) |
                       ((static_cast<unsigned int>(block[j + 3])) << 24);

            const unsigned int S11 {7};
            const unsigned int S12 {12};
            const unsigned int S13 {17};
            const unsigned int S14 {22};
            const unsigned int S21 {5};
            const unsigned int S22 {9};
            const unsigned int S23 {14};
            const unsigned int S24 {20};
            const unsigned int S31 {4};
            const unsigned int S32 {11};
            const unsigned int S33 {16};
            const unsigned int S34 {23};
            const unsigned int S41 {6};
            const unsigned int S42 {10};
            const unsigned int S43 {15};
            const unsigned int S44 {21};

            // fFunc, gFunc, hFunc and iFunc are basic MD5 functions.
            auto fFunc = [](unsigned int x, unsigned int y, unsigned int z) {
                return (x & y) | (~x & z);
            };
            auto gFunc = [](unsigned int x, unsigned int y, unsigned int z) {
                return (x & z) | (y & ~z);
            };
            auto hFunc = [](unsigned int x, unsigned int y, unsigned int z) { return x ^ y ^ z; };
            auto iFunc = [](unsigned int x, unsigned int y, unsigned int z) {
                return y ^ (x | ~z);
            };

            auto rotateLeftFunc = [](unsigned int x, int n) { return (x << n) | (x >> (32 - n)); };

            // ffFunc, ggFunc, hhFunc, and iiFunc transformations for rounds 1, 2, 3, and 4.
            // Rotation is separate from addition to prevent recomputation.
            auto ffFunc = [fFunc, rotateLeftFunc](unsigned int& a, unsigned int b, unsigned int c,
                                                  unsigned int d, unsigned int x, unsigned int s,
                                                  unsigned int ac) {
                a = rotateLeftFunc(a + fFunc(b, c, d) + x + ac, s) + b;
            };

            auto ggFunc = [gFunc, rotateLeftFunc](unsigned int& a, unsigned int b, unsigned int c,
                                                  unsigned int d, unsigned int x, unsigned int s,
                                                  unsigned int ac) {
                a = rotateLeftFunc(a + gFunc(b, c, d) + x + ac, s) + b;
            };

            auto hhFunc = [hFunc, rotateLeftFunc](unsigned int& a, unsigned int b, unsigned int c,
                                                  unsigned int d, unsigned int x, unsigned int s,
                                                  unsigned int ac) {
                a = rotateLeftFunc(a + hFunc(b, c, d) + x + ac, s) + b;
            };

            auto iiFunc = [iFunc, rotateLeftFunc](unsigned int& a, unsigned int b, unsigned int c,
                                                  unsigned int d, unsigned int x, unsigned int s,
                                                  unsigned int ac) {
                a = rotateLeftFunc(a + iFunc(b, c, d) + x + ac, s) + b;
            };

            // Round 1.
            ffFunc(a, b, c, d, x[0], S11, 0xd76aa478); // 1
            ffFunc(d, a, b, c, x[1], S12, 0xe8c7b756); // 2
            ffFunc(c, d, a, b, x[2], S13, 0x242070db); // 3
            ffFunc(b, c, d, a, x[3], S14, 0xc1bdceee); // 4
            ffFunc(a, b, c, d, x[4], S11, 0xf57c0faf); // 5
            ffFunc(d, a, b, c, x[5], S12, 0x4787c62a); // 6
            ffFunc(c, d, a, b, x[6], S13, 0xa8304613); // 7
            ffFunc(b, c, d, a, x[7], S14, 0xfd469501); // 8
            ffFunc(a, b, c, d, x[8], S11, 0x698098d8); // 9
            ffFunc(d, a, b, c, x[9], S12, 0x8b44f7af); // 10
            ffFunc(c, d, a, b, x[10], S13, 0xffff5bb1); // 11
            ffFunc(b, c, d, a, x[11], S14, 0x895cd7be); // 12
            ffFunc(a, b, c, d, x[12], S11, 0x6b901122); // 13
            ffFunc(d, a, b, c, x[13], S12, 0xfd987193); // 14
            ffFunc(c, d, a, b, x[14], S13, 0xa679438e); // 15
            ffFunc(b, c, d, a, x[15], S14, 0x49b40821); // 16

            // Round 2.
            ggFunc(a, b, c, d, x[1], S21, 0xf61e2562); // 17
            ggFunc(d, a, b, c, x[6], S22, 0xc040b340); // 18
            ggFunc(c, d, a, b, x[11], S23, 0x265e5a51); // 19
            ggFunc(b, c, d, a, x[0], S24, 0xe9b6c7aa); // 20
            ggFunc(a, b, c, d, x[5], S21, 0xd62f105d); // 21
            ggFunc(d, a, b, c, x[10], S22, 0x2441453); // 22
            ggFunc(c, d, a, b, x[15], S23, 0xd8a1e681); // 23
            ggFunc(b, c, d, a, x[4], S24, 0xe7d3fbc8); // 24
            ggFunc(a, b, c, d, x[9], S21, 0x21e1cde6); // 25
            ggFunc(d, a, b, c, x[14], S22, 0xc33707d6); // 26
            ggFunc(c, d, a, b, x[3], S23, 0xf4d50d87); // 27
            ggFunc(b, c, d, a, x[8], S24, 0x455a14ed); // 28
            ggFunc(a, b, c, d, x[13], S21, 0xa9e3e905); // 29
            ggFunc(d, a, b, c, x[2], S22, 0xfcefa3f8); // 30
            ggFunc(c, d, a, b, x[7], S23, 0x676f02d9); // 31
            ggFunc(b, c, d, a, x[12], S24, 0x8d2a4c8a); // 32

            // Round 3.
            hhFunc(a, b, c, d, x[5], S31, 0xfffa3942); // 33
            hhFunc(d, a, b, c, x[8], S32, 0x8771f681); // 34
            hhFunc(c, d, a, b, x[11], S33, 0x6d9d6122); // 35
            hhFunc(b, c, d, a, x[14], S34, 0xfde5380c); // 36
            hhFunc(a, b, c, d, x[1], S31, 0xa4beea44); // 37
            hhFunc(d, a, b, c, x[4], S32, 0x4bdecfa9); // 38
            hhFunc(c, d, a, b, x[7], S33, 0xf6bb4b60); // 39
            hhFunc(b, c, d, a, x[10], S34, 0xbebfbc70); // 40
            hhFunc(a, b, c, d, x[13], S31, 0x289b7ec6); // 41
            hhFunc(d, a, b, c, x[0], S32, 0xeaa127fa); // 42
            hhFunc(c, d, a, b, x[3], S33, 0xd4ef3085); // 43
            hhFunc(b, c, d, a, x[6], S34, 0x4881d05); // 44
            hhFunc(a, b, c, d, x[9], S31, 0xd9d4d039); // 45
            hhFunc(d, a, b, c, x[12], S32, 0xe6db99e5); // 46
            hhFunc(c, d, a, b, x[15], S33, 0x1fa27cf8); // 47
            hhFunc(b, c, d, a, x[2], S34, 0xc4ac5665); // 48

            // Round 4.
            iiFunc(a, b, c, d, x[0], S41, 0xf4292244); // 49
            iiFunc(d, a, b, c, x[7], S42, 0x432aff97); // 50
            iiFunc(c, d, a, b, x[14], S43, 0xab9423a7); // 51
            iiFunc(b, c, d, a, x[5], S44, 0xfc93a039); // 52
            iiFunc(a, b, c, d, x[12], S41, 0x655b59c3); // 53
            iiFunc(d, a, b, c, x[3], S42, 0x8f0ccc92); // 54
            iiFunc(c, d, a, b, x[10], S43, 0xffeff47d); // 55
            iiFunc(b, c, d, a, x[1], S44, 0x85845dd1); // 56
            iiFunc(a, b, c, d, x[8], S41, 0x6fa87e4f); // 57
            iiFunc(d, a, b, c, x[15], S42, 0xfe2ce6e0); // 58
            iiFunc(c, d, a, b, x[6], S43, 0xa3014314); // 59
            iiFunc(b, c, d, a, x[13], S44, 0x4e0811a1); // 60
            iiFunc(a, b, c, d, x[4], S41, 0xf7537e82); // 61
            iiFunc(d, a, b, c, x[11], S42, 0xbd3af235); // 62
            iiFunc(c, d, a, b, x[2], S43, 0x2ad7d2bb); // 63
            iiFunc(b, c, d, a, x[9], S44, 0xeb86d391); // 64

            state[0] += a;
            state[1] += b;
            state[2] += c;
            state[3] += d;
        }

    } // namespace Math

} // namespace Utils
