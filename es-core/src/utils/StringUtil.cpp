//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  StringUtil.cpp
//
//  Low-level string functions.
//

// Suppress codecvt deprecation warnings.
#if defined(_MSC_VER) // MSVC compiler.
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#elif defined(__clang__)
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "utils/StringUtil.h"
#include "Log.h"
#include "utils/PlatformUtil.h"

#include <unicode/brkiter.h>
#include <unicode/ustring.h>

#include <algorithm>
#include <cstdarg>
#include <locale>

namespace Utils
{
    namespace String
    {
        unsigned int chars2Unicode(const std::string& stringArg, size_t& cursor)
        {
            unsigned const char checkCharType {static_cast<unsigned char>(stringArg[cursor])};
            unsigned int result {'?'};

            // 0xxxxxxx, one byte character.
            if (checkCharType <= 0x7F) {
                // 0xxxxxxx
                result = (stringArg[cursor++]);
            }
            // 11110xxx, four byte character.
            else if (checkCharType >= 0xF0 && cursor < stringArg.length() - 2) {
                // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
                result = (stringArg[cursor++] & 0x07) << 18;
                result |= (stringArg[cursor++] & 0x3F) << 12;
                result |= (stringArg[cursor++] & 0x3F) << 6;
                result |= stringArg[cursor++] & 0x3F;
            }
            // 1110xxxx, three byte character.
            else if (checkCharType >= 0xE0 && cursor < stringArg.length() - 1) {
                // 1110xxxx 10xxxxxx 10xxxxxx
                result = (stringArg[cursor++] & 0x0F) << 12;
                result |= (stringArg[cursor++] & 0x3F) << 6;
                result |= stringArg[cursor++] & 0x3F;
            }
            // 110xxxxx, two byte character.
            else if (checkCharType >= 0xC0 && cursor < stringArg.length()) {
                // 110xxxxx 10xxxxxx
                result = (stringArg[cursor++] & 0x1F) << 6;
                result |= stringArg[cursor++] & 0x3F;
            }
            else {
                // Error, invalid character.
                ++cursor;
            }

            return result;
        }

        std::string unicode2Chars(const unsigned int unicodeArg)
        {
            std::string result;

            // Normal UTF-8 ASCII character.
            if (unicodeArg < 0x80) {
                result += (unicodeArg & 0xFF);
            }
            // Two-byte character.
            else if (unicodeArg < 0x800) {
                result += ((unicodeArg >> 6) & 0xFF) | 0xC0;
                result += (unicodeArg & 0x3F) | 0x80;
            }
            // Three-byte character.
            else if (unicodeArg < 0xFFFF) {
                result += ((unicodeArg >> 12) & 0xFF) | 0xE0;
                result += ((unicodeArg >> 6) & 0x3F) | 0x80;
                result += (unicodeArg & 0x3F) | 0x80;
            }
            // Four-byte character.
            else if (unicodeArg <= 0x1fffff) {
                result += ((unicodeArg >> 18) & 0xFF) | 0xF0;
                result += ((unicodeArg >> 12) & 0x3F) | 0x80;
                result += ((unicodeArg >> 6) & 0x3F) | 0x80;
                result += (unicodeArg & 0x3F) | 0x80;
            }
            else {
                // Error, invalid character.
                result += '?';
            }

            return result;
        }

        std::string getFirstCharacter(const std::string& stringArg, bool toUpper)
        {
            std::string firstChar;
            unsigned const char checkCharType {static_cast<unsigned char>(stringArg.front())};

            // Normal UTF-8 ASCII character.
            if (checkCharType <= 0x7F)
                (toUpper) ? firstChar = toupper(stringArg.front()) : firstChar = stringArg.front();
            // Four-byte Unicode character.
            else if (checkCharType >= 0xF0)
                firstChar = stringArg.substr(0, 4);
            // Three-byte Unicode character.
            else if (checkCharType >= 0xE0)
                firstChar = stringArg.substr(0, 3);
            // Two-byte Unicode character.
            else if (checkCharType >= 0xC0)
                firstChar = stringArg.substr(0, 2);

            return firstChar;
        }

        size_t nextCursor(const std::string& stringArg, const size_t cursor)
        {
            size_t result {cursor};

            while (result < stringArg.length()) {
                ++result;

                // Break if current character is not 10xxxxxx
                if ((stringArg[result] & 0xC0) != 0x80)
                    break;
            }

            return result;
        }

