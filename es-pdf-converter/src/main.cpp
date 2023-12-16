//  SPDX-License-Identifier: GPL-2.0-only
//
//  ES-DE PDF converter
//  main.cpp
//
//  Converts PDF document pages to raw ARGB32 pixel data for maximum performance.
//  This needs to be separated into its own binary to get around the restrictive GPL
//  license used by the Poppler PDF rendering library.
//
//  The column limit is 100 characters.
//  All ES-DE C++ source code is formatted using clang-format.
//

#include "ConvertPDF.h"

#include <iostream>

#if defined(_WIN64)
#include <fcntl.h>
#include <io.h>
#include <windows.h>

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    wchar_t** argv {__wargv};
    int argc {__argc};

    HANDLE stdoutHandle {GetStdHandle(STD_OUTPUT_HANDLE)};

    if (stdoutHandle == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Invalid stdout handle" << std::endl;
        exit(-1);
    }

    // This is required as Windows is braindead and will otherwise add carriage return characters
    // to the stream when it encounters newline characters, which breaks binary output.
    _setmode(_fileno(stdout), O_BINARY);

    bool validArguments {true};
    std::wstring mode;

    if (argc < 3)
        validArguments = false;
    else
        mode = argv[1];

    if ((mode == L"-fileinfo" && argc != 3) || (mode == L"-convert" && argc != 6))
        validArguments = false;

    if (!validArguments) {
        std::cout << "This binary is only intended to be executed by ES-DE.exe"
                  << std::endl;
        exit(-1);
    }

    const std::wstring path {argv[2]};

    int pageNum {0};
    int width {0};
    int height {0};

    if (mode == L"-convert") {
        pageNum = _wtoi(argv[3]);
        width = _wtoi(argv[4]);
        height = _wtoi(argv[5]);
#else
int main(int argc, char* argv[])
{
    bool validArguments {true};
    std::string mode;

    if (argc < 3)
        validArguments = false;
    else
        mode = argv[1];

    if ((mode == "-fileinfo" && argc != 3) || (mode == "-convert" && argc != 6))
        validArguments = false;

    if (!validArguments) {
#if defined(__APPLE__)
        std::cout << "This binary is only intended to be executed by ES-DE"
#else
        std::cout << "This binary is only intended to be executed by es-de"
#endif
                  << std::endl;
        exit(-1);
    }

    const std::string path {argv[2]};
    int pageNum {0};
    int width {0};
    int height {0};

    if (mode == "-convert") {
        pageNum = atoi(argv[3]);
        width = atoi(argv[4]);
        height = atoi(argv[5]);
#endif // _WIN64
        if (width < 1 || width > 7680) {
            std::cerr << "Invalid horizontal resolution defined: " << argv[3] << std::endl;
            exit(-1);
        }

        if (height < 1 || height > 7680) {
            std::cerr << "Invalid vertical resolution defined: " << argv[4] << std::endl;
            exit(-1);
        }

        // std::cerr << "Converting file \"" << path << "\", page " << pageNum << " to resolution "
        //          << width << "x" << height << " pixels" << std::endl;
    }

    return ConvertPDF::processFile(path, mode, pageNum, width, height);
}
