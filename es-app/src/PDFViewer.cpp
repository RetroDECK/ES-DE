//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  PDFViewer.cpp
//
//  Parses and renders pages using the Poppler library via the external es-pdf-convert binary.
//

#include "PDFViewer.h"

#include "Log.h"
#include "Sound.h"
#include "utils/FileSystemUtil.h"
#include "utils/LocalizationUtil.h"
#include "utils/StringUtil.h"
#include "views/ViewController.h"

#include <array>

#if defined(_WIN64)
#include <windows.h>
#endif

#if defined(__ANDROID__)
#include "ConvertPDF.h"
#endif

#define DEBUG_PDF_CONVERSION false

#define KEY_REPEAT_START_DELAY 600
#define KEY_REPEAT_START_DELAY_ZOOMED 500
#define KEY_REPEAT_SPEED 250
#define KEY_REPEAT_SPEED_ZOOMED 150

PDFViewer::PDFViewer()
    : mRenderer {Renderer::getInstance()}
    , mGame {nullptr}
    , mFrameHeight {0.0f}
    , mScaleFactor {1.0f}
    , mCurrentPage {0}
    , mPageCount {0}
    , mZoom {1.0f}
    , mPanAmount {0.0f}
    , mPanOffset {0.0f, 0.0f, 0.0f}
    , mConversionTime {0}
    , mKeyRepeatLeftRight {0}
    , mKeyRepeatUpDown {0}
    , mKeyRepeatZoom {0}
    , mKeyRepeatTimer {0}
    , mHelpInfoPosition {HelpInfoPosition::TOP}
{
    Window::getInstance()->setPDFViewer(this);
}

bool PDFViewer::startPDFViewer(FileData* game)
{
    ViewController::getInstance()->pauseViewVideos();

#if !defined(__ANDROID__)
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
        ViewController::getInstance()->startViewVideos();
        return false;
    }
#endif // !__ANDROID__

    mGame = game;
    mManualPath = mGame->getManualPath();

    if (!Utils::FileSystem::exists(mManualPath)) {
        LOG(LogError) << "No PDF manual found for game \"" << mGame->getName() << "\"";
        NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
        ViewController::getInstance()->startViewVideos();
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
    mZoom = 1.0f;
    mPanAmount = 0.0f;
    mPanOffset = {0.0f, 0.0f, 0.0f};
    mConversionTime = 0;
    mKeyRepeatLeftRight = 0;
    mKeyRepeatUpDown = 0;
    mKeyRepeatZoom = 0;
    mKeyRepeatTimer = 0;

    // Increase the rasterization resolution when running at lower screen resolutions to make
    // the texture look ok when zoomed in.
    const float resolutionModifier {mRenderer->getScreenResolutionModifier()};
    if (resolutionModifier < 1.0f)
        mScaleFactor = 1.8f;
    else if (resolutionModifier < 1.2f)
        mScaleFactor = 1.3f;
    else if (resolutionModifier < 1.4f)
        mScaleFactor = 1.15f;

    if (!getDocumentInfo()) {
        LOG(LogError) << "PDFViewer: Couldn't load file \"" << mManualPath << "\"";
        ViewController::getInstance()->startViewVideos();
        return false;
    }

    mPageCount = static_cast<int>(mPages.size());

    for (int i {1}; i <= mPageCount; ++i) {
        if (mPages.find(i) == mPages.end()) {
            LOG(LogError) << "Couldn't read information for page " << i << ", invalid PDF file?";
            ViewController::getInstance()->startViewVideos();
            return false;
        }

        float width {mPages[i].width};
        float height {mPages[i].height};

        if (mPages[i].orientation != "portrait" && mPages[i].orientation != "upside_down")
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
        LOG(LogDebug) << "Page " << i << ": Orientation: " << mPages[i].orientation << " / "
                      << "crop box width: " << width << " / " << "crop box height: " << height
                      << " / " << "size ratio: " << width / height << " / "
                      << "texture size: " << mPages[i].width << "x" << mPages[i].height;
#endif
    }

    mCurrentPage = 1;

    if (Settings::getInstance()->getString("MediaViewerHelpPrompts") == "disabled")
        mHelpInfoPosition = HelpInfoPosition::DISABLED;
    else if (Settings::getInstance()->getString("MediaViewerHelpPrompts") == "bottom")
        mHelpInfoPosition = HelpInfoPosition::BOTTOM;
    else
        mHelpInfoPosition = HelpInfoPosition::TOP;

    if (mHelpInfoPosition == HelpInfoPosition::DISABLED)
        mFrameHeight = 0.0f;
    else
        mFrameHeight = Font::get(FONT_SIZE_MINI)->getLetterHeight() * 1.9f;

    HelpStyle style;
    style.font = Font::get(FONT_SIZE_MINI);
    style.origin = {0.5f, 0.5f};
    style.iconColor = 0xAAAAAAFF;
    style.textColor = 0xAAAAAAFF;

    mEntryCount = std::to_string(mPages.size());

    mEntryNumText = std::make_unique<TextComponent>(
        Utils::String::format(_("PAGE %s OF %s"), "1", mEntryCount.c_str()),
        Font::get(FONT_SIZE_MINI, FONT_PATH_REGULAR), 0xAAAAAAFF);
    mEntryNumText->setOrigin(0.0f, 0.5f);

    if (mHelpInfoPosition == HelpInfoPosition::TOP) {
        mEntryNumText->setPosition(mRenderer->getScreenWidth() * 0.01f, mFrameHeight / 2.0f);
        style.position = glm::vec2 {mRenderer->getScreenWidth() / 2.0f, mFrameHeight / 2.0f};
    }
    else if (mHelpInfoPosition == HelpInfoPosition::BOTTOM) {
        mEntryNumText->setPosition(mRenderer->getScreenWidth() * 0.01f,
                                   mRenderer->getScreenHeight() - (mFrameHeight / 2.0f));
        style.position = glm::vec2 {mRenderer->getScreenWidth() / 2.0f,
                                    mRenderer->getScreenHeight() - (mFrameHeight / 2.0f)};
    }

    mHelp = std::make_unique<HelpComponent>();
    mHelp->setStyle(style);
    mHelp->setPrompts(getHelpPrompts());

    convertPage(mCurrentPage);
    return true;
}

