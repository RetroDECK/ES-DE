//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GIFAnimComponent.cpp
//
//  Component to play GIF animations.
//

#define DEBUG_ANIMATION false

#if defined(_MSC_VER) // MSVC compiler.
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "components/GIFAnimComponent.h"

#include "Log.h"
#include "Window.h"
#include "resources/ResourceManager.h"
#include "utils/StringUtil.h"

GIFAnimComponent::GIFAnimComponent()
    : mFrameSize {0}
    , mAnimFile {nullptr}
    , mAnimation {nullptr}
    , mFrame {nullptr}
    , mStartDirection {"normal"}
    , mTotalFrames {0}
    , mFrameNum {0}
    , mFrameTime {0}
    , mFileWidth {0}
    , mFileHeight {0}
    , mFrameRate {0.0}
    , mSpeedModifier {1.0f}
    , mTargetPacing {0}
    , mTimeAccumulator {0}
    , mLastRenderedFrame {-1}
    , mSkippedFrames {0}
    , mHoldFrame {false}
    , mPause {false}
    , mExternalPause {false}
    , mAlternate {false}
    , mKeepAspectRatio {true}
{
    // Get an empty texture for rendering the animation.
    mTexture = TextureResource::get("");
#if defined(USE_OPENGLES_10) || defined(USE_OPENGLES_20)
    // This is not really supported by the OpenGL ES standard so hopefully it works
    // with all drivers and on all operating systems.
    mTexture->setFormat(Renderer::Texture::BGRA);
#endif

    mAnimIO.read_proc = readProc;
    mAnimIO.write_proc = writeProc;
    mAnimIO.seek_proc = seekProc;
    mAnimIO.tell_proc = tellProc;

    // Set component defaults.
    setOrigin(0.5f, 0.5f);
    setSize(Renderer::getScreenWidth() * 0.2f, Renderer::getScreenHeight() * 0.2f);
    setPosition(Renderer::getScreenWidth() * 0.3f, Renderer::getScreenHeight() * 0.3f);
    setDefaultZIndex(10.0f);
    setZIndex(10.0f);
    mTexture->setLinearMagnify(false);
}

GIFAnimComponent::~GIFAnimComponent()
{
    if (mAnimFile != nullptr) {
        fclose(mAnimFile);
        mAnimFile = nullptr;
    }
}

