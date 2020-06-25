//
//  StringUtil.cpp
//
//  Low-level string functions.
//  Convert characters to Unicode, upper-/lowercase conversion, string formatting etc.
//

#include "utils/StringUtil.h"

#include <algorithm>
#include <stdarg.h>

namespace Utils
{
    namespace String
    {
        unsigned int chars2Unicode(const std::string& _string, size_t& _cursor)
        {
            const char& c = _string[_cursor];
            unsigned int result = '?';

            // 0xxxxxxx, one byte character.
            if ((c & 0x80) == 0) {
                // 0xxxxxxx
                result = ((_string[_cursor++]       )      );
            }
            // 110xxxxx, two byte character.
            else if ((c & 0xE0) == 0xC0) {
                // 110xxxxx 10xxxxxx
                result = (_string[_cursor++] & 0x1F) <<  6;
                result |= _string[_cursor++] & 0x3F;
//              result = ((_string[_cursor++] & 0x1F) <<  6) |
//                       ((_string[_cursor++] & 0x3F)      );
            }
            // 1110xxxx, three byte character.
            else if ((c & 0xF0) == 0xE0) {
                // 1110xxxx 10xxxxxx 10xxxxxx
                result = (_string[_cursor++] & 0x0F) << 12;
                result |= (_string[_cursor++] & 0x3F) <<  6;
                result |= _string[_cursor++] & 0x3F;
//              result = ((_string[_cursor++] & 0x0F) << 12) |
//                       ((_string[_cursor++] & 0x3F) <<  6) |
//                       ((_string[_cursor++] & 0x3F)      );
            }
            // 11110xxx, four byte character.
            else if ((c & 0xF8) == 0xF0) {
                // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
                result = (_string[_cursor++] & 0x07) << 18;
                result |= (_string[_cursor++] & 0x3F) << 12;
                result |= (_string[_cursor++] & 0x3F) <<  6;
                result |= _string[_cursor++] & 0x3F;
//              result = ((_string[_cursor++] & 0x07) << 18) |
//                       ((_string[_cursor++] & 0x3F) << 12) |
//                       ((_string[_cursor++] & 0x3F) <<  6) |
//                       ((_string[_cursor++] & 0x3F)      );
            }
            else {
                // Error, invalid unicode.
                ++_cursor;
            }

            return result;
        }

        std::string unicode2Chars(const unsigned int _unicode)
        {
            std::string result;

            // One byte character.
            if (_unicode < 0x80) {
                result += ((_unicode      ) & 0xFF);
            }
            // Two byte character.
            else if (_unicode < 0x800)  {
                result += ((_unicode >>  6) & 0xFF) | 0xC0;
                result += ((_unicode      ) & 0x3F) | 0x80;
            }
            // Three byte character.
            else if (_unicode < 0xFFFF) {
                result += ((_unicode >> 12) & 0xFF) | 0xE0;
                result += ((_unicode >>  6) & 0x3F) | 0x80;
                result += ((_unicode      ) & 0x3F) | 0x80;
            }
            // Four byte character.
            else if (_unicode <= 0x1fffff) {
                result += ((_unicode >> 18) & 0xFF) | 0xF0;
                result += ((_unicode >> 12) & 0x3F) | 0x80;
                result += ((_unicode >>  6) & 0x3F) | 0x80;
                result += ((_unicode      ) & 0x3F) | 0x80;
            }
            else {
                // Error, invalid unicode.
                result += '?';
            }

            return result;
        }

        size_t nextCursor(const std::string& _string, const size_t _cursor)
        {
            size_t result = _cursor;

            while (result < _string.length()) {
                ++result;

                // Break if current character is not 10xxxxxx
                if ((_string[result] & 0xC0) != 0x80)
                    break;
            }

            return result;
        }

        size_t prevCursor(const std::string& _string, const size_t _cursor)
        {
            size_t result = _cursor;

            while (result > 0) {
                --result;

                // Break if current character is not 10xxxxxx
                if ((_string[result] & 0xC0) != 0x80)
                    break;
            }

            return result;
        }

