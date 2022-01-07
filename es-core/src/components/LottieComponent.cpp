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
{
    // Get an empty texture for rendering the animation.
    mTexture = TextureResource::get("");
#if defined(USE_OPENGLES_10)
    // This is not really supported by the OpenGL ES standard so hopefully it works
    // with all drivers and on all operating systems.
    mTexture->setFormat(Renderer::Texture::BGRA);
#endif

    // TODO: Temporary test files.
    std::string filePath{":/animations/a_mountain.json"};
    //        std::string filePath{":/animations/bell.json"};

    if (filePath.empty()) {
        LOG(LogError) << "Path to Lottie animation is empty";
        return;
    }

    if (filePath.front() == ':')
        filePath = ResourceManager::getInstance().getResourcePath(filePath);
    else
        filePath = Utils::FileSystem::expandHomePath(filePath);

    if (!(Utils::FileSystem::isRegularFile(filePath) || Utils::FileSystem::isSymlink(filePath))) {
        LOG(LogError) << "Couldn't open Lottie animation file \"" << filePath << "\"";
        return;
    }

    // TODO: Only meant for development, to be replaced with proper theming support.
    setOrigin(0.5f, 0.5f);
    setSize(500.0f, 500.0f);
    setPosition(mSize.x * 0.35f, mSize.y * 0.5f);
    setDefaultZIndex(70.0f);
    setZIndex(70.0f);
    setVisible(true);

    mAnimation = rlottie::Animation::loadFromFile(filePath);

    if (mAnimation == nullptr) {
        LOG(LogError) << "Couldn't parse Lottie animation file \"" << filePath << "\"";
        return;
    }

    size_t width = static_cast<size_t>(mSize.x);
    size_t height = static_cast<size_t>(mSize.y);

    mPictureRGBA.resize(width * height * 4);

    mSurface = std::make_unique<rlottie::Surface>(reinterpret_cast<uint32_t*>(&mPictureRGBA[0]),
                                                  width, height, width * sizeof(uint32_t));

    // Animation time in seconds.
    double duration = mAnimation->duration();

    mTotalFrames = mAnimation->totalFrame();
    mFrameRate = mAnimation->frameRate();

    mTargetPacing = static_cast<int>(1000.0 / mFrameRate);

    LOG(LogDebug) << "Total number of frames: " << mTotalFrames;
    LOG(LogDebug) << "Frame rate: " << mFrameRate;
    LOG(LogDebug) << "Duration: " << duration;
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
        mFuture = mAnimation->render(mFrameNum, *mSurface);

    if (mTexture->getSize().x != 0.0f) {
        mTexture->bind();

        // clang-format off
        mVertices[0] = {{0.0f,    0.0f   }, {0.0f, 0.0f}, 0xFFFFFFFF};
        mVertices[1] = {{0.0f,    mSize.y}, {0.0f, 1.0f}, 0xFFFFFFFF};
        mVertices[2] = {{mSize.x, 0.0f   }, {1.0f, 0.0f}, 0xFFFFFFFF};
        mVertices[3] = {{mSize.x, mSize.y}, {1.0f, 1.0f}, 0xFFFFFFFF};
        // clang-format on

        // Round vertices.
        for (int i = 0; i < 4; ++i)
            mVertices[i].pos = glm::round(mVertices[i].pos);

#if defined(USE_OPENGL_21)
        // Perform color space conversion from BGRA to RGBA.
        mVertices[0].shaders = Renderer::SHADER_BGRA_TO_RGBA;
#endif

        // Render it.
        glm::mat4 trans{parentTrans * getTransform()};
        Renderer::setMatrix(trans);
        Renderer::drawTriangleStrips(&mVertices[0], 4, trans);
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
