//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  PDFViewer.cpp
//
//  Parses and renders pages using the Poppler library via the external es-pdf-convert binary.
//

#include "PDFViewer.h"

#include "Log.h"
#include "Sound.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "views/ViewController.h"

#include <array>

#if defined(_WIN64)
#include <windows.h>
#endif

#define DEBUG_PDF_CONVERSION false

PDFViewer::PDFViewer()
    : mRenderer {Renderer::getInstance()}
    , mGame {nullptr}
{
    Window::getInstance()->setPDFViewer(this);
    mTexture = TextureResource::get("");
}

bool PDFViewer::startPDFViewer(FileData* game)
{
    ViewController::getInstance()->pauseViewVideos();

#if defined(_WIN64)
    const std::string convertBinary {"/es-pdf-converter/es-pdf-convert.exe"};
#else
    const std::string convertBinary {"/es-pdf-convert"};
#endif
    mESConvertPath = Utils::FileSystem::getExePath() + convertBinary;
    if (!Utils::FileSystem::exists(mESConvertPath)) {
#if defined(_WIN64)
        LOG(LogError) << "Couldn't find PDF conversion binary es-pdf-convert.exe";
#else
        LOG(LogError) << "Couldn't find PDF conversion binary es-pdf-convert";
#endif
        NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
        ViewController::getInstance()->stopViewVideos();
        return false;
    }

    mGame = game;
    mManualPath = mGame->getManualPath();

    if (!Utils::FileSystem::exists(mManualPath)) {
        LOG(LogError) << "No PDF manual found for game \"" << mGame->getName() << "\"";
        NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
        ViewController::getInstance()->stopViewVideos();
        return false;
    }

#if defined(_WIN64)
    mManualPath = Utils::String::replace(mManualPath, "/", "\\");
#endif

    LOG(LogDebug) << "PDFViewer::startPDFViewer(): Opening document \"" << mManualPath << "\"";

    mPages.clear();
    mPageImage.reset();
    mPageCount = 0;
    mCurrentPage = 0;
    mScaleFactor = 1.0f;

    if (!getDocumentInfo()) {
        LOG(LogError) << "PDFViewer: Couldn't load file \"" << mManualPath << "\"";
        NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
        ViewController::getInstance()->stopViewVideos();
        return false;
    }

    mPageCount = static_cast<int>(mPages.size());

    for (int i {1}; i <= mPageCount; ++i) {
        if (mPages.find(i) == mPages.end()) {
            LOG(LogError) << "Couldn't read information for page " << i << ", invalid PDF file?";
            NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
            ViewController::getInstance()->stopViewVideos();
            return false;
        }

        float width {mPages[i].width};
        float height {mPages[i].height};

        if (!mPages[i].portraitOrientation)
            std::swap(width, height);

        // Maintain page aspect ratio.
        glm::vec2 textureSize {glm::vec2 {width, height}};
        const glm::vec2 targetSize {glm::vec2 {mRenderer->getScreenWidth() * mScaleFactor,
                                               mRenderer->getScreenHeight() * mScaleFactor}};
        glm::vec2 resizeScale {targetSize.x / textureSize.x, targetSize.y / textureSize.y};

        if (resizeScale.x < resizeScale.y) {
            textureSize.x *= resizeScale.x;
            textureSize.y = std::min(textureSize.y * resizeScale.x, targetSize.y);
        }
        else {
            textureSize.y *= resizeScale.y;
            textureSize.x = std::min((textureSize.y / height) * width, targetSize.x);
        }

        mPages[i].width = std::round(textureSize.x);
        mPages[i].height = std::round(textureSize.y);

#if (DEBUG_PDF_CONVERSION)
        LOG(LogDebug) << "Page " << i << ": Orientation: "
                      << (mPages[i].portraitOrientation ? "portrait" : "landscape") << " / "
                      << "crop box width: " << width << " / "
                      << "crop box height: " << height << " / "
                      << "size ratio: " << width / height << " / "
                      << "texture size: " << mPages[i].width << "x" << mPages[i].height;
#endif
    }

    mCurrentPage = 1;
    convertPage(mCurrentPage);
    return true;
}

void PDFViewer::stopPDFViewer()
{
    NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
    ViewController::getInstance()->stopViewVideos();

    mPages.clear();
    mPageImage.reset();
}

void PDFViewer::launchMediaViewer()
{
    Window::getInstance()->stopPDFViewer();
    Window::getInstance()->startMediaViewer(mGame);
}

