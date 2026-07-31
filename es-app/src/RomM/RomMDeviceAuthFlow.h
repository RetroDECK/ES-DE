//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMDeviceAuthFlow.h
//
//  Runs RomM's device-authorization pairing flow (POST /api/auth/device/init, then repeated
//  polling of /api/auth/device/token) on a background thread. Mirrors the headless-engine/
//  GUI-driver split RomMLibrarySync uses with ViewController::runRomMSyncWithSplashScreen().
//

#ifndef ES_APP_ROMM_ROMM_DEVICE_AUTH_FLOW_H
#define ES_APP_ROMM_ROMM_DEVICE_AUTH_FLOW_H

#include <atomic>
#include <memory>
#include <string>
#include <thread>

class RomMDeviceAuthFlow
{
public:
    enum class State {
        Init,
        AwaitingApproval,
        Success,
        Denied,
        Expired,
        Error,
        Cancelled
    };

    // rawServerUrl is the as-typed server address, with or without a scheme - resolving it is
    // this class's own first step, on the background thread.
    explicit RomMDeviceAuthFlow(const std::string& rawServerUrl);
    ~RomMDeviceAuthFlow();

    void start();

    State getState() const { return mState; }

    // Valid once getState() is AwaitingApproval or a terminal state reached from it.
    const std::string& getVerificationUrl() const { return mVerificationUrl; }
    const std::string& getResolvedServerUrl() const { return mServerUrl; }

    // Valid once getState() == Success.
    const std::string& getAccessToken() const { return mAccessToken; }
    const std::string& getExpiresAt() const { return mExpiresAt; } // empty if no expiry

    // Valid once getState() == Error.
    const std::string& getLastError() const { return mLastError; }

    // Non-blocking - caller must keep polling getState() until it reaches Cancelled (or
    // another terminal state, if the flow finished just before noticing) before destroying.
    void cancel() { mCancelled = true; }

private:
    void run();

    std::string mServerUrl;
    std::unique_ptr<std::thread> mThread;
    std::atomic<State> mState;
    std::atomic<bool> mCancelled;

    std::string mVerificationUrl;
    std::string mAccessToken;
    std::string mExpiresAt;
    std::string mLastError;
};

#endif // ES_APP_ROMM_ROMM_DEVICE_AUTH_FLOW_H
