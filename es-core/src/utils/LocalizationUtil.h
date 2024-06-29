//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  LocalizationUtil.h
//
//  Localization functions.
//  Provides support for translations using gettext/libintl.
//

#ifndef ES_CORE_UTILS_LOCALIZATION_UTIL_H
#define ES_CORE_UTILS_LOCALIZATION_UTIL_H

#include <libintl.h>
#include <string>
#include <vector>

#define _(STR) std::string(gettext(STR))

namespace Utils
{
    namespace Localization
    {
        static inline std::vector<std::string> sSupportedLanguages {"en_US", "sv_SE"};

        std::string getLocale();
        void setLanguage(const std::string& locale);

    } // namespace Localization

} // namespace Utils

#endif // ES_CORE_UTILS_LOCALIZATION_UTIL_H
