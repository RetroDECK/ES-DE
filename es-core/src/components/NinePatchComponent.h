//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  NinePatchComponent.h
//
//  Breaks up an image into 3x3 patches to accomodate resizing without distortions.
//

#ifndef ES_CORE_COMPONENTS_NINE_PATCH_COMPONENT_H
#define ES_CORE_COMPONENTS_NINE_PATCH_COMPONENT_H

#include "GuiComponent.h"
#include "renderers/Renderer.h"

class TextureResource;

// Display an image in a way so that edges don't get too distorted no matter the final size.
// Useful for UI elements like backgrounds, buttons, etc.
// This is accomplished by splitting an image into 9 pieces:
//  ___________
// |_1_|_2_|_3_|
// |_4_|_5_|_6_|
// |_7_|_8_|_9_|
//
// Corners (1, 3, 7, 9) will not be stretched at all.
// Borders (2, 4, 6, 8) will be stretched along one axis (2 and 8 horizontally, 4 and 6 vertically).
// The center (5) will be stretched.

class NinePatchComponent : public GuiComponent
{
public:
    NinePatchComponent(const std::string& path = "");

    void render(const glm::mat4& parentTrans) override;

    void onSizeChanged() override { buildVertices(); }
    void fitTo(glm::vec2 size,
               glm::vec3 position = {0.0f, 0.0f, 0.0f},
               glm::vec2 padding = {-32.0f, -32.0f});

    void setImagePath(const std::string& path);
    void setFrameColor(unsigned int frameColor);

    const glm::vec2& getCornerSize() const { return mCornerSize; }
    void setCornerSize(const glm::vec2& size)
    {
        mCornerSize = size;
        buildVertices();
    }
    void setSharpCorners(bool state) { mSharpCorners = state; }

private:
    void buildVertices();
    void updateColors();

    Renderer* mRenderer;
    std::unique_ptr<std::vector<Renderer::Vertex>> mVertices;

    std::string mPath;
    glm::vec2 mCornerSize;
    bool mSharpCorners;
    unsigned int mFrameColor;
    std::shared_ptr<TextureResource> mTexture;
};

#endif // ES_CORE_COMPONENTS_NINE_PATCH_COMPONENT_H