        size_t moveCursor(const std::string& _string, const size_t _cursor, const int _amount)
        {
            size_t result = _cursor;

            if (_amount > 0) {
                for (int i = 0; i < _amount; ++i)
                    result = nextCursor(_string, result);
            }
            else if (_amount < 0) {
                for (int i = _amount; i < 0; ++i)
                    result = prevCursor(_string, result);
            }

            return result;
        }

        std::string toLower(const std::string& _string)
        {
            std::string string;

            for (size_t i = 0; i < _string.length(); ++i)
                string += (char)tolower(_string[i]);

            return string;
        }

        std::string toUpper(const std::string& _string)
        {
            std::string string;

            for (size_t i = 0; i < _string.length(); ++i)
                string += (char)toupper(_string[i]);

            return string;
        }

        std::string trim(const std::string& _string)
        {
            const size_t strBegin = _string.find_first_not_of(" \t");
            const size_t strEnd = _string.find_last_not_of(" \t");

            if (strBegin == std::string::npos)
                return "";

            return _string.substr(strBegin, strEnd - strBegin + 1);
        }

        std::string replace(const std::string& _string, const std::string& _replace,
                const std::string& _with)
        {
            std::string string = _string;
            size_t pos;

            while ((pos = string.find(_replace)) != std::string::npos)
                string = string.replace(pos, _replace.length(), _with.c_str(), _with.length());

            return string;
        }

        bool startsWith(const std::string& _string, const std::string& _start)
        {
            return (_string.find(_start) == 0);
        }

        bool endsWith(const std::string& _string, const std::string& _end)
        {
            return (_string.find(_end) == (_string.size() - _end.size()));
        }

        std::string removeParenthesis(const std::string& _string)
        {
            static const char remove[4] = { '(', ')', '[', ']' };
            std::string string = _string;
            size_t start;
            size_t end;
            bool done = false;

            while (!done) {
                done = true;

                for (size_t i = 0; i < sizeof(remove); i += 2) {
                    end   = string.find_first_of(remove[i + 1]);
                    start = string.find_last_of( remove[i + 0], end);

                    if ((start != std::string::npos) && (end != std::string::npos)) {
                        string.erase(start, end - start + 1);
                        done = false;
                    }
                }
            }

            return trim(string);
        }

        stringVector delimitedStringToVector(const std::string& _string,
                const std::string& _delimiter, bool sort)
        {
            stringVector vector;
            size_t start = 0;
            size_t comma = _string.find(_delimiter);

            while (comma != std::string::npos) {
                vector.push_back(_string.substr(start, comma - start));
                start = comma + 1;
                comma = _string.find(_delimiter, start);
            }

            vector.push_back(_string.substr(start));
            if (sort)
                std::sort(vector.begin(), vector.end());

            return vector;
        }

        stringVector commaStringToVector(const std::string& _string, bool sort)
        {
            return delimitedStringToVector(_string, ",", sort);
        }

        std::string vectorToCommaString(stringVector _vector)
        {
            std::string string;

            std::sort(_vector.begin(), _vector.end());

            for (stringVector::const_iterator it = _vector.cbegin(); it != _vector.cend(); ++it)
                string += (string.length() ? "," : "") + (*it);

            return string;
        }

        std::string format(const char* _format, ...)
        {
            va_list	args;
            va_list copy;

            va_start(args, _format);

            va_copy(copy, args);
            const int length = vsnprintf(nullptr, 0, _format, copy);
            va_end(copy);

            char* buffer = new char[length + 1];
            va_copy(copy, args);
            vsnprintf(buffer, length + 1, _format, copy);
            va_end(copy);

            va_end(args);

            std::string out(buffer);
            delete[] buffer;

            return out;
        }

        std::string scramble(const std::string& _input, const std::string& _key)
        {
            std::string buffer = _input;

            for (size_t i = 0; i < _input.size(); ++i)
                buffer[i] = _input[i] ^ _key[i];

            return buffer;
        }

    } // String::
} // Utils::
