//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  LottieAnimComponent.cpp
//
//  Component to play Lottie animations using the rlottie library.
//

#define DEBUG_ANIMATION false

#include "components/LottieAnimComponent.h"

#include "Log.h"
#include "ThemeData.h"
#include "Window.h"
#include "resources/ResourceManager.h"

LottieAnimComponent::LottieAnimComponent()
    : mRenderer {Renderer::getInstance()}
    , mTargetSize {0.0f, 0.0f}
    , mCacheFrames {true}
    , mMaxCacheSize {0}
    , mCacheSize {0}
    , mFrameSize {0}
    , mAnimation {nullptr}
    , mSurface {nullptr}
    , mStartDirection {"normal"}
    , mTotalFrames {0}
    , mFrameNum {0}
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

    // Keep per-file cache size within 0 to 1024 MiB.
    mMaxCacheSize = static_cast<size_t>(
        glm::clamp(Settings::getInstance()->getInt("LottieMaxFileCache"), 0, 1024) * 1024 * 1024);

    // Keep total cache size within 0 to 4096 MiB.
    int maxTotalCache {glm::clamp(Settings::getInstance()->getInt("LottieMaxTotalCache"), 0, 4096) *
                       1024 * 1024};

    if (mMaxTotalFrameCache != static_cast<size_t>(maxTotalCache))
        mMaxTotalFrameCache = static_cast<size_t>(maxTotalCache);

    // Set component defaults.
    setSize(mRenderer->getScreenWidth() * 0.2f, mRenderer->getScreenHeight() * 0.2f);
    setPosition(mRenderer->getScreenWidth() * 0.3f, mRenderer->getScreenHeight() * 0.3f);
    setDefaultZIndex(35.0f);
    setZIndex(35.0f);
}

LottieAnimComponent::~LottieAnimComponent()
{
    // This is required as rlottie could otherwise crash on application shutdown.
    if (mFuture.valid())
        mFuture.get();

    mTotalFrameCache -= mCacheSize;
}

