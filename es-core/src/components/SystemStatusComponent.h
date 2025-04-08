//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  SystemStatusComponent.h
//
//  Displays system status information (Bluetooth, Wi-Fi, cellular and battery).
//

#ifndef ES_CORE_COMPONENTS_SYSTEM_STATUS_COMPONENT_H
#define ES_CORE_COMPONENTS_SYSTEM_STATUS_COMPONENT_H

#include "GuiComponent.h"
#include "components/ComponentGrid.h"
#include "components/ImageComponent.h"
#include "components/TextComponent.h"
#include "renderers/Renderer.h"
#include "resources/Font.h"

class SystemStatusComponent : public GuiComponent
{
public:
    SystemStatusComponent();
    void updateGrid();

    void applyTheme(const std::shared_ptr<ThemeData>& theme,
                    const std::string& view,
                    const std::string& element,
                    unsigned int properties) override;

    void update(int deltaTime) override;
    void render(const glm::mat4& parent) override;

private:
    Renderer* mRenderer;
    std::shared_ptr<ComponentGrid> mGrid;
    std::shared_ptr<Font> mFont;

    std::shared_ptr<ImageComponent> mBattery;
    std::shared_ptr<TextComponent> mBatteryPercentage;

    bool mHasBluetooth;
    bool mHasWifi;
    bool mHasCellular;
    bool mHasBattery;
    bool mBatteryCharging;
    bool mBatteryText;
    int mBatteryCapacity;

    std::vector<std::string> mEntries;
    std::vector<std::string> mDisplayEntries;
    std::map<std::string, int> mEntryMap;
    std::map<std::string, std::string> mIconPathMap;

    static inline std::vector<std::string> sAllowedEntries {"bluetooth", "wifi", "cellular",
                                                            "battery"};

    unsigned int mColorShift;
    unsigned int mBackgroundColor;
    unsigned int mBackgroundColorEnd;
    int mAccumulator;
    int mAccumulatorAndroid;
    glm::vec2 mBackgroundHorizontalPadding;
    glm::vec2 mBackgroundVerticalPadding;
    float mBackgroundCornerRadius;
    bool mColorGradientHorizontal;
    float mEntrySpacing;
};

#endif // ES_CORE_COMPONENTS_SYSTEM_STATUS_COMPONENT_H
