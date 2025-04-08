//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiSystemStatusOptions.cpp
//
//  User interface for the system status options.
//  Submenu to the GuiMenu main menu.
//
#include "guis/GuiSystemStatusOptions.h"

#include "Settings.h"
#include "components/OptionListComponent.h"
#include "components/SwitchComponent.h"
#include "utils/LocalizationUtil.h"

GuiSystemStatusOptions::GuiSystemStatusOptions(const std::string& title)
    : GuiSettings {title}
{
    // Display Bluetooth indicator.
    auto systemStatusBluetooth = std::make_shared<SwitchComponent>();
    systemStatusBluetooth->setState(Settings::getInstance()->getBool("SystemStatusBluetooth"));
    addWithLabel(_("DISPLAY BLUETOOTH STATUS INDICATOR"), systemStatusBluetooth);
    addSaveFunc([systemStatusBluetooth, this] {
        if (systemStatusBluetooth->getState() !=
            Settings::getInstance()->getBool("SystemStatusBluetooth")) {
            Settings::getInstance()->setBool("SystemStatusBluetooth",
                                             systemStatusBluetooth->getState());
            setNeedsSaving();
            setNeedsUpdateStatusComponents();
        }
    });

    // Display WiFi indicator.
    auto systemStatusWifi = std::make_shared<SwitchComponent>();
    systemStatusWifi->setState(Settings::getInstance()->getBool("SystemStatusWifi"));
    addWithLabel(_("DISPLAY WI-FI STATUS INDICATOR"), systemStatusWifi);
    addSaveFunc([systemStatusWifi, this] {
        if (systemStatusWifi->getState() != Settings::getInstance()->getBool("SystemStatusWifi")) {
            Settings::getInstance()->setBool("SystemStatusWifi", systemStatusWifi->getState());
            setNeedsSaving();
            setNeedsUpdateStatusComponents();
        }
    });

#if defined(__ANDROID__)
    // Display cellular indicator.
    auto systemStatusCellular = std::make_shared<SwitchComponent>();
    systemStatusCellular->setState(Settings::getInstance()->getBool("SystemStatusCellular"));
    addWithLabel(_("DISPLAY CELLULAR STATUS INDICATOR"), systemStatusCellular);
    addSaveFunc([systemStatusCellular, this] {
        if (systemStatusCellular->getState() !=
            Settings::getInstance()->getBool("SystemStatusCellular")) {
            Settings::getInstance()->setBool("SystemStatusCellular",
                                             systemStatusCellular->getState());
            setNeedsSaving();
            setNeedsUpdateStatusComponents();
        }
    });
#endif

    // Display battery indicator.
    auto systemStatusBattery = std::make_shared<SwitchComponent>();
    systemStatusBattery->setState(Settings::getInstance()->getBool("SystemStatusBattery"));
    addWithLabel(_("DISPLAY BATTERY STATUS INDICATOR"), systemStatusBattery);
    addSaveFunc([systemStatusBattery, this] {
        if (systemStatusBattery->getState() !=
            Settings::getInstance()->getBool("SystemStatusBattery")) {
            Settings::getInstance()->setBool("SystemStatusBattery",
                                             systemStatusBattery->getState());
            setNeedsSaving();
            setNeedsUpdateStatusComponents();
        }
    });

    // Display battery charge percentage.
    auto systemStatusBatteryPercentage = std::make_shared<SwitchComponent>();
    systemStatusBatteryPercentage->setState(
        Settings::getInstance()->getBool("SystemStatusBatteryPercentage"));
    addWithLabel(_("DISPLAY BATTERY CHARGE PERCENTAGE"), systemStatusBatteryPercentage);
    addSaveFunc([systemStatusBatteryPercentage, this] {
        if (systemStatusBatteryPercentage->getState() !=
            Settings::getInstance()->getBool("SystemStatusBatteryPercentage")) {
            Settings::getInstance()->setBool("SystemStatusBatteryPercentage",
                                             systemStatusBatteryPercentage->getState());
            setNeedsSaving();
            setNeedsUpdateStatusComponents();
        }
    });

    // Gray out the battery charge percentage option if the battery setting has been disabled.
    if (!Settings::getInstance()->getBool("SystemStatusBattery")) {
        systemStatusBatteryPercentage->setEnabled(false);
        systemStatusBatteryPercentage->setOpacity(DISABLED_OPACITY);
        systemStatusBatteryPercentage->getParent()
            ->getChild(systemStatusBatteryPercentage->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    auto batteryToggleFunc = [systemStatusBatteryPercentage]() {
        if (systemStatusBatteryPercentage->getEnabled()) {
            systemStatusBatteryPercentage->setEnabled(false);
            systemStatusBatteryPercentage->setOpacity(DISABLED_OPACITY);
            systemStatusBatteryPercentage->getParent()
                ->getChild(systemStatusBatteryPercentage->getChildIndex() - 1)
                ->setOpacity(DISABLED_OPACITY);
        }
        else {
            systemStatusBatteryPercentage->setEnabled(true);
            systemStatusBatteryPercentage->setOpacity(1.0f);
            systemStatusBatteryPercentage->getParent()
                ->getChild(systemStatusBatteryPercentage->getChildIndex() - 1)
                ->setOpacity(1.0f);
        }
    };

    systemStatusBattery->setCallback(batteryToggleFunc);
}
