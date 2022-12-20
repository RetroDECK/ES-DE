//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiComponent.cpp
//
//  Basic GUI component handling such as placement, rotation, Z-order, rendering and animation.
//

#include "GuiComponent.h"

#include "Log.h"
#include "ThemeData.h"
#include "Window.h"
#include "animations/Animation.h"
#include "renderers/Renderer.h"

#include <algorithm>

GuiComponent::GuiComponent()
    : mWindow {Window::getInstance()}
    , mParent {nullptr}
    , mThemeGameSelectorEntry {0}
    , mColor {0}
    , mColorShift {0}
    , mColorShiftEnd {0}
    , mColorOriginalValue {0}
    , mColorChangedValue {0}
    , mComponentThemeFlags {0}
    , mPosition {0.0f, 0.0f, 0.0f}
    , mOrigin {0.0f, 0.0f}
    , mRotationOrigin {0.5f, 0.5f}
    , mSize {0.0f, 0.0f}
    , mBrightness {0.0f}
    , mOpacity {1.0f}
    , mSaturation {1.0f}
    , mDimming {1.0f}
    , mThemeOpacity {1.0f}
    , mThemeSaturation {1.0f}
    , mRotation {0.0f}
    , mScale {1.0f}
    , mDefaultZIndex {0.0f}
    , mZIndex {0.0f}
    , mIsProcessing {false}
    , mVisible {true}
    , mEnabled {true}
    , mTransform {Renderer::getIdentity()}
{
    for (unsigned char i = 0; i < MAX_ANIMATIONS; ++i)
        mAnimationMap[i] = nullptr;
}

GuiComponent::~GuiComponent()
{
    mWindow->removeGui(this);

    cancelAllAnimations();

    if (mParent)
        mParent->removeChild(this);

    for (unsigned int i = 0; i < getChildCount(); ++i)
        getChild(i)->setParent(nullptr);
}

bool GuiComponent::input(InputConfig* config, Input input)
{
    for (unsigned int i = 0; i < getChildCount(); ++i) {
        if (getChild(i)->input(config, input))
            return true;
    }

    return false;
}

void GuiComponent::updateSelf(int deltaTime)
{
    for (unsigned char i = 0; i < MAX_ANIMATIONS; ++i)
        advanceAnimation(i, deltaTime);
}

void GuiComponent::updateChildren(int deltaTime)
{
    for (unsigned int i = 0; i < getChildCount(); ++i)
        getChild(i)->update(deltaTime);
}

void GuiComponent::update(int deltaTime)
{
    updateSelf(deltaTime);
    updateChildren(deltaTime);
}

void GuiComponent::render(const glm::mat4& parentTrans)
{
    if (!isVisible())
        return;

    glm::mat4 trans {parentTrans * getTransform()};
    renderChildren(trans);
}

void GuiComponent::renderChildren(const glm::mat4& transform) const
{
    for (unsigned int i = 0; i < getChildCount(); ++i)
        getChild(i)->render(transform);
}

void GuiComponent::setPosition(float x, float y, float z)
{
    if (mPosition.x == x && mPosition.y == y && mPosition.z == z)
        return;

    mPosition = glm::vec3 {x, y, z};
    onPositionChanged();
}

void GuiComponent::setOrigin(float x, float y)
{
    if (mOrigin.x == x && mOrigin.y == y)
        return;

    mOrigin = glm::vec2 {x, y};
    onOriginChanged();
}

void GuiComponent::setSize(const float w, const float h)
{
    if (mSize.x == w && mSize.y == h)
        return;

    mSize = glm::vec2 {w, h};
    onSizeChanged();
}

const glm::vec2 GuiComponent::getCenter() const
{
    return glm::vec2 {mPosition.x - (getSize().x * mOrigin.x) + getSize().x / 2.0f,
                      mPosition.y - (getSize().y * mOrigin.y) + getSize().y / 2.0f};
}

void GuiComponent::addChild(GuiComponent* cmp)
{
    mChildren.push_back(cmp);

    if (cmp->getParent())
        cmp->getParent()->removeChild(cmp);

    cmp->setParent(this);
}

void GuiComponent::removeChild(GuiComponent* cmp)
{
    if (!cmp->getParent())
        return;

    if (cmp->getParent() != this) {
        LOG(LogError) << "Tried to remove child from incorrect parent!";
    }

    cmp->setParent(nullptr);

    for (auto i = mChildren.cbegin(); i != mChildren.cend(); ++i) {
        if (*i == cmp) {
            mChildren.erase(i);
            return;
        }
    }
}

