//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMUtils.h
//
//  Stateless RomM helpers shared across the API client, the device-pairing flow and the GUIs -
//  none of these need (or, in the case of resolveServerUrl()/checkHeartbeat(), even have yet)
//  a fixed server to be instance methods of a client.
//

#ifndef ES_APP_ROMM_ROMM_UTILS_H
#define ES_APP_ROMM_ROMM_UTILS_H

#include <string>

namespace RomMUtils
{
    // Non-empty RomMToken with an unexpired (or absent) RomMTokenExpiresAt. Local check only.
    bool isLoggedIn();

    std::string joinUrl(const std::string& serverURL, const std::string& path);

    bool checkHeartbeat(const std::string& serverURL);

    // Tries https:// then http:// if rawInput has no scheme. Returns true and sets
    // outResolvedUrl on success.
    bool resolveServerUrl(const std::string& rawInput, std::string& outResolvedUrl);
} // namespace RomMUtils

#endif // ES_APP_ROMM_ROMM_UTILS_H
