//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
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

#define DEFAULT_TEXTCOLOR 0x777777FF
#define DEFAULT_INVERTED_TEXTCOLOR 0x444444FF
#define DEFAULT_INVERTED_IMAGECOLOR 0x666666FF
#define DEFAULT_COLORSHIFT 0xFFFFFFFF
#define ICONCOLOR_SCRAPERMARKED 0xFF7777FF
#define ICONCOLOR_USERMARKED 0x7777FFFF
#define TEXTCOLOR_SCRAPERMARKED 0x992222FF
#define TEXTCOLOR_USERMARKED 0x222299FF
#define DISABLED_OPACITY 80

class Animation;
class AnimationController;
class Font;
class InputConfig;
class ThemeData;
class Window;

class GuiComponent
{
public:
    GuiComponent(Window* window);
    virtual ~GuiComponent();

    virtual void textInput(const std::string& text);

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
        mRotationOrigin = glm::vec2{originX, originY};
    }
    void setRotationOrigin(glm::vec2 origin) { setRotationOrigin(origin.x, origin.y); }

    virtual glm::vec2 getSize() const { return mSize; }
    void setSize(const glm::vec2& size) { setSize(size.x, size.y); }
    void setSize(float w, float h);
    virtual void setResize(float width, float height) {}
    virtual void onSizeChanged() {}

    virtual glm::vec2 getRotationSize() const { return getSize(); }
    float getRotation() const { return mRotation; }
    void setRotation(float rotation) { mRotation = rotation; }
    void setRotationDegrees(float rotation)
    {
        setRotation(static_cast<float>(glm::radians(rotation)));
    }

    float getScale() const { return mScale; }
    void setScale(float scale) { mScale = scale; }

    float getZIndex() const { return mZIndex; }
    void setZIndex(float zIndex) { mZIndex = zIndex; }

    float getDefaultZIndex() const { return mDefaultZIndex; }
    void setDefaultZIndex(float zIndex) { mDefaultZIndex = zIndex; }

    bool isVisible() const { return mVisible; }
    void setVisible(bool visible) { mVisible = visible; }

    // Returns the center point of the image (takes origin into account).
    glm::vec2 getCenter() const;

    void setParent(GuiComponent* parent) { mParent = parent; }
    GuiComponent* getParent() const { return mParent; }

    void addChild(GuiComponent* cmp);
    void removeChild(GuiComponent* cmp);
    void clearChildren() { mChildren.clear(); }
    void sortChildren();
    unsigned int getChildCount() const { return static_cast<int>(mChildren.size()); }
    int getChildIndex() const;
    GuiComponent* getChild(unsigned int i) const { return mChildren.at(i); }

    // Animation will be automatically deleted when it completes or is stopped.
    bool isAnimationPlaying(unsigned char slot) const { return mAnimationMap[slot] != nullptr; }
    bool isAnimationReversed(unsigned char slot) const
    {
        assert(mAnimationMap[slot] != nullptr);
        return mAnimationMap[slot]->isReversed();
    }
    int getAnimationTime(unsigned char slot) const
    {
        assert(mAnimationMap[slot] != nullptr);
        return mAnimationMap[slot]->getTime();
    }
    void setAnimation(Animation* animation,
                      int delay = 0,
                      std::function<void()> finishedCallback = nullptr,
                      bool reverse = false,
                      unsigned char slot = 0);
    bool stopAnimation(unsigned char slot);
    // Like stopAnimation, but doesn't call finishedCallback - only removes the animation, leaving
    // things in their current state. Returns true if successful (an animation was in this slot).
    bool cancelAnimation(unsigned char slot);
    // Calls update(1.f) and finishedCallback, then deletes the animation - basically skips
    // to the end.  Returns true if successful (an animation was in this slot).
    bool finishAnimation(unsigned char slot);
    // Returns true if successful (an animation was in this slot).
    bool advanceAnimation(unsigned char slot, unsigned int time);
    void stopAllAnimations();
    void cancelAllAnimations();

    virtual bool isListScrolling() { return false; }
    virtual void stopListScrolling() {}
    virtual unsigned char getOpacity() const { return mOpacity; }
    virtual void setOpacity(unsigned char opacity);
    virtual unsigned int getColor() const { return mColor; }
    virtual unsigned int getColorShift() const { return mColorShift; }
    virtual void setColor(unsigned int color)
    {
        mColor = color;
        mColorOpacity = mColor & 0x000000FF;
    }
    virtual float getSaturation() const { return static_cast<float>(mColor); }
    virtual void setSaturation(float saturation) { mSaturation = saturation; }
    virtual void setColorShift(unsigned int color)
    {
        mColorShift = color;
        mColorShiftEnd = color;
    }
    virtual void setOriginalColor(unsigned int color) { mColorOriginalValue = color; }
    virtual void setChangedColor(unsigned int color) { mColorChangedValue = color; }

    // These functions are used to enable and disable options in menus, i.e. switches and similar.
    virtual bool getEnabled() { return mEnabled; }
    virtual void setEnabled(bool state) { mEnabled = state; }

    virtual std::shared_ptr<Font> getFont() const { return nullptr; }

    const glm::mat4& getTransform();

    virtual std::string getValue() const { return ""; }
    virtual void setValue(const std::string& value) {}

    virtual std::string getHiddenValue() const { return ""; }
    virtual void setHiddenValue(const std::string& value) {}

    // Used to set the parameters for ScrollableContainer.
    virtual void setScrollParameters(float, float, int) {}

    virtual void onFocusGained() {}
    virtual void onFocusLost() {}

    virtual void onShow();
    virtual void onHide();
    virtual void onStopVideo();
    virtual void onPauseVideo();
    virtual void onUnpauseVideo();
    virtual bool isVideoPaused() { return false; }

    virtual void onScreensaverActivate();
    virtual void onScreensaverDeactivate();
    virtual void onGameLaunchedActivate();
    virtual void onGameLaunchedDeactivate();
    virtual void topWindow(bool isTop);

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
    bool isProcessing() const { return mIsProcessing; }

    const static unsigned char MAX_ANIMATIONS = 4;

protected:
    void renderChildren(const glm::mat4 &transform) const;

    void updateSelf(int deltaTime); // Updates animations.
    void updateChildren(int deltaTime); // Updates animations.

    Window *mWindow;

    GuiComponent *mParent;
    std::vector<GuiComponent *> mChildren;

    unsigned char mOpacity;
    unsigned int mColor;
    float mSaturation;
    unsigned char mColorOpacity;
    unsigned int mColorShift;
    unsigned int mColorShiftEnd;
    unsigned int mColorOriginalValue;
    unsigned int mColorChangedValue;

    glm::vec3 mPosition;
    glm::vec2 mOrigin;
    glm::vec2 mRotationOrigin;
    glm::vec2 mSize;

    float mRotation = 0.0;
    float mScale = 1.0;

    float mDefaultZIndex = 0;
    float mZIndex = 0;

    bool mIsProcessing;
    bool mVisible;
    bool mEnabled;

private:
    // Don't access this directly, instead use getTransform().
    glm::mat4 mTransform;
    AnimationController* mAnimationMap[MAX_ANIMATIONS];
};

#endif // ES_CORE_GUI_COMPONENT_H
