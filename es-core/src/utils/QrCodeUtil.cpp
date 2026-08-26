//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  QrCodeUtil.cpp
//

#include "utils/QrCodeUtil.h"

#include "qrcodegen.h"

#include <cstdint>

namespace Utils
{
    namespace QrCode
    {
        namespace
        {
            constexpr int quietZoneModules {4}; // ISO/IEC 18004 standard quiet zone width.
        } // namespace

        bool encodeToRgba(const std::string& text,
                          size_t moduleScale,
                          std::vector<unsigned char>& outRgba,
                          size_t& outSize)
        {
            std::vector<uint8_t> tempBuffer(qrcodegen_BUFFER_LEN_MAX);
            std::vector<uint8_t> qrcode(qrcodegen_BUFFER_LEN_MAX);

            const bool encodeOk {qrcodegen_encodeText(
                text.c_str(), tempBuffer.data(), qrcode.data(), qrcodegen_Ecc_MEDIUM,
                qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true)};
            if (!encodeOk)
                return false;

            const int qrSize {qrcodegen_getSize(qrcode.data())};
            const size_t scale {moduleScale == 0 ? 1 : moduleScale};
            const size_t imageModules {static_cast<size_t>(qrSize) +
                                       static_cast<size_t>(quietZoneModules) * 2};
            const size_t imageSize {imageModules * scale};

            outRgba.assign(imageSize * imageSize * 4, 0xFF);

            for (size_t py {0}; py < imageSize; ++py) {
                const int moduleY {static_cast<int>(py / scale) - quietZoneModules};
                for (size_t px {0}; px < imageSize; ++px) {
                    const int moduleX {static_cast<int>(px / scale) - quietZoneModules};
                    const bool dark {moduleX >= 0 && moduleY >= 0 && moduleX < qrSize &&
                                     moduleY < qrSize &&
                                     qrcodegen_getModule(qrcode.data(), moduleX, moduleY)};
                    if (dark) {
                        const size_t offset {(py * imageSize + px) * 4};
                        outRgba[offset] = 0;
                        outRgba[offset + 1] = 0;
                        outRgba[offset + 2] = 0;
                    }
                }
            }

            outSize = imageSize;
            return true;
        }
    } // namespace QrCode
} // namespace Utils
