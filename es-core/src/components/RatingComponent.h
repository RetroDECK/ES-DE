//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  RatingComponent.h
//
//  Game rating icons.
//  Used by gamelist views, metadata editor and scraper.
//

#ifndef ES_APP_COMPONENTS_RATING_COMPONENT_H
#define ES_APP_COMPONENTS_RATING_COMPONENT_H

#include "GuiComponent.h"
#include "renderers/Renderer.h"

class TextureResource;

#define NUM_RATING_STARS 5

class RatingComponent : public GuiComponent
{
public:
    RatingComponent(bool colorizeChanges = false);

    std::string getValue() const override;
    std::string getRatingValue() const;
    // Should be a normalized float (in the range [0..1]) - if it's not, it will be clamped.
    void setValue(const std::string& value) override;

    bool input(InputConfig* config, Input input) override;
    void render(const glm::mat4& parentTrans) override;

    void onSizeChanged() override;

    void setOpacity(float opacity) override;

    // Multiply all pixels in the image by this color when rendering.
    void setColorShift(unsigned int color) override;
    unsigned int getColorShift() const override { return mColorShift; }

    void setOriginalColor(unsigned int color) override { mColorOriginalValue = color; }
    void setChangedColor(unsigned int color) override { mColorChangedValue = color; }

    void applyTheme(const std::shared_ptr<ThemeData>& theme,
                    const std::string& view,
                    const std::string& element,
                    unsigned int properties) override;

    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    void updateVertices();
    void updateColors();

    Renderer* mRenderer;
    float mValue;
    int mOriginalValue;
    unsigned int mColorOriginalValue;
    unsigned int mColorChangedValue;

    Renderer::Vertex mVertices[8];

    unsigned int mColorShift;
    unsigned int mColorShiftEnd;
    unsigned int mUnfilledColor;

    std::shared_ptr<TextureResource> mFilledTexture;
    std::shared_ptr<TextureResource> mUnfilledTexture;

    bool mColorizeChanges;
};

#endif // ES_APP_COMPONENTS_RATING_COMPONENT_H
