//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  NinePatchComponent.cpp
//
//  Breaks up an image into 3x3 patches to accomodate resizing without distortions.
//

#include "components/NinePatchComponent.h"

#include "Log.h"
#include "ThemeData.h"
#include "resources/Font.h"
#include "resources/TextureResource.h"

NinePatchComponent::NinePatchComponent(const std::string& path,
                                       unsigned int edgeColor,
                                       unsigned int centerColor)
    : mRenderer {Renderer::getInstance()}
    , mVertices {nullptr}
    , mPath {path}
    , mCornerSize {16.0f, 16.0f}
    , mSharpCorners {false}
    , mEdgeColor {edgeColor}
    , mCenterColor {centerColor}
{
    if (!mPath.empty())
        buildVertices();
}

NinePatchComponent::~NinePatchComponent()
{
    if (mVertices != nullptr)
        delete[] mVertices;
}

void NinePatchComponent::updateColors()
{
    for (int i = 0; i < 6 * 9; ++i)
        mVertices[i].color = mEdgeColor;

    for (int i = 6 * 4; i < 6; ++i)
        mVertices[(6 * 4) + i].color = mCenterColor;
}

void NinePatchComponent::buildVertices()
{
    if (mSize.x == 0.0f || mSize.y == 0.0f)
        return;

    if (mVertices != nullptr)
        delete[] mVertices;

    glm::vec2 relCornerSize {0.0f, 0.0f};

    // Don't scale the rasterized version of the frame as it would look bad.
    if (mPath.substr(mPath.size() - 4, std::string::npos) == ".png") {
        relCornerSize = mCornerSize;
    }
    else {
        // Scale the corner size relative to the screen resolution (using the medium sized
        // default font as size reference).
        relCornerSize = mCornerSize * (Font::get(FONT_SIZE_MEDIUM_FIXED)->getLetterHeight() *
                                       (mSharpCorners == true ? 0.0568f : 0.09f) / 2.0f);
    }

    glm::vec2 texSize {relCornerSize * 3.0f};
    mTexture = TextureResource::get(mPath, false, false, false, false, false,
                                    static_cast<size_t>(texSize.x), static_cast<size_t>(texSize.y));

    mTexture->rasterizeAt(texSize.x, texSize.y);

    if (mTexture->getSize() == glm::ivec2 {}) {
        mVertices = nullptr;
        LOG(LogWarning) << "NinePatchComponent has no texture";
        return;
    }

    mVertices = new Renderer::Vertex[6 * 9];

    const float imgSizeX[3] {relCornerSize.x, mSize.x - relCornerSize.x * 2.0f, relCornerSize.x};
    const float imgSizeY[3] {relCornerSize.y, mSize.y - relCornerSize.y * 2.0f, relCornerSize.y};
    const float imgPosX[3] {0, imgSizeX[0], imgSizeX[0] + imgSizeX[1]};
    const float imgPosY[3] {0, imgSizeY[0], imgSizeY[0] + imgSizeY[1]};

    // The "1 +" in posY and "-" in sizeY is to deal with texture coordinates having a bottom
    // left corner origin vs. verticies having a top left origin.
    // clang-format off
    const float texSizeX[3] {relCornerSize.x / texSize.x,  (texSize.x - relCornerSize.x * 2.0f) / texSize.x,  relCornerSize.x / texSize.x};
    const float texSizeY[3] {-relCornerSize.y / texSize.y, -(texSize.y - relCornerSize.y * 2.0f) / texSize.y, -relCornerSize.y / texSize.y};

    const float texPosX[3] {0.0f,        texSizeX[0],        texSizeX[0] + texSizeX[1]};
    const float texPosY[3] {1.0f, 1.0f + texSizeY[0], 1.0f + texSizeY[0] + texSizeY[1]};
    // clang-format on

    int v = 0;

    for (int slice = 0; slice < 9; ++slice) {
        const int sliceX {slice % 3};
        const int sliceY {slice / 3};
        const glm::vec2 imgPos {imgPosX[sliceX], imgPosY[sliceY]};
        const glm::vec2 imgSize {imgSizeX[sliceX], imgSizeY[sliceY]};
        const glm::vec2 texPos {texPosX[sliceX], texPosY[sliceY]};
        const glm::vec2 texSizeSlice {texSizeX[sliceX], texSizeY[sliceY]};

        // clang-format off
        mVertices[v + 1] = {{imgPos.x            , imgPos.y            }, {texPos.x,                  texPos.y                 }, 0};
        mVertices[v + 2] = {{imgPos.x            , imgPos.y + imgSize.y}, {texPos.x,                  texPos.y + texSizeSlice.y}, 0};
        mVertices[v + 3] = {{imgPos.x + imgSize.x, imgPos.y            }, {texPos.x + texSizeSlice.x, texPos.y                 }, 0};
        mVertices[v + 4] = {{imgPos.x + imgSize.x, imgPos.y + imgSize.y}, {texPos.x + texSizeSlice.x, texPos.y + texSizeSlice.y}, 0};
        // clang-format on

        // Round vertices.
        for (int i = 1; i < 5; ++i)
            mVertices[v + i].position = glm::round(mVertices[v + i].position);

        // Make duplicates of first and last vertex so this can be rendered as a triangle strip.
        mVertices[v + 0] = mVertices[v + 1];
        mVertices[v + 5] = mVertices[v + 4];

        v += 6;
    }

    updateColors();
}

void NinePatchComponent::render(const glm::mat4& parentTrans)
{
    if (!isVisible())
        return;

    glm::mat4 trans {parentTrans * getTransform()};

    if (mTexture && mVertices != nullptr) {
        mRenderer->setMatrix(trans);
        mVertices->opacity = mOpacity;
        mVertices->shaderFlags = Renderer::ShaderFlags::PREMULTIPLIED;
        mTexture->bind();
        mRenderer->drawTriangleStrips(&mVertices[0], 6 * 9);
    }

    renderChildren(trans);
}

void NinePatchComponent::fitTo(glm::vec2 size, glm::vec3 position, glm::vec2 padding)
{
    size += padding;
    position[0] -= padding.x / 2.0f;
    position[1] -= padding.y / 2.0f;

    setSize(size + mCornerSize * 2.0f);
    setPosition(position.x + glm::mix(-mCornerSize.x, mCornerSize.x, mOrigin.x),
                position.y + glm::mix(-mCornerSize.y, mCornerSize.y, mOrigin.y));
}

void NinePatchComponent::setImagePath(const std::string& path)
{
    mPath = path;
    buildVertices();
}

void NinePatchComponent::setEdgeColor(unsigned int edgeColor)
{
    mEdgeColor = edgeColor;
    updateColors();
}

void NinePatchComponent::setCenterColor(unsigned int centerColor)
{
    mCenterColor = centerColor;
    updateColors();
}
