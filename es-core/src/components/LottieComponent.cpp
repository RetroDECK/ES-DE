//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  LottieComponent.cpp
//
//  Component to play Lottie animations using the rlottie library.
//

#include "components/LottieComponent.h"

#include "Log.h"
#include "Window.h"
#include "resources/ResourceManager.h"
#include "utils/FileSystemUtil.h"

#include <chrono>

LottieComponent::LottieComponent(Window* window)
    : GuiComponent{window}
    , mAnimation{nullptr}
    , mSurface{nullptr}
    , mTotalFrames{0}
    , mFrameNum{0}
    , mFrameRate{0.0}
    , mTargetPacing{0}
    , mTimeAccumulator{0}
    , mHoldFrame{false}
    , mKeepAspectRatio{true}
{
    // Get an empty texture for rendering the animation.
    mTexture = TextureResource::get("");
#if defined(USE_OPENGLES_10)
    // This is not really supported by the OpenGL ES standard so hopefully it works
    // with all drivers and on all operating systems.
    mTexture->setFormat(Renderer::Texture::BGRA);
#endif

    // Set component defaults.
    setOrigin(0.5f, 0.5f);
    setSize(Renderer::getScreenWidth() * 0.2f, Renderer::getScreenHeight() * 0.2f);
    setPosition(Renderer::getScreenWidth() * 0.3f, Renderer::getScreenHeight() * 0.3f);
    setDefaultZIndex(30.0f);
    setZIndex(30.0f);
}

void LottieComponent::setAnimation(const std::string& path)
{
    if (mAnimation != nullptr) {
        if (mFuture.valid())
            mFuture.get();
        mSurface.reset();
        mAnimation.reset();
        mPictureRGBA.clear();
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

    mAnimation = rlottie::Animation::loadFromFile(mPath);

    if (mAnimation == nullptr) {
        LOG(LogError) << "Couldn't parse Lottie animation file \"" << mPath << "\"";
        return;
    }

    size_t width = static_cast<size_t>(mSize.x);
    size_t height = static_cast<size_t>(mSize.y);

    mPictureRGBA.resize(width * height * 4);
    mSurface = std::make_unique<rlottie::Surface>(reinterpret_cast<uint32_t*>(&mPictureRGBA[0]),
                                                  width, height, width * sizeof(uint32_t));

    if (mSurface == nullptr) {
        LOG(LogError) << "Couldn't create Lottie surface for file \"" << mPath << "\"";
        mAnimation.reset();
        return;
    }

    // Render the first frame as a type of preload to decrease the chance of seeing a blank
    // texture when first entering a view that uses this animation.
    mFuture = mAnimation->render(mFrameNum, *mSurface, mKeepAspectRatio);

    // Some statistics for the file.
    double duration = mAnimation->duration();
    mTotalFrames = mAnimation->totalFrame();
    mFrameRate = mAnimation->frameRate();

    mTargetPacing = static_cast<int>(1000.0 / mFrameRate);

    LOG(LogDebug) << "Total number of frames: " << mTotalFrames;
    LOG(LogDebug) << "Frame rate: " << mFrameRate;
    LOG(LogDebug) << "Duration: " << duration;
}

void LottieComponent::onSizeChanged()
{
    // Setting the animation again will completely reinitialize it.
    setAnimation(mPath);
}

void LottieComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                 const std::string& view,
                                 const std::string& element,
                                 unsigned int properties)
{
    //    using namespace ThemeFlags;
    GuiComponent::applyTheme(theme, view, element, properties);
}

void LottieComponent::update(int deltaTime)
{
    if (mAnimation == nullptr)
        return;

    if (mTimeAccumulator < mTargetPacing) {
        mHoldFrame = true;
        mTimeAccumulator += deltaTime;
    }
    else {
        mHoldFrame = false;
        mTimeAccumulator = mTimeAccumulator - mTargetPacing + deltaTime;
    }
}

void LottieComponent::render(const glm::mat4& parentTrans)
{
    if (!isVisible())
        return;

    if (mAnimation == nullptr)
        return;

    glm::mat4 trans{parentTrans * getTransform()};

    if (mFrameNum >= mTotalFrames)
        mFrameNum = 0;

    if (mFrameNum == 0)
        mAnimationStartTime = std::chrono::system_clock::now();

    bool renderNextFrame = false;

    if (mFuture.valid()) {
        if (mFuture.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready) {
            mTexture->initFromPixels(&mPictureRGBA.at(0), static_cast<size_t>(mSize.x),
                                     static_cast<size_t>(mSize.y));
            mFuture.get();
            ++mFrameNum;
            renderNextFrame = true;
        }
    }
    else {
        renderNextFrame = true;
    }
    if (renderNextFrame && !mHoldFrame)
        mFuture = mAnimation->render(mFrameNum, *mSurface, mKeepAspectRatio);

    Renderer::setMatrix(trans);

    if (Settings::getInstance()->getBool("DebugImage"))
        Renderer::drawRect(0.0f, 0.0f, mSize.x, mSize.y, 0xFF000033, 0xFF000033);

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

#if defined(USE_OPENGL_21)
        // Perform color space conversion from BGRA to RGBA.
        vertices[0].shaders = Renderer::SHADER_BGRA_TO_RGBA;
#endif

        // Render it.
        Renderer::drawTriangleStrips(&vertices[0], 4, trans);
    }

    if (!mHoldFrame && mFrameNum == mTotalFrames - 1) {
        mAnimationEndTime = std::chrono::system_clock::now();
        LOG(LogDebug) << "LottieComponent::render(): Animation duration: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(mAnimationEndTime -
                                                                               mAnimationStartTime)
                             .count()
                      << " ms";
    }
}
