//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  HelpComponent.h
//
//  Help information in icon and text pairs.
//

#ifndef ES_CORE_COMPONENTS_HELP_COMPONENT_H
#define ES_CORE_COMPONENTS_HELP_COMPONENT_H

#include "GuiComponent.h"
#include "components/ComponentGrid.h"
#include "renderers/Renderer.h"
#include "resources/Font.h"

class HelpComponent : public GuiComponent
{
public:
    HelpComponent(std::shared_ptr<Font> font = Renderer::getIsVerticalOrientation() ?
                                                   Font::get(0.025f * Renderer::getScreenWidth()) :
                                                   Font::get(FONT_SIZE_SMALL));

    void clearPrompts();
    void setPrompts(const std::vector<HelpPrompt>& prompts);

    void setOpacity(float opacity) override;
    void setHelpPosition(const glm::vec2 position) { mHelpPosition = position; }
    void setHelpOrigin(const glm::vec2 origin) { mHelpOrigin = origin; }
    void setHelpTextColor(const unsigned int textColor) { mTextColor = textColor; }
    void setHelpIconColor(const unsigned int iconColor) { mIconColor = iconColor; }

    void applyTheme(const std::shared_ptr<ThemeData>& theme,
                    const std::string& view,
                    const std::string& element,
                    unsigned int properties) override;

    void render(const glm::mat4& parent) override;

private:
    void assignIcons();
    void updateGrid();

    Renderer* mRenderer;

    std::shared_ptr<ComponentGrid> mGrid;

    std::vector<HelpPrompt> mPrompts;
    std::map<std::string, std::string> mIconPathMap;

    std::shared_ptr<Font> mFont;
    std::shared_ptr<Font> mFontDimmed;

    std::vector<std::string> mEntries;
    static inline std::vector<std::string> sAllowedEntries {"thumbstickclick",
                                                            "lr",
                                                            "ltrt",
                                                            "up/down/left/right",
                                                            "up/down",
                                                            "up",
                                                            "down",
                                                            "left/right",
                                                            "rt",
                                                            "lt",
                                                            "r",
                                                            "l",
                                                            "y",
                                                            "x",
                                                            "b",
                                                            "a",
                                                            "start",
                                                            "back"};

    enum class EntryLayout {
        ICON_FIRST,
        TEXT_FIRST
    };

    glm::vec2 mHelpPosition;
    glm::vec2 mHelpPositionDimmed;
    glm::vec2 mHelpOrigin;
    glm::vec2 mHelpOriginDimmed;
    glm::vec2 mHelpRotationOrigin;
    unsigned int mTextColor;
    unsigned int mTextColorDimmed;
    unsigned int mIconColor;
    unsigned int mIconColorDimmed;
    unsigned int mBackgroundColor;
    unsigned int mBackgroundColorEnd;
    glm::vec2 mBackgroundHorizontalPadding;
    glm::vec2 mBackgroundVerticalPadding;
    float mBackgroundCornerRadius;
    bool mColorGradientHorizontal;
    EntryLayout mEntryLayout;
    float mEntryRelativeScale;
    float mLetterHeight;
    float mLetterHeightDimmed;
    float mHelpRotation;
    float mEntrySpacing;
    float mEntrySpacingDimmed;
    float mIconTextSpacing;
    float mIconTextSpacingDimmed;
    float mHelpOpacity;
    float mHelpOpacityDimmed;
    std::string mLetterCase;

    struct CustomButtonIcons {
        // Generic
        std::string dpad_updown;
        std::string dpad_up;
        std::string dpad_down;
        std::string dpad_leftright;
        std::string dpad_all;
        std::string thumbstick_click;
        std::string button_l;
        std::string button_r;
        std::string button_lr;
        std::string button_lt;
        std::string button_rt;
        std::string button_ltrt;

        // SNES
        std::string button_a_SNES;
        std::string button_b_SNES;
        std::string button_x_SNES;
        std::string button_y_SNES;
        std::string button_back_SNES;
        std::string button_start_SNES;

        // Switch Pro
        std::string button_a_switch;
        std::string button_b_switch;
        std::string button_x_switch;
        std::string button_y_switch;
        std::string button_back_switch;
        std::string button_start_switch;

        // PlayStation
        std::string button_a_PS;
        std::string button_b_PS;
        std::string button_x_PS;
        std::string button_y_PS;
        std::string button_back_PS123;
        std::string button_start_PS123;
        std::string button_back_PS4;
        std::string button_start_PS4;
        std::string button_back_PS5;
        std::string button_start_PS5;

        // XBOX
        std::string button_a_XBOX;
        std::string button_b_XBOX;
        std::string button_x_XBOX;
        std::string button_y_XBOX;
        std::string button_back_XBOX;
        std::string button_start_XBOX;
        std::string button_back_XBOX360;
        std::string button_start_XBOX360;
    };

    CustomButtonIcons mCustomButtons;
};

#endif // ES_CORE_COMPONENTS_HELP_COMPONENT_H
