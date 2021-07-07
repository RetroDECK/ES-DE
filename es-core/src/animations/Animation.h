//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Animation.h
//
//  Animation base class.
//

#ifndef ES_CORE_ANIMATIONS_ANIMATION_H
#define ES_CORE_ANIMATIONS_ANIMATION_H

class Animation
{
public:
    virtual ~Animation() {}
    virtual int getDuration() const = 0;
    virtual void apply(float t) = 0;
};

#endif // ES_CORE_ANIMATIONS_ANIMATION_H
