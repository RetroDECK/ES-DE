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
        std::string getLocale()
        {
#if defined(_WIN64)
            std::wstring localeName(LOCALE_NAME_MAX_LENGTH, '\0');
            if (GetUserDefaultLocaleName(&localeName[0], LOCALE_NAME_MAX_LENGTH) == 0)
                return "en_US";

            // Of course Windows doesn't follow standards and names locales with dashes instead
            // of underscores, such as "sv-SE" instead of "sv_SE".
            std::string locale {
                Utils::String::replace(Utils::String::wideStringToString(localeName), "-", "_")};
            locale.erase(locale.find('\0'));

            return locale;
#else
            // SDL_GetPreferredLocales() does not seem to always return accurate results
            // on Windows but for all other operating systems we use it.
            SDL_Locale* preferredLocales {SDL_GetPreferredLocales()};

            if (preferredLocales == nullptr)
                return "en_US";

            std::string primaryLocale {preferredLocales->language};
            if (preferredLocales->country != nullptr)
                primaryLocale.append("_").append(preferredLocales->country);

            SDL_free(preferredLocales);
            return primaryLocale;
#endif
        }

        void setLanguage(const std::string& locale)
        {
            if (std::find(sSupportedLanguages.cbegin(), sSupportedLanguages.cend(), locale) ==
                sSupportedLanguages.cend()) {
                LOG(LogInfo) << "No support for language \"" << locale
                             << "\", reverting to default language \"en_US\"";
                return;
            }
            else {
                LOG(LogInfo) << "Setting application language to \"" << locale << "\"";
            }

            // No need to perform translations if we're using the default language.
            if (locale == "en_US") {
                setenv("LANGUAGE", locale.c_str(), 1);
                setenv("LANG", locale.c_str(), 1);
                setlocale(LC_MESSAGES, std::string {locale + ".UTF-8"}.c_str());
                textdomain(locale.c_str());
                return;
            }

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
            setlocale(LC_MESSAGES, std::string {locale + ".UTF-8"}.c_str());
#endif
            textdomain(locale.c_str());
            bindtextdomain(locale.c_str(), objectPath.c_str());
            bind_textdomain_codeset(locale.c_str(), "UTF-8");
        }

    } // namespace Localization

} // namespace Utils