        size_t prevCursor(const std::string& stringArg, const size_t cursor)
        {
            size_t result {cursor};

            while (result > 0) {
                --result;

                // Break if current character is not 10xxxxxx
                if ((stringArg[result] & 0xC0) != 0x80)
                    break;
            }

            return result;
        }

        size_t moveCursor(const std::string& stringArg, const size_t cursor, const int amount)
        {
            size_t result {cursor};

            if (amount > 0) {
                for (int i {0}; i < amount; ++i)
                    result = nextCursor(stringArg, result);
            }
            else if (amount < 0) {
                for (int i {amount}; i < 0; ++i)
                    result = prevCursor(stringArg, result);
            }

            return result;
        }

        size_t unicodeLength(const std::string& stringArg)
        {
            size_t length {0};
            size_t charLength {0};

            for (size_t i {0}; i < stringArg.length(); i += charLength) {
                charLength = moveCursor(stringArg, i, 1) - i;
                ++length;
            }

            return length;
        }

        std::string toLower(const std::string& stringArg)
        {
            // IMPORTANT: On Windows specifically the StringPiece constructor which is implicitly
            // called by fromUTF8() crashes the application if a std::string is passed as the
            // argument. It's therefore necessary to use c_str() to work around this issue.
            // This behavior has been observed with ICU 75.1.
            icu::UnicodeString convert {icu::UnicodeString::fromUTF8(stringArg.c_str())};
            std::string stringLower;

            convert.toLower();
            return convert.toUTF8String(stringLower);
        }

        std::string toUpper(const std::string& stringArg)
        {
            icu::UnicodeString convert {icu::UnicodeString::fromUTF8(stringArg.c_str())};
            std::string stringUpper;

            convert.toUpper();
            return convert.toUTF8String(stringUpper);
        }

        std::string toCapitalized(const std::string& stringArg)
        {
            if (stringArg == "")
                return stringArg;

            UErrorCode status {U_ZERO_ERROR};
            std::unique_ptr<icu::BreakIterator> iterator {nullptr};
            // Since we don't know the actual text language we set it to locale en_US.
            iterator.reset(icu::BreakIterator::createWordInstance(icu::Locale::getUS(), status));

            if (U_FAILURE(status) || iterator == nullptr)
                return stringArg;

            icu::UnicodeString iterateString {
                icu::UnicodeString::fromUTF8(stringArg.c_str()).toLower()};

            if (iterateString != nullptr) {
                iterator->setText(iterateString);
                int32_t pos {iterator->first()};
                int32_t lastPos {pos};

                while (pos != icu::BreakIterator::DONE) {
                    iterateString.replace(lastPos, 1,
                                          icu::UnicodeString(iterateString, pos, 1).toUpper());
                    pos = iterator->next();
                    lastPos = pos;
                }
            }
            else {
                return stringArg;
            }

            std::string stringCapitalized;
            return iterateString.toUTF8String(stringCapitalized);
        }

        std::string filterUtf8(const std::string& stringArg)
        {
            std::string tempString;
            utf8::replace_invalid(stringArg.begin(), stringArg.end(), back_inserter(tempString));
            return tempString;
        }

        std::string trim(const std::string& stringArg)
        {
            std::string trimString {stringArg};

            // Trim leading and trailing whitespaces.
            trimString.erase(trimString.begin(),
                             std::find_if(trimString.begin(), trimString.end(), [](char c) {
                                 return !std::isspace(static_cast<unsigned char>(c));
                             }));
            trimString.erase(
                std::find_if(trimString.rbegin(), trimString.rend(),
                             [](char c) { return !std::isspace(static_cast<unsigned char>(c)); })
                    .base(),
                trimString.end());

            return trimString;
        }

        std::string replace(const std::string& stringArg,
                            const std::string& from,
                            const std::string& to)
        {
            std::string result {stringArg};

            // The outer loop makes sure that we're eliminating all repeating occurances
            // of the 'from' value.
            while (result.find(from) != std::string::npos) {
                // Prevent endless loops.
                if (from == to)
                    break;

                std::string replaced;
                size_t lastPos {0};
                size_t findPos {0};

                while ((findPos = result.find(from, lastPos)) != std::string::npos) {
                    replaced.append(result, lastPos, findPos - lastPos).append(to);
                    lastPos = findPos + from.length();
                }

                replaced.append(result.substr(lastPos));
                result = replaced;

                // Prevent endless loops.
                if (to.find(from) != std::string::npos)
                    break;
            }
            return result;
        }

