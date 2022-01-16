//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GridTileComponent.cpp
//
//  X*Y tile grid, used indirectly by GridGamelistView via ImageGridComponent.
//

#include "GridTileComponent.h"

#include "ThemeData.h"
#include "animations/LambdaAnimation.h"
#include "resources/TextureResource.h"

GridTileComponent::GridTileComponent(Window* window)
    : GuiComponent(window)
    , mBackground(window, ":/graphics/frame.png")
{
    mDefaultProperties.mSize = getDefaultTileSize();
    mDefaultProperties.mPadding = glm::vec2 {16.0f * Renderer::getScreenWidthModifier(),
                                             16.0f * Renderer::getScreenHeightModifier()};
    mDefaultProperties.mImageColor = 0xAAAAAABB;
    // Attempting to use frame.svg instead causes quite severe performance problems.
    mDefaultProperties.mBackgroundImage = ":/graphics/frame.png";
    mDefaultProperties.mBackgroundCornerSize = glm::vec2 {16.0f, 16.0f};
    mDefaultProperties.mBackgroundCenterColor = 0xAAAAEEFF;
    mDefaultProperties.mBackgroundEdgeColor = 0xAAAAEEFF;

    mSelectedProperties.mSize = getSelectedTileSize();
    mSelectedProperties.mPadding = mDefaultProperties.mPadding;
    mSelectedProperties.mImageColor = 0xFFFFFFFF;
    mSelectedProperties.mBackgroundImage = mDefaultProperties.mBackgroundImage;
    mSelectedProperties.mBackgroundCornerSize = mDefaultProperties.mBackgroundCornerSize;
    mSelectedProperties.mBackgroundCenterColor = 0xFFFFFFFF;
    mSelectedProperties.mBackgroundEdgeColor = 0xFFFFFFFF;

    mImage = std::make_shared<ImageComponent>(mWindow);
    mImage->setOrigin(0.5f, 0.5f);

    mBackground.setOrigin(0.5f, 0.5f);

    addChild(&mBackground);
    addChild(&(*mImage));

    mSelectedZoomPercent = 0;
    mSelected = false;
    mVisible = false;

    setSelected(false, false);
    setVisible(true);
}

void GridTileComponent::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans {getTransform() * parentTrans};

    if (mVisible)
        renderChildren(trans);
}

// Update all the tile properties to the new status (selected or default).
void GridTileComponent::update(int deltaTime)
{
    GuiComponent::update(deltaTime);

    calcCurrentProperties();

    mBackground.setImagePath(mCurrentProperties.mBackgroundImage);

    mImage->setColorShift(mCurrentProperties.mImageColor);
    mBackground.setCenterColor(mCurrentProperties.mBackgroundCenterColor);
    mBackground.setEdgeColor(mCurrentProperties.mBackgroundEdgeColor);

    resize();
}

void applyThemeToProperties(const ThemeData::ThemeElement* elem, GridTileProperties* properties)
{
    glm::vec2 screen {static_cast<float>(Renderer::getScreenWidth()),
                      static_cast<float>(Renderer::getScreenHeight())};

    if (elem->has("size"))
        properties->mSize = elem->get<glm::vec2>("size") * screen;

    if (elem->has("padding"))
        properties->mPadding = elem->get<glm::vec2>("padding");

    if (elem->has("imageColor"))
        properties->mImageColor = elem->get<unsigned int>("imageColor");

    if (elem->has("backgroundImage"))
        properties->mBackgroundImage = elem->get<std::string>("backgroundImage");

    if (elem->has("backgroundCornerSize"))
        properties->mBackgroundCornerSize = elem->get<glm::vec2>("backgroundCornerSize");

    if (elem->has("backgroundColor")) {
        properties->mBackgroundCenterColor = elem->get<unsigned int>("backgroundColor");
        properties->mBackgroundEdgeColor = elem->get<unsigned int>("backgroundColor");
    }

    if (elem->has("backgroundCenterColor"))
        properties->mBackgroundCenterColor = elem->get<unsigned int>("backgroundCenterColor");

    if (elem->has("backgroundEdgeColor"))
        properties->mBackgroundEdgeColor = elem->get<unsigned int>("backgroundEdgeColor");
}

void GridTileComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                   const std::string& view,
                                   const std::string& /*element*/,
                                   unsigned int /*properties*/)
{
    // Apply theme to the default gridtile.
    const ThemeData::ThemeElement* elem = theme->getElement(view, "default", "gridtile");
    if (elem)
        applyThemeToProperties(elem, &mDefaultProperties);

    // Apply theme to the selected gridtile. Note that some of the default gridtile
    // properties have influence on the selected gridtile properties.
    // See THEMES.md for more information.
    elem = theme->getElement(view, "selected", "gridtile");

    mSelectedProperties.mSize = getSelectedTileSize();
    mSelectedProperties.mPadding = mDefaultProperties.mPadding;
    mSelectedProperties.mBackgroundImage = mDefaultProperties.mBackgroundImage;
    mSelectedProperties.mBackgroundCornerSize = mDefaultProperties.mBackgroundCornerSize;

    if (elem)
        applyThemeToProperties(elem, &mSelectedProperties);
}

glm::vec2 GridTileComponent::getDefaultTileSize()
{
    glm::vec2 screen {glm::vec2(static_cast<float>(Renderer::getScreenWidth()),
                                static_cast<float>(Renderer::getScreenHeight()))};

    return screen * 0.22f;
}

glm::vec2 GridTileComponent::getSelectedTileSize() const
{
    // Return the tile size.
    return mDefaultProperties.mSize * 1.2f;
}

bool GridTileComponent::isSelected() const
{
    // Return whether the tile is selected.
    return mSelected;
}

void GridTileComponent::setImage(const std::string& path)
{
    mImage->setImage(path);

    // Resize now to prevent flickering images when scrolling.
    resize();
}

void GridTileComponent::setImage(const std::shared_ptr<TextureResource>& texture)
{
    mImage->setImage(texture);

    // Resize now to prevent flickering images when scrolling.
    resize();
}

void GridTileComponent::setSelected(bool selected,
                                    bool allowAnimation,
                                    glm::vec3* pPosition,
                                    bool force)
{
    if (mSelected == selected && !force)
        return;

    mSelected = selected;

    if (selected) {
        if (pPosition == nullptr || !allowAnimation) {
            cancelAnimation(3);

            this->setSelectedZoom(1);
            mAnimPosition = {};

            resize();
        }
        else {
            mAnimPosition = glm::vec3 {pPosition->x, pPosition->y, pPosition->z};

            auto func = [this](float t) {
                t -= 1;
                float pct = glm::mix(0.0f, 1.0f, t * t * t + 1.0f);
                this->setSelectedZoom(pct);
            };

            cancelAnimation(3);
            setAnimation(
                new LambdaAnimation(func, 250), 0,
                [this] {
                    this->setSelectedZoom(1);
                    mAnimPosition = {};
                },
                false, 3);
        }
    }
    else {
        if (!allowAnimation) {
            cancelAnimation(3);
            this->setSelectedZoom(0);

            resize();
        }
        else {
            this->setSelectedZoom(1);

            auto func = [this](float t) {
                t -= 1.0f;
                float pct = glm::mix(0.0f, 1.0f, t * t * t + 1.0f);
                this->setSelectedZoom(1.0f - pct);
            };

            cancelAnimation(3);
            setAnimation(
                new LambdaAnimation(func, 250), 0, [this] { this->setSelectedZoom(0); }, false, 3);
        }
    }
}

void GridTileComponent::setSelectedZoom(float percent)
{
    if (mSelectedZoomPercent == percent)
        return;

    mSelectedZoomPercent = percent;
    resize();
}

void GridTileComponent::setVisible(bool visible) { mVisible = visible; }

