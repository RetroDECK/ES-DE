//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ImageComponent.h
//
//  Handles images: loading, resizing, cropping, color shifting etc.
//

#ifndef ES_CORE_COMPONENTS_IMAGE_COMPONENT_H
#define ES_CORE_COMPONENTS_IMAGE_COMPONENT_H

#include "GuiComponent.h"
#include "renderers/Renderer.h"

class TextureResource;

class ImageComponent : public GuiComponent
{
public:
    ImageComponent(bool forceLoad = false, bool dynamic = true);
    virtual ~ImageComponent() {}

    void setDefaultImage(const std::string& path) { mDefaultPath = path; }

    // Loads the image at the given filepath. Will tile if tile is true (retrieves texture
    // as tiling, creates vertices accordingly).
    void setImage(const std::string& path, bool tile = false) override;
    // Loads an image from memory.
    void setImage(const char* data, size_t length, bool tile = false);
    // Use an already existing texture.
    void setImage(const std::shared_ptr<TextureResource>& texture, bool resizeTexture = true);

    void onSizeChanged() override { updateVertices(); }

    // Resize the image to fit this size. If one axis is zero, scale that axis to maintain
    // aspect ratio. If both are non-zero, potentially break the aspect ratio.  If both are
    // zero, don't do any resizing.
    // Can be set before or after an image is loaded.
    // setMaxSize() and setResize() are mutually exclusive.
    void setResize(const float width, const float height) override;
    void setResize(const glm::vec2& size) { setResize(size.x, size.y); }

    // Resize the image to be as large as possible but fit within a box of this size.
    // Can be set before or after an image is loaded.
    // Never breaks the aspect ratio. setMaxSize() and setResize() are mutually exclusive.
    void setMaxSize(const float width, const float height);
    void setMaxSize(const glm::vec2& size) { setMaxSize(size.x, size.y); }

    void setMinSize(const float width, const float height);
    void setMinSize(const glm::vec2& size) { setMinSize(size.x, size.y); }

    glm::vec2 getRotationSize() const override { return mRotateByTargetSize ? mTargetSize : mSize; }

    // Applied AFTER image positioning and sizing.
    // cropTop(0.2) will crop 20% of the top of the image.
    void cropLeft(const float percent);
    void cropTop(const float percent);
    void cropRight(const float percent);
    void cropBot(const float percent);
    void crop(const float left, const float top, const float right, const float bot);
    void uncrop();

    // This crops any entirely transparent areas around the actual image.
    // The arguments restrict how much the end result is allowed to be scaled.
    void cropTransparentPadding(const float maxSizeX, const float maxSizeY);

    // Multiply all pixels in the image by this color when rendering.
    void setColorShift(unsigned int color) override;
    void setColorShiftEnd(unsigned int color) override;
    void setColorGradientHorizontal(bool horizontal) override;

    unsigned int getColorShift() const override { return mColorShift; }

    void setOpacity(float opacity) override;
    void setSaturation(float saturation) override;
    void setDimming(float dimming) override;

    void setReflectionsFalloff(float falloff) override { mReflectionsFalloff = falloff; }
    void setFlipX(bool flip) override; // Mirror on the X axis.
    void setFlipY(bool flip) override; // Mirror on the Y axis.

    // Flag indicating if rotation should be based on target size vs. actual size.
    void setRotateByTargetSize(bool rotate) { mRotateByTargetSize = rotate; }
    // Whether to use smooth texture magnification by utilizing linear interpolation.
    void setLinearInterpolation(bool state) { mLinearInterpolation = state; }

    // Returns the size of the current texture, or (0, 0) if none is loaded.
    // May be different than drawn size (use getSize() for that).
    glm::ivec2 getTextureSize() const;

    glm::vec2 getSize() const override;

    bool hasImage() { return static_cast<bool>(mTexture); }
    std::shared_ptr<TextureResource> getTexture() { return mTexture; }

    void render(const glm::mat4& parentTrans) override;

    void applyTheme(const std::shared_ptr<ThemeData>& theme,
                    const std::string& view,
                    const std::string& element,
                    unsigned int properties) override;

    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    Renderer* mRenderer;
    glm::vec2 mTargetSize;

    bool mFlipX;
    bool mFlipY;
    bool mTargetIsMax;
    bool mTargetIsMin;

    // Calculates the correct mSize from our resizing information (set by setResize/setMaxSize).
    // Used internally whenever the resizing parameters or texture change. This function also
    // initiates the SVG rasterization.
    void resize();

    Renderer::Vertex mVertices[4];

    void updateVertices();
    void updateColors();
    void fadeIn(bool textureLoaded);

    unsigned int mColorShift;
    unsigned int mColorShiftEnd;
    bool mColorGradientHorizontal;

    std::string mDefaultPath;

    std::shared_ptr<TextureResource> mTexture;
    float mFadeOpacity;
    float mReflectionsFalloff;
    bool mFading;
    bool mForceLoad;
    bool mDynamic;
    bool mRotateByTargetSize;
    bool mLinearInterpolation;

    glm::vec2 mTopLeftCrop;
    glm::vec2 mBottomRightCrop;
};

#endif // ES_CORE_COMPONENTS_IMAGE_COMPONENT_H