void GIFAnimComponent::setAnimation(const std::string& path)
{
    if (mAnimation != nullptr) {
        FreeImage_CloseMultiBitmap(mAnimation, 0);
        mAnimation = nullptr;
        mPictureRGBA.clear();
        mLastRenderedFrame = -1;
        mFileWidth = 0;
        mFileHeight = 0;
    }

    mPath = path;

    if (mPath.empty()) {
        LOG(LogError) << "Path to GIF animation is empty";
        return;
    }

    if (mPath.front() == ':')
        mPath = ResourceManager::getInstance().getResourcePath(mPath);
    else
        mPath = Utils::FileSystem::expandHomePath(mPath);

    if (!(Utils::FileSystem::isRegularFile(mPath) || Utils::FileSystem::isSymlink(mPath))) {
        LOG(LogError) << "Couldn't open GIF animation file \"" << mPath << "\"";
        return;
    }

    FREE_IMAGE_FORMAT fileFormat;

#if defined(_WIN64)
    fileFormat = FreeImage_GetFileTypeU(Utils::String::stringToWideString(mPath).c_str());
#else
    fileFormat = FreeImage_GetFileType(mPath.c_str());
#endif

    if (fileFormat == FIF_UNKNOWN)
#if defined(_WIN64)
        fileFormat =
            FreeImage_GetFIFFromFilenameU(Utils::String::stringToWideString(mPath).c_str());
#else
        fileFormat = FreeImage_GetFIFFromFilename(mPath.c_str());
#endif

    if (fileFormat != FIF_GIF) {
        LOG(LogError)
            << "GIFAnimComponent::setAnimation(): Image not recognized as being in GIF format";
        return;
    }

    // Make sure that we can actually read this format.
    if (FreeImage_FIFSupportsReading(fileFormat)) {

#if defined(_WIN64)
        mAnimFile = _wfopen(Utils::String::stringToWideString(mPath).c_str(), L"r+b");
#else
        mAnimFile = fopen(mPath.c_str(), "r+b");
#endif
        if (mAnimFile != nullptr)
            mAnimation = FreeImage_OpenMultiBitmapFromHandle(
                fileFormat, &mAnimIO, static_cast<fi_handle>(mAnimFile), GIF_PLAYBACK);

        mFrame = FreeImage_LockPage(mAnimation, 0);
        FITAG* tagFrameTime {nullptr};

        FreeImage_GetMetadata(FIMD_ANIMATION, mFrame, "FrameTime", &tagFrameTime);
        if (tagFrameTime != nullptr) {
            if (FreeImage_GetTagCount(tagFrameTime) == 1) {
                const uint32_t frameTime {
                    *static_cast<const uint32_t*>(FreeImage_GetTagValue(tagFrameTime))};
                if (frameTime >= 20 && frameTime <= 1000)
                    mFrameTime = frameTime;
            }
        }
    }
    else {
        LOG(LogError) << "GIFAnimComponent::setAnimation(): Couldn't process file \"" << mPath
                      << "\"";
        return;
    }

    if (!mAnimation) {
        LOG(LogError) << "GIFAnimComponent::setAnimation(): Couldn't load animation file \""
                      << mPath << "\"";
        return;
    }

    if (!mKeepAspectRatio && (mSize.x == 0.0f || mSize.y == 0.0f)) {
        LOG(LogWarning) << "GIFAnimComponent: Width or height auto sizing is incompatible with "
                           "disabling of <keepAspectRatio> so ignoring this setting";
    }

    size_t width {0};
    size_t height {0};

    unsigned int filePitch {0};

    mTotalFrames = static_cast<size_t>(FreeImage_GetPageCount(mAnimation));

    mFileWidth = FreeImage_GetWidth(mFrame);
    mFileHeight = FreeImage_GetHeight(mFrame);
    filePitch = FreeImage_GetPitch(mFrame);

    if (mSize.x == 0.0f || mSize.y == 0.0f) {
        double sizeRatio {static_cast<double>(mFileWidth) / static_cast<double>(mFileHeight)};

        if (mSize.x == 0) {
            width = static_cast<size_t>(static_cast<double>(mSize.y) * sizeRatio);
            height = static_cast<size_t>(mSize.y);
        }
        else {
            width = static_cast<size_t>(mSize.x);
            height = static_cast<size_t>(static_cast<double>(mSize.x) / sizeRatio);
        }
    }
    else {
        width = static_cast<size_t>(mSize.x);
        height = static_cast<size_t>(mSize.y);
    }

    mSize.x = static_cast<float>(width);
    mSize.y = static_cast<float>(height);

    mPictureRGBA.resize(mFileWidth * mFileHeight * 4);

    FreeImage_ConvertToRawBits(reinterpret_cast<BYTE*>(&mPictureRGBA.at(0)), mFrame, filePitch, 32,
                               FI_RGBA_RED, FI_RGBA_GREEN, FI_RGBA_BLUE, 1);

    mTexture->initFromPixels(&mPictureRGBA.at(0), mFileWidth, mFileHeight);

    FreeImage_UnlockPage(mAnimation, mFrame, false);
    FreeImage_CloseMultiBitmap(mAnimation, 0);

    mDirection = mStartDirection;
    mFrameRate = 1000.0 / static_cast<double>(mFrameTime);
    mFrameSize = mFileWidth * mFileHeight * 4;
    mTargetPacing = static_cast<int>((1000.0 / mFrameRate) / static_cast<double>(mSpeedModifier));
    int duration {mTargetPacing * mTotalFrames};

    if (mDirection == "reverse")
        mFrameNum = mTotalFrames - 1;

    if (DEBUG_ANIMATION) {
        LOG(LogDebug) << "GIFAnimComponent::setAnimation(): Width: " << mFileWidth;
        LOG(LogDebug) << "GIFAnimComponent::setAnimation(): Height: " << mFileHeight;
        LOG(LogDebug) << "GIFAnimComponent::setAnimation(): Total number of frames: "
                      << mTotalFrames;
        LOG(LogDebug) << "GIFAnimComponent::setAnimation(): Frame rate: " << mFrameRate;
        LOG(LogDebug) << "GIFAnimComponent::setAnimation(): Speed modifier: " << mSpeedModifier;
        // This figure does not double if direction has been set to alternate or alternateReverse,
        // it only tells the duration of a single playthrough of all frames.
        LOG(LogDebug) << "GIFAnimComponent::setAnimation(): Target duration: " << duration << " ms";
        LOG(LogDebug) << "GIFAnimComponent::setAnimation(): Frame size: " << mFrameSize
                      << " bytes (" << std::fixed << std::setprecision(1)
                      << static_cast<double>(mFrameSize) / 1024.0 / 1024.0 << " MiB)";
        LOG(LogDebug) << "GIFAnimComponent::setAnimation(): Animation size: "
                      << mFrameSize * mTotalFrames << " bytes (" << std::fixed
                      << std::setprecision(1)
                      << static_cast<double>(mFrameSize * mTotalFrames) / 1024.0 / 1024.0
                      << " MiB)";
    }

    mAnimationStartTime = std::chrono::system_clock::now();
}