void PDFViewer::stopPDFViewer()
{
    NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
    ViewController::getInstance()->startViewVideos();

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

    DWORD exitCode {0};
    if (GetExitCodeProcess(pi.hProcess, &exitCode) && exitCode != 0) {
        LOG(LogError) << "Couldn't read PDF document information";
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return false;
    }

    // Close process and thread handles.
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#elif defined(__ANDROID__)
    if (ConvertPDF::processFile(mManualPath, "-fileinfo", 0, 0, 0, commandOutput) == -1)
        return false;
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
                                                    rowValues[1],
                                                    {}};
    }

    return true;
}

void PDFViewer::convertPage(int pageNum)
{
    assert(pageNum <= static_cast<int>(mPages.size()));
    const auto conversionStartTime {std::chrono::system_clock::now()};
    mConversionTime = 0;

    if (mPages[pageNum].imageData.empty()) {
#if defined(_WIN64)
        std::wstring command {
            Utils::String::stringToWideString(Utils::FileSystem::getEscapedPath(mESConvertPath))};
        command.append(L" -convert ")
            .append(
                Utils::String::stringToWideString(Utils::FileSystem::getEscapedPath(mManualPath)))
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
#elif (__ANDROID__)
        ConvertPDF::processFile(mManualPath, "-convert", pageNum,
                                static_cast<int>(mPages[pageNum].width),
                                static_cast<int>(mPages[pageNum].height), imageData);
        mPages[pageNum].imageData.insert(mPages[pageNum].imageData.end(),
                                         std::make_move_iterator(imageData.begin()),
                                         std::make_move_iterator(imageData.end()));
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
#elif defined(__ANDROID__)
        if (static_cast<int>(imageDataSize) < mPages[pageNum].width * mPages[pageNum].height * 4) {
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
    mPageImage->setFlipY(true);
    mPageImage->setLinearInterpolation(true);
    mPageImage->setOrigin(0.5f, 0.5f);
    if (mHelpInfoPosition == HelpInfoPosition::TOP) {
        mPageImage->setPosition(mRenderer->getScreenWidth() / 2.0f,
                                (mRenderer->getScreenHeight() / 2.0f) + (mFrameHeight / 2.0f));
    }
    else if (mHelpInfoPosition == HelpInfoPosition::BOTTOM) {
        mPageImage->setPosition(Renderer::getScreenWidth() / 2.0f,
                                (Renderer::getScreenHeight() / 2.0f) - (mFrameHeight / 2.0f));
    }
    else {
        mPageImage->setPosition(mRenderer->getScreenWidth() / 2.0f,
                                mRenderer->getScreenHeight() / 2.0f);
    }

    float sizeReduction {0.0f};
    if (mPages[pageNum].height / mScaleFactor > mRenderer->getScreenHeight() - mFrameHeight)
        sizeReduction =
            (mPages[pageNum].height / mScaleFactor) - (mRenderer->getScreenHeight() - mFrameHeight);

    mPageImage->setMaxSize(
        glm::vec2 {(mPages[pageNum].width / mScaleFactor) * mZoom,
                   ((mPages[pageNum].height / mScaleFactor) * mZoom) - sizeReduction});

    mPageImage->setRawImage(reinterpret_cast<const unsigned char*>(&mPages[pageNum].imageData[0]),
                            static_cast<size_t>(mPages[pageNum].width),
                            static_cast<size_t>(mPages[pageNum].height));

    mPanAmount = std::min(mRenderer->getScreenWidth(), mRenderer->getScreenHeight()) * 0.1f;

    mConversionTime = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                           std::chrono::system_clock::now() - conversionStartTime)
                                           .count());
#if (DEBUG_PDF_CONVERSION)
    LOG(LogDebug) << "ABGR32 data stream size: " << mPages[pageNum].imageData.size();
#endif
}