void LottieAnimComponent::setAnimation(const std::string& path)
{
    if (mAnimation != nullptr) {
        if (mFuture.valid())
            mFuture.get();
        mSurface.reset();
        mAnimation.reset();
        mPictureRGBA.clear();
        mCacheSize = 0;
        mLastRenderedFrame = -1;
    }

    mPath = path;

    if (mPath.empty()) {
        LOG(LogError) << "Path to Lottie animation is empty";
        return;
    }

    if (mPath.front() == ':')
        mPath = ResourceManager::getInstance().getResourcePath(mPath);
    else
        mPath = Utils::FileSystem::expandHomePath(mPath);

    if (!(Utils::FileSystem::isRegularFile(mPath) || Utils::FileSystem::isSymlink(mPath))) {
        LOG(LogError) << "Couldn't open Lottie animation file \"" << mPath << "\"";
        return;
    }

    ResourceData animData {ResourceManager::getInstance().getFileData(mPath)};
    std::string cache;

    // If in debug mode, then disable the rlottie caching so that animations can be replaced on
    // the fly using Ctrl+r reloads.
    if (Settings::getInstance()->getBool("Debug")) {
        mAnimation = rlottie::Animation::loadFromData(
            std::string(reinterpret_cast<char*>(animData.ptr.get()), animData.length), cache, "",
            false);
    }
    else {
        mAnimation = rlottie::Animation::loadFromData(
            std::string(reinterpret_cast<char*>(animData.ptr.get()), animData.length), cache);
    }

    if (mAnimation == nullptr) {
        LOG(LogError) << "Couldn't parse Lottie animation file \"" << mPath << "\"";
        return;
    }

    size_t width {0};
    size_t height {0};

    if (mTargetIsMax || mSize.x == 0.0f || mSize.y == 0.0f) {
        size_t viewportWidth {0};
        size_t viewportHeight {0};

        mAnimation->size(viewportWidth, viewportHeight);
        const double sizeRatio {static_cast<double>(viewportWidth) /
                                static_cast<double>(viewportHeight)};

        if (mTargetIsMax) {
            // Just a precaution if rlottie::Animation::size() would return zero for some reason.
            if (viewportWidth == 0)
                viewportWidth = 1;
            if (viewportHeight == 0)
                viewportHeight = 1;

            mSize.x = static_cast<float>(viewportWidth);
            mSize.y = static_cast<float>(viewportHeight);

            // Preserve aspect ratio.
            const glm::vec2 resizeScale {mTargetSize.x / mSize.x, mTargetSize.y / mSize.y};

            if (resizeScale.x < resizeScale.y) {
                mSize.x *= resizeScale.x;
                mSize.y = std::min(mSize.y * resizeScale.x, mTargetSize.y);
            }
            else {
                mSize.y *= resizeScale.y;
                mSize.x = std::min((mSize.y / static_cast<float>(viewportHeight)) *
                                       static_cast<float>(viewportWidth),
                                   mTargetSize.x);
            }
            width = static_cast<size_t>(mSize.x);
            height = static_cast<size_t>(mSize.y);
        }
        else if (mSize.x == 0.0f) {
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

    mPictureRGBA.resize(width * height * 4);
    mSurface = std::make_unique<rlottie::Surface>(reinterpret_cast<uint32_t*>(&mPictureRGBA[0]),
                                                  width, height, width * sizeof(uint32_t));

    if (mSurface == nullptr) {
        LOG(LogError) << "Couldn't create Lottie surface for file \"" << mPath << "\"";
        mAnimation.reset();
        return;
    }

    // Some statistics for the file.
    double duration {mAnimation->duration()};
    mTotalFrames = mAnimation->totalFrame();
    mFrameRate = mAnimation->frameRate();
    mFrameSize = width * height * 4;
    mTargetPacing = static_cast<int>((1000.0 / mFrameRate) / static_cast<double>(mSpeedModifier));

    mDirection = mStartDirection;

    if (mDirection == "reverse")
        mFrameNum = mTotalFrames - 1;

    if (DEBUG_ANIMATION) {
        LOG(LogDebug) << "LottieAnimComponent::setAnimation(): Rasterized width: " << mSize.x;
        LOG(LogDebug) << "LottieAnimComponent::setAnimation(): Rasterized height: " << mSize.y;
        LOG(LogDebug) << "LottieAnimComponent::setAnimation(): Total number of frames: "
                      << mTotalFrames;
        LOG(LogDebug) << "LottieAnimComponent::setAnimation(): Frame rate: " << mFrameRate;
        LOG(LogDebug) << "LottieAnimComponent::setAnimation(): Speed modifier: " << mSpeedModifier;
        // This figure does not double if direction has been set to alternate or alternateReverse,
        // it only tells the duration of a single playthrough of all frames.
        LOG(LogDebug) << "LottieAnimComponent::setAnimation(): Target duration: "
                      << duration / mSpeedModifier * 1000.0 << " ms";
        LOG(LogDebug) << "LottieAnimComponent::setAnimation(): Frame size: " << mFrameSize
                      << " bytes (" << std::fixed << std::setprecision(1)
                      << static_cast<double>(mFrameSize) / 1024.0 / 1024.0 << " MiB)";
        LOG(LogDebug) << "LottieAnimComponent::setAnimation(): Animation size: "
                      << mFrameSize * mTotalFrames << " bytes (" << std::fixed
                      << std::setprecision(1)
                      << static_cast<double>(mFrameSize * mTotalFrames) / 1024.0 / 1024.0
                      << " MiB)";
        LOG(LogDebug) << "LottieAnimComponent::setAnimation(): Per file maximum cache size: "
                      << mMaxCacheSize << " bytes (" << std::fixed << std::setprecision(1)
                      << static_cast<double>(mMaxCacheSize) / 1024.0 / 1024.0 << " MiB)";
    }

    mAnimationStartTime = std::chrono::system_clock::now();
}

void LottieAnimComponent::resetFileAnimation()
{
    mExternalPause = false;
    mPlayCount = 0;
    mTimeAccumulator = 0;
    mDirection = mStartDirection;
    mFrameNum = mStartDirection == "reverse" ? mTotalFrames - 1 : 0;

    if (mAnimation != nullptr) {
        if (mFuture.valid())
            mFuture.get();
        mFuture = mAnimation->render(mFrameNum, *mSurface, false);
        mLastRenderedFrame = static_cast<int>(mFrameNum);
    }
}

void LottieAnimComponent::onSizeChanged()
{
    // Setting the animation again will completely reinitialize it.
    if (mPath != "")
        setAnimation(mPath);
}

void LottieAnimComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
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
            LOG(LogWarning) << "LottieAnimComponent: Invalid theme configuration, property "
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
            LOG(LogWarning) << "LottieAnimComponent: Invalid theme configuration, property "
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
                LOG(LogWarning) << "LottieAnimComponent: Invalid theme configuration, property "
                                   "\"gradientType\" for element \""
                                << element.substr(10) << "\" defined as \"" << gradientType << "\"";
            }
        }
    }

    if (elem->has("path"))
        setAnimation(elem->get<std::string>("path"));
}

