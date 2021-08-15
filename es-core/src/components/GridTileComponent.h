//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GridTileComponent.h
//
//  X*Y tile grid, used indirectly by GridGameListView via ImageGridComponent.
//

#ifndef ES_CORE_COMPONENTS_GRID_TILE_COMPONENT_H
#define ES_CORE_COMPONENTS_GRID_TILE_COMPONENT_H

#include "ImageComponent.h"
#include "NinePatchComponent.h"

struct GridTileProperties {
    Vector2f mSize;
    Vector2f mPadding;
    unsigned int mImageColor;
    std::string mBackgroundImage;
    Vector2f mBackgroundCornerSize;
    unsigned int mBackgroundCenterColor;
    unsigned int mBackgroundEdgeColor;
};

class GridTileComponent : public GuiComponent
{
public:
    GridTileComponent(Window* window);

    void render(const glm::mat4& parentTrans) override;
    virtual void applyTheme(const std::shared_ptr<ThemeData>& theme,
                            const std::string& view,
                            const std::string& element,
                            unsigned int properties) override;

    // Made this a static function because the ImageGridComponent needs to know the default tile
    // max size to calculate the grid dimension before it instantiates the GridTileComponents.
    static Vector2f getDefaultTileSize();
    Vector2f getSelectedTileSize() const;
    bool isSelected() const;

    void reset() { setImage(""); }

    void setImage(const std::string& path);
    void setImage(const std::shared_ptr<TextureResource>& texture);
    void setSelected(bool selected,
                     bool allowAnimation = true,
                     glm::vec3* pPosition = nullptr,
                     bool force = false);
    void setVisible(bool visible);

    void forceSize(Vector2f size, float selectedZoom);

    glm::vec3 getBackgroundPosition();

    virtual void update(int deltaTime) override;

    std::shared_ptr<TextureResource> getTexture();

private:
    void resize();
    void calcCurrentProperties();
    void setSelectedZoom(float percent);

    std::shared_ptr<ImageComponent> mImage;
    NinePatchComponent mBackground;

    GridTileProperties mDefaultProperties;
    GridTileProperties mSelectedProperties;
    GridTileProperties mCurrentProperties;

    float mSelectedZoomPercent;
    bool mSelected;
    bool mVisible;

    Vector3f mAnimPosition;
};

#endif // ES_CORE_COMPONENTS_GRID_TILE_COMPONENT_H
