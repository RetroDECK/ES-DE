//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMDeviceAuthFlow.cpp
//

#include "RomM/RomMDeviceAuthFlow.h"

#include "Log.h"
#include "RomM/RomMDeviceAuthClient.h"
#include "RomM/RomMUtils.h"
#include "Settings.h"

#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>

namespace
{
    std::string generateRandomHex(size_t byteCount)
    {
        std::random_device randomDevice;
        std::mt19937_64 generator {randomDevice()};
        std::uniform_int_distribution<int> byteDist {0, 255};

        std::ostringstream stream;
        for (size_t i {0}; i < byteCount; ++i)
            stream << std::hex << std::setfill('0') << std::setw(2) << byteDist(generator);
        return stream.str();
    }

    // Persisted so re-pairing updates the same RomM Device record instead of creating a new one.
    std::string getOrCreateDeviceIdentifier()
    {
        std::string identifier {Settings::getInstance()->getString("RomMDeviceIdentifier")};
        if (identifier.empty()) {
            identifier = generateRandomHex(16);
            Settings::getInstance()->setString("RomMDeviceIdentifier", identifier);
            Settings::getInstance()->saveFile();
        }
        return identifier;
    }

    std::string joinScopes(const std::vector<std::string>& scopes)
    {
        std::string joined;
        for (const auto& scope : scopes)
            joined += (joined.empty() ? "" : " ") + scope;
        return joined;
    }
} // namespace

RomMDeviceAuthFlow::RomMDeviceAuthFlow(const std::string& rawServerUrl)
    : mServerUrl {rawServerUrl}
    , mState {State::Init}
    , mCancelled {false}
{
}

RomMDeviceAuthFlow::~RomMDeviceAuthFlow()
{
    if (mThread) {
        mThread->join();
        mThread.reset();
    }
}

void RomMDeviceAuthFlow::start()
{
    mThread = std::make_unique<std::thread>(&RomMDeviceAuthFlow::run, this);
}

void RomMDeviceAuthFlow::run()
{
    LOG(LogInfo) << "RomM Device Pairing: Starting pairing server";

    std::string resolvedServerUrl;
    if (!RomMUtils::resolveServerUrl(mServerUrl, resolvedServerUrl)) {
        mLastError = "Couldn't reach the RomM server - check the server address";
        LOG(LogWarning) << "RomM Device Pairing: " << mLastError;
        mState = State::Error;
        return;
    }
    mServerUrl = resolvedServerUrl;

    RomMDeviceAuthClient authClient {mServerUrl};
    RomMDeviceAuthClient::DeviceAuthInit init;
    std::string error;

    if (!authClient.initDeviceAuth(getOrCreateDeviceIdentifier(), "ES-DE", init, error)) {
        mLastError = error;
        LOG(LogWarning) << "RomM Device Pairing: Init failed for server \"" << mServerUrl
                        << "\": " << mLastError;
        mState = State::Error;
        return;
    }

    mVerificationUrl = RomMUtils::joinUrl(mServerUrl, init.verificationPathComplete);
    mUserCode = init.userCode;
    mState = State::AwaitingApproval;

    int intervalSeconds {init.interval > 0 ? init.interval : 5};
    const int expiresInSeconds {init.expiresIn > 0 ? init.expiresIn : 600};
    const std::chrono::steady_clock::time_point deadline {std::chrono::steady_clock::now() +
                                                          std::chrono::seconds(expiresInSeconds)};

    LOG(LogInfo) << "RomM Device Pairing: Awaiting for user approval";

    while (true) {
        if (mCancelled) {
            LOG(LogInfo) << "RomM Device Pairing: Cancelled by the user";
            mState = State::Cancelled;
            return;
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            LOG(LogInfo) << "RomM Device Pairing: Expired without approval";
            mState = State::Expired;
            return;
        }

        for (int sleptMs {0}; sleptMs < intervalSeconds * 1000 && !mCancelled; sleptMs += 200)
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        if (mCancelled) {
            LOG(LogInfo) << "RomM Device Pairing: Cancelled by the user";
            mState = State::Cancelled;
            return;
        }

        RomMDeviceAuthClient::DeviceAuthToken token;
        const RomMDeviceAuthClient::TokenPollResult result {
            authClient.pollDeviceAuthToken(init.deviceCode, token, error)};

        switch (result) {
            case RomMDeviceAuthClient::TokenPollResult::Success:
                mAccessToken = token.accessToken;
                mExpiresAt = token.expiresAt;
                mState = State::Success;
                LOG(LogInfo) << "RomM Device Pairing: Paired successfully (device_id="
                             << token.deviceId << ", scopes=" << joinScopes(token.scopes)
                             << (mExpiresAt.empty() ? ", no expiry" : ", expires_at=" + mExpiresAt)
                             << ")";
                return;
            case RomMDeviceAuthClient::TokenPollResult::Denied:
                LOG(LogInfo) << "RomM Device Pairing: Denied by the user on the server";
                mState = State::Denied;
                return;
            case RomMDeviceAuthClient::TokenPollResult::Expired:
                LOG(LogInfo) << "RomM Device Pairing: Server reported the pairing code as "
                                "expired";
                mState = State::Expired;
                return;
            case RomMDeviceAuthClient::TokenPollResult::SlowDown:
                intervalSeconds += 5;
                LOG(LogDebug) << "RomM Device Pairing: Server asked to slow down polling, "
                                 "interval now "
                              << intervalSeconds << "s";
                break;
            case RomMDeviceAuthClient::TokenPollResult::Pending:
                break;
            case RomMDeviceAuthClient::TokenPollResult::Error:
                mLastError = error;
                LOG(LogWarning) << "RomM Device Pairing: Poll failed: " << mLastError;
                mState = State::Error;
                return;
        }
    }
}
