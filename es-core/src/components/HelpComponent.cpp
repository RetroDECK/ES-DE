//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  HelpComponent.cpp
//
//  Help information in icon and text pairs.
//

#include "components/HelpComponent.h"

#include "Settings.h"
#include "Window.h"
#include "components/ImageComponent.h"
#include "components/TextComponent.h"
#include "utils/MathUtil.h"

#define PREFIX "button_"

HelpComponent::HelpComponent(std::shared_ptr<Font> font)
    : mRenderer {Renderer::getInstance()}
    , mFont {font}
    , mFontDimmed {font}
    , mHelpPosition {glm::vec2 {Renderer::getScreenWidth() * 0.012f,
                                Renderer::getScreenHeight() *
                                    (Renderer::getIsVerticalOrientation() ? 0.975f : 0.9515f)}}
    , mHelpPositionDimmed {mHelpPosition}
    , mHelpOrigin {glm::vec2 {0.0f, 0.0f}}
    , mHelpOriginDimmed {mHelpOrigin}
    , mHelpRotationOrigin {0.5f, 0.5f}
    , mTextColor {0x777777FF}
    , mTextColorDimmed {0x777777FF}
    , mIconColor {0x777777FF}
    , mIconColorDimmed {0x777777FF}
    , mBackgroundColor {0x00000000}
    , mBackgroundColorEnd {0x00000000}
    , mBackgroundHorizontalPadding {0.0f, 0.0f}
    , mBackgroundVerticalPadding {0.0f, 0.0f}
    , mBackgroundCornerRadius {0.0f}
    , mColorGradientHorizontal {true}
    , mEntryLayout {EntryLayout::ICON_FIRST}
    , mEntryRelativeScale {1.0f}
    , mLetterHeight {mFont->getLetterHeight() * 1.25f}
    , mLetterHeightDimmed {mLetterHeight}
    , mHelpRotation {0.0f}
    , mEntrySpacing {0.00833f}
    , mEntrySpacingDimmed {mEntrySpacing}
    , mIconTextSpacing {0.00416f}
    , mIconTextSpacingDimmed {mIconTextSpacing}
    , mHelpOpacity {1.0f}
    , mHelpOpacityDimmed {mHelpOpacity}
    , mLetterCase {"uppercase"}
{
    assignIcons();
    updateGrid();
}

void HelpComponent::clearPrompts()
{
    mPrompts.clear();
    updateGrid();
}

void HelpComponent::setPrompts(const std::vector<HelpPrompt>& prompts)
{
    mPrompts = prompts;
    assignIcons();
    updateGrid();
}

void HelpComponent::setOpacity(float opacity)
{
    if (!mGrid)
        return;

    GuiComponent::setOpacity(opacity *
                             (mWindow->isBackgroundDimmed() ? mHelpOpacityDimmed : mHelpOpacity));

    for (unsigned int i {0}; i < mGrid->getChildCount(); ++i)
        mGrid->getChild(i)->setOpacity(
            opacity * (mWindow->isBackgroundDimmed() ? mHelpOpacityDimmed : mHelpOpacity));
}

void HelpComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
                               const std::string& view,
                               const std::string& element,
                               unsigned int properties)
{
    const ThemeData::ThemeElement* elem {theme->getElement(view, element, "helpsystem")};

    if (!elem)
        return;

    if (elem->has("pos"))
        mHelpPosition = elem->get<glm::vec2>("pos") *
                        glm::vec2 {Renderer::getScreenWidth(), Renderer::getScreenHeight()};

    if (elem->has("posDimmed"))
        mHelpPositionDimmed = elem->get<glm::vec2>("posDimmed") *
                              glm::vec2 {Renderer::getScreenWidth(), Renderer::getScreenHeight()};
    else
        mHelpPositionDimmed = mHelpPosition;

    if (elem->has("origin"))
        mHelpOrigin = elem->get<glm::vec2>("origin");

    if (elem->has("originDimmed"))
        mHelpOriginDimmed = elem->get<glm::vec2>("originDimmed");
    else
        mHelpOriginDimmed = mHelpOrigin;

    if (elem->has("rotation"))
        mHelpRotation = static_cast<float>(glm::radians(elem->get<float>("rotation")));

    if (elem->has("rotationOrigin"))
        mHelpRotationOrigin = glm::clamp(elem->get<glm::vec2>("rotationOrigin"), 0.0f, 1.0f);

    if (elem->has("textColor"))
        mTextColor = elem->get<unsigned int>("textColor");

    if (elem->has("textColorDimmed"))
        mTextColorDimmed = elem->get<unsigned int>("textColorDimmed");
    else
        mTextColorDimmed = mTextColor;

    if (elem->has("iconColor"))
        mIconColor = elem->get<unsigned int>("iconColor");

    if (elem->has("iconColorDimmed"))
        mIconColorDimmed = elem->get<unsigned int>("iconColorDimmed");
    else
        mIconColorDimmed = mIconColor;

    if (elem->has("backgroundColor")) {
        mBackgroundColor = elem->get<unsigned int>("backgroundColor");

        if (elem->has("backgroundColorEnd"))
            mBackgroundColorEnd = elem->get<unsigned int>("backgroundColorEnd");
        else
            mBackgroundColorEnd = mBackgroundColor;

        if (elem->has("backgroundGradientType")) {
            const std::string& backgroundGradientType {
                elem->get<std::string>("backgroundGradientType")};
            if (backgroundGradientType == "horizontal") {
                mColorGradientHorizontal = true;
            }
            else if (backgroundGradientType == "vertical") {
                mColorGradientHorizontal = false;
            }
            else {
                mColorGradientHorizontal = true;
                LOG(LogWarning) << "HelpComponent: Invalid theme configuration, property "
                                   "\"backgroundGradientType\" for element \""
                                << element.substr(11) << "\" defined as \""
                                << backgroundGradientType << "\"";
            }
        }
    }

    if (elem->has("backgroundHorizontalPadding")) {
        const glm::vec2 backgroundHorizontalPadding {
            glm::clamp(elem->get<glm::vec2>("backgroundHorizontalPadding"), 0.0f, 1.0f)};
        mBackgroundHorizontalPadding.x =
            backgroundHorizontalPadding.x * mRenderer->getScreenWidth();
        mBackgroundHorizontalPadding.y =
            backgroundHorizontalPadding.y * mRenderer->getScreenWidth();
    }

    if (elem->has("backgroundVerticalPadding")) {
        const glm::vec2 backgroundVerticalPadding {
            glm::clamp(elem->get<glm::vec2>("backgroundVerticalPadding"), 0.0f, 1.0f)};
        mBackgroundVerticalPadding.x = backgroundVerticalPadding.x * mRenderer->getScreenHeight();
        mBackgroundVerticalPadding.y = backgroundVerticalPadding.y * mRenderer->getScreenHeight();
    }

    if (elem->has("backgroundCornerRadius")) {
        mBackgroundCornerRadius =
            glm::clamp(elem->get<float>("backgroundCornerRadius"), 0.0f, 0.5f) *
            mRenderer->getScreenWidth();
    }

    if (elem->has("entryRelativeScale"))
        mEntryRelativeScale = glm::clamp(elem->get<float>("entryRelativeScale"), 0.2f, 3.0f);

    if (elem->has("fontPath") || elem->has("fontSize")) {
        mFont = Font::getFromTheme(elem, ThemeFlags::ALL, mFont, 0.0f, 1.0f, false, true);
        mLetterHeight = mFont->getLetterHeight() * 1.25f;
        if (!elem->has("fontSizeDimmed")) {
            mFontDimmed = Font::getFromTheme(
                elem, ThemeFlags::ALL, mFont, 0.0f,
                (mEntryRelativeScale < 1.0f ? mEntryRelativeScale : 1.0f), true, true);
            mLetterHeightDimmed = mLetterHeight;
        }
        if (mEntryRelativeScale < 1.0f)
            mFont = Font::getFromTheme(elem, ThemeFlags::ALL, mFont, 0.0f, mEntryRelativeScale,
                                       false, true);
    }
    else if (mEntryRelativeScale < 1.0f) {
        mFont = Font::getFromTheme(elem, ThemeFlags::ALL, mFont, 0.0f, mEntryRelativeScale, false,
                                   true);
    }

    if (elem->has("fontSizeDimmed")) {
        mFontDimmed = Font::getFromTheme(elem, ThemeFlags::ALL, mFont, 0.0f, 1.0f, true, true);
        mLetterHeightDimmed = mFontDimmed->getLetterHeight() * 1.25f;
        if (mEntryRelativeScale < 1.0f)
            mFontDimmed = Font::getFromTheme(elem, ThemeFlags::ALL, mFont, 0.0f,
                                             mEntryRelativeScale, true, true);
    }
    else if (mEntryRelativeScale < 1.0f && !elem->has("fontPath") && !elem->has("fontSize")) {
        mFontDimmed = mFont;
    }

    if (elem->has("scope")) {
        const std::string& scope {elem->get<std::string>("scope")};
        if (scope == "shared") {
            mComponentScope = ComponentScope::SHARED;
        }
        else if (scope == "view") {
            mComponentScope = ComponentScope::VIEW;
        }
        else if (scope == "menu") {
            mComponentScope = ComponentScope::MENU;
        }
        else if (scope == "none") {
            mComponentScope = ComponentScope::NONE;
        }
        else {
            LOG(LogWarning) << "HelpComponent: Invalid theme configuration, property "
                               "\"scope\" for element \""
                            << element.substr(11) << "\" defined as \"" << scope << "\"";
        }
    }

    if (elem->has("entries")) {
        // Replace possible whitespace separators with commas.
        std::string entriesTag {Utils::String::toLower(elem->get<std::string>("entries"))};
        for (auto& character : entriesTag) {
            if (std::isspace(character))
                character = ',';
        }
        entriesTag = Utils::String::replace(entriesTag, ",,", ",");
        std::vector<std::string> entries {Utils::String::delimitedStringToVector(entriesTag, ",")};

        // If the "all" value has been set then leave mEntries blank (allow all entries).
        if (std::find(entries.begin(), entries.end(), "all") == entries.end()) {
            for (auto& allowedEntry : sAllowedEntries) {
                if (std::find(entries.cbegin(), entries.cend(), allowedEntry) != entries.cend())
                    mEntries.emplace_back(allowedEntry);
            }
        }
    }

    if (elem->has("entryLayout")) {
        const std::string& entryLayout {elem->get<std::string>("entryLayout")};
        if (entryLayout == "iconFirst") {
            mEntryLayout = EntryLayout::ICON_FIRST;
        }
        else if (entryLayout == "textFirst") {
            mEntryLayout = EntryLayout::TEXT_FIRST;
        }
        else {
            LOG(LogWarning) << "HelpComponent: Invalid theme configuration, property "
                               "\"entryLayout\" for element \""
                            << element.substr(11) << "\" defined as \"" << entryLayout << "\"";
        }
    }

    if (elem->has("entrySpacing"))
        mEntrySpacing = glm::clamp(elem->get<float>("entrySpacing"), 0.0f, 0.04f);

    if (elem->has("entrySpacingDimmed"))
        mEntrySpacingDimmed = glm::clamp(elem->get<float>("entrySpacingDimmed"), 0.0f, 0.04f);
    else
        mEntrySpacingDimmed = mEntrySpacing;

    if (elem->has("iconTextSpacing"))
        mIconTextSpacing = glm::clamp(elem->get<float>("iconTextSpacing"), 0.0f, 0.04f);

    if (elem->has("iconTextSpacingDimmed"))
        mIconTextSpacingDimmed = glm::clamp(elem->get<float>("iconTextSpacingDimmed"), 0.0f, 0.04f);
    else
        mIconTextSpacingDimmed = mIconTextSpacing;

    if (elem->has("letterCase"))
        mLetterCase = elem->get<std::string>("letterCase");

    if (elem->has("opacity"))
        mHelpOpacity = glm::clamp(elem->get<float>("opacity"), 0.2f, 1.0f);

    if (elem->has("opacityDimmed"))
        mHelpOpacityDimmed = glm::clamp(elem->get<float>("opacityDimmed"), 0.2f, 1.0f);
    else
        mHelpOpacityDimmed = mHelpOpacity;

    // Load custom button icons.
    // The names may look a bit strange when combined with the PREFIX string "button_" but it's
    // because ThemeData adds this prefix to avoid name collisions when using XML attributes.

    // General.
    if (elem->has(PREFIX "dpad_updown"))
        mCustomButtons.dpad_updown = elem->get<std::string>(PREFIX "dpad_updown");
    if (elem->has(PREFIX "dpad_leftright"))
        mCustomButtons.dpad_leftright = elem->get<std::string>(PREFIX "dpad_leftright");
    if (elem->has(PREFIX "dpad_all"))
        mCustomButtons.dpad_all = elem->get<std::string>(PREFIX "dpad_all");
    if (elem->has(PREFIX "thumbstick_click"))
        mCustomButtons.thumbstick_click = elem->get<std::string>(PREFIX "thumbstick_click");
    if (elem->has(PREFIX "button_l"))
        mCustomButtons.button_l = elem->get<std::string>(PREFIX "button_l");
    if (elem->has(PREFIX "button_r"))
        mCustomButtons.button_r = elem->get<std::string>(PREFIX "button_r");
    if (elem->has(PREFIX "button_lr"))
        mCustomButtons.button_lr = elem->get<std::string>(PREFIX "button_lr");
    if (elem->has(PREFIX "button_lt"))
        mCustomButtons.button_lt = elem->get<std::string>(PREFIX "button_lt");
    if (elem->has(PREFIX "button_rt"))
        mCustomButtons.button_rt = elem->get<std::string>(PREFIX "button_rt");
    if (elem->has(PREFIX "button_ltrt"))
        mCustomButtons.button_ltrt = elem->get<std::string>(PREFIX "button_ltrt");

    // SNES.
    if (elem->has(PREFIX "button_a_SNES"))
        mCustomButtons.button_a_SNES = elem->get<std::string>(PREFIX "button_a_SNES");
    if (elem->has(PREFIX "button_b_SNES"))
        mCustomButtons.button_b_SNES = elem->get<std::string>(PREFIX "button_b_SNES");
    if (elem->has(PREFIX "button_x_SNES"))
        mCustomButtons.button_x_SNES = elem->get<std::string>(PREFIX "button_x_SNES");
    if (elem->has(PREFIX "button_y_SNES"))
        mCustomButtons.button_y_SNES = elem->get<std::string>(PREFIX "button_y_SNES");
    if (elem->has(PREFIX "button_back_SNES"))
        mCustomButtons.button_back_SNES = elem->get<std::string>(PREFIX "button_back_SNES");
    if (elem->has(PREFIX "button_start_SNES"))
        mCustomButtons.button_start_SNES = elem->get<std::string>(PREFIX "button_start_SNES");

    // Switch Pro.
    if (elem->has(PREFIX "button_a_switch"))
        mCustomButtons.button_a_switch = elem->get<std::string>(PREFIX "button_a_switch");
    if (elem->has(PREFIX "button_b_switch"))
        mCustomButtons.button_b_switch = elem->get<std::string>(PREFIX "button_b_switch");
    if (elem->has(PREFIX "button_x_switch"))
        mCustomButtons.button_x_switch = elem->get<std::string>(PREFIX "button_x_switch");
    if (elem->has(PREFIX "button_y_switch"))
        mCustomButtons.button_y_switch = elem->get<std::string>(PREFIX "button_y_switch");
    if (elem->has(PREFIX "button_back_switch"))
        mCustomButtons.button_back_switch = elem->get<std::string>(PREFIX "button_back_switch");
    if (elem->has(PREFIX "button_start_switch"))
        mCustomButtons.button_start_switch = elem->get<std::string>(PREFIX "button_start_switch");

    // PlayStation.
    if (elem->has(PREFIX "button_a_PS"))
        mCustomButtons.button_a_PS = elem->get<std::string>(PREFIX "button_a_PS");
    if (elem->has(PREFIX "button_b_PS"))
        mCustomButtons.button_b_PS = elem->get<std::string>(PREFIX "button_b_PS");
    if (elem->has(PREFIX "button_x_PS"))
        mCustomButtons.button_x_PS = elem->get<std::string>(PREFIX "button_x_PS");
    if (elem->has(PREFIX "button_y_PS"))
        mCustomButtons.button_y_PS = elem->get<std::string>(PREFIX "button_y_PS");
    if (elem->has(PREFIX "button_back_PS123"))
        mCustomButtons.button_back_PS123 = elem->get<std::string>(PREFIX "button_back_PS123");
    if (elem->has(PREFIX "button_start_PS123"))
        mCustomButtons.button_start_PS123 = elem->get<std::string>(PREFIX "button_start_PS123");
    if (elem->has(PREFIX "button_back_PS4"))
        mCustomButtons.button_back_PS4 = elem->get<std::string>(PREFIX "button_back_PS4");
    if (elem->has(PREFIX "button_start_PS4"))
        mCustomButtons.button_start_PS4 = elem->get<std::string>(PREFIX "button_start_PS4");
    if (elem->has(PREFIX "button_back_PS5"))
        mCustomButtons.button_back_PS5 = elem->get<std::string>(PREFIX "button_back_PS5");
    if (elem->has(PREFIX "button_start_PS5"))
        mCustomButtons.button_start_PS5 = elem->get<std::string>(PREFIX "button_start_PS5");

    // XBOX.
    if (elem->has(PREFIX "button_a_XBOX"))
        mCustomButtons.button_a_XBOX = elem->get<std::string>(PREFIX "button_a_XBOX");
    if (elem->has(PREFIX "button_b_XBOX"))
        mCustomButtons.button_b_XBOX = elem->get<std::string>(PREFIX "button_b_XBOX");
    if (elem->has(PREFIX "button_x_XBOX"))
        mCustomButtons.button_x_XBOX = elem->get<std::string>(PREFIX "button_x_XBOX");
    if (elem->has(PREFIX "button_y_XBOX"))
        mCustomButtons.button_y_XBOX = elem->get<std::string>(PREFIX "button_y_XBOX");
    if (elem->has(PREFIX "button_back_XBOX"))
        mCustomButtons.button_back_XBOX = elem->get<std::string>(PREFIX "button_back_XBOX");
    if (elem->has(PREFIX "button_start_XBOX"))
        mCustomButtons.button_start_XBOX = elem->get<std::string>(PREFIX "button_start_XBOX");
    if (elem->has(PREFIX "button_back_XBOX360"))
        mCustomButtons.button_back_XBOX360 = elem->get<std::string>(PREFIX "button_back_XBOX360");
    if (elem->has(PREFIX "button_start_XBOX360"))
        mCustomButtons.button_start_XBOX360 = elem->get<std::string>(PREFIX "button_start_XBOX360");

    assignIcons();
}

