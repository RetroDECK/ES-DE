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
    ImageComponent(Window* window, bool forceLoad = false, bool dynamic = true);
    virtual ~ImageComponent() {}

    void setDefaultImage(std::string path) { mDefaultPath = path; }

    // Loads the image at the given filepath. Will tile if tile is true (retrieves texture
    // as tiling, creates vertices accordingly).
    void setImage(std::string path, bool tile = false, bool linearMagnify = false);
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
    void setResize(float width, float height) override;
    void setResize(const glm::vec2& size) { setResize(size.x, size.y); }

    // Resize the image to be as large as possible but fit within a box of this size.
    // Can be set before or after an image is loaded.
    // Never breaks the aspect ratio. setMaxSize() and setResize() are mutually exclusive.
    void setMaxSize(float width, float height);
    void setMaxSize(const glm::vec2& size) { setMaxSize(size.x, size.y); }

    void setMinSize(float width, float height);
    void setMinSize(const glm::vec2& size) { setMinSize(size.x, size.y); }

    glm::vec2 getRotationSize() const override { return mRotateByTargetSize ? mTargetSize : mSize; }

    // Applied AFTER image positioning and sizing.
    // cropTop(0.2) will crop 20% of the top of the image.
    void cropLeft(float percent);
    void cropTop(float percent);
    void cropRight(float percent);
    void cropBot(float percent);
    void crop(float left, float top, float right, float bot);
    void uncrop();

    // This crops any entirely transparent areas around the actual image.
    // The arguments restrict how much the end result is allowed to be scaled.
    void cropTransparentPadding(float maxSizeX, float maxSizeY);

    // Multiply all pixels in the image by this color when rendering.
    void setColorShift(unsigned int color) override;
    void setColorShiftEnd(unsigned int color);
    void setColorGradientHorizontal(bool horizontal);

    unsigned int getColorShift() const override { return mColorShift; }

    void setOpacity(unsigned char opacity) override;
    void setSaturation(float saturation) override;

    void setFlipX(bool flip); // Mirror on the X axis.
    void setFlipY(bool flip); // Mirror on the Y axis.

    // Flag indicating if rotation should be based on target size vs. actual size.
    void setRotateByTargetSize(bool rotate) { mRotateByTargetSize = rotate; }

    // Returns the size of the current texture, or (0, 0) if none is loaded.
    // May be different than drawn size (use getSize() for that).
    glm::ivec2 getTextureSize() const;

    glm::vec2 getSize() const override;

    bool hasImage() { return static_cast<bool>(mTexture); }
    std::shared_ptr<TextureResource> getTexture() { return mTexture; }

    void render(const glm::mat4& parentTrans) override;

    virtual void applyTheme(const std::shared_ptr<ThemeData>& theme,
                            const std::string& view,
                            const std::string& element,
                            unsigned int properties) override;

    virtual std::vector<HelpPrompt> getHelpPrompts() override;

private:
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
    unsigned char mFadeOpacity;
    bool mFading;
    bool mForceLoad;
    bool mDynamic;
    bool mRotateByTargetSize;

    glm::vec2 mTopLeftCrop;
    glm::vec2 mBottomRightCrop;
};

#endif // ES_CORE_COMPONENTS_IMAGE_COMPONENT_H
