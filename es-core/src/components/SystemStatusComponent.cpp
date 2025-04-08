//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  SystemStatusComponent.cpp
//
//  Displays system status information (Bluetooth, Wi-Fi, cellular and battery).
//

#include "components/SystemStatusComponent.h"

#include "SystemStatus.h"
#include "Window.h"
#include "utils/FileSystemUtil.h"

#define PREFIX "icon_"

SystemStatusComponent::SystemStatusComponent()
    : mRenderer {Renderer::getInstance()}
    , mHasBluetooth {false}
    , mHasWifi {false}
    , mHasCellular {false}
    , mHasBattery {false}
    , mBatteryCharging {false}
    , mBatteryText {Settings::getInstance()->getBool("SystemStatusBattery") &&
                    Settings::getInstance()->getBool("SystemStatusBatteryPercentage")}
    , mBatteryCapacity {100}
    , mEntries {sAllowedEntries}
    , mColorShift {0xFFFFFFFF}
    , mBackgroundColor {0x00000000}
    , mBackgroundColorEnd {0x00000000}
    , mAccumulator {0}
    , mAccumulatorAndroid {0}
    , mBackgroundHorizontalPadding {0.0f, 0.0f}
    , mBackgroundVerticalPadding {0.0f, 0.0f}
    , mBackgroundCornerRadius {0.0f}
    , mColorGradientHorizontal {true}
    , mEntrySpacing {0.005f * mRenderer->getScreenWidth()}
{
}

void SystemStatusComponent::updateGrid()
{
    mGrid.reset();

    if (Settings::getInstance()->getBool("SystemStatusDisplayAll")) {
        mHasBluetooth = true;
        mHasWifi = true;
        mHasCellular = true;
        mHasBattery = true;
    }

    mDisplayEntries.clear();

    if (mHasBluetooth && Settings::getInstance()->getBool("SystemStatusBluetooth") &&
        std::find(mEntries.cbegin(), mEntries.cend(), "bluetooth") != mEntries.cend())
        mDisplayEntries.emplace_back("bluetooth");
    if (mHasWifi && Settings::getInstance()->getBool("SystemStatusWifi") &&
        std::find(mEntries.cbegin(), mEntries.cend(), "wifi") != mEntries.cend())
        mDisplayEntries.emplace_back("wifi");
    if (mHasCellular && Settings::getInstance()->getBool("SystemStatusCellular") &&
        std::find(mEntries.cbegin(), mEntries.cend(), "cellular") != mEntries.cend())
        mDisplayEntries.emplace_back("cellular");
    if (mHasBattery && Settings::getInstance()->getBool("SystemStatusBattery") &&
        std::find(mEntries.cbegin(), mEntries.cend(), "battery") != mEntries.cend())
        mDisplayEntries.emplace_back("battery");

    if (mDisplayEntries.empty())
        return;

    mBatteryText = Settings::getInstance()->getBool("SystemStatusBattery") &&
                   Settings::getInstance()->getBool("SystemStatusBatteryPercentage");

    int numEntries {static_cast<int>(mDisplayEntries.size())};
    if (mEntrySpacing != 0.0f)
        numEntries += numEntries - 1;
    if (mHasBattery && mBatteryText)
        ++numEntries;

    mGrid = std::make_shared<ComponentGrid>(glm::ivec2 {numEntries, 1});
    mEntryMap.clear();

    float width {0.0f};
    int i {0};

    for (auto it = mDisplayEntries.cbegin(); it != mDisplayEntries.cend(); ++it) {
        if (*it == "battery") {
            mBattery = std::make_shared<ImageComponent>(false, true);
            if (mBatteryCharging)
                mBattery->setImage(mIconPathMap["battery_charging"]);
            else if (mBatteryCapacity >= 0 && mBatteryCapacity <= 25)
                mBattery->setImage(mIconPathMap["battery_low"]);
            else if (mBatteryCapacity >= 26 && mBatteryCapacity <= 60)
                mBattery->setImage(mIconPathMap["battery_medium"]);
            else if (mBatteryCapacity >= 61 && mBatteryCapacity <= 90)
                mBattery->setImage(mIconPathMap["battery_high"]);
            else
                mBattery->setImage(mIconPathMap["battery_full"]);
            mBattery->setColorShift(mColorShift);
            mBattery->setResize(0, mSize.y);
            mBattery->setOpacity(mThemeOpacity);
            width += std::round(mBattery->getSize().x);
            mGrid->setEntry(mBattery, glm::ivec2 {i, 0}, false, false);
        }
        else {
            std::shared_ptr<ImageComponent> icon {std::make_shared<ImageComponent>(false, true)};
            icon->setImage(mIconPathMap[*it]);
            icon->setColorShift(mColorShift);
            icon->setResize(0, mSize.y);
            icon->setOpacity(mThemeOpacity);
            width += std::round(icon->getSize().x);
            mGrid->setEntry(icon, glm::ivec2 {i, 0}, false, false);
        }

        mEntryMap[*it] = i;
        ++i;

        if (mEntrySpacing != 0.0f && *it != mDisplayEntries.back()) {
            ++i;
            width += mEntrySpacing;
            mGrid->setEntry(std::make_shared<GuiComponent>(), glm::ivec2 {i, 0}, false, false);
        }
    }

    if (mHasBattery && mBatteryText &&
        std::find(mDisplayEntries.cbegin(), mDisplayEntries.cend(), "battery") !=
            mDisplayEntries.cend()) {
        // We set the initial value to "100%" to calculate the cell size based on this, as this
        // will be the longest text that will ever be displayed for the battery capacity.
        mBatteryPercentage = std::make_shared<TextComponent>(
            "100%", mFont, 0xFFFFFFFF, ALIGN_LEFT, ALIGN_CENTER, glm::ivec2 {1, 0},
            glm::vec3 {0.0f, 0.0f, 0.0f}, glm::vec2 {0.0f, 0.0f}, 0x00000000, 1.0f);
        mBatteryPercentage->setColor(mColorShift);
        mBatteryPercentage->setOpacity(mThemeOpacity);
        width += mBatteryPercentage->getSize().x;
        mEntryMap["batteryText"] = i;
        mGrid->setEntry(mBatteryPercentage, glm::ivec2 {i, 0}, false, false);
        mBatteryPercentage->setValue(std::to_string(mBatteryCapacity) + "%");
    }

    for (int i {0}; i < static_cast<int>(mGrid->getChildCount()); ++i) {
        mGrid->setColWidthPerc(i, mGrid->getChild(i)->getSize().x / width);
        if (mHasBattery && mBatteryText && i == static_cast<int>(mGrid->getChildCount()) - 2)
            continue;

        if (mEntrySpacing != 0.0f && i != static_cast<int>(mGrid->getChildCount()) - 1) {
            ++i;
            mGrid->setColWidthPerc(i, mEntrySpacing / width);
        }
    }

    mGrid->setSize(width, mSize.y);
    mGrid->setOrigin(mOrigin);
    mSize.x = width;
}

void SystemStatusComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                       const std::string& view,
                                       const std::string& element,
                                       unsigned int properties)
{
    // Apply default settings as the theme may not define any configuration.
    const float scale {mRenderer->getIsVerticalOrientation() ? mRenderer->getScreenWidth() :
                                                               mRenderer->getScreenHeight()};
    mPosition = glm::vec3 {0.982f * mRenderer->getScreenWidth(),
                           0.016f * mRenderer->getScreenHeight(), 0.0f};
    mOrigin = glm::vec2 {1.0f, 0.0f};
    mColor = 0xFFFFFFFF;

    mIconPathMap.clear();
    mIconPathMap["bluetooth"] = ":/graphics/systemstatus/bluetooth.svg";
    mIconPathMap["wifi"] = ":/graphics/systemstatus/wifi.svg";
    mIconPathMap["cellular"] = ":/graphics/systemstatus/cellular.svg";
    mIconPathMap["battery_charging"] = ":/graphics/systemstatus/battery_charging.svg";
    mIconPathMap["battery_low"] = ":/graphics/systemstatus/battery_low.svg";
    mIconPathMap["battery_medium"] = ":/graphics/systemstatus/battery_medium.svg";
    mIconPathMap["battery_high"] = ":/graphics/systemstatus/battery_high.svg";
    mIconPathMap["battery_full"] = ":/graphics/systemstatus/battery_full.svg";

    GuiComponent::applyTheme(theme, view, element, properties);

    const ThemeData::ThemeElement* elem {theme->getElement(view, element, "systemstatus")};

    mSize = glm::vec2 {0.0f, std::round(0.035f * scale)};
    float textRelativeScale {0.9f};

    if (!elem) {
        mFont = {Font::get(mSize.y * textRelativeScale, FONT_PATH_LIGHT)};
        return;
    }

    if (elem->has("height")) {
        mSize.y =
            std::round(glm::clamp(elem->get<float>("height") * scale, 0.01f * scale, 0.5f * scale));
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
            LOG(LogWarning) << "SystemStatusComponent: Invalid theme configuration, property "
                               "\"scope\" for element \""
                            << element.substr(13) << "\" defined as \"" << scope << "\"";
        }
    }

    if (elem->has("textRelativeScale"))
        textRelativeScale = glm::clamp(elem->get<float>("textRelativeScale"), 0.5f, 1.0f);

    if (elem->has("fontPath"))
        mFont = {Font::get(mSize.y * textRelativeScale, elem->get<std::string>("fontPath"))};
    else
        mFont = {Font::get(mSize.y * textRelativeScale, FONT_PATH_LIGHT)};

    if (elem->has("color"))
        mColorShift = elem->get<unsigned int>("color");

    if (elem->has("colorEnd"))
        mColorShiftEnd = elem->get<unsigned int>("colorEnd");
    else
        mColorShiftEnd = mColorShift;

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
                LOG(LogWarning) << "SystemStatusComponent: Invalid theme configuration, property "
                                   "\"backgroundGradientType\" for element \""
                                << element.substr(13) << "\" defined as \""
                                << backgroundGradientType << "\"";
            }
        }

        if (elem->has("backgroundHorizontalPadding")) {
            const glm::vec2 backgroundHorizontalPadding {
                glm::clamp(elem->get<glm::vec2>("backgroundHorizontalPadding"), 0.0f, 0.2f)};
            mBackgroundHorizontalPadding.x =
                backgroundHorizontalPadding.x * mRenderer->getScreenWidth();
            mBackgroundHorizontalPadding.y =
                backgroundHorizontalPadding.y * mRenderer->getScreenWidth();
        }

        if (elem->has("backgroundVerticalPadding")) {
            const glm::vec2 backgroundVerticalPadding {
                glm::clamp(elem->get<glm::vec2>("backgroundVerticalPadding"), 0.0f, 0.2f)};
            mBackgroundVerticalPadding.x =
                backgroundVerticalPadding.x * mRenderer->getScreenHeight();
            mBackgroundVerticalPadding.y =
                backgroundVerticalPadding.y * mRenderer->getScreenHeight();
        }

        if (elem->has("backgroundCornerRadius")) {
            mBackgroundCornerRadius =
                glm::clamp(elem->get<float>("backgroundCornerRadius"), 0.0f, 0.5f) *
                mRenderer->getScreenWidth();
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

        // If the "all" value has been set then leave mEntries fully populated.
        if (std::find(entries.begin(), entries.end(), "all") == entries.end()) {
            mEntries.clear();
            for (auto& allowedEntry : sAllowedEntries) {
                if (std::find(entries.cbegin(), entries.cend(), allowedEntry) != entries.cend())
                    mEntries.emplace_back(allowedEntry);
            }
        }
    }

    if (elem->has("entrySpacing")) {
        mEntrySpacing = std::round(glm::clamp(elem->get<float>("entrySpacing"), 0.0f, 0.04f) *
                                   mRenderer->getScreenWidth());
    }

    // Custom entry icons.
    // The names may look a bit strange when combined with the PREFIX string "icon_" but it's
    // because ThemeData adds this prefix to avoid name collisions when using XML attributes.
    if (elem->has(PREFIX "icon_wifi"))
        mIconPathMap["wifi"] = elem->get<std::string>(PREFIX "icon_wifi");
    if (elem->has(PREFIX "icon_bluetooth"))
        mIconPathMap["bluetooth"] = elem->get<std::string>(PREFIX "icon_bluetooth");
    if (elem->has(PREFIX "icon_cellular"))
        mIconPathMap["cellular"] = elem->get<std::string>(PREFIX "icon_cellular");
    if (elem->has(PREFIX "icon_battery_charging"))
        mIconPathMap["battery_charging"] = elem->get<std::string>(PREFIX "icon_battery_charging");
    if (elem->has(PREFIX "icon_battery_low"))
        mIconPathMap["battery_low"] = elem->get<std::string>(PREFIX "icon_battery_low");
    if (elem->has(PREFIX "icon_battery_medium"))
        mIconPathMap["battery_medium"] = elem->get<std::string>(PREFIX "icon_battery_medium");
    if (elem->has(PREFIX "icon_battery_high"))
        mIconPathMap["battery_high"] = elem->get<std::string>(PREFIX "icon_battery_high");
    if (elem->has(PREFIX "icon_battery_full"))
        mIconPathMap["battery_full"] = elem->get<std::string>(PREFIX "icon_battery_full");
}