void HelpComponent::render(const glm::mat4& parentTrans)
{
    if (!mVisible || mGrid == nullptr)
        return;

    if (mBackgroundColor != 0x00000000) {
        mPosition = mGrid->getPosition();
        mSize = mGrid->getSize();
        mOrigin = mGrid->getOrigin();
        mRotation = mHelpRotation;
        mRotationOrigin = mHelpRotationOrigin;

        glm::mat4 trans {parentTrans * getTransform()};
        trans = glm::translate(trans, glm::vec3 {-mBackgroundHorizontalPadding.x,
                                                 -mBackgroundVerticalPadding.x, 0.0f});
        mRenderer->setMatrix(trans);

        mRenderer->drawRect(0.0f, 0.0f,
                            mSize.x + mBackgroundHorizontalPadding.x +
                                mBackgroundHorizontalPadding.y -
                                (mEntrySpacing * mRenderer->getScreenWidth()),
                            mSize.y + mBackgroundVerticalPadding.x + mBackgroundVerticalPadding.y,
                            mBackgroundColor, mBackgroundColorEnd, mColorGradientHorizontal,
                            mThemeOpacity, 1.0f, Renderer::BlendFactor::SRC_ALPHA,
                            Renderer::BlendFactor::ONE_MINUS_SRC_ALPHA, mBackgroundCornerRadius);

        mPosition = {0.0f, 0.0f, 0.0f};
        mSize = {0.0f, 0.0f};
        mOrigin = {0.0f, 0.0f};
        mRotation = 0.0f;
        mRotationOrigin = {0.5f, 0.5f};
    }

    const glm::mat4 trans {parentTrans * getTransform()};

    if (mGrid) {
        mGrid->setRotationOrigin(mHelpRotationOrigin);
        mGrid->setRotation(mHelpRotation);
        mGrid->render(trans);
    }
}

