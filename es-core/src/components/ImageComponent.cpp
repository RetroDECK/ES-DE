//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ImageComponent.cpp
//
//  Handles images: loading, resizing, cropping, color shifting etc.
//

#include "components/ImageComponent.h"

#include "Log.h"
#include "Settings.h"
#include "ThemeData.h"
#include "resources/TextureResource.h"
#include "utils/CImgUtil.h"

Vector2i ImageComponent::getTextureSize() const
{
    if (mTexture)
        return mTexture->getSize();
    else
        return Vector2i::Zero();
}

Vector2f ImageComponent::getSize() const
{
    return GuiComponent::getSize() * (mBottomRightCrop - mTopLeftCrop);
}

ImageComponent::ImageComponent(Window* window, bool forceLoad, bool dynamic)
    : GuiComponent(window)
    , mTargetIsMax(false)
    , mTargetIsMin(false)
    , mFlipX(false)
    , mFlipY(false)
    , mTargetSize(0, 0)
    , mColorShift(0xFFFFFFFF)
    , mColorShiftEnd(0xFFFFFFFF)
    , mColorGradientHorizontal(true)
    , mForceLoad(forceLoad)
    , mDynamic(dynamic)
    , mFadeOpacity(0)
    , mFading(false)
    , mRotateByTargetSize(false)
    , mTopLeftCrop(0.0f, 0.0f)
    , mBottomRightCrop(1.0f, 1.0f)
{
    updateColors();
}

void ImageComponent::resize()
{
    if (!mTexture)
        return;

    const Vector2f textureSize = mTexture->getSourceImageSize();
    if (textureSize == Vector2f::Zero())
        return;

    if (mTexture->isTiled()) {
        mSize = mTargetSize;
    }
    else {
        // SVG rasterization is determined by height and rasterization is done in terms of pixels.
        // If rounding is off enough in the rasterization step (for images with extreme aspect
        // ratios), it can cause cutoff when the aspect ratio breaks.
        // So we always make sure the resultant height is an integer to make sure cutoff doesn't
        // happen, and scale width from that (you'll see this scattered throughout the function).
        // It's important to use floorf rather than round for this, as we never want to round up
        // since that can lead to the cutoff just described.
        if (mTargetIsMax) {
            mSize = textureSize;

            Vector2f resizeScale((mTargetSize.x() / mSize.x()), (mTargetSize.y() / mSize.y()));

            if (resizeScale.x() < resizeScale.y()) {
                // This will be mTargetSize.x(). We can't exceed it, nor be lower than it.
                mSize[0] *= resizeScale.x();
                // We need to make sure we're not creating an image larger than max size.
                mSize[1] = std::min(floorf(mSize[1] *= resizeScale.x()), mTargetSize.y());
            }
            else {
                // This will be mTargetSize.y(). We can't exceed it.
                mSize[1] = floorf(mSize[1] * resizeScale.y());
                // For SVG rasterization, always calculate width from rounded height (see comment
                // above). We need to make sure we're not creating an image larger than max size.
                mSize[0] =
                    std::min((mSize[1] / textureSize.y()) * textureSize.x(), mTargetSize.x());
            }
        }
        else if (mTargetIsMin) {
            mSize = textureSize;

            Vector2f resizeScale((mTargetSize.x() / mSize.x()), (mTargetSize.y() / mSize.y()));

            if (resizeScale.x() > resizeScale.y()) {
                mSize[0] *= resizeScale.x();
                mSize[1] *= resizeScale.x();

                float cropPercent = (mSize.y() - mTargetSize.y()) / (mSize.y() * 2);
                crop(0, cropPercent, 0, cropPercent);
            }
            else {
                mSize[0] *= resizeScale.y();
                mSize[1] *= resizeScale.y();

                float cropPercent = (mSize.x() - mTargetSize.x()) / (mSize.x() * 2);
                crop(cropPercent, 0, cropPercent, 0);
            }
            // For SVG rasterization, always calculate width from rounded height (see comment
            // above). We need to make sure we're not creating an image smaller than min size.
            mSize[1] = std::max(floorf(mSize[1]), mTargetSize.y());
            mSize[0] = std::max((mSize[1] / textureSize.y()) * textureSize.x(), mTargetSize.x());
        }
        else {
            // If both components are set, we just stretch.
            // If no components are set, we don't resize at all.
            mSize = mTargetSize == Vector2f::Zero() ? textureSize : mTargetSize;

            // If only one component is set, we resize in a way that maintains aspect ratio.
            // For SVG rasterization, we always calculate width from rounded height (see
            // comment above).
            if (!mTargetSize.x() && mTargetSize.y()) {
                mSize[1] = floorf(mTargetSize.y());
                mSize[0] = (mSize.y() / textureSize.y()) * textureSize.x();
            }
            else if (mTargetSize.x() && !mTargetSize.y()) {
                mSize[1] = floorf((mTargetSize.x() / textureSize.x()) * textureSize.y());
                mSize[0] = (mSize.y() / textureSize.y()) * textureSize.x();
            }
        }
    }

    mSize[0] = floorf(mSize.x());
    mSize[1] = floorf(mSize.y());
    // mSize.y() should already be rounded.
    mTexture->rasterizeAt(static_cast<size_t>(mSize.x()), static_cast<size_t>(mSize.y()));

    onSizeChanged();
}

