//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  SystemStatus.cpp
//
//  Queries system status information from the operating system.
//  This includes Bluetooth, Wi-Fi, cellular and battery.
//

#include "SystemStatus.h"

#include "Log.h"
#include "Settings.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

#include <SDL2/SDL_timer.h>

#include <algorithm>

#if defined(__linux__) && !defined(__ANDROID__)
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#endif

#if defined(__APPLE__) && !defined(__IOS__)
#include "BluetoothStatusApple.h"
#include <IOKit/ps/IOPSKeys.h>
#include <IOKit/ps/IOPowerSources.h>
#include <SystemConfiguration/SCNetworkConfiguration.h>
#endif

#if defined(_WIN64)
// clang-format off
// Because of course building fails if the files are included in the "wrong" order.
#include <windows.h>
#include <iphlpapi.h>
#include <bluetoothapis.h>
// clang-format on
#endif

#if defined(__ANDROID__)
#include "utils/PlatformUtilAndroid.h"
#endif

#define DEBUG_SYSTEM_STATUS false

SystemStatus::SystemStatus() noexcept
    : mExitPolling {false}
    , mPollImmediately {false}
    , mHasBluetooth {false}
    , mHasWifi {false}
    , mHasCellular {false}
    , mHasBattery {false}
    , mBatteryCharging {false}
    , mBatteryCapacity {0}
{
    setCheckFlags();

#if defined(__ANDROID__)
    // Polling the device status is very fast on Android and it's quite problematic to run
    // these calls in a separate thread anyway.
    getStatusBluetooth();
    getStatusWifi();
    getStatusCellular();
    getStatusBattery();
#elif !defined(__FreeBSD__) && !defined(__HAIKU__)
    mPollThread = std::make_unique<std::thread>(&SystemStatus::pollStatus, this);
#endif
}

SystemStatus::~SystemStatus()
{
#if !defined(__ANDROID__) && !defined(__FreeBSD__) && !defined(__HAIKU__)
    mExitPolling = true;

    if (mPollThread != nullptr && mPollThread->joinable()) {
        mPollThread->join();
        mPollThread.reset();
    }
#endif
}

SystemStatus& SystemStatus::getInstance()
{
    static SystemStatus instance;
    return instance;
}

void SystemStatus::setCheckFlags()
{
    std::unique_lock<std::mutex> statusLock {mStatusMutex};
    mCheckBluetooth = Settings::getInstance()->getBool("SystemStatusBluetooth");
    mCheckWifi = Settings::getInstance()->getBool("SystemStatusWifi");
    mCheckCellular = Settings::getInstance()->getBool("SystemStatusCellular");
    mCheckBattery = Settings::getInstance()->getBool("SystemStatusBattery");

    if (!mCheckBluetooth)
        mHasBluetooth = false;
    if (!mCheckWifi)
        mHasWifi = false;
    if (!mCheckCellular)
        mHasCellular = false;
    if (!mCheckBattery)
        mHasBattery = false;
}

void SystemStatus::setPolling(const bool state)
{
#if defined(__ANDROID__)
    return;
#endif

    if (state == false) {
        mExitPolling = true;
        if (mPollThread != nullptr && mPollThread->joinable()) {
            mPollThread->join();
            mPollThread.reset();
        }
    }
    else if (mPollThread == nullptr) {
        mExitPolling = false;
        mPollThread = std::make_unique<std::thread>(&SystemStatus::pollStatus, this);
    }
}

SystemStatus::Status SystemStatus::getStatus(const bool update)
{
#if defined(__ANDROID__)
    if (update) {
        getStatusBluetooth();
        getStatusWifi();
        getStatusCellular();
        getStatusBattery();
#if (DEBUG_SYSTEM_STATUS)
        std::string status {"Bluetooth "};
        status.append(mHasBluetooth ? "enabled" : "disabled")
            .append(", Wi-Fi ")
            .append(mHasWifi ? "enabled" : "disabled")
            .append(", cellular ")
            .append(mHasCellular ? "enabled" : "disabled")
            .append(", battery ")
            .append(mHasBattery ? "enabled" : "disabled");
        if (mHasBattery) {
            status.append(" (")
                .append(mBatteryCharging ? "charging" : "not charging")
                .append(" and at ")
                .append(std::to_string(mBatteryCapacity))
                .append("% capacity)");
        }
        LOG(LogDebug) << "SystemStatus::getStatus(): " << status;
#endif
    }
#endif

    mStatus.hasBluetooth = mHasBluetooth;
    mStatus.hasWifi = mHasWifi;
    mStatus.hasCellular = mHasCellular;
    mStatus.hasBattery = mHasBattery;
    mStatus.batteryCharging = mBatteryCharging;
    mStatus.batteryCapacity = mBatteryCapacity;

    return mStatus;
}

