//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  StringUtil.h
//
//  Low-level string functions.
//  Convert characters to Unicode, upper-/lowercase conversion, string formatting etc.
//

#ifndef ES_CORE_UTILS_STRING_UTIL_H
#define ES_CORE_UTILS_STRING_UTIL_H

#include <codecvt>
#include <string>
#include <vector>

namespace Utils
{
    namespace String
    {
        unsigned int chars2Unicode(const std::string& stringArg, size_t& cursor);
        std::string unicode2Chars(const unsigned int unicodeArg);
        // Return the first character, which could be normal ASCII or 2, 3 or 4 byte Unicode.
        std::string getFirstCharacter(const std::string& stringArg, bool toUpper = true);
        size_t nextCursor(const std::string& stringArg, const size_t cursor);
        size_t prevCursor(const std::string& stringArg, const size_t cursor);
        size_t moveCursor(const std::string& stringArg, const size_t cursor, const int amount);
        std::string toLower(const std::string& stringArg);
        std::string toUpper(const std::string& stringArg);
        std::string toCapitalized(const std::string& stringArg);
        std::string trim(const std::string& stringArg);
        std::string replace(const std::string& stringArg,
                            const std::string& from,
                            const std::string& to);
        std::wstring stringToWideString(const std::string& stringArg);
        std::string wideStringToString(const std::wstring& stringArg);
        bool startsWith(const std::string& stringArg, const std::string& start);
        bool endsWith(const std::string& stringArg, const std::string& end);
        std::string removeParenthesis(const std::string& stringArg);
        std::vector<std::string> delimitedStringToVector(const std::string& stringArg,
                                                         const std::string& delimiter,
                                                         bool sort = false,
                                                         bool caseInsensitive = false);
        std::string vectorToDelimitedString(std::vector<std::string> vectorArg,
                                            const std::string& delimiter,
                                            bool caseInsensitive = false);
        std::string scramble(const std::string& input, const std::string& key);

    } // namespace String

} // namespace Utils

#endif // ES_CORE_UTILS_STRING_UTIL_H