void GIFAnimComponent::resetFileAnimation()
{
    mExternalPause = false;
    mTimeAccumulator = 0;
    mFrameNum = mStartDirection == "reverse" ? mTotalFrames - 1 : 0;

    if (mAnimation != nullptr)
        mLastRenderedFrame = static_cast<int>(mFrameNum);
}

void GIFAnimComponent::onSizeChanged()
{
    // Setting the animation again will completely reinitialize it.
    if (mPath != "")
        setAnimation(mPath);
}

void GIFAnimComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                  const std::string& view,
                                  const std::string& element,
                                  unsigned int properties)
{
    using namespace ThemeFlags;
    const ThemeData::ThemeElement* elem {theme->getElement(view, element, "animation")};

    if (elem->has("size")) {
        glm::vec2 size = elem->get<glm::vec2>("size");
        if (size.x == 0.0f && size.y == 0.0f) {
            LOG(LogWarning) << "GIFAnimComponent: Invalid theme configuration, <size> defined as \""
                            << size.x << " " << size.y << "\"";
            return;
        }
    }

    if (elem->has("speed")) {
        const float speed {elem->get<float>("speed")};
        if (speed < 0.2f || speed > 3.0f) {
            LOG(LogWarning)
                << "GIFAnimComponent: Invalid theme configuration, <speed> defined as \""
                << std::fixed << std::setprecision(1) << speed << "\"";
        }
        else {
            mSpeedModifier = speed;
        }
    }

    if (elem->has("keepAspectRatio"))
        mKeepAspectRatio = elem->get<bool>("keepAspectRatio");

    if (elem->has("direction")) {
        std::string direction = elem->get<std::string>("direction");
        if (direction == "normal") {
            mStartDirection = "normal";
            mAlternate = false;
        }
        else if (direction == "reverse") {
            mStartDirection = "reverse";
            mAlternate = false;
        }
        else if (direction == "alternate") {
            mStartDirection = "normal";
            mAlternate = true;
        }
        else if (direction == "alternateReverse") {
            mStartDirection = "reverse";
            mAlternate = true;
        }
        else {
            LOG(LogWarning)
                << "GIFAnimComponent: Invalid theme configuration, <direction> defined as \""
                << direction << "\"";
            mStartDirection = "normal";
            mAlternate = false;
        }
    }

    if (elem->has("interpolation")) {
        const std::string interpolation {elem->get<std::string>("interpolation")};
        if (interpolation == "linear") {
            mTexture->setLinearMagnify(true);
        }
        else if (interpolation == "nearest") {
            mTexture->setLinearMagnify(false);
        }
        else {
            mTexture->setLinearMagnify(false);
            LOG(LogWarning)
                << "GIFAnimComponent::applyTheme(): Invalid theme configuration, property "
                   "<interpolation> defined as \""
                << interpolation << "\"";
        }
    }

    GuiComponent::applyTheme(theme, view, element, properties);

    if (elem->has("path")) {
        std::string path {elem->get<std::string>("path")};
        if (path != "") {
            setAnimation(path);
        }
    }
    else {
        LOG(LogWarning) << "GIFAnimComponent: Invalid theme configuration, <path> not set";
        return;
    }
}

void GIFAnimComponent::update(int deltaTime)
{
    if (!isVisible() || mThemeOpacity == 0.0f || mAnimation == nullptr)
        return;

    if (mWindow->getAllowFileAnimation()) {
        mPause = false;
    }
    else {
        mPause = true;
        mTimeAccumulator = 0;
        return;
    }

    // If the time accumulator value is really high something must have happened such as the
    // application having been suspended. Reset it to zero in this case as it would otherwise
    // never recover.
    if (mTimeAccumulator > deltaTime * 200)
        mTimeAccumulator = 0;

    // Prevent animation from playing too quickly.
    if (mTimeAccumulator + deltaTime < mTargetPacing) {
        mHoldFrame = true;
        mTimeAccumulator += deltaTime;
    }
    else {
        mHoldFrame = false;
        mTimeAccumulator = mTimeAccumulator - mTargetPacing + deltaTime;
    }

    // Rudimentary frame skipping logic, not entirely accurate but probably good enough.
    while (mTimeAccumulator - deltaTime > mTargetPacing) {
        if (DEBUG_ANIMATION && 0) {
            LOG(LogDebug) << "GIFAnimComponent::update(): Skipped Frame, mTimeAccumulator / "
                             "mTargetPacing: "
                          << mTimeAccumulator - deltaTime << " / " << mTargetPacing;
        }

        if (mDirection == "reverse")
            --mFrameNum;
        else
            ++mFrameNum;

        ++mSkippedFrames;
        mTimeAccumulator -= mTargetPacing;
    }
}

