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
    : mRenderer {Renderer::getInstance()}
    , mTargetSize {0.0f, 0.0f}
    , mFrameSize {0}
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
    , mHoldFrame {true}
    , mPause {false}
    , mExternalPause {false}
    , mAlternate {false}
    , mIterationCount {0}
    , mPlayCount {0}
    , mTargetIsMax {false}
    , mColorShift {0xFFFFFFFF}
    , mColorShiftEnd {0xFFFFFFFF}
    , mColorGradientHorizontal {true}
{
    // Get an empty texture for rendering the animation.
    mTexture = TextureResource::get("");

    mAnimIO.read_proc = readProc;
    mAnimIO.write_proc = writeProc;
    mAnimIO.seek_proc = seekProc;
    mAnimIO.tell_proc = tellProc;

    // Set component defaults.
    setSize(Renderer::getScreenWidth() * 0.2f, Renderer::getScreenHeight() * 0.2f);
    setPosition(Renderer::getScreenWidth() * 0.3f, Renderer::getScreenHeight() * 0.3f);
    setDefaultZIndex(35.0f);
    setZIndex(35.0f);
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

    size_t width {0};
    size_t height {0};

    unsigned int filePitch {0};

    mTotalFrames = static_cast<size_t>(FreeImage_GetPageCount(mAnimation));

    mFileWidth = FreeImage_GetWidth(mFrame);
    mFileHeight = FreeImage_GetHeight(mFrame);
    filePitch = FreeImage_GetPitch(mFrame);

    if (mTargetIsMax || mSize.x == 0.0f || mSize.y == 0.0f) {
        const double sizeRatio {static_cast<double>(mFileWidth) / static_cast<double>(mFileHeight)};

        if (mTargetIsMax) {
            // Just a precaution if FreeImage would return zero for some reason.
            if (mFileWidth == 0)
                mFileWidth = 1;
            if (mFileHeight == 0)
                mFileHeight = 1;

            mSize.x = static_cast<float>(mFileWidth);
            mSize.y = static_cast<float>(mFileHeight);

            // Preserve aspect ratio.
            const glm::vec2 resizeScale {mTargetSize.x / mSize.x, mTargetSize.y / mSize.y};

            if (resizeScale.x < resizeScale.y) {
                mSize.x *= resizeScale.x;
                mSize.y = std::min(mSize.y * resizeScale.x, mTargetSize.y);
            }
            else {
                mSize.y *= resizeScale.y;
                mSize.x = std::min((mSize.y / static_cast<float>(mFileHeight)) *
                                       static_cast<float>(mFileWidth),
                                   mTargetSize.x);
            }
            width = static_cast<size_t>(mSize.x);
            height = static_cast<size_t>(mSize.y);
        }
        else if (mSize.x == 0) {
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

    if (!mTargetIsMax)
        mTargetSize = mSize;

    FreeImage_PreMultiplyWithAlpha(mFrame);
    mPictureRGBA.resize(mFileWidth * mFileHeight * 4);

    FreeImage_ConvertToRawBits(reinterpret_cast<BYTE*>(&mPictureRGBA.at(0)), mFrame, filePitch, 32,
                               FI_RGBA_BLUE, FI_RGBA_GREEN, FI_RGBA_RED, 1);

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
    mPlayCount = 0;
    mTimeAccumulator = 0;
    mDirection = mStartDirection;
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
    GuiComponent::applyTheme(theme, view, element, properties ^ ThemeFlags::SIZE);

    const ThemeData::ThemeElement* elem {theme->getElement(view, element, "animation")};
    if (!elem)
        return;

    const glm::vec2 scale {glm::vec2(Renderer::getScreenWidth(), Renderer::getScreenHeight())};

    if (elem->has("size")) {
        glm::vec2 animationSize {elem->get<glm::vec2>("size")};
        if (animationSize == glm::vec2 {0.0f, 0.0f}) {
            LOG(LogWarning) << "GIFAnimComponent: Invalid theme configuration, property "
                               "\"size\" for element \""
                            << element.substr(10) << "\" is set to zero";
            animationSize = {0.01f, 0.01f};
        }
        if (animationSize.x > 0.0f)
            animationSize.x = glm::clamp(animationSize.x, 0.01f, 1.0f);
        if (animationSize.y > 0.0f)
            animationSize.y = glm::clamp(animationSize.y, 0.01f, 1.0f);
        setSize(animationSize * scale);
    }
    else if (elem->has("maxSize")) {
        const glm::vec2 animationMaxSize {glm::clamp(elem->get<glm::vec2>("maxSize"), 0.01f, 1.0f)};
        setSize(animationMaxSize * scale);
        mTargetIsMax = true;
        mTargetSize = mSize;
    }

    if (elem->has("metadataElement") && elem->get<bool>("metadataElement"))
        mComponentThemeFlags |= ComponentThemeFlags::METADATA_ELEMENT;

    if (elem->has("speed"))
        mSpeedModifier = glm::clamp(elem->get<float>("speed"), 0.2f, 3.0f);

    if (elem->has("direction")) {
        const std::string& direction {elem->get<std::string>("direction")};
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
            LOG(LogWarning) << "GIFAnimComponent: Invalid theme configuration, property "
                               "\"direction\" for element \""
                            << element.substr(10) << "\" defined as \"" << direction << "\"";
            mStartDirection = "normal";
            mAlternate = false;
        }
    }

    if (elem->has("iterationCount")) {
        mIterationCount = glm::clamp(elem->get<unsigned int>("iterationCount"), 0u, 10u);
        if (mAlternate)
            mIterationCount *= 2;
    }

    if (elem->has("interpolation")) {
        const std::string& interpolation {elem->get<std::string>("interpolation")};
        if (interpolation == "linear") {
            mTexture->setLinearMagnify(true);
        }
        else if (interpolation == "nearest") {
            mTexture->setLinearMagnify(false);
        }
        else {
            mTexture->setLinearMagnify(false);
            LOG(LogWarning) << "GIFAnimComponent: Invalid theme configuration, property "
                               "\"interpolation\" for element \""
                            << element.substr(10) << "\" defined as \"" << interpolation << "\"";
        }
    }

    if (properties & COLOR) {
        if (elem->has("color")) {
            mColorShift = elem->get<unsigned int>("color");
            mColorShiftEnd = mColorShift;
        }
        if (elem->has("colorEnd"))
            mColorShiftEnd = elem->get<unsigned int>("colorEnd");
        if (elem->has("gradientType")) {
            const std::string& gradientType {elem->get<std::string>("gradientType")};
            if (gradientType == "horizontal") {
                mColorGradientHorizontal = true;
            }
            else if (gradientType == "vertical") {
                mColorGradientHorizontal = false;
            }
            else {
                mColorGradientHorizontal = true;
                LOG(LogWarning) << "GIFAnimComponent: Invalid theme configuration, property "
                                   "\"gradientType\" for element \""
                                << element.substr(10) << "\" defined as \"" << gradientType << "\"";
            }
        }
    }

    if (elem->has("path"))
        setAnimation(elem->get<std::string>("path"));
}

void GIFAnimComponent::update(int deltaTime)
{
    if (mAnimation == nullptr || !isVisible() || mOpacity == 0.0f || mThemeOpacity == 0.0f)
        return;

    if (mWindow->getAllowFileAnimation()) {
        mPause = false;
    }
    else {
        mPause = true;
        mTimeAccumulator = 0;
        return;
    }

    // Make sure no frames are advanced unless update() has been called.
    mHoldFrame = false;

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
    if (mAnimation == nullptr || !isVisible() || mOpacity == 0.0f || mThemeOpacity == 0.0f)
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
            ++mPlayCount;

            if (mDirection == "reverse" && mAlternate)
                mFrameNum = mTotalFrames - 2;
            else if (mDirection == "reverse" && !mAlternate)
                mFrameNum = mTotalFrames - 1;
            else if (mDirection == "normal" && mAlternate)
                mFrameNum = 1;
            else
                mFrameNum = 0;

            if (mIterationCount != 0 && mPlayCount >= mIterationCount) {
                mPlayCount = 0;
                mExternalPause = true;
                mFrameNum = mTotalFrames;
            }

            if (DEBUG_ANIMATION)
                mAnimationStartTime = std::chrono::system_clock::now();
        }

        if (!mHoldFrame) {
            mAnimation = FreeImage_OpenMultiBitmapFromHandle(
                FIF_GIF, &mAnimIO, static_cast<fi_handle>(mAnimFile), GIF_PLAYBACK);

            mFrame = FreeImage_LockPage(mAnimation, mFrameNum);
            FreeImage_PreMultiplyWithAlpha(mFrame);
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

    mRenderer->setMatrix(trans);

    if (Settings::getInstance()->getBool("DebugImage")) {
        if (mTargetIsMax) {
            const glm::vec2 targetSizePos {
                glm::round((mTargetSize - mSize) * mOrigin * glm::vec2 {-1.0f})};
            mRenderer->drawRect(targetSizePos.x, targetSizePos.y, mTargetSize.x, mTargetSize.y,
                                0xFF000033, 0xFF000033);
        }
        mRenderer->drawRect(0.0f, 0.0f, mSize.x, mSize.y, 0xFF000033, 0xFF000033);
    }

    if (mTexture->getSize().x != 0.0f) {
        mTexture->bind();

        Renderer::Vertex vertices[4];

        // clang-format off
        vertices[0] = {{0.0f,    0.0f   }, {0.0f, 0.0f}, mColorShift};
        vertices[1] = {{0.0f,    mSize.y}, {0.0f, 1.0f}, mColorGradientHorizontal ? mColorShift : mColorShiftEnd};
        vertices[2] = {{mSize.x, 0.0f   }, {1.0f, 0.0f}, mColorGradientHorizontal ? mColorShiftEnd : mColorShift};
        vertices[3] = {{mSize.x, mSize.y}, {1.0f, 1.0f}, mColorShiftEnd};
        // clang-format on

        // Round vertices.
        for (int i = 0; i < 4; ++i)
            vertices[i].position = glm::round(vertices[i].position);

        vertices->brightness = mBrightness;
        vertices->saturation = mSaturation * mThemeSaturation;
        vertices->opacity = mOpacity * mThemeOpacity;
        vertices->dimming = mDimming;
        vertices->shaderFlags = Renderer::ShaderFlags::PREMULTIPLIED;

        // Render it.
        mRenderer->drawTriangleStrips(&vertices[0], 4);
    }

    mHoldFrame = true;
}