void SystemStatus::pollStatus()
{
    while (!mExitPolling) {
        std::unique_lock<std::mutex> statusLock {mStatusMutex};

        getStatusBluetooth();
        getStatusWifi();
        getStatusCellular();
        getStatusBattery();
        statusLock.unlock();

#if (DEBUG_SYSTEM_STATUS)
        std::string status {"Bluetooth "};
        status.append(mHasBluetooth ? "enabled" : "disabled")
            .append(", Wi-Fi ")
            .append(mHasWifi ? "enabled" : "disabled")
            .append(", cellular ")
            .append(mHasCellular ? "enabled" : "disabled")
            .append(", battery ")
            .append(mHasBattery ? "enabled" : "disabled");
        if (mHasBattery) {
            status.append(" (")
                .append(mBatteryCharging ? "charging" : "not charging")
                .append(" and at ")
                .append(std::to_string(mBatteryCapacity))
                .append("% capacity)");
        }
        LOG(LogDebug) << "SystemStatus::pollStatus(): " << status;
#endif

        int delayValue {0};
        while (!mPollImmediately && !mExitPolling && delayValue < pollingTime) {
            delayValue += 100;
            SDL_Delay(100);
        }

        mPollImmediately = false;
    }
}

void SystemStatus::getStatusBluetooth()
{
    if (!mCheckBluetooth)
        return;

    bool hasBluetooth {false};

#if defined(__APPLE__) && !defined(__IOS__)
    if (getBluetoothStatus() == 1)
        hasBluetooth = true;

#elif defined(_WIN64)
    BLUETOOTH_FIND_RADIO_PARAMS btFindRadio {sizeof(BLUETOOTH_FIND_RADIO_PARAMS)};
    HANDLE btRadio {nullptr};
    BLUETOOTH_RADIO_INFO btInfo {sizeof(BLUETOOTH_RADIO_INFO), 0};

    if (BluetoothFindFirstRadio(&btFindRadio, &btRadio) != nullptr) {
        if (BluetoothGetRadioInfo(btRadio, &btInfo) == ERROR_SUCCESS)
            hasBluetooth = true;
    }

#elif defined(__ANDROID__)
    if (Utils::Platform::Android::getBluetoothStatus())
        hasBluetooth = true;

#elif defined(__linux__)
    if (hci_get_route(nullptr) != -1)
        hasBluetooth = true;
#endif

    mHasBluetooth = hasBluetooth;
}

