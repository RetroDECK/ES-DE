//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiComponent.h
//
//  Basic GUI component handling such as placement, rotation, Z-order, rendering and animation.
//

#ifndef ES_CORE_GUI_COMPONENT_H
#define ES_CORE_GUI_COMPONENT_H

#include "HelpPrompt.h"
#include "HelpStyle.h"
#include "InputConfig.h"
#include "animations/AnimationController.h"

#include <functional>
#include <memory>

#define DISABLED_OPACITY 0.314f

class Animation;
class AnimationController;
class Font;
class InputConfig;
class TextCache;
class ThemeData;
class Window;

enum class ViewTransition {
    SYSTEM_TO_SYSTEM,
    SYSTEM_TO_GAMELIST,
    GAMELIST_TO_GAMELIST,
    GAMELIST_TO_SYSTEM,
    STARTUP_TO_SYSTEM,
    STARTUP_TO_GAMELIST
};

enum ViewTransitionAnimation {
    INSTANT,
    SLIDE,
    FADE
};

enum Alignment {
    ALIGN_LEFT,
    ALIGN_CENTER, // Used for both horizontal and vertical alignments.
    ALIGN_RIGHT,
    ALIGN_TOP,
    ALIGN_BOTTOM
};

enum class LetterCase {
    UPPERCASE,
    LOWERCASE,
    CAPITALIZE,
    NONE,
    UNDEFINED
};

enum class Stationary {
    NEVER,
    ALWAYS,
    WITHIN_VIEW,
    BETWEEN_VIEWS
};

class GuiComponent
{
public:
    GuiComponent();
    virtual ~GuiComponent();

    virtual void textInput(const std::string& text, const bool pasting = false);

    // Called when input is received.
    // Return true if the input is consumed, false if it should continue to be passed
    // to other children.
    virtual bool input(InputConfig* config, Input input);

    // Called when time passes.
    // Default implementation calls updateSelf(deltaTime) and updateChildren(deltaTime).
    // So you should probably call GuiComponent::update(deltaTime) at some point (or at
    // least updateSelf so animations work).
    virtual void update(int deltaTime);

    // Called when it's time to render.
    // By default, just calls renderChildren(parentTrans * getTransform())
    // Normally the following steps are required:
    // 1. Calculate the new transform that your component will draw at
    //    glm::mat4 trans{parentTrans * getTransform()};
    // 2. Set the renderer to use that new transform as the model matrix
    //    Renderer::setMatrix(trans);
    // 3. Draw your component
    // 4. Tell your children to render, based on your component's transform
    //    renderChildren(trans);
    virtual void render(const glm::mat4& parentTrans);

    glm::vec3 getPosition() const { return mPosition; }
    void setPosition(const glm::vec3& offset) { setPosition(offset.x, offset.y, offset.z); }
    void setPosition(float x, float y, float z = 0.0f);
    virtual void onPositionChanged() {}

    glm::vec2 getOrigin() const { return mOrigin; }
    // Sets the origin as a percentage of this image.
    // (e.g. (0, 0) is top left, (0.5, 0.5) is the center.)
    void setOrigin(float originX, float originY);
    void setOrigin(glm::vec2 origin) { setOrigin(origin.x, origin.y); }
    virtual void onOriginChanged() {}

    glm::vec2 getRotationOrigin() const { return mRotationOrigin; }
    // Sets the rotation origin as a percentage of this image.
    // (e.g. (0, 0) is top left, (0.5, 0.5) is the center.)
    void setRotationOrigin(float originX, float originY)
    {
        mRotationOrigin = glm::vec2 {originX, originY};
    }
    void setRotationOrigin(glm::vec2 origin) { setRotationOrigin(origin.x, origin.y); }

    const Stationary getStationary() const { return mStationary; }
    const bool getRenderDuringTransitions() const { return mRenderDuringTransitions; }

    virtual glm::vec2 getSize() const { return mSize; }
    void setSize(const glm::vec2& size) { setSize(size.x, size.y); }
    void setSize(const float w, const float h);
    virtual void setResize(const float width, const float height) {}
    virtual void setResize(const glm::vec2& size, bool rasterize = true) {}
    virtual void onSizeChanged() {}

