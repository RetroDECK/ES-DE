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

#include <string>
#include <vector>

namespace Utils
{
    namespace String
    {
        unsigned int chars2Unicode(const std::string& _string, size_t& _cursor);
        std::string unicode2Chars(const unsigned int _unicode);
        // Return the first character, which could be normal ASCII or 2, 3 or 4 byte Unicode.
        std::string getFirstCharacter(const std::string& _string, bool _toUpper = true);
        size_t nextCursor(const std::string& _string, const size_t _cursor);
        size_t prevCursor(const std::string& _string, const size_t _cursor);
        size_t moveCursor(const std::string& _string, const size_t _cursor, const int _amount);
        std::string toLower(const std::string& _string);
        std::string toUpper(const std::string& _string);
        std::string trim(const std::string& _string);
        std::string replace(const std::string& _string, const std::string& _replace,
                const std::string& _with);
        std::wstring stringToWideString(const std::string& _string);
        std::string wideStringToString(const std::wstring& _string);
        bool startsWith(const std::string& _string, const std::string& _start);
        bool endsWith(const std::string& _string, const std::string& _end);
        std::string removeParenthesis(const std::string& _string);
        std::vector<std::string> delimitedStringToVector(const std::string& _string,
                const std::string& _delimiter, bool sort = false, bool caseInsensitive = false);
        std::string vectorToDelimitedString(std::vector<std::string> _vector,
                const std::string& _delimiter, bool caseInsensitive = false);
        std::string format(const char* _string, ...);
        std::string scramble(const std::string& _input, const std::string& key);
    }
}

#endif // ES_CORE_UTILS_STRING_UTIL_H
