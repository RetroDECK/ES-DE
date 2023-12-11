//  SPDX-License-Identifier: GPL-2.0-only
//
//  ES-DE
//  ConvertPDF.h
//
//  Converts PDF document pages to raw ARGB32 pixel data for maximum performance.
//  This needs to be separated into its own binary to get around the restrictive GPL
//  license used by the Poppler PDF rendering library.
//

#include <string>

#ifndef ES_PDF_CONVERTER_CONVERT_PDF_H
#define ES_PDF_CONVERTER_CONVERT_PDF_H

class ConvertPDF
{
public:
#if defined(_WIN64)
    static int processFile(
        const std::wstring path, const std::wstring mode, int pageNum, int width, int height);
#elif defined(__ANDROID__)
    __attribute__((visibility("default"))) static int processFile(const std::string path,
                                                                  const std::string mode,
                                                                  int pageNum,
                                                                  int width,
                                                                  int height,
                                                                  std::string& result);
#else
    static int processFile(
        const std::string path, const std::string mode, int pageNum, int width, int height);
#endif
};

#endif // ES_PDF_CONVERTER_CONVERT_PDF_H