void GuiComponent::sortChildren()
{
    std::stable_sort(mChildren.begin(), mChildren.end(), [](GuiComponent* a, GuiComponent* b) {
        return b->getZIndex() > a->getZIndex();
    });
}

const int GuiComponent::getChildIndex() const
{
    std::vector<GuiComponent*>::iterator it {
        std::find(getParent()->mChildren.begin(), getParent()->mChildren.end(), this)};

    if (it != getParent()->mChildren.end())
        return static_cast<int>(std::distance(getParent()->mChildren.begin(), it));
    else
        return -1;
}

void GuiComponent::setBrightness(float brightness)
{
    if (mBrightness == brightness)
        return;

    mBrightness = brightness;
    for (auto it = mChildren.cbegin(); it != mChildren.cend(); ++it)
        (*it)->setBrightness(brightness);
}

void GuiComponent::setOpacity(float opacity)
{
    if (mOpacity == opacity)
        return;

    mOpacity = opacity;
    for (auto it = mChildren.cbegin(); it != mChildren.cend(); ++it)
        (*it)->setOpacity(opacity);
}

void GuiComponent::setDimming(float dimming)
{
    if (mDimming == dimming)
        return;

    mDimming = dimming;
    for (auto it = mChildren.cbegin(); it != mChildren.cend(); ++it)
        (*it)->setDimming(dimming);
}

const glm::mat4& GuiComponent::getTransform()
{
    mTransform = Renderer::getIdentity();
    mTransform = glm::translate(mTransform, glm::round(mPosition));

    if (mScale != 1.0f)
        mTransform = glm::scale(mTransform, glm::vec3 {mScale});

    if (mRotation != 0.0f) {
        // Calculate offset as difference between origin and rotation origin.
        glm::vec2 rotationSize {getRotationSize()};
        float xOff {(mOrigin.x - mRotationOrigin.x) * rotationSize.x};
        float yOff {(mOrigin.y - mRotationOrigin.y) * rotationSize.y};

        // Transform to offset point.
        if (xOff != 0.0f || yOff != 0.0f)
            mTransform = glm::translate(mTransform, glm::vec3 {xOff * -1.0f, yOff * -1.0f, 0.0f});

        // Apply rotation transform.
        mTransform = glm::rotate(mTransform, mRotation, glm::vec3 {0.0f, 0.0f, 1.0f});

        // Transform back to original point.
        if (xOff != 0.0f || yOff != 0.0f)
            mTransform = glm::translate(mTransform, glm::vec3 {xOff, yOff, 0.0f});
    }

    mTransform = glm::translate(
        mTransform,
        glm::round(glm::vec3 {mOrigin.x * mSize.x * -1.0f, mOrigin.y * mSize.y * -1.0f, 0.0f}));

    return mTransform;
}

void GuiComponent::textInput(const std::string& text)
{
    for (auto it = mChildren.cbegin(); it != mChildren.cend(); ++it)
        (*it)->textInput(text);
}

void GuiComponent::setAnimation(Animation* anim,
                                int delay,
                                std::function<void()> finishedCallback,
                                bool reverse,
                                unsigned char slot)
{
    assert(slot < MAX_ANIMATIONS);

    AnimationController* oldAnim {mAnimationMap[slot]};
    mAnimationMap[slot] = new AnimationController(anim, delay, finishedCallback, reverse);

    if (oldAnim)
        delete oldAnim;
}

const bool GuiComponent::stopAnimation(unsigned char slot)
{
    assert(slot < MAX_ANIMATIONS);
    if (mAnimationMap[slot]) {
        delete mAnimationMap[slot];
        mAnimationMap[slot] = nullptr;
        return true;
    }
    else {
        return false;
    }
}

const bool GuiComponent::cancelAnimation(unsigned char slot)
{
    assert(slot < MAX_ANIMATIONS);
    if (mAnimationMap[slot]) {
        mAnimationMap[slot]->removeFinishedCallback();
        delete mAnimationMap[slot];
        mAnimationMap[slot] = nullptr;
        return true;
    }
    else {
        return false;
    }
}