void GridTileComponent::resize()
{
    calcCurrentProperties();

    mImage->setMaxSize(mCurrentProperties.mSize - mCurrentProperties.mPadding * 2.0f);
    mBackground.setCornerSize(mCurrentProperties.mBackgroundCornerSize);
    mBackground.fitTo(mCurrentProperties.mSize - mBackground.getCornerSize() * 2.0f);
}

unsigned int mixColors(unsigned int first, unsigned int second, float percent)
{
    unsigned char alpha0 = (first >> 24) & 0xFF;
    unsigned char blue0 = (first >> 16) & 0xFF;
    unsigned char green0 = (first >> 8) & 0xFF;
    unsigned char red0 = first & 0xFF;

    unsigned char alpha1 = (second >> 24) & 0xFF;
    unsigned char blue1 = (second >> 16) & 0xFF;
    unsigned char green1 = (second >> 8) & 0xFF;
    unsigned char red1 = second & 0xFF;

    unsigned char alpha = static_cast<unsigned char>(alpha0 * (1.0 - percent) + alpha1 * percent);
    unsigned char blue = static_cast<unsigned char>(blue0 * (1.0 - percent) + blue1 * percent);
    unsigned char green = static_cast<unsigned char>(green0 * (1.0 - percent) + green1 * percent);
    unsigned char red = static_cast<unsigned char>(red0 * (1.0 - percent) + red1 * percent);

    return (alpha << 24) | (blue << 16) | (green << 8) | red;
}

void GridTileComponent::calcCurrentProperties()
{
    mCurrentProperties = mSelected ? mSelectedProperties : mDefaultProperties;

    float zoomPercentInverse = 1.0f - mSelectedZoomPercent;

    if (mSelectedZoomPercent != 0.0f && mSelectedZoomPercent != 1.0f) {
        if (mDefaultProperties.mSize != mSelectedProperties.mSize)
            mCurrentProperties.mSize = mDefaultProperties.mSize * zoomPercentInverse +
                                       mSelectedProperties.mSize * mSelectedZoomPercent;

        if (mDefaultProperties.mPadding != mSelectedProperties.mPadding)
            mCurrentProperties.mPadding = mDefaultProperties.mPadding * zoomPercentInverse +
                                          mSelectedProperties.mPadding * mSelectedZoomPercent;

        if (mDefaultProperties.mImageColor != mSelectedProperties.mImageColor)
            mCurrentProperties.mImageColor =
                mixColors(mDefaultProperties.mImageColor, mSelectedProperties.mImageColor,
                          mSelectedZoomPercent);

        if (mDefaultProperties.mBackgroundCornerSize != mSelectedProperties.mBackgroundCornerSize)
            mCurrentProperties.mBackgroundCornerSize =
                mDefaultProperties.mBackgroundCornerSize * zoomPercentInverse +
                mSelectedProperties.mBackgroundCornerSize * mSelectedZoomPercent;

        if (mDefaultProperties.mBackgroundCenterColor != mSelectedProperties.mBackgroundCenterColor)
            mCurrentProperties.mBackgroundCenterColor =
                mixColors(mDefaultProperties.mBackgroundCenterColor,
                          mSelectedProperties.mBackgroundCenterColor, mSelectedZoomPercent);

        if (mDefaultProperties.mBackgroundEdgeColor != mSelectedProperties.mBackgroundEdgeColor)
            mCurrentProperties.mBackgroundEdgeColor =
                mixColors(mDefaultProperties.mBackgroundEdgeColor,
                          mSelectedProperties.mBackgroundEdgeColor, mSelectedZoomPercent);
    }
}

glm::vec3 GridTileComponent::getBackgroundPosition()
{
    return mBackground.getPosition() + mPosition;
}

std::shared_ptr<TextureResource> GridTileComponent::getTexture()
{
    if (mImage != nullptr)
        return mImage->getTexture();

    return nullptr;
}

void GridTileComponent::forceSize(glm::vec2 size, float selectedZoom)
{
    mDefaultProperties.mSize = size;
    mSelectedProperties.mSize = size * selectedZoom;
}
