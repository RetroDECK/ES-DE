//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMDeviceAuthClient.cpp
//

#include "RomM/RomMDeviceAuthClient.h"

#include "ApplicationVersion.h"
#include "HttpReq.h"
#include "Log.h"
#include "RomM/RomMUtils.h"
#include "utils/StringUtil.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <chrono>
#include <thread>

using namespace rapidjson;

namespace
{
    // Unlike a normal RomMApiClient request, the device-auth endpoints use HTTP 400 as a
    // normal outcome (pending/denied/etc.) whose body callers must inspect, so this treats a
    // bad status code as a terminal response too rather than a failure.
    bool waitForAnyResponse(HttpReq& req, std::string& outError)
    {
        constexpr int maxIterations {30000 / 50};
        for (int i {0}; i < maxIterations; ++i) {
            const HttpReq::Status status {req.status()};
            if (status == HttpReq::REQ_SUCCESS || status == HttpReq::REQ_BAD_STATUS_CODE)
                return true;
            if (status != HttpReq::REQ_IN_PROGRESS) {
                outError = req.getErrorMsg();
                if (outError.empty())
                    outError = "RomM request failed";
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        outError = "RomM request timed out";
        return false;
    }

    std::string deviceAuthPlatformString()
    {
#if defined(__FreeBSD__)
        return "freebsd";
#elif defined(__HAIKU__)
        return "haiku";
#elif defined(__ANDROID__)
        return "android";
#elif defined(_WIN64)
        return "windows";
#elif defined(__IOS__)
        return "ios";
#elif defined(__APPLE__)
        return "macos";
#elif defined(__linux__)
        return "linux";
#elif defined(__unix__)
        return "unix";
#else
        return "unknown";
#endif
    }

    const std::vector<std::string>& deviceAuthScopes()
    {
        static const std::vector<std::string> scopes {
            "me.read",         "roms.read",      "roms.write",  "roms.user.read",
            "roms.user.write", "platforms.read", "assets.read", "collections.read",
            "firmware.read",   "devices.read"};
        return scopes;
    }

    std::string buildDeviceAuthInitBody(const std::string& clientDeviceIdentifier,
                                        const std::string& deviceName)
    {
        StringBuffer buffer;
        Writer<StringBuffer> writer {buffer};
        writer.StartObject();
        writer.Key("client_device_identifier");
        writer.String(clientDeviceIdentifier.c_str());
        writer.Key("name");
        writer.String(deviceName.c_str());
        writer.Key("client");
        writer.String("es-de");
        writer.Key("platform");
        writer.String(deviceAuthPlatformString().c_str());
        writer.Key("client_version");
        writer.String(PROGRAM_VERSION_STRING);
        writer.Key("requested_scopes");
        writer.StartArray();
        for (const auto& scope : deviceAuthScopes())
            writer.String(scope.c_str());
        writer.EndArray();
        writer.EndObject();
        return buffer.GetString();
    }

    std::string buildDeviceAuthTokenBody(const std::string& deviceCode)
    {
        StringBuffer buffer;
        Writer<StringBuffer> writer {buffer};
        writer.StartObject();
        writer.Key("device_code");
        writer.String(deviceCode.c_str());
        writer.EndObject();
        return buffer.GetString();
    }
} // namespace

RomMDeviceAuthClient::RomMDeviceAuthClient(const std::string& serverURL)
    : mServerURL {serverURL}
{
}

bool RomMDeviceAuthClient::initDeviceAuth(const std::string& clientDeviceIdentifier,
                                          const std::string& deviceName,
                                          DeviceAuthInit& outInit,
                                          std::string& outError)
{
    // failOnHttpError=false: a non-201 response's body still needs reading below.
    const std::string body {buildDeviceAuthInitBody(clientDeviceIdentifier, deviceName)};
    HttpReq req {
        RomMUtils::joinUrl(mServerURL, "/api/auth/device/init"), false, "", "", body, false};
    if (!waitForAnyResponse(req, outError))
        return false;

    if (req.getHttpStatusCode() != 201) {
        outError = Utils::String::format("RomM device pairing init failed (HTTP %d)",
                                         static_cast<int>(req.getHttpStatusCode()));
        return false;
    }

    Document doc;
    doc.Parse(req.getContent().c_str());
    if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("device_code") ||
        !doc["device_code"].IsString() || !doc.HasMember("user_code") ||
        !doc["user_code"].IsString() || !doc.HasMember("verification_path") ||
        !doc["verification_path"].IsString() || !doc.HasMember("verification_path_complete") ||
        !doc["verification_path_complete"].IsString() || !doc.HasMember("expires_in") ||
        !doc["expires_in"].IsInt() || !doc.HasMember("interval") || !doc["interval"].IsInt()) {
        outError = "Unexpected RomM device-auth init response format";
        LOG(LogError) << outError;
        return false;
    }

    outInit.deviceCode = doc["device_code"].GetString();
    outInit.userCode = doc["user_code"].GetString();
    outInit.verificationPath = doc["verification_path"].GetString();
    outInit.verificationPathComplete = doc["verification_path_complete"].GetString();
    outInit.expiresIn = doc["expires_in"].GetInt();
    outInit.interval = doc["interval"].GetInt();
    return true;
}

RomMDeviceAuthClient::TokenPollResult RomMDeviceAuthClient::pollDeviceAuthToken(
    const std::string& deviceCode, DeviceAuthToken& outToken, std::string& outError)
{
    // failOnHttpError=false: pending/denied/expired all arrive as HTTP 400 with a JSON body
    // that must be read below (CURLOPT_FAILONERROR would otherwise discard it).
    const std::string body {buildDeviceAuthTokenBody(deviceCode)};
    HttpReq req {
        RomMUtils::joinUrl(mServerURL, "/api/auth/device/token"), false, "", "", body, false};
    if (!waitForAnyResponse(req, outError))
        return TokenPollResult::Error;

    const long httpStatus {req.getHttpStatusCode()};

    if (httpStatus == 200) {
        Document doc;
        doc.Parse(req.getContent().c_str());
        if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("access_token") ||
            !doc["access_token"].IsString() || !doc.HasMember("device_id") ||
            !doc["device_id"].IsString()) {
            outError = "Unexpected RomM device-auth token response format";
            LOG(LogError) << outError;
            return TokenPollResult::Error;
        }
        outToken.accessToken = doc["access_token"].GetString();
        outToken.deviceId = doc["device_id"].GetString();
        outToken.scopes.clear();
        if (doc.HasMember("scopes") && doc["scopes"].IsArray()) {
            for (const auto& scope : doc["scopes"].GetArray()) {
                if (scope.IsString())
                    outToken.scopes.emplace_back(scope.GetString());
            }
        }
        outToken.expiresAt.clear();
        if (doc.HasMember("expires_at") && doc["expires_at"].IsString())
            outToken.expiresAt = doc["expires_at"].GetString();
        return TokenPollResult::Success;
    }

    if (httpStatus == 429)
        return TokenPollResult::SlowDown;

    if (httpStatus == 400) {
        Document doc;
        doc.Parse(req.getContent().c_str());
        std::string detail;
        if (!doc.HasParseError() && doc.IsObject() && doc.HasMember("detail") &&
            doc["detail"].IsString())
            detail = doc["detail"].GetString();

        if (detail == "authorization_pending")
            return TokenPollResult::Pending;
        if (detail == "slow_down")
            return TokenPollResult::SlowDown;
        if (detail == "access_denied")
            return TokenPollResult::Denied;
        if (detail == "expired_token")
            return TokenPollResult::Expired;

        if (detail.empty()) {
            outError = "Unexpected RomM device-auth token response";
            LOG(LogError) << outError;
        }
        else {
            outError = detail;
        }
        return TokenPollResult::Error;
    }

    outError = Utils::String::format("RomM device pairing poll failed (HTTP %d)",
                                     static_cast<int>(httpStatus));
    return TokenPollResult::Error;
}