void SystemStatusComponent::update(int deltaTime)
{
    if (mEntries.empty())
        return;

    mAccumulator += deltaTime;
    mAccumulatorAndroid += deltaTime;

    if (mAccumulator >= SystemStatus::updateTime) {
#if defined(__ANDROID__)
        // For Android we poll on the main thread instead of in a separate thread.
        SystemStatus::Status status;
        const bool pollImmediately {SystemStatus::getInstance().getPollImmediately()};
        if (mAccumulatorAndroid >= SystemStatus::pollingTime || pollImmediately) {
            status = SystemStatus::getInstance().getStatus(true);
            mAccumulatorAndroid = 0;
            if (pollImmediately)
                SystemStatus::getInstance().setPollImmediately(false);
        }
        else {
            status = SystemStatus::getInstance().getStatus(false);
        }
#else
        SystemStatus::Status status {SystemStatus::getInstance().getStatus()};
#endif
        mAccumulator = 0;

        bool statusChanged {false};
        bool batteryStatusChanged {false};

        if (mHasBluetooth != status.hasBluetooth) {
            mHasBluetooth = status.hasBluetooth;
            statusChanged = true;
        }
        if (mHasWifi != status.hasWifi) {
            mHasWifi = status.hasWifi;
            statusChanged = true;
        }
        if (mHasCellular != status.hasCellular) {
            mHasCellular = status.hasCellular;
            statusChanged = true;
        }
        if (mHasBattery != status.hasBattery) {
            mHasBattery = status.hasBattery;
            statusChanged = true;
            batteryStatusChanged = true;
        }
        if (mHasBattery) {
            if (mBatteryCharging != status.batteryCharging) {
                mBatteryCharging = status.batteryCharging;
                batteryStatusChanged = true;
            }
            if (mBatteryCapacity != status.batteryCapacity) {
                mBatteryCapacity = status.batteryCapacity;
                batteryStatusChanged = true;
            }
            if ((Settings::getInstance()->getBool("SystemStatusBattery") &&
                 Settings::getInstance()->getBool("SystemStatusBatteryPercentage")) !=
                mBatteryText) {
                statusChanged = true;
            }
        }

        if (statusChanged) {
            updateGrid();
        }
        else if (mHasBattery && batteryStatusChanged) {
            // Slight optimization, just update the battery charge percentage and icon in
            // case only the battery status has changed, instead of having to recreate the
            // entire grid when this happens.
            if (mBatteryPercentage != nullptr)
                mBatteryPercentage->setValue(std::to_string(mBatteryCapacity) + "%");

            if (mBatteryCharging)
                mBattery->setImage(mIconPathMap["battery_charging"]);
            else if (mBatteryCapacity >= 0 && mBatteryCapacity <= 25)
                mBattery->setImage(mIconPathMap["battery_low"]);
            else if (mBatteryCapacity >= 26 && mBatteryCapacity <= 60)
                mBattery->setImage(mIconPathMap["battery_medium"]);
            else if (mBatteryCapacity >= 61 && mBatteryCapacity <= 90)
                mBattery->setImage(mIconPathMap["battery_high"]);
            else if (mBatteryCapacity > 90)
                mBattery->setImage(mIconPathMap["battery_full"]);
        }
    }
}

