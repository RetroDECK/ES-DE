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
#include "resources/TextureResource.h"

NinePatchComponent::NinePatchComponent(Window* window,
                                       const std::string& path,
                                       unsigned int edgeColor,
                                       unsigned int centerColor)
    : GuiComponent(window)
    , mCornerSize(16.0f, 16.0f)
    , mEdgeColor(edgeColor)
    , mCenterColor(centerColor)
    , mPath(path)
    , mVertices(nullptr)
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
    const unsigned int edgeColor = Renderer::convertRGBAToABGR(mEdgeColor);
    const unsigned int centerColor = Renderer::convertRGBAToABGR(mCenterColor);

    for (int i = 0; i < 6 * 9; i++)
        mVertices[i].col = edgeColor;

    for (int i = 6 * 4; i < 6; i++)
        mVertices[(6 * 4) + i].col = centerColor;
}

void NinePatchComponent::buildVertices()
{
    if (mVertices != nullptr)
        delete[] mVertices;

    // Scale the corner size relative to the screen resolution, but keep the scale factor
    // within reason as extreme resolutions may cause artifacts. Any "normal" resolution
    // (e.g. from 720p to 4K) will be within these boundaries though.
    float scaleFactor;
    if (Renderer::getScreenWidth() > Renderer::getScreenHeight())
        scaleFactor = glm::clamp(Renderer::getScreenHeightModifier(), 0.4f, 3.0f);
    else
        scaleFactor = glm::clamp(Renderer::getScreenWidthModifier(), 0.4f, 3.0f);

    mTexture = TextureResource::get(mPath, false, false, true, scaleFactor);

    if (mTexture->getSize() == glm::ivec2{}) {
        mVertices = nullptr;
        LOG(LogWarning) << "NinePatchComponent has no texture";
        return;
    }

    mVertices = new Renderer::Vertex[6 * 9];

    glm::vec2 texSize{static_cast<float>(mTexture->getSize().x),
                      static_cast<float>(mTexture->getSize().y)};

    const float imgSizeX[3]{mCornerSize.x, mSize.x - mCornerSize.x * 2.0f, mCornerSize.x};
    const float imgSizeY[3]{mCornerSize.y, mSize.y - mCornerSize.y * 2.0f, mCornerSize.y};
    const float imgPosX[3]{0, imgSizeX[0], imgSizeX[0] + imgSizeX[1]};
    const float imgPosY[3]{0, imgSizeY[0], imgSizeY[0] + imgSizeY[1]};

    // The "1 +" in posY and "-" in sizeY is to deal with texture coordinates having a bottom
    // left corner origin vs. verticies having a top left origin.
    // clang-format off
    const float texSizeX[3]{mCornerSize.x / texSize.x,  (texSize.x - mCornerSize.x * 2.0f) / texSize.x,  mCornerSize.x / texSize.x};
    const float texSizeY[3]{-mCornerSize.y / texSize.y, -(texSize.y - mCornerSize.y * 2.0f) / texSize.y, -mCornerSize.y / texSize.y};

    const float texPosX[3]{0.0f,        texSizeX[0],        texSizeX[0] + texSizeX[1]};
    const float texPosY[3]{1.0f, 1.0f + texSizeY[0], 1.0f + texSizeY[0] + texSizeY[1]};
    // clang-format on

    int v = 0;

    for (int slice = 0; slice < 9; slice++) {
        const int sliceX{slice % 3};
        const int sliceY{slice / 3};
        const glm::vec2 imgPos{imgPosX[sliceX], imgPosY[sliceY]};
        const glm::vec2 imgSize{imgSizeX[sliceX], imgSizeY[sliceY]};
        const glm::vec2 texPos{texPosX[sliceX], texPosY[sliceY]};
        const glm::vec2 texSize{texSizeX[sliceX], texSizeY[sliceY]};

        // clang-format off
        mVertices[v + 1] = {{imgPos.x            , imgPos.y            }, {texPos.x,             texPos.y            }, 0};
        mVertices[v + 2] = {{imgPos.x            , imgPos.y + imgSize.y}, {texPos.x,             texPos.y + texSize.y}, 0};
        mVertices[v + 3] = {{imgPos.x + imgSize.x, imgPos.y            }, {texPos.x + texSize.x, texPos.y            }, 0};
        mVertices[v + 4] = {{imgPos.x + imgSize.x, imgPos.y + imgSize.y}, {texPos.x + texSize.x, texPos.y + texSize.y}, 0};
        // clang-format on

        // Round vertices.
        for (int i = 1; i < 5; i++)
            mVertices[v + i].pos = glm::round(mVertices[v + i].pos);

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

    glm::mat4 trans{parentTrans * getTransform()};

    if (mTexture && mVertices != nullptr) {
        Renderer::setMatrix(trans);
        if (mOpacity < 255) {
            mVertices[0].shaders = Renderer::SHADER_OPACITY;
            mVertices[0].opacity = mOpacity / 255.0f;
        }
        else if (mVertices[0].shaders & Renderer::SHADER_OPACITY) {
            // We have reached full opacity, so disable the opacity shader and set
            // the vertex opacity to 1.0.
            mVertices[0].shaders ^= Renderer::SHADER_OPACITY;
            mVertices[0].opacity = 1.0f;
        }
        mTexture->bind();
        Renderer::drawTriangleStrips(&mVertices[0], 6 * 9, trans);
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

void NinePatchComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                    const std::string& view,
                                    const std::string& element,
                                    unsigned int properties)
{
    GuiComponent::applyTheme(theme, view, element, properties);

    using namespace ThemeFlags;
    const ThemeData::ThemeElement* elem = theme->getElement(view, element, "ninepatch");

    if (!elem)
        return;

    if (properties & PATH && elem->has("path"))
        setImagePath(elem->get<std::string>("path"));
}