    virtual glm::vec2 getRotationSize() const { return getSize(); }
    const float getRotation() const { return mRotation; }
    void setRotation(float rotation) { mRotation = rotation; }
    void setRotationDegrees(float rotation)
    {
        setRotation(static_cast<float>(glm::radians(rotation)));
    }

    const float getScale() const { return mScale; }
    void setScale(float scale) { mScale = scale; }

    const float getZIndex() const { return mZIndex; }
    void setZIndex(float zIndex) { mZIndex = zIndex; }

    const float getDefaultZIndex() const { return mDefaultZIndex; }
    void setDefaultZIndex(float zIndex) { mDefaultZIndex = zIndex; }

    const bool isVisible() const { return mVisible; }
    void setVisible(bool visible) { mVisible = visible; }

    // clang-format off
    enum ComponentThemeFlags : unsigned int {
        SCROLL_HIDE      = 0x00000001,
        SCROLL_FADE_IN   = 0x00000002,
        METADATA_ELEMENT = 0x00000004
    };
    // clang-format on

    const bool getScrollHide() { return mComponentThemeFlags & ComponentThemeFlags::SCROLL_HIDE; }
    void setScrollHide(bool state)
    {
        if (state)
            mComponentThemeFlags |= ComponentThemeFlags::SCROLL_HIDE;
        else
            mComponentThemeFlags ^= ComponentThemeFlags::SCROLL_HIDE;
    }
    const bool getScrollFadeIn()
    {
        return mComponentThemeFlags & ComponentThemeFlags::SCROLL_FADE_IN;
    }
    void setScrollFadeIn(bool state)
    {
        if (state)
            mComponentThemeFlags |= ComponentThemeFlags::SCROLL_FADE_IN;
        else
            mComponentThemeFlags ^= ComponentThemeFlags::SCROLL_FADE_IN;
    }
    const bool getMetadataElement()
    {
        return mComponentThemeFlags & ComponentThemeFlags::METADATA_ELEMENT;
    }
    void setMetadataElement(bool state)
    {
        if (state)
            mComponentThemeFlags |= ComponentThemeFlags::METADATA_ELEMENT;
        else
            mComponentThemeFlags ^= ComponentThemeFlags::METADATA_ELEMENT;
    }

    virtual const TextCache* getTextCache() { return nullptr; }
    virtual void setRemoveLineBreaks(bool state) {}
    virtual void setAutoCalcExtent(glm::ivec2 extent) {};

    // Returns the center point of the image (takes origin into account).
    const glm::vec2 getCenter() const;

    void setParent(GuiComponent* parent) { mParent = parent; }
    GuiComponent* getParent() const { return mParent; }

    void addChild(GuiComponent* cmp);
    void removeChild(GuiComponent* cmp);
    void clearChildren() { mChildren.clear(); }
    void sortChildren();
    const unsigned int getChildCount() const { return static_cast<int>(mChildren.size()); }
    const int getChildIndex() const;
    GuiComponent* getChild(unsigned int i) const
    {
        assert(mChildren.size() >= i);
        return mChildren.at(i);
    }

    // Animation will be automatically deleted when it completes or is stopped.
    const bool isAnimationPlaying(unsigned char slot) const
    {
        return mAnimationMap[slot] != nullptr;
    }
    const bool isAnimationReversed(unsigned char slot) const
    {
        assert(mAnimationMap[slot] != nullptr);
        return mAnimationMap[slot]->isReversed();
    }
    const int getAnimationTime(unsigned char slot) const
    {
        assert(mAnimationMap[slot] != nullptr);
        return mAnimationMap[slot]->getTime();
    }
    void setAnimation(Animation* animation,
                      int delay = 0,
                      std::function<void()> finishedCallback = nullptr,
                      bool reverse = false,
                      unsigned char slot = 0);
    const bool stopAnimation(unsigned char slot);
    // Like stopAnimation, but doesn't call finishedCallback - only removes the animation, leaving
    // things in their current state. Returns true if successful (an animation was in this slot).
    const bool cancelAnimation(unsigned char slot);
    // Calls update(1.f) and finishedCallback, then deletes the animation - basically skips
    // to the end.  Returns true if successful (an animation was in this slot).
    const bool finishAnimation(unsigned char slot);
    // Returns true if successful (an animation was in this slot).
    const bool advanceAnimation(unsigned char slot, unsigned int time);
    void stopAllAnimations();
    void cancelAllAnimations();