void ImageComponent::setImage(std::string path, bool tile)
{
    // Always load bundled graphic resources statically, unless mForceLoad has been set.
    // This eliminates annoying texture pop-in problems that would otherwise occur.
    if (!mForceLoad && (path[0] == ':') && (path[1] == '/')) {
        mDynamic = false;
    }

    if (path.empty() || !ResourceManager::getInstance()->fileExists(path)) {
        if (mDefaultPath.empty() || !ResourceManager::getInstance()->fileExists(mDefaultPath))
            mTexture.reset();
        else
            mTexture = TextureResource::get(mDefaultPath, tile, mForceLoad, mDynamic);
    }
    else {
        mTexture = TextureResource::get(path, tile, mForceLoad, mDynamic);
    }

    resize();
}

void ImageComponent::setImage(const char* data, size_t length, bool tile)
{
    mTexture.reset();

    mTexture = TextureResource::get("", tile);
    mTexture->initFromMemory(data, length);

    resize();
}

void ImageComponent::setImage(const std::shared_ptr<TextureResource>& texture)
{
    mTexture = texture;
    resize();
}

void ImageComponent::setResize(float width, float height)
{
    mTargetSize = Vector2f(width, height);
    mTargetIsMax = false;
    mTargetIsMin = false;
    resize();
}

void ImageComponent::setMaxSize(float width, float height)
{
    mTargetSize = Vector2f(width, height);
    mTargetIsMax = true;
    mTargetIsMin = false;
    resize();
}

void ImageComponent::setMinSize(float width, float height)
{
    mTargetSize = Vector2f(width, height);
    mTargetIsMax = false;
    mTargetIsMin = true;
    resize();
}

void ImageComponent::cropLeft(float percent)
{
    assert(percent >= 0.0f && percent <= 1.0f);
    mTopLeftCrop.x() = percent;
}

void ImageComponent::cropTop(float percent)
{
    assert(percent >= 0.0f && percent <= 1.0f);
    mTopLeftCrop.y() = percent;
}

void ImageComponent::cropRight(float percent)
{
    assert(percent >= 0.0f && percent <= 1.0f);
    mBottomRightCrop.x() = 1.0f - percent;
}

void ImageComponent::cropBot(float percent)
{
    assert(percent >= 0.0f && percent <= 1.0f);
    mBottomRightCrop.y() = 1.0f - percent;
}

void ImageComponent::crop(float left, float top, float right, float bot)
{
    cropLeft(left);
    cropTop(top);
    cropRight(right);
    cropBot(bot);
}

void ImageComponent::uncrop()
{
    // Remove any applied crop.
    crop(0, 0, 0, 0);
}

void ImageComponent::cropTransparentPadding(float maxSizeX, float maxSizeY)
{
    if (mSize == 0)
        return;

    std::vector<unsigned char> imageRGBA = mTexture.get()->getRawRGBAData();

    if (imageRGBA.size() == 0)
        return;

    Vector2i imageSize = mTexture.get()->getSize();
    cimg_library::CImg<unsigned char> imageCImg(imageSize.x(), imageSize.y(), 1, 4, 0);

    int paddingCoords[4] = {};

    // We need to convert our RGBA data to the CImg internal format as CImg does not interleave
    // the pixels (as in RGBARGBARGBA).
    Utils::CImg::convertRGBAToCImg(imageRGBA, imageCImg);

    // This will give us the coordinates for the fully transparent areas.
    Utils::CImg::getTransparentPaddingCoords(imageCImg, paddingCoords);

    Vector2f originalSize = mSize;

    float cropLeft = static_cast<float>(paddingCoords[0]) / static_cast<float>(imageSize.x());
    float cropTop = static_cast<float>(paddingCoords[1]) / static_cast<float>(imageSize.y());
    float cropRight = static_cast<float>(paddingCoords[2]) / static_cast<float>(imageSize.x());
    float cropBottom = static_cast<float>(paddingCoords[3]) / static_cast<float>(imageSize.y());

    crop(cropLeft, cropTop, cropRight, cropBottom);

    // Cropping the image obviously leads to a reduction in size, so we need to determine
    // how much to scale up after cropping to keep within the max size restrictions that
    // were passed as arguments.
    mSize.x() -= mSize.x() * (cropLeft + cropRight);
    mSize.y() -= mSize.y() * (cropTop + cropBottom);

    float scaleFactor = originalSize.y() / mSize.y();

    if (scaleFactor * mSize.x() < maxSizeX)
        scaleFactor = maxSizeX / mSize.x();

    if (scaleFactor * mSize.y() < maxSizeY)
        scaleFactor = maxSizeY / mSize.y();

    if (scaleFactor * mSize.x() > maxSizeX)
        scaleFactor = maxSizeX / mSize.x();

    if (scaleFactor * mSize.y() > maxSizeY)
        scaleFactor = maxSizeY / mSize.y();

    setResize(mSize.x() * scaleFactor, mSize.y() * scaleFactor);
    updateVertices();
}