void PDFViewer::input(InputConfig* config, Input input)
{
    if (config->isMappedLike("up", input)) {
        if (input.value) {
            mKeyRepeatUpDown = -1;
            mKeyRepeatLeftRight = 0;
            mKeyRepeatZoom = 0;
            mKeyRepeatTimer =
                -((mZoom > 1.0f ? KEY_REPEAT_START_DELAY_ZOOMED : KEY_REPEAT_START_DELAY) -
                  KEY_REPEAT_SPEED);
            navigateUp();
        }
        else {
            mKeyRepeatUpDown = 0;
        }
    }
    else if (config->isMappedLike("down", input)) {
        if (input.value) {
            mKeyRepeatUpDown = 1;
            mKeyRepeatLeftRight = 0;
            mKeyRepeatZoom = 0;
            mKeyRepeatTimer =
                -((mZoom > 1.0f ? KEY_REPEAT_START_DELAY_ZOOMED : KEY_REPEAT_START_DELAY) -
                  KEY_REPEAT_SPEED);
            navigateDown();
        }
        else {
            mKeyRepeatUpDown = 0;
        }
        return;
    }
    else if (config->isMappedLike("left", input)) {
        if (input.value) {
            mKeyRepeatLeftRight = -1;
            mKeyRepeatUpDown = 0;
            mKeyRepeatZoom = 0;
            mKeyRepeatTimer =
                -((mZoom > 1.0f ? KEY_REPEAT_START_DELAY_ZOOMED : KEY_REPEAT_START_DELAY) -
                  KEY_REPEAT_SPEED);
            navigateLeft();
        }
        else {
            mKeyRepeatLeftRight = 0;
        }
    }
    else if (config->isMappedLike("right", input)) {
        if (input.value) {
            mKeyRepeatLeftRight = 1;
            mKeyRepeatUpDown = 0;
            mKeyRepeatZoom = 0;
            mKeyRepeatTimer =
                -((mZoom > 1.0f ? KEY_REPEAT_START_DELAY_ZOOMED : KEY_REPEAT_START_DELAY) -
                  KEY_REPEAT_SPEED);
            navigateRight();
        }
        else {
            mKeyRepeatLeftRight = 0;
        }
    }
    else if (config->isMappedLike("leftshoulder", input)) {
        if (input.value) {
            mKeyRepeatZoom = -1;
            mKeyRepeatLeftRight = 0;
            mKeyRepeatUpDown = 0;
            mKeyRepeatTimer = -(KEY_REPEAT_START_DELAY_ZOOMED - KEY_REPEAT_SPEED_ZOOMED);
            navigateLeftShoulder();
        }
        else {
            mKeyRepeatZoom = 0;
        }
    }
    else if (config->isMappedLike("rightshoulder", input)) {
        if (input.value) {
            mKeyRepeatZoom = 1;
            mKeyRepeatLeftRight = 0;
            mKeyRepeatUpDown = 0;
            mKeyRepeatTimer = -(KEY_REPEAT_START_DELAY_ZOOMED - KEY_REPEAT_SPEED_ZOOMED);
            navigateRightShoulder();
        }
        else {
            mKeyRepeatZoom = 0;
        }
    }
    else if (config->isMappedLike("lefttrigger", input) && input.value != 0) {
        mKeyRepeatLeftRight = 0;
        mKeyRepeatUpDown = 0;
        mKeyRepeatZoom = 0;
        navigateLeftTrigger();
    }
    else if (config->isMappedLike("righttrigger", input) && input.value != 0) {
        mKeyRepeatLeftRight = 0;
        mKeyRepeatUpDown = 0;
        mKeyRepeatZoom = 0;
        navigateRightTrigger();
    }
    else if (input.value != 0) {
        // Any other input stops the PDF viewer.
        Window::getInstance()->stopPDFViewer();
    }
}