    virtual void stopGamelistFadeAnimations() {}
    virtual bool isListScrolling() { return false; }
    virtual void stopListScrolling() {}
    virtual const float getBrightness() const { return mBrightness; }
    virtual const float getOpacity() const { return mOpacity; }
    virtual const float getColorOpacity() const { return 1.0f; }
    virtual void setBrightness(float brightness);
    virtual void setOpacity(float opacity);
    virtual float getSaturation() const { return static_cast<float>(mColor); }
    virtual void setSaturation(float saturation) { mSaturation = saturation; }
    virtual const float getDimming() const { return mDimming; }
    virtual void setDimming(float dimming);
    virtual unsigned int getColor() const { return mColor; }
    virtual unsigned int getColorShift() const { return mColorShift; }
    virtual float getLineSpacing() { return 0.0f; }
    virtual void setColor(unsigned int color) { mColor = color; }
    virtual void setBackgroundColor(unsigned int color) {};
    virtual void setColorShift(unsigned int color)
    {
        mColorShift = color;
        mColorShiftEnd = color;
    }
    virtual void setColorShiftEnd(unsigned int color) { mColorShiftEnd = color; }
    virtual void setOriginalColor(unsigned int color) { mColorOriginalValue = color; }
    virtual void setChangedColor(unsigned int color) { mColorChangedValue = color; }
    virtual void setColorGradientHorizontal(bool horizontal) {}
    virtual void setReflectionsFalloff(float falloff) {}
    virtual void setFlipX(bool flip) {}
    virtual void setFlipY(bool flip) {}

    virtual void setImage(const std::string& path, bool tile = false) {}

    // These functions are used to enable and disable options in menus, i.e. switches and similar.
    virtual bool getEnabled() { return mEnabled; }
    virtual void setEnabled(bool state) { mEnabled = state; }

    const std::string& getThemeSystemdata() { return mThemeSystemdata; }
    void setThemeSystemdata(const std::string& text) { mThemeSystemdata = text; }
    const std::string& getThemeMetadata() { return mThemeMetadata; }
    void setThemeMetadata(const std::string& text) { mThemeMetadata = text; }
    const std::vector<std::string>& getThemeImageTypes() { return mThemeImageTypes; }
    const std::string& getThemeGameSelector() const { return mThemeGameSelector; }
    const unsigned int getThemeGameSelectorEntry() const { return mThemeGameSelectorEntry; }
    virtual const std::string getDefaultImage() const { return ""; }
    virtual void setGameOverrideImage(const std::string& basename, const std::string& system) {}
    const float getThemeOpacity() const { return mThemeOpacity; }

    virtual std::shared_ptr<Font> getFont() const { return nullptr; }

    const glm::mat4& getTransform();

    virtual std::string getValue() const { return ""; }
    virtual void setValue(const std::string& value) {}

    virtual std::string getHiddenValue() const { return ""; }
    virtual void setHiddenValue(const std::string& value) {}

    // Used to set the parameters for ScrollableContainer.
    virtual void setScrollParameters(float, float, float) {}
    virtual const bool isScrollable() const { return false; }

    virtual void onFocusGained() {}
    virtual void onFocusLost() {}

    virtual void onShow();
    virtual void onHide();
    virtual void onTransition() {}

    // System view and gamelist view video controls.
    virtual void startViewVideos() {}
    virtual void stopViewVideos() {}
    virtual void pauseViewVideos() {}
    virtual void muteViewVideos() {}
    // Needed on Android to reset the static image delay timer on activity resume.
    virtual void resetViewVideosTimer() {}

    // Used to reset various components like text scrolling, animations etc.
    virtual void resetComponent() {}
    // Used by TextComponent.
    virtual void setHorizontalScrolling(bool state) {}

    // Default implementation just handles <pos> and <size> tags as normalized float pairs.
    // You probably want to keep this behavior for any derived classes as well as add your own.
    virtual void applyTheme(const std::shared_ptr<ThemeData>& theme,
                            const std::string& view,
                            const std::string& element,
                            unsigned int properties);

    // Returns a list of help prompts.
    virtual std::vector<HelpPrompt> getHelpPrompts() { return std::vector<HelpPrompt>(); }

    // Called whenever help prompts change.
    void updateHelpPrompts();

