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
#define _n(STR1, STR2, NUM) std::string(ngettext(STR1, STR2, NUM))

namespace Utils
{
    namespace Localization
    {
        extern const std::vector<std::pair<std::string, std::string>> sSupportedLocales;
        extern float sMenuTitleScaleFactor;

        std::pair<std::string, std::string> getLocale();
        void setLocale();

    } // namespace Localization

} // namespace Utils

#endif // ES_CORE_UTILS_LOCALIZATION_UTIL_H