void PDFViewer::update(int deltaTime)
{
    if (mKeyRepeatLeftRight != 0) {
        // Limit the accumulated time if the computer can't keep up.
        mKeyRepeatTimer += (deltaTime < KEY_REPEAT_SPEED ? deltaTime : deltaTime - mConversionTime);
        while (mKeyRepeatTimer >= (mZoom > 1.0f ? KEY_REPEAT_SPEED_ZOOMED : KEY_REPEAT_SPEED)) {
            mKeyRepeatTimer -= (mZoom > 1.0f ? KEY_REPEAT_SPEED_ZOOMED : KEY_REPEAT_SPEED);
            if (mKeyRepeatLeftRight == 1)
                navigateRight();
            else
                navigateLeft();
        }
    }
    if (mKeyRepeatUpDown != 0) {
        mKeyRepeatTimer += deltaTime;
        while (mKeyRepeatTimer >= (mZoom > 1.0f ? KEY_REPEAT_SPEED_ZOOMED : KEY_REPEAT_SPEED)) {
            mKeyRepeatTimer -= (mZoom > 1.0f ? KEY_REPEAT_SPEED_ZOOMED : KEY_REPEAT_SPEED);
            if (mKeyRepeatUpDown == 1)
                navigateDown();
            else
                navigateUp();
        }
    }
    if (mKeyRepeatZoom != 0) {
        mKeyRepeatTimer += deltaTime;
        while (mKeyRepeatTimer >= KEY_REPEAT_SPEED_ZOOMED) {
            mKeyRepeatTimer -= KEY_REPEAT_SPEED_ZOOMED;
            if (mKeyRepeatZoom == 1)
                navigateRightShoulder();
            else
                navigateLeftShoulder();
        }
    }
}

void PDFViewer::render(const glm::mat4& /*parentTrans*/)
{
    glm::mat4 trans {Renderer::getIdentity()};
    mRenderer->setMatrix(trans);

    // Render a black background below the game media.
    mRenderer->drawRect(0.0f, 0.0f, Renderer::getScreenWidth(), Renderer::getScreenHeight(),
                        0x000000FF, 0x000000FF);

    if (mZoom != 1.0f)
        mPageImage->setPosition(mPageImage->getPosition() + (mPanOffset * mZoom));

    if (mPageImage != nullptr)
        mPageImage->render(trans);

    if (mZoom != 1.0f)
        mPageImage->setPosition(mPageImage->getPosition() - (mPanOffset * mZoom));

    if (mHelpInfoPosition != HelpInfoPosition::DISABLED) {
        // Render a dark gray frame behind the help info.
        mRenderer->setMatrix(mRenderer->getIdentity());
        mRenderer->drawRect(0.0f,
                            (mHelpInfoPosition == HelpInfoPosition::TOP ?
                                 0.0f :
                                 Renderer::getScreenHeight() - mFrameHeight),
                            Renderer::getScreenWidth(), mFrameHeight, 0x222222FF, 0x222222FF);
        mHelp->render(trans);
        mEntryNumText->render(trans);
    }
}