void ImageComponent::setFlipX(bool flip)
{
    mFlipX = flip;
    updateVertices();
}

void ImageComponent::setFlipY(bool flip)
{
    mFlipY = flip;
    updateVertices();
}

void ImageComponent::setColorShift(unsigned int color)
{
    mColorShift = color;
    mColorShiftEnd = color;
    updateColors();
}

void ImageComponent::setColorShiftEnd(unsigned int color)
{
    mColorShiftEnd = color;
    updateColors();
}

void ImageComponent::setColorGradientHorizontal(bool horizontal)
{
    mColorGradientHorizontal = horizontal;
    updateColors();
}

void ImageComponent::setOpacity(unsigned char opacity)
{
    mOpacity = opacity;
    updateColors();
}

void ImageComponent::setSaturation(float saturation)
{
    mSaturation = saturation;
    updateColors();
}

void ImageComponent::updateVertices()
{
    if (!mTexture || !mTexture->isInitialized())
        return;

    // We go through this mess to make sure everything is properly rounded.
    // If we just round vertices at the end, edge cases occur near sizes of 0.5.
    const Vector2f topLeft = { 0, 0 };
    const Vector2f bottomRight = mSize;
    const float px = mTexture->isTiled() ? mSize.x() / getTextureSize().x() : 1.0f;
    const float py = mTexture->isTiled() ? mSize.y() / getTextureSize().y() : 1.0f;

    // clang-format off
    mVertices[0] = { { topLeft.x(),     topLeft.y()     }, { mTopLeftCrop.x(),          py   - mTopLeftCrop.y()     }, 0 };
    mVertices[1] = { { topLeft.x(),     bottomRight.y() }, { mTopLeftCrop.x(),          1.0f - mBottomRightCrop.y() }, 0 };
    mVertices[2] = { { bottomRight.x(), topLeft.y()     }, { mBottomRightCrop.x() * px, py   - mTopLeftCrop.y()     }, 0 };
    mVertices[3] = { { bottomRight.x(), bottomRight.y() }, { mBottomRightCrop.x() * px, 1.0f - mBottomRightCrop.y() }, 0 };
    // clang-format on

    updateColors();

    // Round vertices.
    for (int i = 0; i < 4; i++)
        mVertices[i].pos.round();

    if (mFlipX) {
        for (int i = 0; i < 4; i++)
            mVertices[i].tex[0] = px - mVertices[i].tex[0];
    }

    if (mFlipY) {
        for (int i = 0; i < 4; i++)
            mVertices[i].tex[1] = py - mVertices[i].tex[1];
    }
}

void ImageComponent::updateColors()
{
    const float opacity = (mOpacity * (mFading ? mFadeOpacity / 255.0f : 1.0f)) / 255.0f;
    const unsigned int color = Renderer::convertRGBAToABGR(
        (mColorShift & 0xFFFFFF00) | static_cast<unsigned char>((mColorShift & 0xFF) * opacity));
    const unsigned int colorEnd =
        Renderer::convertRGBAToABGR((mColorShiftEnd & 0xFFFFFF00) |
                                    static_cast<unsigned char>((mColorShiftEnd & 0xFF) * opacity));

    mVertices[0].col = color;
    mVertices[1].col = mColorGradientHorizontal ? colorEnd : color;
    mVertices[2].col = mColorGradientHorizontal ? color : colorEnd;
    mVertices[3].col = colorEnd;
}