bool PDFViewer::getDocumentInfo()
{
    std::string commandOutput;

#if defined(_WIN64)
    std::wstring command {
        Utils::String::stringToWideString(Utils::FileSystem::getEscapedPath(mESConvertPath))};
    command.append(L" -fileinfo ")
        .append(Utils::String::stringToWideString(Utils::FileSystem::getEscapedPath(mManualPath)));

    STARTUPINFOW si {};
    PROCESS_INFORMATION pi;
    HANDLE childStdoutRead {nullptr};
    HANDLE childStdoutWrite {nullptr};
    SECURITY_ATTRIBUTES saAttr {};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = true;
    saAttr.lpSecurityDescriptor = nullptr;

    CreatePipe(&childStdoutRead, &childStdoutWrite, &saAttr, 0);
    SetHandleInformation(childStdoutRead, HANDLE_FLAG_INHERIT, 0);

    si.cb = sizeof(STARTUPINFOW);
    si.hStdOutput = childStdoutWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;

    bool processReturnValue {true};

    // clang-format off
    processReturnValue = CreateProcessW(
        nullptr,                                // No application name (use command line).
        const_cast<wchar_t*>(command.c_str()),  // Command line.
        nullptr,                                // Process attributes.
        nullptr,                                // Thread attributes.
        TRUE,                                   // Handles inheritance.
        0,                                      // Creation flags.
        nullptr,                                // Use parent's environment block.
        nullptr,                                // Starting directory, possibly the same as parent.
        &si,                                    // Pointer to the STARTUPINFOW structure.
        &pi);                                   // Pointer to the PROCESS_INFORMATION structure.
    // clang-format on

    if (!processReturnValue) {
        LOG(LogError) << "Couldn't read PDF document information";
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return false;
    }

    // Close process and thread handles.
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(childStdoutWrite);

    std::array<char, 512> buffer {};
    DWORD dwRead;
    bool readValue {true};

    while (readValue) {
        readValue = ReadFile(childStdoutRead, &buffer[0], 512, &dwRead, nullptr);
        if (readValue) {
            for (int i {0}; i < 512; ++i) {
                if (buffer[i] == '\0')
                    break;
                commandOutput.append(1, buffer[i]);
            }
            buffer.fill('\0');
        }
    }

    CloseHandle(childStdoutRead);
    WaitForSingleObject(pi.hThread, INFINITE);
    WaitForSingleObject(pi.hProcess, INFINITE);
#else
    FILE* commandPipe;
    std::array<char, 512> buffer {};

    std::string command {Utils::FileSystem::getEscapedPath(mESConvertPath)};
    command.append(" -fileinfo ").append(Utils::FileSystem::getEscapedPath(mManualPath));

    if (!(commandPipe = reinterpret_cast<FILE*>(popen(command.c_str(), "r")))) {
        LOG(LogError) << "Couldn't open pipe to es-pdf-convert";
        return false;
    }

    while (fread(buffer.data(), 1, 512, commandPipe)) {
        for (int i {0}; i < 512; ++i) {
            if (buffer[i] == '\0')
                break;
            commandOutput.append(1, buffer[i]);
        }
        buffer.fill('\0');
    }

    if (pclose(commandPipe) != 0)
        return false;
#endif

    const std::vector<std::string> pageRows {
        Utils::String::delimitedStringToVector(commandOutput, "\n")};

    for (auto& row : pageRows) {
        const std::vector<std::string> rowValues {Utils::String::delimitedStringToVector(row, ";")};
        if (rowValues.size() != 4)
            continue;
        mPages[atoi(&rowValues[0][0])] = PageEntry {static_cast<float>(atof(&rowValues[2][0])),
                                                    static_cast<float>(atof(&rowValues[3][0])),
                                                    (rowValues[1] == "portrait" ? true : false),
                                                    {}};
    }

    return true;
}