void SystemStatusComponent::render(const glm::mat4& parentTrans)
{
    if (mDisplayEntries.empty())
        return;

    if (mGrid) {
        mGrid->setPosition(mPosition);
        mGrid->setRotationOrigin(mRotationOrigin);
        mGrid->setRotation(mRotation);

        if (Settings::getInstance()->getBool("DebugImage")) {
            const glm::mat4 trans {parentTrans * getTransform()};
            mRenderer->setMatrix(trans);
            mRenderer->drawRect(0.0f, 0.0f, mSize.x, mSize.y, 0xFF000033, 0xFF000033);
        }

        if (mBackgroundColor != 0x00000000) {
            glm::vec3 positionTemp {mPosition};
            glm::vec2 sizeTemp {mSize};
            glm::vec2 originTemp {mOrigin};
            float rotationTemp {mRotation};
            glm::vec2 rotationOriginTemp {mRotationOrigin};

            mPosition = mGrid->getPosition();
            mSize = mGrid->getSize();
            mOrigin = mGrid->getOrigin();
            mRotation = mRotation;
            mRotationOrigin = mRotationOrigin;

            glm::mat4 trans {parentTrans * getTransform()};
            trans = glm::translate(trans, glm::vec3 {-mBackgroundHorizontalPadding.x,
                                                     -mBackgroundVerticalPadding.x, 0.0f});
            mRenderer->setMatrix(trans);

            mRenderer->drawRect(
                0.0f, 0.0f,
                mSize.x + mBackgroundHorizontalPadding.x + mBackgroundHorizontalPadding.y,
                mSize.y + mBackgroundVerticalPadding.x + mBackgroundVerticalPadding.y,
                mBackgroundColor, mBackgroundColorEnd, mColorGradientHorizontal, mThemeOpacity,
                1.0f, Renderer::BlendFactor::SRC_ALPHA, Renderer::BlendFactor::ONE_MINUS_SRC_ALPHA,
                mBackgroundCornerRadius);

            mPosition = positionTemp;
            mSize = sizeTemp;
            mOrigin = originTemp;
            mRotation = rotationTemp;
            mRotationOrigin = rotationOriginTemp;
        }

        mGrid->render(parentTrans);
    }
}
