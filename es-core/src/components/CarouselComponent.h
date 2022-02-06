//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  CarouselComponent.h
//
//  Carousel.
//

#include "GuiComponent.h"
#include "Sound.h"
#include "components/IList.h"
#include "components/ImageComponent.h"
#include "components/TextComponent.h"
#include "resources/Font.h"

class SystemData;

#ifndef ES_CORE_COMPONENTS_CAROUSEL_COMPONENT_H
#define ES_CORE_COMPONENTS_CAROUSEL_COMPONENT_H

struct CarouselElement {
    std::shared_ptr<GuiComponent> logo;
};

class CarouselComponent : public IList<CarouselElement, SystemData*>
{
public:
    CarouselComponent();

    void addElement(const std::shared_ptr<GuiComponent>& component,
                    const std::string& name,
                    SystemData* object)
    {
        Entry listEntry;
        listEntry.name = name;
        listEntry.object = object;
        listEntry.data.logo = component;
        add(listEntry);
    }

    void addEntry(const std::shared_ptr<ThemeData>& theme, Entry& entry);
    Entry& getEntry(int index) { return mEntries.at(index); }

    enum CarouselType {
        HORIZONTAL, // Replace with AllowShortEnumsOnASingleLine: false (clang-format >=11.0).
        VERTICAL,
        VERTICAL_WHEEL,
        HORIZONTAL_WHEEL
    };

    int getCursor() { return mCursor; }
    const CarouselType getType() { return mType; }
    size_t getNumEntries() { return mEntries.size(); }

    void setCursorChangedCallback(const std::function<void(CursorState state)>& func)
    {
        mCursorChangedCallback = func;
    }
    void setCancelTransitionsCallback(const std::function<void()>& func)
    {
        mCancelTransitionsCallback = func;
    }

    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;

    void applyTheme(const std::shared_ptr<ThemeData>& theme,
                    const std::string& view,
                    const std::string& element,
                    unsigned int properties) override;

protected:
    void onCursorChanged(const CursorState& state) override;
    void onScroll() override
    {
        NavigationSounds::getInstance().playThemeNavigationSound(SYSTEMBROWSESOUND);
    }

private:
    std::function<void(CursorState state)> mCursorChangedCallback;
    std::function<void()> mCancelTransitionsCallback;

    float mCamOffset;
    int mPreviousScrollVelocity;

    CarouselType mType;
    Alignment mLogoAlignment;
    int mMaxLogoCount;
    glm::vec2 mLogoSize;
    float mLogoScale;
    float mLogoRotation;
    glm::vec2 mLogoRotationOrigin;
    unsigned int mCarouselColor;
    unsigned int mCarouselColorEnd;
    bool mColorGradientHorizontal;
};

#endif // ES_CORE_COMPONENTS_CAROUSEL_COMPONENT_H
