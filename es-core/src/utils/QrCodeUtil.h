//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  QrCodeUtil.h
//
//  Rasterizes a QR code into an RGBA pixel buffer via the vendored qrcodegen library.
//

#ifndef ES_CORE_UTILS_QR_CODE_UTIL_H
#define ES_CORE_UTILS_QR_CODE_UTIL_H

#include <cstddef>
#include <string>
#include <vector>

namespace Utils
{
    namespace QrCode
    {
        // moduleScale is the pixel size of one QR module. Returns false (outRgba/outSize
        // unmodified) if text is too long to encode.
        bool encodeToRgba(const std::string& text,
                          size_t moduleScale,
                          std::vector<unsigned char>& outRgba,
                          size_t& outSize);
    } // namespace QrCode
} // namespace Utils

#endif // ES_CORE_UTILS_QR_CODE_UTIL_H
