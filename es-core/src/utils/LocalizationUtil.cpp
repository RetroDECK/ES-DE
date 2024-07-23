//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  LocalizationUtil.cpp
//
//  Localization functions.
//  Provides support for translations using gettext/libintl.
//

#include "utils/LocalizationUtil.h"

#include "Log.h"
#include "Settings.h"
#include "resources/ResourceManager.h"
#include "utils/StringUtil.h"

#include <SDL2/SDL_locale.h>

#include <algorithm>
#include <iostream>

#if defined(_WIN64)
#include <Windows.h>
#endif

namespace Utils
{
    namespace Localization
    {
        // clang-format off
        const std::vector<std::pair<std::string, std::string>> sSupportedLocales {{{"en"}, {"US"}},
                                                                                  {{"en"}, {"GB"}},
                                                                                  {{"el"}, {"GR"}},
                                                                                  {{"es"}, {"ES"}},
                                                                                  {{"fr"}, {"FR"}},
                                                                                  {{"it"}, {"IT"}},
                                                                                  {{"ja"}, {"JP"}},
                                                                                  {{"pt"}, {"BR"}},
                                                                                  {{"ro"}, {"RO"}},
                                                                                  {{"ru"}, {"RU"}},
                                                                                  {{"sv"}, {"SE"}},
                                                                                  {{"zh"}, {"CN"}}};
        // clang-format on
        float sMenuTitleScaleFactor {1.0f};

        std::pair<std::string, std::string> getLocale()
        {
#if defined(_WIN64)
            std::wstring localeNameWide(LOCALE_NAME_MAX_LENGTH, '\0');
            if (GetUserDefaultLocaleName(&localeNameWide[0], LOCALE_NAME_MAX_LENGTH) == 0)
                return std::make_pair("en", "US");

            std::string localeName {Utils::String::wideStringToString(localeNameWide)};
            localeName.erase(localeName.find('\0'));

            // This should never happen, but who knows with Windows.
            if (localeName.empty())
                return std::make_pair("en", "US");

            std::vector<std::string> localeVector;

            // Of course Windows doesn't follow standards and names locales with dashes
            // instead of underscores, such as "sv-SE" instead of "sv_SE". But who knows
            // if this is consistent, so we check for underscores as an extra precaution.
            if (localeName.find("_") != std::string::npos)
                localeVector = Utils::String::delimitedStringToVector(localeName, "_");
            else
                localeVector = Utils::String::delimitedStringToVector(localeName, "-");

            if (localeVector.size() == 1)
                return std::make_pair(localeVector[0], "");
            else
                return std::make_pair(localeVector[0], localeVector[1]);
#else
            // SDL_GetPreferredLocales() does not seem to always return accurate results
            // on Windows but for all other operating systems we use it.
            SDL_Locale* preferredLocales {SDL_GetPreferredLocales()};

            if (preferredLocales == nullptr)
                return std::make_pair("en", "US");

            std::string language {preferredLocales->language};
            std::string country;
            if (preferredLocales->country != nullptr)
                country = preferredLocales->country;

            SDL_free(preferredLocales);
            return std::make_pair(language, country);
#endif
        }

        void setLocale()
        {
            // Only detect locale once (on application startup).
            if (Settings::getInstance()->getString("DetectedLocale") == "") {
                const std::pair<std::string, std::string> detectedLocale {getLocale()};
                if (detectedLocale.second == "")
                    Settings::getInstance()->setString("DetectedLocale", detectedLocale.first);
                else {
                    Settings::getInstance()->setString(
                        "DetectedLocale", detectedLocale.first + "_" + detectedLocale.second);
                }
            }

            sMenuTitleScaleFactor = 1.0f;
            std::string languageSetting {Settings::getInstance()->getString("ApplicationLanguage")};
            std::vector<std::string> localeVector;
            std::pair<std::string, std::string> localePair;

            if (languageSetting == "automatic") {
                localeVector = Utils::String::delimitedStringToVector(
                    Settings::getInstance()->getString("DetectedLocale"), "_");
            }
            else {
                localeVector = Utils::String::delimitedStringToVector(languageSetting, "_");
            }
            if (localeVector.size() == 1)
                localePair = std::make_pair(localeVector[0], "");
            else
                localePair = std::make_pair(localeVector[0], localeVector[1]);

            std::string locale;
            std::string localePairCombined;

            if (localePair.second == "")
                localePairCombined = localePair.first;
            else
                localePairCombined = localePair.first + "_" + localePair.second;

            if (std::find(sSupportedLocales.cbegin(), sSupportedLocales.cend(), localePair) !=
                sSupportedLocales.cend()) {
                locale = localePairCombined;
                LOG(LogInfo) << "Setting application locale to \"" << locale << "\"";
            }
            else {
                for (auto& localeEntry : sSupportedLocales) {
                    if (localeEntry.first == localePair.first) {
                        LOG(LogInfo) << "No support for locale \"" << localePairCombined
                                     << "\", falling back to closest match \""
                                     << localeEntry.first + "_" + localeEntry.second << "\"";
                        locale = localeEntry.first + "_" + localeEntry.second;
                        break;
                    }
                }
            }

            if (locale == "") {
                LOG(LogInfo) << "No support for locale \"" << localePairCombined
                             << "\", falling back to default \"en_US\"";
                locale = "en_US";
            }

            // Language-specific menu title scale factor.
            if (localePair.first == "sv")
                sMenuTitleScaleFactor = 0.87f;
            else if (localePair.first == "el")
                sMenuTitleScaleFactor = 0.92f;
            else if (localePair.first == "zh")
                sMenuTitleScaleFactor = 0.94f;

            std::string localePath;
            localePath.append("/")
                .append(locale)
                .append("/LC_MESSAGES/")
                .append(locale)
                .append(".mo");

            // If the message catalog file is not found then an emergency shutdown will be
            // initiated by ResourceManager.
            std::string objectPath {
                ResourceManager::getInstance().getResourcePath(":/locale" + localePath)};

            // This makes it possible to override the message catalog with a file in the
            // application data directory.
            if (objectPath.length() > localePath.length())
                objectPath = objectPath.substr(0, objectPath.length() - localePath.length());

#if defined(_WIN64)
            _configthreadlocale(_DISABLE_PER_THREAD_LOCALE);
            const LCID localeID {LocaleNameToLCID(Utils::String::stringToWideString(locale).c_str(),
                                                  LOCALE_ALLOW_NEUTRAL_NAMES)};
            SetThreadLocale(localeID);
#else
            setenv("LANGUAGE", locale.c_str(), 1);
            setenv("LANG", locale.c_str(), 1);
            // For some bizarre reason we need to first set the locale to en_US.UTF-8 before
            // we set it to the requested locale as some specific locales like pt_BR and zh_CN
            // otherwise won't work consistently. This must be some kind of library or OS bug as
            // it only happens on regular Linux, and not on macOS, Windows, Android or FreeBSD.
            setlocale(LC_MESSAGES, std::string {"en_US.UTF-8"}.c_str());

            setlocale(LC_MESSAGES, std::string {locale + ".UTF-8"}.c_str());
#endif
            textdomain(locale.c_str());
            bindtextdomain(locale.c_str(), objectPath.c_str());
            bind_textdomain_codeset(locale.c_str(), "UTF-8");
        }

    } // namespace Localization

} // namespace Utils