void LottieAnimComponent::update(int deltaTime)
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
            LOG(LogDebug) << "LottieAnimComponent::update(): Skipped frame, mTimeAccumulator / "
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

void LottieAnimComponent::render(const glm::mat4& parentTrans)
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
            (mDirection == "reverse" && mFrameNum > mTotalFrames)) {
            if (DEBUG_ANIMATION) {
                LOG(LogDebug) << "LottieAnimComponent::render(): Skipped frames: "
                              << mSkippedFrames;
                LOG(LogDebug) << "LottieAnimComponent::render(): Actual duration: "
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

        bool renderNextFrame {false};

        if (mFuture.valid()) {
            if (mFuture.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready) {
                mFuture.get();
                // Cache frame if caching is enabled and we're not exceeding either the per-file
                // max cache size or the total cache size. Note that this is completely unrelated
                // to the texture caching used for images.
                if (mCacheFrames && mLastRenderedFrame != -1 &&
                    mFrameCache.find(mLastRenderedFrame) == mFrameCache.end()) {
                    size_t newCacheSize {mCacheSize + mFrameSize};
                    if (newCacheSize < mMaxCacheSize &&
                        mTotalFrameCache + mFrameSize < mMaxTotalFrameCache) {
                        mFrameCache[mLastRenderedFrame] = mPictureRGBA;
                        mCacheSize += mFrameSize;
                        mTotalFrameCache += mFrameSize;
                        mLastRenderedFrame = -1;
                    }
                }

                mTexture->initFromPixels(&mPictureRGBA.at(0), static_cast<size_t>(mSize.x),
                                         static_cast<size_t>(mSize.y));

                if (mDirection == "reverse")
                    --mFrameNum;
                else
                    ++mFrameNum;

                if (mDirection == "reverse" && mFrameNum == 0)
                    renderNextFrame = false;
                else if (mFrameNum == mTotalFrames)
                    renderNextFrame = false;
                else
                    renderNextFrame = true;
            }
        }
        else {
            if (mFrameCache.find(mFrameNum) != mFrameCache.end()) {
                if (!mHoldFrame) {
                    mTexture->initFromPixels(&mFrameCache[mFrameNum][0],
                                             static_cast<size_t>(mSize.x),
                                             static_cast<size_t>(mSize.y));

                    if (mDirection == "reverse")
                        --mFrameNum;
                    else
                        ++mFrameNum;
                }
            }
            else {
                renderNextFrame = true;
            }
        }

        if (renderNextFrame && !mHoldFrame) {
            mFuture = mAnimation->render(mFrameNum, *mSurface, false);
            mLastRenderedFrame = static_cast<int>(mFrameNum);
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
        for (int i {0}; i < 4; ++i)
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