const bool GuiComponent::finishAnimation(unsigned char slot)
{
    assert(slot < MAX_ANIMATIONS);
    AnimationController* anim {mAnimationMap[slot]};
    if (anim) {
        // Skip to animation's end.
        const bool done {anim->update(anim->getAnimation()->getDuration() - anim->getTime())};
        if (done) {
            mAnimationMap[slot] = nullptr;
            delete anim; // Will also call finishedCallback.
        }
        return true;
    }
    else {
        return false;
    }
}

const bool GuiComponent::advanceAnimation(unsigned char slot, unsigned int time)
{
    assert(slot < MAX_ANIMATIONS);
    AnimationController* anim {mAnimationMap[slot]};
    if (anim) {
        bool done {anim->update(time)};
        if (done) {
            mAnimationMap[slot] = nullptr;
            delete anim; // Will also call finishedCallback.
        }
        return true;
    }
    else {
        return false;
    }
}

void GuiComponent::stopAllAnimations()
{
    for (unsigned char i = 0; i < MAX_ANIMATIONS; ++i)
        stopAnimation(i);
}

void GuiComponent::cancelAllAnimations()
{
    for (unsigned char i = 0; i < MAX_ANIMATIONS; ++i)
        cancelAnimation(i);
}

void GuiComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
                              const std::string& view,
                              const std::string& element,
                              unsigned int properties)
{
    const glm::vec2 scale {getParent() ?
                               getParent()->getSize() :
                               glm::vec2 {Renderer::getScreenWidth(), Renderer::getScreenHeight()}};

    const ThemeData::ThemeElement* elem {theme->getElement(view, element, "")};
    if (!elem)
        return;

    using namespace ThemeFlags;
    if (properties & POSITION && elem->has("pos")) {
        glm::vec2 denormalized {elem->get<glm::vec2>("pos") * scale};
        setPosition(glm::vec3 {denormalized.x, denormalized.y, 0.0f});
    }

    if (properties & ThemeFlags::SIZE && elem->has("size"))
        setSize(elem->get<glm::vec2>("size") * scale);

    // Position + size also implies origin.
    if ((properties & ORIGIN || (properties & POSITION && properties & ThemeFlags::SIZE)) &&
        elem->has("origin")) {
        setOrigin(glm::clamp(elem->get<glm::vec2>("origin"), 0.0f, 1.0f));
    }

    if (properties & ThemeFlags::ROTATION) {
        if (elem->has("rotation"))
            setRotationDegrees(elem->get<float>("rotation"));
        if (elem->has("rotationOrigin"))
            setRotationOrigin(glm::clamp(elem->get<glm::vec2>("rotationOrigin"), 0.0f, 1.0f));
    }

    if (properties & ThemeFlags::Z_INDEX && elem->has("zIndex"))
        setZIndex(elem->get<float>("zIndex"));
    else
        setZIndex(getDefaultZIndex());

    if (properties & ThemeFlags::BRIGHTNESS && elem->has("brightness"))
        mBrightness = glm::clamp(elem->get<float>("brightness"), -2.0f, 2.0f);

    if (properties & ThemeFlags::OPACITY && elem->has("opacity"))
        mThemeOpacity = glm::clamp(elem->get<float>("opacity"), 0.0f, 1.0f);

    if (properties & ThemeFlags::VISIBLE && elem->has("visible") && !elem->get<bool>("visible"))
        mThemeOpacity = 0.0f;
    else
        setVisible(true);

    if (properties & ThemeFlags::SATURATION && elem->has("saturation"))
        mThemeSaturation = glm::clamp(elem->get<float>("saturation"), 0.0f, 1.0f);

    if (properties && elem->has("gameselector"))
        mThemeGameSelector = elem->get<std::string>("gameselector");

    if (properties && elem->has("gameselectorEntry"))
        mThemeGameSelectorEntry = elem->get<unsigned int>("gameselectorEntry");
}

void GuiComponent::updateHelpPrompts()
{
    if (getParent()) {
        getParent()->updateHelpPrompts();
        return;
    }

    std::vector<HelpPrompt> prompts {getHelpPrompts()};

    if (mWindow->peekGui() == this)
        mWindow->setHelpPrompts(prompts, getHelpStyle());
}

void GuiComponent::onShow()
{
    for (unsigned int i = 0; i < getChildCount(); ++i)
        getChild(i)->onShow();
}

void GuiComponent::onHide()
{
    for (unsigned int i = 0; i < getChildCount(); ++i)
        getChild(i)->onHide();
}