void PDFViewer::convertPage(int pageNum)
{
    assert(pageNum <= static_cast<int>(mPages.size()));

#if defined(_WIN64)
    std::wstring command {
        Utils::String::stringToWideString(Utils::FileSystem::getEscapedPath(mESConvertPath))};
    command.append(L" -convert ")
        .append(Utils::String::stringToWideString(Utils::FileSystem::getEscapedPath(mManualPath)))
        .append(L" ")
        .append(std::to_wstring(pageNum))
        .append(L" ")
        .append(std::to_wstring(static_cast<int>(mPages[pageNum].width)))
        .append(L" ")
        .append(std::to_wstring(static_cast<int>(mPages[pageNum].height)));
#else
    std::string command {Utils::FileSystem::getEscapedPath(mESConvertPath)};
    command.append(" -convert ")
        .append(Utils::FileSystem::getEscapedPath(mManualPath))
        .append(" ")
        .append(std::to_string(pageNum))
        .append(" ")
        .append(std::to_string(static_cast<int>(mPages[pageNum].width)))
        .append(" ")
        .append(std::to_string(static_cast<int>(mPages[pageNum].height)));
#endif

    if (mPages[pageNum].imageData.empty()) {
#if (DEBUG_PDF_CONVERSION)
        LOG(LogDebug) << "Converting page: " << mCurrentPage;
#if defined(_WIN64)
        LOG(LogDebug) << Utils::String::wideStringToString(command);
#else
        LOG(LogDebug) << command;
#endif
#endif
        std::string imageData;
#if defined(_WIN64)
        STARTUPINFOW si {};
        PROCESS_INFORMATION pi;
        HANDLE childStdoutRead {nullptr};
        HANDLE childStdoutWrite {nullptr};
        SECURITY_ATTRIBUTES saAttr {};
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = true;
        saAttr.lpSecurityDescriptor = nullptr;

        CreatePipe(&childStdoutRead, &childStdoutWrite, &saAttr, 0);
        SetHandleInformation(childStdoutRead, HANDLE_FLAG_INHERIT, 0);

        si.cb = sizeof(STARTUPINFOW);
        si.hStdOutput = childStdoutWrite;
        si.dwFlags |= STARTF_USESTDHANDLES;

        bool processReturnValue {true};

        // clang-format off
        processReturnValue = CreateProcessW(
            nullptr,                            // No application name (use command line).
            const_cast<wchar_t*>(command.c_str()), // Command line.
            nullptr,                            // Process attributes.
            nullptr,                            // Thread attributes.
            TRUE,                               // Handles inheritance.
            0,                                  // Creation flags.
            nullptr,                            // Use parent's environment block.
            nullptr,                            // Starting directory, possibly the same as parent.
            &si,                                // Pointer to the STARTUPINFOW structure.
            &pi);                               // Pointer to the PROCESS_INFORMATION structure.
        // clang-format on

        if (!processReturnValue) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return;
        }

        // Close process and thread handles.
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(childStdoutWrite);

        std::array<char, 512> buffer {};
        DWORD dwRead;
        bool readValue {true};

        while (readValue) {
            readValue = ReadFile(childStdoutRead, &buffer[0], 512, &dwRead, nullptr);
            if (readValue) {
                mPages[pageNum].imageData.insert(mPages[pageNum].imageData.end(),
                                                 std::make_move_iterator(buffer.begin()),
                                                 std::make_move_iterator(buffer.end()));
            }
        }

        CloseHandle(childStdoutRead);
        WaitForSingleObject(pi.hThread, INFINITE);
        WaitForSingleObject(pi.hProcess, INFINITE);
#else
        FILE* commandPipe;
        std::array<char, 512> buffer {};
        int returnValue;

        if (!(commandPipe = reinterpret_cast<FILE*>(popen(command.c_str(), "r")))) {
            LOG(LogError) << "Couldn't open pipe to es-pdf-convert";
            return;
        }

        while (fread(buffer.data(), 1, 512, commandPipe)) {
            mPages[pageNum].imageData.insert(mPages[pageNum].imageData.end(),
                                             std::make_move_iterator(buffer.begin()),
                                             std::make_move_iterator(buffer.end()));
        }

        returnValue = pclose(commandPipe);
#endif
        const size_t imageDataSize {mPages[pageNum].imageData.size()};
#if defined(_WIN64)
        if (!processReturnValue || (static_cast<int>(imageDataSize) <
                                    mPages[pageNum].width * mPages[pageNum].height * 4)) {
#else
        if (returnValue != 0 || (static_cast<int>(imageDataSize) <
                                 mPages[pageNum].width * mPages[pageNum].height * 4)) {
#endif
            LOG(LogError) << "Error reading PDF file";
            mPages[pageNum].imageData.clear();
            return;
        }
    }
    else {
#if (DEBUG_PDF_CONVERSION)
        LOG(LogDebug) << "Using cached texture for page: " << mCurrentPage;
#endif
    }

    mPageImage.reset();
    mPageImage = std::make_unique<ImageComponent>(false, false);
    mPageImage->setOrigin(0.5f, 0.5f);
    mPageImage->setPosition(mRenderer->getScreenWidth() / 2.0f,
                            mRenderer->getScreenHeight() / 2.0f);

    mPageImage->setFlipY(true);
    mPageImage->setMaxSize(
        glm::vec2 {mPages[pageNum].width / mScaleFactor, mPages[pageNum].height / mScaleFactor});
    mPageImage->setRawImage(reinterpret_cast<const unsigned char*>(&mPages[pageNum].imageData[0]),
                            static_cast<size_t>(mPages[pageNum].width),
                            static_cast<size_t>(mPages[pageNum].height));

#if (DEBUG_PDF_CONVERSION)
    LOG(LogDebug) << "ABGR32 data stream size: " << mPages[pageNum].imageData.size();
#endif
}

void PDFViewer::render(const glm::mat4& /*parentTrans*/)
{
    glm::mat4 trans {Renderer::getIdentity()};
    mRenderer->setMatrix(trans);

    // Render a black background below the document.
    mRenderer->drawRect(0.0f, 0.0f, Renderer::getScreenWidth(), Renderer::getScreenHeight(),
                        0x000000FF, 0x000000FF);

    if (mPageImage != nullptr) {
        mPageImage->render(trans);
    }
}

void PDFViewer::showNextPage()
{
    if (mCurrentPage == mPageCount)
        return;

    NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
    ++mCurrentPage;
    convertPage(mCurrentPage);
}

void PDFViewer::showPreviousPage()
{
    if (mCurrentPage == 1)
        return;

    NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
    --mCurrentPage;
    convertPage(mCurrentPage);
}

void PDFViewer::showFirstPage()
{
    if (mCurrentPage == 1)
        return;

    NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
    mCurrentPage = 1;
    convertPage(mCurrentPage);
}

void PDFViewer::showLastPage()
{
    if (mCurrentPage == mPageCount)
        return;

    NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
    mCurrentPage = mPageCount;
    convertPage(mCurrentPage);
}