void ImageComponent::render(const glm::mat4& parentTrans)
{
    if (!isVisible())
        return;

    glm::mat4 trans = parentTrans * getTransform();
    Renderer::setMatrix(trans);

    if (mTexture && mOpacity > 0) {
        if (Settings::getInstance()->getBool("DebugImage")) {
            Vector2f targetSizePos = (mTargetSize - mSize) * mOrigin * -1;
            Renderer::drawRect(targetSizePos.x(), targetSizePos.y(), mTargetSize.x(),
                               mTargetSize.y(), 0xFF000033, 0xFF000033);
            Renderer::drawRect(0.0f, 0.0f, mSize.x(), mSize.y(), 0xFF000033, 0xFF000033);
        }
        // An image with zero size would normally indicate a corrupt image file.
        if (mTexture->isInitialized() && mTexture->getSize() != 0) {
            // Actually draw the image.
            // The bind() function returns false if the texture is not currently loaded. A blank
            // texture is bound in this case but we want to handle a fade so it doesn't just
            // 'jump' in when it finally loads.
            fadeIn(mTexture->bind());
#if defined(USE_OPENGL_21)
            if (mSaturation < 1.0) {
                mVertices[0].shaders = Renderer::SHADER_DESATURATE;
                mVertices[0].saturation = mSaturation;
            }
#endif
            Renderer::drawTriangleStrips(&mVertices[0], 4, trans);
        }
        else {
            if (!mTexture) {
                LOG(LogError) << "Image texture is not initialized";
            }
            else {
                std::string textureFilePath = mTexture->getTextureFilePath();
                if (textureFilePath != "") {
                    LOG(LogError) << "Image texture for file \"" << textureFilePath
                                  << "\" has zero size";
                }
                else {
                    LOG(LogError) << "Image texture has zero size";
                }
            }
            mTexture.reset();
        }
    }

    GuiComponent::renderChildren(trans);
}

void ImageComponent::fadeIn(bool textureLoaded)
{
    if (!mForceLoad) {
        if (!textureLoaded) {
            // Start the fade if this is the first time we've encountered the unloaded texture.
            if (!mFading) {
                // Start with a zero opacity and flag it as fading.
                mFadeOpacity = 0;
                mFading = true;
                updateColors();
            }
        }
        else if (mFading) {
            // The texture is loaded and we need to fade it in. The fade is based on the frame
            // rate and is 1/4 second if running at 60 frames per second although the actual
            // value is not that important.
            int opacity = mFadeOpacity + 255 / 15;
            // See if we've finished fading.
            if (opacity >= 255) {
                mFadeOpacity = 255;
                mFading = false;
            }
            else {
                mFadeOpacity = static_cast<unsigned char>(opacity);
            }
            updateColors();
        }
    }
}

void ImageComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                const std::string& view,
                                const std::string& element,
                                unsigned int properties)
{
    using namespace ThemeFlags;

    GuiComponent::applyTheme(theme, view, element,
                             (properties ^ ThemeFlags::SIZE) |
                                 ((properties & (ThemeFlags::SIZE | POSITION)) ? ORIGIN : 0));

    const ThemeData::ThemeElement* elem = theme->getElement(view, element, "image");
    if (!elem)
        return;

    Vector2f scale = getParent() ? getParent()->getSize() :
                                   Vector2f(static_cast<float>(Renderer::getScreenWidth()),
                                            static_cast<float>(Renderer::getScreenHeight()));

    if (properties & ThemeFlags::SIZE) {
        if (elem->has("size"))
            setResize(elem->get<Vector2f>("size") * scale);
        else if (elem->has("maxSize"))
            setMaxSize(elem->get<Vector2f>("maxSize") * scale);
        else if (elem->has("minSize"))
            setMinSize(elem->get<Vector2f>("minSize") * scale);
    }

    if (elem->has("default"))
        setDefaultImage(elem->get<std::string>("default"));

    if (properties & PATH && elem->has("path")) {
        bool tile = (elem->has("tile") && elem->get<bool>("tile"));
        setImage(elem->get<std::string>("path"), tile);
    }

    if (properties & COLOR) {
        if (elem->has("color"))
            setColorShift(elem->get<unsigned int>("color"));
        if (elem->has("colorEnd"))
            setColorShiftEnd(elem->get<unsigned int>("colorEnd"));
        if (elem->has("gradientType"))
            setColorGradientHorizontal(
                !(elem->get<std::string>("gradientType").compare("horizontal")));
    }
}

std::vector<HelpPrompt> ImageComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> ret;
    ret.push_back(HelpPrompt("a", "select"));
    return ret;
}