void GIFAnimComponent::render(const glm::mat4& parentTrans)
{
    if (!isVisible() || mThemeOpacity == 0.0f || mAnimation == nullptr)
        return;

    glm::mat4 trans {parentTrans * getTransform()};

    // This is necessary as there may otherwise be no texture to render when paused.
    if ((mExternalPause || mPause) && mTexture->getSize().x == 0.0f) {
        mTexture->initFromPixels(&mPictureRGBA.at(0), static_cast<size_t>(mSize.x),
                                 static_cast<size_t>(mSize.y));
    }

    bool doRender {true};

    // Don't render if a menu is open except if the cached background is getting invalidated.
    if (mWindow->getGuiStackSize() > 1 && !mWindow->isInvalidatingCachedBackground())
        doRender = false;

    // Don't render any new frames if paused or if a menu is open (unless invalidating background).
    if ((!mPause && !mExternalPause) && doRender) {
        if ((mDirection == "normal" && mFrameNum >= mTotalFrames) ||
            (mDirection == "reverse" && mFrameNum < 0)) {
            if (DEBUG_ANIMATION) {
                LOG(LogDebug) << "GIFAnimComponent::render(): Skipped frames: " << mSkippedFrames;
                LOG(LogDebug) << "GIFAnimComponent::render(): Actual duration: "
                              << std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::system_clock::now() - mAnimationStartTime)
                                     .count()
                              << " ms";
            }

            if (mAlternate) {
                if (mDirection == "normal")
                    mDirection = "reverse";
                else
                    mDirection = "normal";
            }

            mTimeAccumulator = 0;
            mSkippedFrames = 0;

            if (mDirection == "reverse" && mAlternate)
                mFrameNum = mTotalFrames - 2;
            else if (mDirection == "reverse" && !mAlternate)
                mFrameNum = mTotalFrames - 1;
            else if (mDirection == "normal" && mAlternate)
                mFrameNum = 1;
            else
                mFrameNum = 0;

            if (DEBUG_ANIMATION)
                mAnimationStartTime = std::chrono::system_clock::now();
        }

        if (!mHoldFrame) {
            mAnimation = FreeImage_OpenMultiBitmapFromHandle(
                FIF_GIF, &mAnimIO, static_cast<fi_handle>(mAnimFile), GIF_PLAYBACK);

            mFrame = FreeImage_LockPage(mAnimation, mFrameNum);
            mPictureRGBA.clear();
            mPictureRGBA.resize(mFrameSize);

            FreeImage_ConvertToRawBits(reinterpret_cast<BYTE*>(&mPictureRGBA.at(0)), mFrame,
                                       FreeImage_GetPitch(mFrame), 32, FI_RGBA_RED, FI_RGBA_GREEN,
                                       FI_RGBA_BLUE, 1);

            mTexture->initFromPixels(&mPictureRGBA.at(0), mFileWidth, mFileHeight);

            FreeImage_UnlockPage(mAnimation, mFrame, false);
            FreeImage_CloseMultiBitmap(mAnimation, 0);

            if (mDirection == "reverse")
                --mFrameNum;
            else
                ++mFrameNum;
        }
    }

    if (mTexture->getSize().x != 0.0f) {
        mTexture->bind();

        Renderer::Vertex vertices[4];

        // clang-format off
        vertices[0] = {{0.0f,    0.0f   }, {0.0f, 0.0f}, 0xFFFFFFFF};
        vertices[1] = {{0.0f,    mSize.y}, {0.0f, 1.0f}, 0xFFFFFFFF};
        vertices[2] = {{mSize.x, 0.0f   }, {1.0f, 0.0f}, 0xFFFFFFFF};
        vertices[3] = {{mSize.x, mSize.y}, {1.0f, 1.0f}, 0xFFFFFFFF};
        // clang-format on

        // Round vertices.
        for (int i = 0; i < 4; ++i)
            vertices[i].pos = glm::round(vertices[i].pos);

        vertices->saturation = mSaturation;
        vertices->opacity = mOpacity * mThemeOpacity;
        vertices->dim = mDim;
        vertices->convertBGRAToRGBA = true;

        // Render it.
        Renderer::setMatrix(trans);
        Renderer::drawTriangleStrips(&vertices[0], 4);
    }
}