        std::string format(const std::string stringArg, ...)
        {
            if (stringArg.empty())
                return "";

            // Extract all the variadic function arguments.
            va_list args;
            va_list copy;

            va_start(args, stringArg);
            va_copy(copy, args);

            const int length {vsnprintf(nullptr, 0, &stringArg[0], copy)};
            va_end(copy);
            std::string buffer(length, '\0');

            va_copy(copy, args);
            vsnprintf(&buffer[0], length + 1, &stringArg[0], copy);

            va_end(copy);
            va_end(args);

            return buffer;
        }

        std::wstring stringToWideString(const std::string& stringArg)
        {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> stringConverter;
            try {
                return stringConverter.from_bytes(stringArg);
            }
            catch (...) {
                LOG(LogError) << "StringUtil::stringToWideString(): Conversion failed, invalid "
                                 "characters in source string?";
                LOG(LogError) << stringArg;
                Utils::Platform::emergencyShutdown();
                return L"";
            }
        }

        std::string wideStringToString(const std::wstring& stringArg)
        {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> stringConverter;
            try {
                return stringConverter.to_bytes(stringArg);
            }
            catch (...) {
                LOG(LogError) << "StringUtil::wideStringToString(): Conversion failed, invalid "
                                 "characters in source string?";
                Utils::Platform::emergencyShutdown();
                return "";
            }
        }

        bool startsWith(const std::string& stringArg, const std::string& start)
        {
            return (stringArg.find(start) == 0);
        }

        bool endsWith(const std::string& stringArg, const std::string& end)
        {
            return (stringArg.find(end) == (stringArg.size() - end.size()));
        }

        std::string removeParenthesis(const std::string& stringArg)
        {
            static std::vector<char> remove {'(', ')', '[', ']'};
            std::string stringRemove {stringArg};
            size_t start;
            size_t end;
            bool done {false};

            while (!done) {
                done = true;

                for (size_t i {0}; i < remove.size(); i += 2) {
                    end = stringRemove.find_first_of(remove[i + 1]);
                    start = stringRemove.find_last_of(remove[i + 0], end);

                    if ((start != std::string::npos) && (end != std::string::npos)) {
                        stringRemove.erase(start, end - start + 1);
                        done = false;
                    }
                }
            }

            return trim(stringRemove);
        }

        std::vector<std::string> delimitedStringToVector(const std::string& stringArg,
                                                         const std::string& delimiter,
                                                         bool sort,
                                                         bool caseInsensitive)
        {
            std::vector<std::string> vectorResult;
            size_t start {0};
            size_t delimPos {stringArg.find(delimiter)};

            while (delimPos != std::string::npos) {
                vectorResult.push_back(stringArg.substr(start, delimPos - start));
                start = delimPos + 1;
                delimPos = stringArg.find(delimiter, start);
            }

            vectorResult.push_back(stringArg.substr(start));
            if (sort) {
                if (caseInsensitive)
                    std::sort(std::begin(vectorResult), std::end(vectorResult),
                              [](std::string a, std::string b) {
                                  return std::toupper(a.front()) < std::toupper(b.front());
                              });
                else
                    std::sort(vectorResult.begin(), vectorResult.end());
            }

            // Remove any empty elements.
            vectorResult.erase(remove(vectorResult.begin(), vectorResult.end(), ""),
                               vectorResult.end());

            return vectorResult;
        }

        std::string vectorToDelimitedString(std::vector<std::string> vectorArg,
                                            const std::string& delimiter,
                                            bool caseInsensitive)
        {
            std::string resultString;

            if (caseInsensitive) {
                std::sort(std::begin(vectorArg), std::end(vectorArg),
                          [](std::string a, std::string b) {
                              return std::toupper(a.front()) < std::toupper(b.front());
                          });
            }
            else {
                std::sort(vectorArg.begin(), vectorArg.end());
            }

            for (std::vector<std::string>::const_iterator it = vectorArg.cbegin();
                 it != vectorArg.cend(); ++it)
                resultString += (resultString.length() ? delimiter : "") + (*it);

            return resultString;
        }

        std::string scramble(const std::string& input, const std::string& key)
        {
            std::string buffer {input};

            for (size_t i {0}; i < input.size(); ++i)
                buffer[i] = input[i] ^ key[i];

            return buffer;
        }

    } // namespace String

} // namespace Utils
