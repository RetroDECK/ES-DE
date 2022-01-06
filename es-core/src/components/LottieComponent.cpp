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
    , mLastRenderedFrame{0}
    , mLastDisplayedFrame{0}
    , mFrameRate{0.0}
    , mTargetPacing{0}
    , mSkipAccumulator{0}
    , mSkipFrame{false}
{
    // Get an empty texture for rendering the animation.
    mTexture = TextureResource::get("");
    mBuffer.clear();

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
    size_t bytesPerLine = width * sizeof(uint32_t);

    mBuffer.resize(width * height);

    mSurface = std::make_unique<rlottie::Surface>(&mBuffer[0], width, height, bytesPerLine);

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
    //    LOG(LogDebug) << "deltatime: " << deltaTime;

    if (deltaTime + mSkipAccumulator < mTargetPacing) {
        mSkipFrame = true;
        mSkipAccumulator += deltaTime;
    }
    else {
        mSkipFrame = false;
        mSkipAccumulator = 0;
    }

    if (mLastRenderedFrame == mTotalFrames)
        return;

    //    const auto updateStartTime = std::chrono::system_clock::now();

    mAnimation->renderSync(mLastRenderedFrame, *mSurface);
    mPictureRGBA.clear();

    // TODO: This is way too slow.
    for (auto i : mBuffer) {
        mPictureRGBA.emplace_back((i >> 16) & 0xff);
        mPictureRGBA.emplace_back((i >> 8) & 0xff);
        mPictureRGBA.emplace_back(i & 0xff);
        mPictureRGBA.emplace_back(i >> 24);
    }

    if (mBuffer2.size() < mTotalFrames)
        mBuffer2.emplace_back(mPictureRGBA);

    ++mLastRenderedFrame;

    //    LOG(LogDebug) << "Update cycle time: "
    //                  << std::chrono::duration_cast<std::chrono::milliseconds>(
    //                         std::chrono::system_clock::now() - updateStartTime)
    //                         .count()
    //                  << " ms";
}

void LottieComponent::render(const glm::mat4& parentTrans)
{
    if (!isVisible())
        return;

    if (mPictureRGBA.empty())
        return;

    if (!mSkipFrame && mLastDisplayedFrame == mTotalFrames - 1)
        mLastDisplayedFrame = 0;

    if (mLastDisplayedFrame == 0)
        mAnimationStartTime = std::chrono::system_clock::now();

    glm::mat4 trans{parentTrans * getTransform()};

    mTexture->initFromPixels(&mBuffer2.at(mLastDisplayedFrame).at(0), static_cast<size_t>(mSize.x),
                             static_cast<size_t>(mSize.y));
    mTexture->bind();

    if (!mSkipFrame)
        ++mLastDisplayedFrame;

    // clang-format off
    mVertices[0] = {{0.0f,    0.0f   }, {0.0f, 0.0f}, 0xFFFFFFFF};
    mVertices[1] = {{0.0f,    mSize.y}, {0.0f, 1.0f}, 0xFFFFFFFF};
    mVertices[2] = {{mSize.x, 0.0f   }, {1.0f, 0.0f}, 0xFFFFFFFF};
    mVertices[3] = {{mSize.x, mSize.y}, {1.0f, 1.0f}, 0xFFFFFFFF};
    // clang-format on

    // Round vertices.
    for (int i = 0; i < 4; ++i)
        mVertices[i].pos = glm::round(mVertices[i].pos);

    // Render it.
    Renderer::setMatrix(trans);
    Renderer::drawTriangleStrips(&mVertices[0], 4, trans);

    if (!mSkipFrame && mLastDisplayedFrame == mTotalFrames - 1) {
        mAnimationEndTime = std::chrono::system_clock::now();
        LOG(LogInfo) << "Actual duration time: "
                     << std::chrono::duration_cast<std::chrono::milliseconds>(mAnimationEndTime -
                                                                              mAnimationStartTime)
                            .count()
                     << " ms";
    }
}
