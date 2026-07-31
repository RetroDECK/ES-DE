//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMDeviceAuthClient.h
//
//  Thin client for RomM's device-authorization pairing protocol (POST /api/auth/device/init,
//  then polling /api/auth/device/token) - the unauthenticated counterpart to RomMApiClient.
//  Constructed with an already-resolved server URL (see RomMUtils::resolveServerUrl()).
//
//  Every method performs a blocking network call and must therefore only be invoked from a
//  background thread, never from the render/main thread.
//

#ifndef ES_APP_ROMM_ROMM_DEVICE_AUTH_CLIENT_H
#define ES_APP_ROMM_ROMM_DEVICE_AUTH_CLIENT_H

#include <string>
#include <vector>

class RomMDeviceAuthClient
{
public:
    // Mirrors RomM's DeviceAuthInitResponse schema.
    struct DeviceAuthInit {
        std::string deviceCode;
        std::string userCode;
        // Relative - join with the resolved server origin for a displayable/QR-able URL.
        std::string verificationPath;
        std::string verificationPathComplete;
        int expiresIn {0};
        int interval {0};
    };

    // Mirrors RomM's DeviceAuthTokenResponse schema.
    struct DeviceAuthToken {
        std::string accessToken;
        std::string deviceId;
        std::vector<std::string> scopes;
        std::string expiresAt; // empty if the token never expires
    };

    // Mirrors the "detail" values RomM's backend returns while polling /api/auth/device/token.
    enum class TokenPollResult {
        Pending,
        SlowDown,
        Denied,
        Expired,
        Success,
        Error
    };

    explicit RomMDeviceAuthClient(const std::string& serverURL);

    static const std::vector<std::string>& requiredScopes();

    // Succeeds only on HTTP 201.
    bool initDeviceAuth(const std::string& clientDeviceIdentifier,
                        const std::string& deviceName,
                        DeviceAuthInit& outInit,
                        std::string& outError);

    // A single poll attempt - caller owns the sleep/retry loop. Pending/SlowDown mean keep
    // polling; Denied/Expired/Error are terminal.
    TokenPollResult pollDeviceAuthToken(const std::string& deviceCode,
                                        DeviceAuthToken& outToken,
                                        std::string& outError);

private:
    std::string mServerURL;
};

#endif // ES_APP_ROMM_ROMM_DEVICE_AUTH_CLIENT_H