void SystemStatus::getStatusWifi()
{
    if (!mCheckWifi)
        return;

    bool hasWifi {false};

#if defined(__APPLE__) && !defined(__IOS__)
    const CFArrayRef interfaces {SCNetworkInterfaceCopyAll()};

    if (interfaces != nullptr) {
        for (CFIndex i {0}; i < CFArrayGetCount(interfaces); ++i) {
            SCNetworkInterfaceRef interface {
                static_cast<SCNetworkInterfaceRef>(CFArrayGetValueAtIndex(interfaces, i))};

            if (SCNetworkInterfaceGetInterfaceType(interface) == kSCNetworkInterfaceTypeIEEE80211) {
                const CFStringRef bsdName {SCNetworkInterfaceGetBSDName(interface)};

                const SCDynamicStoreRef session {
                    SCDynamicStoreCreate(nullptr, CFSTR("Custom"), nullptr, nullptr)};

                const CFStringRef resolvedQuery {CFStringCreateWithFormat(
                    nullptr, nullptr, CFSTR("State:/Network/Interface/%@/IPv4"), bsdName)};

                const CFDictionaryRef dict {
                    static_cast<CFDictionaryRef>(SCDynamicStoreCopyValue(session, resolvedQuery))};

                if (dict != nullptr) {
                    hasWifi = true;
                    CFRelease(dict);
                    CFRelease(resolvedQuery);
                    CFRelease(session);
                    break;
                }
                else {
                    CFRelease(resolvedQuery);
                    CFRelease(session);
                }
            }
        }
        CFRelease(interfaces);
    }

#elif defined(_WIN64)
    PIP_ADAPTER_INFO pAdapterInfo {nullptr};
    PIP_ADAPTER_INFO pAdapter {nullptr};
    ULONG ulOutBufLen {sizeof(IP_ADAPTER_INFO)};
    pAdapterInfo = reinterpret_cast<IP_ADAPTER_INFO*>(malloc(sizeof(IP_ADAPTER_INFO)));

    if (pAdapterInfo != nullptr) {
        // Make an initial call to GetAdaptersInfo to get the necessary size into the
        // ulOutBufLen variable, which may or may not be big enough.
        if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
            free(pAdapterInfo);
            pAdapterInfo = reinterpret_cast<IP_ADAPTER_INFO*>(malloc(ulOutBufLen));
        }
        if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR) {
            pAdapter = pAdapterInfo;
            while (pAdapter) {
                if (pAdapter->Type == IF_TYPE_IEEE80211) {
                    // Checking whether the interface has an IP address is crude but
                    // it seems to get the job done. And there is no other obvious
                    // way to query the interface status without using additional
                    // convoluted API calls.
                    if (const std::string {pAdapter->IpAddressList.IpAddress.String} != "0.0.0.0") {
                        hasWifi = true;
                        break;
                    }
                }
                pAdapter = pAdapter->Next;
            }
        }

        if (pAdapterInfo)
            free(pAdapterInfo);
    }

#elif defined(__ANDROID__)
    if (Utils::Platform::Android::getWifiStatus() == 1)
        hasWifi = true;

#elif defined(__linux__)
    const std::string sysEntry {"/sys/class/net"};
    auto entries {Utils::FileSystem::getDirContent(sysEntry, false)};
    for (auto& entry : entries) {
        if (Utils::FileSystem::exists(entry + "/wireless") &&
            Utils::FileSystem::exists(entry + "/operstate")) {
            std::string wifiState;
            std::ifstream fileStream;
            fileStream.open(entry + "/operstate");
            getline(fileStream, wifiState);
            fileStream.close();
            if (Utils::String::toLower(wifiState) == "up")
                hasWifi = true;
        }
    }
#endif

    mHasWifi = hasWifi;
}

void SystemStatus::getStatusCellular()
{
    if (!mCheckCellular)
        return;

    bool hasCellular {false};

#if defined(__ANDROID__)
    if (Utils::Platform::Android::getCellularStatus() >= 1)
        hasCellular = true;
#endif

    mHasCellular = hasCellular;
}