    virtual HelpStyle getHelpStyle() { return HelpStyle(); }

    // Returns true if the component is busy doing background processing (e.g. HTTP downloads).
    const bool isProcessing() const { return mIsProcessing; }

    const static unsigned char MAX_ANIMATIONS = 4;

protected:
    void updateSelf(int deltaTime); // Updates animations.
    void updateChildren(int deltaTime); // Updates animations.
    void renderChildren(const glm::mat4& transform) const;

    Window* mWindow;

    GuiComponent* mParent;
    std::vector<GuiComponent*> mChildren;

    std::vector<std::string> mThemeImageTypes;
    std::string mThemeSystemdata;
    std::string mThemeMetadata;
    std::string mThemeGameSelector;
    unsigned int mThemeGameSelectorEntry;

    unsigned int mColor;
    unsigned int mColorShift;
    unsigned int mColorShiftEnd;
    unsigned int mColorOriginalValue;
    unsigned int mColorChangedValue;
    unsigned int mComponentThemeFlags;

    // Default values are for the "light" color scheme.
    static inline unsigned int mMenuColorFrame {0xEFEFEFFF};
    static inline unsigned int mMenuColorFrameLaunchScreen {0xDFDFDFFF};
    static inline unsigned int mMenuColorFrameBusyComponent {0xFFFFFFFF};
    static inline unsigned int mMenuColorPanelDimmed {0x00000009};

    static inline unsigned int mMenuColorTitle {0x555555FF};
    static inline unsigned int mMenuColorPrimary {0x777777FF};
    static inline unsigned int mMenuColorSecondary {0x888888FF};
    static inline unsigned int mMenuColorTertiary {0x666666FF};
    static inline unsigned int mMenuColorRed {0x992222FF};
    static inline unsigned int mMenuColorGreen {0x449944FF};
    static inline unsigned int mMenuColorBlue {0x222299FF};

    static inline unsigned int mMenuColorSelector {0xFFFFFFFF};
    static inline unsigned int mMenuColorSeparators {0xC6C7C6FF};
    static inline unsigned int mMenuColorBusyComponent {0xB8B8B8FF};
    static inline unsigned int mMenuColorScrollIndicators {0x888888FF};
    static inline unsigned int mMenuColorPopupText {0x444444FF};

    static inline unsigned int mMenuColorButtonFocused {0x777777FF};
    static inline unsigned int mMenuColorButtonTextFocused {0xFFFFFFFF};
    static inline unsigned int mMenuColorButtonTextUnfocused {0x777777FF};
    static inline unsigned int mMenuColorButtonFlatFocused {0x878787FF};
    static inline unsigned int mMenuColorButtonFlatUnfocused {0xDADADAFF};

    static inline unsigned int mMenuColorKeyboardModifier {0xF26767FF};
    static inline unsigned int mMenuColorKeyboardCursorFocused {0x777777FF};
    static inline unsigned int mMenuColorKeyboardCursorUnfocused {0xC7C7C7FF};
    static inline unsigned int mMenuColorKeyboardText {0x77777700};
    static inline unsigned int mMenuColorTextInputFrameFocused {0x777777FF};
    static inline unsigned int mMenuColorTextInputFrameUnfocused {0xFFFFFFFF};

    static inline unsigned int mMenuColorSliderKnobDisabled {0xC9C9C9FF};
    static inline unsigned int mMenuColorDateTimeEditMarker {0x00000022};
    static inline unsigned int mMenuColorDetectDeviceHeld {0x44444400};

    glm::vec3 mPosition;
    glm::vec2 mOrigin;
    glm::vec2 mRotationOrigin;
    glm::vec2 mSize;
    Stationary mStationary;
    bool mRenderDuringTransitions;

    float mBrightness;
    float mOpacity;
    float mSaturation;
    float mDimming;
    float mThemeOpacity;
    float mThemeSaturation;
    float mRotation;
    float mScale;
    float mDefaultZIndex;
    float mZIndex;

    bool mIsProcessing;
    bool mVisible;
    bool mEnabled;

private:
    // Don't access this directly, instead use getTransform().
    glm::mat4 mTransform;
    AnimationController* mAnimationMap[MAX_ANIMATIONS];
};

#endif // ES_CORE_GUI_COMPONENT_H