void HelpComponent::assignIcons()
{
    std::string controllerType {Settings::getInstance()->getString("InputControllerType")};

    mIconPathMap.clear();

    std::string buttonA {"a"};
    std::string buttonB {"b"};
    std::string buttonX {"x"};
    std::string buttonY {"y"};

    if (Settings::getInstance()->getBool("InputSwapButtons")) {
        buttonA = "b";
        buttonB = "a";
        buttonX = "y";
        buttonY = "x";
    }

    // These graphics files are common between all controller types.
    mIconPathMap["up/down"] = mCustomButtons.dpad_updown.empty() ?
                                  ":/graphics/help/dpad_updown.svg" :
                                  mCustomButtons.dpad_updown;
    mIconPathMap["up"] =
        mCustomButtons.dpad_up.empty() ? ":/graphics/help/dpad_up.svg" : mCustomButtons.dpad_up;
    mIconPathMap["down"] = mCustomButtons.dpad_down.empty() ? ":/graphics/help/dpad_down.svg" :
                                                              mCustomButtons.dpad_down;
    mIconPathMap["left/right"] = mCustomButtons.dpad_leftright.empty() ?
                                     ":/graphics/help/dpad_leftright.svg" :
                                     mCustomButtons.dpad_leftright;
    mIconPathMap["up/down/left/right"] =
        mCustomButtons.dpad_all.empty() ? ":/graphics/help/dpad_all.svg" : mCustomButtons.dpad_all;
    mIconPathMap["thumbstickclick"] = mCustomButtons.thumbstick_click.empty() ?
                                          ":/graphics/help/thumbstick_click.svg" :
                                          mCustomButtons.thumbstick_click;
    mIconPathMap["l"] =
        mCustomButtons.button_l.empty() ? ":/graphics/help/button_l.svg" : mCustomButtons.button_l;
    mIconPathMap["r"] =
        mCustomButtons.button_r.empty() ? ":/graphics/help/button_r.svg" : mCustomButtons.button_r;
    mIconPathMap["lr"] = mCustomButtons.button_lr.empty() ? ":/graphics/help/button_lr.svg" :
                                                            mCustomButtons.button_lr;
    mIconPathMap["lt"] = mCustomButtons.button_lt.empty() ? ":/graphics/help/button_lt.svg" :
                                                            mCustomButtons.button_lt;
    mIconPathMap["rt"] = mCustomButtons.button_rt.empty() ? ":/graphics/help/button_rt.svg" :
                                                            mCustomButtons.button_rt;
    mIconPathMap["ltrt"] = mCustomButtons.button_ltrt.empty() ? ":/graphics/help/button_ltrt.svg" :
                                                                mCustomButtons.button_ltrt;

    // These graphics files are custom per controller type.
    if (controllerType == "snes") {
        mIconPathMap[buttonA] = mCustomButtons.button_a_SNES.empty() ?
                                    ":/graphics/help/button_a_SNES.svg" :
                                    mCustomButtons.button_a_SNES;
        mIconPathMap[buttonB] = mCustomButtons.button_b_SNES.empty() ?
                                    ":/graphics/help/button_b_SNES.svg" :
                                    mCustomButtons.button_b_SNES;
        mIconPathMap[buttonX] = mCustomButtons.button_x_SNES.empty() ?
                                    ":/graphics/help/button_x_SNES.svg" :
                                    mCustomButtons.button_x_SNES;
        mIconPathMap[buttonY] = mCustomButtons.button_y_SNES.empty() ?
                                    ":/graphics/help/button_y_SNES.svg" :
                                    mCustomButtons.button_y_SNES;
        mIconPathMap["back"] = mCustomButtons.button_back_SNES.empty() ?
                                   ":/graphics/help/button_back_SNES.svg" :
                                   mCustomButtons.button_back_SNES;
        mIconPathMap["start"] = mCustomButtons.button_start_SNES.empty() ?
                                    ":/graphics/help/button_start_SNES.svg" :
                                    mCustomButtons.button_start_SNES;
    }
    else if (controllerType == "switchpro") {
        mIconPathMap[buttonA] = mCustomButtons.button_a_switch.empty() ?
                                    ":/graphics/help/button_a_switch.svg" :
                                    mCustomButtons.button_a_switch;
        mIconPathMap[buttonB] = mCustomButtons.button_b_switch.empty() ?
                                    ":/graphics/help/button_b_switch.svg" :
                                    mCustomButtons.button_b_switch;
        mIconPathMap[buttonX] = mCustomButtons.button_x_switch.empty() ?
                                    ":/graphics/help/button_x_switch.svg" :
                                    mCustomButtons.button_x_switch;
        mIconPathMap[buttonY] = mCustomButtons.button_y_switch.empty() ?
                                    ":/graphics/help/button_y_switch.svg" :
                                    mCustomButtons.button_y_switch;
        mIconPathMap["back"] = mCustomButtons.button_back_switch.empty() ?
                                   ":/graphics/help/button_back_switch.svg" :
                                   mCustomButtons.button_back_switch;
        mIconPathMap["start"] = mCustomButtons.button_start_switch.empty() ?
                                    ":/graphics/help/button_start_switch.svg" :
                                    mCustomButtons.button_start_switch;
    }
    else if (controllerType == "ps123") {
        mIconPathMap[buttonA] = mCustomButtons.button_a_PS.empty() ?
                                    ":/graphics/help/button_a_PS.svg" :
                                    mCustomButtons.button_a_PS;
        mIconPathMap[buttonB] = mCustomButtons.button_b_PS.empty() ?
                                    ":/graphics/help/button_b_PS.svg" :
                                    mCustomButtons.button_b_PS;
        mIconPathMap[buttonX] = mCustomButtons.button_x_PS.empty() ?
                                    ":/graphics/help/button_x_PS.svg" :
                                    mCustomButtons.button_x_PS;
        mIconPathMap[buttonY] = mCustomButtons.button_y_PS.empty() ?
                                    ":/graphics/help/button_y_PS.svg" :
                                    mCustomButtons.button_y_PS;
        mIconPathMap["back"] = mCustomButtons.button_back_PS123.empty() ?
                                   ":/graphics/help/button_back_PS123.svg" :
                                   mCustomButtons.button_back_PS123;
        mIconPathMap["start"] = mCustomButtons.button_start_PS123.empty() ?
                                    ":/graphics/help/button_start_PS123.svg" :
                                    mCustomButtons.button_start_PS123;
    }
    else if (controllerType == "ps4") {
        mIconPathMap[buttonA] = mCustomButtons.button_a_PS.empty() ?
                                    ":/graphics/help/button_a_PS.svg" :
                                    mCustomButtons.button_a_PS;
        mIconPathMap[buttonB] = mCustomButtons.button_b_PS.empty() ?
                                    ":/graphics/help/button_b_PS.svg" :
                                    mCustomButtons.button_b_PS;
        mIconPathMap[buttonX] = mCustomButtons.button_x_PS.empty() ?
                                    ":/graphics/help/button_x_PS.svg" :
                                    mCustomButtons.button_x_PS;
        mIconPathMap[buttonY] = mCustomButtons.button_y_PS.empty() ?
                                    ":/graphics/help/button_y_PS.svg" :
                                    mCustomButtons.button_y_PS;
        mIconPathMap["back"] = mCustomButtons.button_back_PS4.empty() ?
                                   ":/graphics/help/button_back_PS4.svg" :
                                   mCustomButtons.button_back_PS4;
        mIconPathMap["start"] = mCustomButtons.button_start_PS4.empty() ?
                                    ":/graphics/help/button_start_PS4.svg" :
                                    mCustomButtons.button_start_PS4;
    }
    else if (controllerType == "ps5") {
        mIconPathMap[buttonA] = mCustomButtons.button_a_PS.empty() ?
                                    ":/graphics/help/button_a_PS.svg" :
                                    mCustomButtons.button_a_PS;
        mIconPathMap[buttonB] = mCustomButtons.button_b_PS.empty() ?
                                    ":/graphics/help/button_b_PS.svg" :
                                    mCustomButtons.button_b_PS;
        mIconPathMap[buttonX] = mCustomButtons.button_x_PS.empty() ?
                                    ":/graphics/help/button_x_PS.svg" :
                                    mCustomButtons.button_x_PS;
        mIconPathMap[buttonY] = mCustomButtons.button_y_PS.empty() ?
                                    ":/graphics/help/button_y_PS.svg" :
                                    mCustomButtons.button_y_PS;
        mIconPathMap["back"] = mCustomButtons.button_back_PS5.empty() ?
                                   ":/graphics/help/button_back_PS5.svg" :
                                   mCustomButtons.button_back_PS5;
        mIconPathMap["start"] = mCustomButtons.button_start_PS5.empty() ?
                                    ":/graphics/help/button_start_PS5.svg" :
                                    mCustomButtons.button_start_PS5;
    }
    else if (controllerType == "xbox360") {

        mIconPathMap[buttonA] = mCustomButtons.button_a_XBOX.empty() ?
                                    ":/graphics/help/button_a_XBOX.svg" :
                                    mCustomButtons.button_a_XBOX;
        mIconPathMap[buttonB] = mCustomButtons.button_b_XBOX.empty() ?
                                    ":/graphics/help/button_b_XBOX.svg" :
                                    mCustomButtons.button_b_XBOX;
        mIconPathMap[buttonX] = mCustomButtons.button_x_XBOX.empty() ?
                                    ":/graphics/help/button_x_XBOX.svg" :
                                    mCustomButtons.button_x_XBOX;
        mIconPathMap[buttonY] = mCustomButtons.button_y_XBOX.empty() ?
                                    ":/graphics/help/button_y_XBOX.svg" :
                                    mCustomButtons.button_y_XBOX;
        mIconPathMap["back"] = mCustomButtons.button_back_XBOX360.empty() ?
                                   ":/graphics/help/button_back_XBOX360.svg" :
                                   mCustomButtons.button_back_XBOX360;
        mIconPathMap["start"] = mCustomButtons.button_start_XBOX360.empty() ?
                                    ":/graphics/help/button_start_XBOX360.svg" :
                                    mCustomButtons.button_start_XBOX360;
    }
    else {
        // Xbox One and later.
        mIconPathMap[buttonA] = mCustomButtons.button_a_XBOX.empty() ?
                                    ":/graphics/help/button_a_XBOX.svg" :
                                    mCustomButtons.button_a_XBOX;
        mIconPathMap[buttonB] = mCustomButtons.button_b_XBOX.empty() ?
                                    ":/graphics/help/button_b_XBOX.svg" :
                                    mCustomButtons.button_b_XBOX;
        mIconPathMap[buttonX] = mCustomButtons.button_x_XBOX.empty() ?
                                    ":/graphics/help/button_x_XBOX.svg" :
                                    mCustomButtons.button_x_XBOX;
        mIconPathMap[buttonY] = mCustomButtons.button_y_XBOX.empty() ?
                                    ":/graphics/help/button_y_XBOX.svg" :
                                    mCustomButtons.button_y_XBOX;
        mIconPathMap["back"] = mCustomButtons.button_back_XBOX.empty() ?
                                   ":/graphics/help/button_back_XBOX.svg" :
                                   mCustomButtons.button_back_XBOX;
        mIconPathMap["start"] = mCustomButtons.button_start_XBOX.empty() ?
                                    ":/graphics/help/button_start_XBOX.svg" :
                                    mCustomButtons.button_start_XBOX;
    }
}