std::vector<HelpPrompt> PDFViewer::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    if (mZoom > 1.0f) {
        prompts.push_back(HelpPrompt("up/down/left/right", _("pan")));
        prompts.push_back(HelpPrompt("ltrt", _("reset")));
    }
    else {
        prompts.push_back(HelpPrompt("left/right", _("browse")));
        prompts.push_back(HelpPrompt("down", _("game media")));
        prompts.push_back(HelpPrompt("lt", _("first")));
        prompts.push_back(HelpPrompt("rt", _("last")));
    }

    prompts.push_back(HelpPrompt("lr", _("zoom")));

    return prompts;
}

void PDFViewer::showNextPage()
{
    if (mCurrentPage == mPageCount)
        return;

    NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
    ++mCurrentPage;
    mEntryNumText->setText(Utils::String::format(
        _("PAGE %s OF %s"), std::to_string(mCurrentPage).c_str(), mEntryCount.c_str()));
    convertPage(mCurrentPage);
}

void PDFViewer::showPreviousPage()
{
    if (mCurrentPage == 1)
        return;

    NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
    --mCurrentPage;
    mEntryNumText->setText(Utils::String::format(
        _("PAGE %s OF %s"), std::to_string(mCurrentPage).c_str(), mEntryCount.c_str()));
    convertPage(mCurrentPage);
}

void PDFViewer::navigateUp()
{
    if (mZoom != 1.0f) {
        if (mPanOffset.y * mZoom <= mPageImage->getSize().y / 2.0f)
            mPanOffset.y += mPanAmount;
    }
}

void PDFViewer::navigateDown()
{
    if (mZoom != 1.0f) {
        if (mPanOffset.y * mZoom >= -(mPageImage->getSize().y / 2.0f))
            mPanOffset.y -= mPanAmount;
    }
    else {
        launchMediaViewer();
    }
}

void PDFViewer::navigateLeft()
{
    if (mZoom != 1.0f) {
        if (mPanOffset.x * mZoom <= mPageImage->getSize().x / 2.0f)
            mPanOffset.x += mPanAmount;
    }
    else {
        mPanOffset = {0.0f, 0.0f, 0.0f};
        showPreviousPage();
    }
}

void PDFViewer::navigateRight()
{
    if (mZoom != 1.0f) {
        if (mPanOffset.x * mZoom > -(mPageImage->getSize().x / 2.0f))
            mPanOffset.x -= mPanAmount;
    }
    else {
        mPanOffset = {0.0f, 0.0f, 0.0f};
        showNextPage();
    }
}

void PDFViewer::navigateRightShoulder()
{
    if (mZoom <= 2.5f)
        mZoom += 0.5f;

    if (mZoom == 1.5f)
        mHelp->setPrompts(getHelpPrompts());

    convertPage(mCurrentPage);
}

void PDFViewer::navigateLeftShoulder()
{
    if (mZoom == 1.0f)
        mPanOffset = {0.0f, 0.0f, 0.0f};

    if (mZoom >= 1.5f)
        mZoom -= 0.5f;

    if (mZoom == 1.0f)
        mHelp->setPrompts(getHelpPrompts());

    convertPage(mCurrentPage);
}

void PDFViewer::navigateLeftTrigger()
{
    if (mZoom != 1.0f) {
        mZoom = 1.0f;
        mPanOffset = {0.0f, 0.0f, 0.0f};
        mHelp->setPrompts(getHelpPrompts());
        convertPage(mCurrentPage);
        return;
    }

    if (mCurrentPage == 1)
        return;

    mPanOffset = {0.0f, 0.0f, 0.0f};

    NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
    mCurrentPage = 1;
    mEntryNumText->setText(Utils::String::format(
        _("PAGE %s OF %s"), std::to_string(mCurrentPage).c_str(), mEntryCount.c_str()));
    convertPage(mCurrentPage);
}

void PDFViewer::navigateRightTrigger()
{
    if (mZoom != 1.0f) {
        mZoom = 1.0f;
        mPanOffset = {0.0f, 0.0f, 0.0f};
        mHelp->setPrompts(getHelpPrompts());
        convertPage(mCurrentPage);
        return;
    }

    if (mCurrentPage == mPageCount)
        return;

    mPanOffset = {0.0f, 0.0f, 0.0f};

    NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
    mCurrentPage = mPageCount;
    mEntryNumText->setText(Utils::String::format(
        _("PAGE %s OF %s"), std::to_string(mCurrentPage).c_str(), mEntryCount.c_str()));
    convertPage(mCurrentPage);
}
