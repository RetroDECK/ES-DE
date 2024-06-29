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

#include <algorithm>
#include <iostream>

namespace Utils
{
    namespace Localization
    {
        std::string getLocale()
        {
            std::string language;

            // The LANGUAGE environment variable takes precedence over LANG.
            if (getenv("LANGUAGE") != nullptr)
                language = getenv("LANGUAGE");

            const std::vector<std::string> languageValues {
                Utils::String::delimitedStringToVector(language, ":")};

            for (auto value : languageValues) {
                if (std::find(sSupportedLanguages.cbegin(), sSupportedLanguages.cend(), value) !=
                    sSupportedLanguages.cend()) {
                    return value;
                }
            }

            if (getenv("LANG") != nullptr)
                language = getenv("LANG");

            if (language.empty())
                return "en_US";

            return language.substr(0, language.find("."));
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
            if (locale == "en_US")
                return;

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

            setenv("LANGUAGE", locale.c_str(), 1);
            setlocale(LC_MESSAGES, "");
            textdomain(locale.c_str());
            bindtextdomain(locale.c_str(), objectPath.c_str());
            bind_textdomain_codeset(locale.c_str(), "UTF-8");
        }

    } // namespace Localization

} // namespace Utils