void HelpComponent::updateGrid()
{
    if (!Settings::getInstance()->getBool("ShowHelpPrompts") || mPrompts.empty()) {
        mGrid.reset();
        return;
    }

    const bool isDimmed {mWindow->isBackgroundDimmed()};

    std::shared_ptr<Font>& font {isDimmed ? mFontDimmed : mFont};
    mGrid = std::make_shared<ComponentGrid>(glm::ivec2 {static_cast<int>(mPrompts.size()) * 5, 1});

    std::vector<std::shared_ptr<ImageComponent>> icons;
    std::vector<std::shared_ptr<TextComponent>> labels;

    float width {0.0f};
    const float height {isDimmed ? mLetterHeightDimmed : mLetterHeight};

    for (auto it = mPrompts.cbegin(); it != mPrompts.cend(); ++it) {
        if (!mEntries.empty() &&
            std::find(mEntries.cbegin(), mEntries.cend(), (*it).first) == mEntries.cend())
            continue;

        std::shared_ptr<ImageComponent> icon;
        auto& imageCache = mWindow->getHelpPromptsImageCache();

        if (imageCache.find(mIconPathMap[it->first]) != imageCache.end()) {
            icon = imageCache[mIconPathMap[it->first]];
        }
        else {
            icon = std::make_shared<ImageComponent>(false, true);
            icon->setImage(mIconPathMap[it->first]);
            imageCache[mIconPathMap[it->first]] = icon;
        }

        icon->setColorShift(isDimmed ? mIconColorDimmed : mIconColor);

        if (mEntryRelativeScale < 1.0f)
            icon->setResize(0, height);
        else
            icon->setResize(0, height / mEntryRelativeScale);

        icon->setOpacity(isDimmed ? mHelpOpacityDimmed : mHelpOpacity);
        icons.push_back(icon);

        // Apply text style and color from the theme to the label and add it to the label list.
        std::string lblInput {it->second};
        if (mLetterCase == "lowercase")
            lblInput = Utils::String::toLower(lblInput);
        else if (mLetterCase == "capitalize")
            lblInput = Utils::String::toCapitalized(lblInput);
        else
            lblInput = Utils::String::toUpper(lblInput);
        auto lbl = std::make_shared<TextComponent>(lblInput, font,
                                                   isDimmed ? mTextColorDimmed : mTextColor);
        lbl->setOpacity(isDimmed ? mHelpOpacityDimmed : mHelpOpacity);
        labels.push_back(lbl);

        width +=
            icon->getSize().x + lbl->getSize().x +
            (((isDimmed ? mIconTextSpacingDimmed : mIconTextSpacing) * mRenderer->getScreenWidth() +
              (isDimmed ? mEntrySpacingDimmed : mEntrySpacing) * mRenderer->getScreenWidth()));
    }

    mGrid->setSize(width, height);

    if (mEntryLayout == EntryLayout::ICON_FIRST) {
        for (int i {0}; i < static_cast<int>(icons.size()); ++i) {
            const int col {i * 5};
            mGrid->setColWidthPerc(col, icons.at(i)->getSize().x / width);
            mGrid->setColWidthPerc(col + 1,
                                   ((isDimmed ? mIconTextSpacingDimmed : mIconTextSpacing) *
                                    mRenderer->getScreenWidth()) /
                                       width);
            mGrid->setColWidthPerc(col + 2, labels.at(i)->getSize().x / width);
            mGrid->setColWidthPerc(col + 3, ((isDimmed ? mEntrySpacingDimmed : mEntrySpacing) *
                                             mRenderer->getScreenWidth()) /
                                                width);

            mGrid->setEntry(icons.at(i), glm::ivec2 {col, 0}, false, false);
            mGrid->setEntry(labels.at(i), glm::ivec2 {col + 2, 0}, false, false);
        }
    }
    else {
        for (int i {0}; i < static_cast<int>(icons.size()); ++i) {
            const int col {i * 5};
            mGrid->setColWidthPerc(col, labels.at(i)->getSize().x / width);
            mGrid->setColWidthPerc(col + 1,
                                   ((isDimmed ? mIconTextSpacingDimmed : mIconTextSpacing) *
                                    mRenderer->getScreenWidth()) /
                                       width);
            mGrid->setColWidthPerc(col + 2, icons.at(i)->getSize().x / width);
            mGrid->setColWidthPerc(col + 3, ((isDimmed ? mEntrySpacingDimmed : mEntrySpacing) *
                                             mRenderer->getScreenWidth()) /
                                                width);

            mGrid->setEntry(labels.at(i), glm::ivec2 {col, 0}, false, false);
            mGrid->setEntry(icons.at(i), glm::ivec2 {col + 2, 0}, false, false);
        }
    }

    if (isDimmed) {
        mGrid->setPosition(
            {mHelpPositionDimmed.x +
                 ((mEntrySpacingDimmed * mRenderer->getScreenWidth()) * mHelpOriginDimmed.x),
             mHelpPositionDimmed.y, 0.0f});
    }
    else {
        mGrid->setPosition(
            {mHelpPosition.x + ((mEntrySpacing * mRenderer->getScreenWidth()) * mHelpOrigin.x),
             mHelpPosition.y, 0.0f});
    }

    mGrid->setOrigin(isDimmed ? mHelpOriginDimmed : mHelpOrigin);
}
