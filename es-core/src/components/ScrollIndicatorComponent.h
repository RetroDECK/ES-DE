//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ScrollIndicatorComponent.h
//
//  Visually indicates whether a menu can be scrolled (up, up/down or down).
//

#ifndef ES_CORE_COMPONENTS_SCROLL_INDICATOR_COMPONENT_H
#define ES_CORE_COMPONENTS_SCROLL_INDICATOR_COMPONENT_H

#define FADE_IN_TIME 90.0f

#include "animations/LambdaAnimation.h"
#include "components/ComponentList.h"

class ScrollIndicatorComponent
{
public:
    ScrollIndicatorComponent(std::shared_ptr<ComponentList> componentList,
                             std::shared_ptr<ImageComponent> scrollUp,
                             std::shared_ptr<ImageComponent> scrollDown)
        : mPreviousScrollState(ComponentList::SCROLL_NONE)
    {
        assert(componentList != nullptr && scrollUp != nullptr && scrollDown != nullptr);

        scrollUp->setImage(":/graphics/scroll_up.svg");
        scrollDown->setImage(":/graphics/scroll_down.svg");

        scrollUp->setOpacity(0);
        scrollDown->setOpacity(0);

        if (!Settings::getInstance()->getBool("ScrollIndicators")) {
            // If the scroll indicators setting is disabled, then show a permanent down arrow
            // symbol when the component list contains more entries than can fit on screen.
            componentList.get()->setScrollIndicatorChangedCallback(
                [scrollUp, scrollDown](ComponentList::ScrollIndicator state) {
                    if (state == ComponentList::SCROLL_UP ||
                        state == ComponentList::SCROLL_UP_DOWN ||
                        state == ComponentList::SCROLL_DOWN) {
                        scrollDown->setOpacity(255);
                    }
                });
        }
        else {
            // If the scroll indicator setting is enabled, then also show the up and up/down
            // combination and switch between these as the list is scrolled.
            componentList.get()->setScrollIndicatorChangedCallback(
                [this, scrollUp, scrollDown](ComponentList::ScrollIndicator state) {
                    float fadeInTime{FADE_IN_TIME};

                    bool upFadeIn = false;
                    bool upFadeOut = false;
                    bool downFadeIn = false;
                    bool downFadeOut = false;

                    scrollUp->finishAnimation(0);
                    scrollDown->finishAnimation(0);

                    if (state == ComponentList::SCROLL_UP &&
                        mPreviousScrollState == ComponentList::SCROLL_NONE) {
                        scrollUp->setOpacity(255);
                    }
                    else if (state == ComponentList::SCROLL_UP &&
                             mPreviousScrollState == ComponentList::SCROLL_UP_DOWN) {
                        downFadeOut = true;
                    }
                    else if (state == ComponentList::SCROLL_UP &&
                             mPreviousScrollState == ComponentList::SCROLL_DOWN) {
                        upFadeIn = true;
                        fadeInTime *= 1.5f;
                        scrollDown->setOpacity(0);
                    }
                    else if (state == ComponentList::SCROLL_UP_DOWN &&
                             mPreviousScrollState == ComponentList::SCROLL_NONE) {
                        scrollUp->setOpacity(255);
                        scrollDown->setOpacity(255);
                    }
                    else if (state == ComponentList::SCROLL_UP_DOWN &&
                             mPreviousScrollState == ComponentList::SCROLL_DOWN) {
                        upFadeIn = true;
                    }
                    else if (state == ComponentList::SCROLL_UP_DOWN &&
                             mPreviousScrollState == ComponentList::SCROLL_UP) {
                        downFadeIn = true;
                    }
                    else if (state == ComponentList::SCROLL_DOWN &&
                             mPreviousScrollState == ComponentList::SCROLL_NONE) {
                        scrollDown->setOpacity(255);
                    }
                    else if (state == ComponentList::SCROLL_DOWN &&
                             mPreviousScrollState == ComponentList::SCROLL_UP_DOWN) {
                        upFadeOut = true;
                    }
                    else if (state == ComponentList::SCROLL_DOWN &&
                             mPreviousScrollState == ComponentList::SCROLL_UP) {
                        downFadeIn = true;
                        fadeInTime *= 1.5f;
                        scrollUp->setOpacity(0);
                    }

                    if (upFadeIn) {
                        auto upFadeInFunc = [scrollUp](float t) {
                            scrollUp->setOpacity(
                                static_cast<unsigned char>(glm::mix(0.0f, 1.0f, t) * 255));
                        };
                        scrollUp->setAnimation(
                            new LambdaAnimation(upFadeInFunc, static_cast<int>(fadeInTime)), 0,
                            nullptr, false);
                    }

                    if (upFadeOut) {
                        auto upFadeOutFunc = [scrollUp](float t) {
                            scrollUp->setOpacity(
                                static_cast<unsigned char>(glm::mix(0.0f, 1.0f, t) * 255));
                        };
                        scrollUp->setAnimation(
                            new LambdaAnimation(upFadeOutFunc, static_cast<int>(fadeInTime)), 0,
                            nullptr, true);
                    }

                    if (downFadeIn) {
                        auto downFadeInFunc = [scrollDown](float t) {
                            scrollDown->setOpacity(
                                static_cast<unsigned char>(glm::mix(0.0f, 1.0f, t) * 255));
                        };
                        scrollDown->setAnimation(
                            new LambdaAnimation(downFadeInFunc, static_cast<int>(fadeInTime)), 0,
                            nullptr, false);
                    }

                    if (downFadeOut) {
                        auto downFadeOutFunc = [scrollDown](float t) {
                            scrollDown->setOpacity(
                                static_cast<unsigned char>(glm::mix(0.0f, 1.0f, t) * 255));
                        };
                        scrollDown->setAnimation(
                            new LambdaAnimation(downFadeOutFunc, static_cast<int>(fadeInTime)), 0,
                            nullptr, true);
                    }

                    mPreviousScrollState = state;
                });
        }
    }

private:
    ComponentList::ScrollIndicator mPreviousScrollState;
};

#endif // ES_CORE_COMPONENTS_SCROLL_INDICATOR_COMPONENT_H
