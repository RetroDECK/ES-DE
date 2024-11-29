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
#define _p(STR1, STR2) Utils::Localization::pgettextBuiltin(STR1, STR2)
#define _np(STR1, STR2, STR3, NUM) Utils::Localization::npgettextBuiltin(STR1, STR2, STR3, NUM)

namespace Utils
{
    namespace Localization
    {
        extern const std::vector<std::pair<std::string, std::string>> sSupportedLocales;
        extern std::string sCurrentLocale;
#if defined(_WIN64)
        extern unsigned long sLocaleID;
#endif
        extern float sMenuTitleScaleFactor;

        const char* pgettextBuiltin(const char* msgctxt, const char* msgid);
        const char* npgettextBuiltin(const char* msgctxt,
                                     const char* msgid1,
                                     const char* msgid2,
                                     unsigned long int n);
        std::pair<std::string, std::string> getLocale();
        void setLocale();
#if defined(_WIN64)
        void setThreadLocale();
#endif

    } // namespace Localization

} // namespace Utils

#endif // ES_CORE_UTILS_LOCALIZATION_UTIL_H
