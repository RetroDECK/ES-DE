//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  SystemStatus.h
//
//  Queries system status information from the operating system.
//  This includes Bluetooth, Wi-Fi, cellular and battery.
//

#ifndef ES_CORE_SYSTEM_STATUS_H
#define ES_CORE_SYSTEM_STATUS_H

#include <atomic>
#include <mutex>
#include <thread>

class SystemStatus
{
public:
    ~SystemStatus();
    static SystemStatus& getInstance();

    void setCheckFlags();
    void setPolling(const bool state);
    void setPollImmediately(const bool state) { mPollImmediately = state; }
    const bool getPollImmediately() { return mPollImmediately; }

    struct Status {
        bool hasBluetooth;
        bool hasWifi;
        bool hasCellular;
        bool hasBattery;
        bool batteryCharging;
        int batteryCapacity;
        Status()
            : hasBluetooth {false}
            , hasWifi {false}
            , hasCellular {false}
            , hasBattery {false}
            , batteryCharging {false}
            , batteryCapacity {0}
        {
        }
    };

    Status getStatus(const bool update = true);

    static constexpr int updateTime {300};
    static constexpr int pollingTime {2500};

private:
    SystemStatus() noexcept;

    void pollStatus();

    void getStatusBluetooth();
    void getStatusWifi();
    void getStatusCellular();
    void getStatusBattery();

    bool mCheckBluetooth;
    bool mCheckWifi;
    bool mCheckCellular;
    bool mCheckBattery;

    std::unique_ptr<std::thread> mPollThread;
    Status mStatus;
    std::mutex mStatusMutex;

    std::atomic<bool> mExitPolling;
    std::atomic<bool> mPollImmediately;

    std::atomic<bool> mHasBluetooth;
    std::atomic<bool> mHasWifi;
    std::atomic<bool> mHasCellular;
    std::atomic<bool> mHasBattery;
    std::atomic<bool> mBatteryCharging;
    std::atomic<int> mBatteryCapacity;
};

#endif // ES_CORE_SYSTEM_STATUS_H