void SystemStatus::getStatusBattery()
{
    if (!mCheckBattery)
        return;

    bool hasBattery {false};
    bool batteryCharging {false};
    int batteryCapacity {0};

#if defined(__APPLE__) && !defined(__IOS__)
    CFTypeRef sourceInfo {IOPSCopyPowerSourcesInfo()};
    CFArrayRef sourceList {IOPSCopyPowerSourcesList(sourceInfo)};

    if (sourceList != nullptr && CFArrayGetCount(sourceList) > 0) {
        CFDictionaryRef source {nullptr};

        for (CFIndex i {0}; i < CFArrayGetCount(sourceList); ++i) {
            source =
                IOPSGetPowerSourceDescription(sourceInfo, CFArrayGetValueAtIndex(sourceList, i));
            // Check if this is a battery.
            const CFStringRef type {static_cast<CFStringRef>(
                CFDictionaryGetValue(source, CFSTR(kIOPSTransportTypeKey)))};
            if (kCFCompareEqualTo == CFStringCompare(type, CFSTR(kIOPSInternalType), 0))
                break;
            else
                source = nullptr;
        }

        if (source != nullptr) {
            hasBattery = true;

            if (CFDictionaryGetValue(source, CFSTR(kIOPSIsChargingKey)) != nullptr) {
                batteryCharging = CFBooleanGetValue(static_cast<CFBooleanRef>(
                    CFDictionaryGetValue(source, CFSTR(kIOPSIsChargingKey))));
            }

            int curCapacity {0};
            const CFNumberRef curCapacityNum {static_cast<CFNumberRef>(
                CFDictionaryGetValue(source, CFSTR(kIOPSCurrentCapacityKey)))};
            CFNumberGetValue(curCapacityNum, kCFNumberIntType, &curCapacity);

            int maxCapacity {0};
            const CFNumberRef maxCapacityNum {
                static_cast<CFNumberRef>(CFDictionaryGetValue(source, CFSTR(kIOPSMaxCapacityKey)))};
            CFNumberGetValue(maxCapacityNum, kCFNumberIntType, &maxCapacity);

            if (maxCapacity > 0) {
                batteryCapacity =
                    static_cast<float>(curCapacity) / static_cast<float>(maxCapacity) * 100.0f;
            }
        }
    }

    if (sourceInfo != nullptr)
        CFRelease(sourceInfo);
    if (sourceList != nullptr)
        CFRelease(sourceList);

#elif defined(_WIN64)
    SYSTEM_POWER_STATUS powerStatus;

    if (GetSystemPowerStatus(&powerStatus)) {
        if (powerStatus.BatteryFlag != 128 && powerStatus.BatteryFlag != 255) {
            hasBattery = true;

            if (powerStatus.ACLineStatus == 1)
                batteryCharging = true;

            batteryCapacity = powerStatus.BatteryLifePercent;
        }
        else {
            hasBattery = false;
        }
    }

#elif defined(__ANDROID__)
    std::pair<int, int> batteryStatus {Utils::Platform::Android::getBatteryStatus()};
    hasBattery = static_cast<bool>(batteryStatus.first);

    if (batteryStatus.first == -1 && batteryStatus.second == -1) {
        hasBattery = false;
    }
    else {
        hasBattery = true;
        if (batteryStatus.first == 1)
            batteryCharging = true;
    }

    batteryCapacity = batteryStatus.second;

#elif defined(__linux__)
    const std::string sysEntry {"/sys/class/power_supply"};
    std::string batteryDir;
    auto entries {Utils::FileSystem::getDirContent(sysEntry, false)};
    if (std::find(entries.cbegin(), entries.cend(), sysEntry + "/BAT0") != entries.cend())
        batteryDir = sysEntry + "/BAT0";
    else if (std::find(entries.cbegin(), entries.cend(), sysEntry + "/BAT1") != entries.cend())
        batteryDir = sysEntry + "/BAT1";
    else if (std::find(entries.cbegin(), entries.cend(), sysEntry + "/battery") != entries.cend())
        batteryDir = sysEntry + "/battery";

    hasBattery = true;

    if (!Utils::FileSystem::exists(batteryDir + "/status"))
        hasBattery = false;
    if (!Utils::FileSystem::exists(batteryDir + "/capacity"))
        hasBattery = false;

    if (hasBattery) {
        std::string batteryStatusValue;
        std::string batteryCapacityValue;
        std::ifstream fileStream;
        fileStream.open(batteryDir + "/status");
        getline(fileStream, batteryStatusValue);
        batteryStatusValue = Utils::String::toLower(batteryStatusValue);
        fileStream.close();

        if (batteryStatusValue != "discharging")
            batteryCharging = true;

        fileStream.open(batteryDir + "/capacity");
        getline(fileStream, batteryCapacityValue);
        fileStream.close();

        batteryCapacity = std::stoi(batteryCapacityValue);
    }
#endif

    if (batteryCapacity < 0)
        batteryCapacity = 0;
    if (batteryCapacity > 100)
        batteryCapacity = 100;

    mHasBattery = hasBattery;
    mBatteryCharging = batteryCharging;
    mBatteryCapacity = batteryCapacity;
}
