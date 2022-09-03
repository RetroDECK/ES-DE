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
#include "components/ImageComponent.h"
#include "renderers/Renderer.h"

class TextureResource;

#define NUM_RATING_STARS 5.0f

class RatingComponent : public GuiComponent
{
public:
    RatingComponent(bool colorizeChanges = false, bool linearInterpolation = false);

    std::string getValue() const override;
    // Returns a rating value between 0 and 5 as a string.
    static std::string getRatingValue(const std::string& rating);
    // Should be a normalized float (in the range [0..1]) - if it's not, it will be clamped.
    void setValue(const std::string& value) override;

    bool input(InputConfig* config, Input input) override;
    void render(const glm::mat4& parentTrans) override;

    void onSizeChanged() override;
    void setDimming(float dimming) override;

    void setOriginalColor(unsigned int color) override { mColorOriginalValue = color; }
    void setChangedColor(unsigned int color) override { mColorChangedValue = color; }

    void applyTheme(const std::shared_ptr<ThemeData>& theme,
                    const std::string& view,
                    const std::string& element,
                    unsigned int properties) override;

    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    Renderer* mRenderer;
    ImageComponent mIconFilled;
    ImageComponent mIconUnfilled;

    float mValue;
    float mImageRatio;
    int mOriginalValue;

    unsigned int mColorOriginalValue;
    unsigned int mColorChangedValue;

    bool mColorizeChanges;
    bool mOverlay;
};

#endif // ES_APP_COMPONENTS_RATING_COMPONENT_H
