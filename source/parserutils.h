#ifndef PARSERUTILS_H
#define PARSERUTILS_H

#include <cstring>
#include <cmath>
#include <limits>
#include <string>
#include <algorithm>
#include <string_view>
#include <cassert>

namespace lunasvg {

#define IS_ALPHA(c) ((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z')
#define IS_NUM(c) ((c) >= '0' && (c) <= '9')
#define IS_WS(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')

namespace Utils {

inline const char* rtrim(const char* start, const char* end)
{
    while(end > start && IS_WS(end[-1]))
        --end;

    return end;
}

inline const char* ltrim(const char* start, const char* end)
{
    while(start < end && IS_WS(*start))
        ++start;

    return start;
}

inline bool skipDesc(const char*& ptr, const char* end, const char ch)
{
    if(ptr >= end || *ptr != ch)
        return false;

    ++ptr;
    return true;
}

inline bool skipDesc(const char*& ptr, const char* end, const char* data)
{
    int read = 0;
    while(data[read])
    {
        if(ptr >= end || *ptr != data[read])
        {
            ptr -= read;
            return false;
        }

        ++read;
        ++ptr;
    }

    return true;
}

inline bool skipUntil(const char*& ptr, const char* end, const char ch)
{
    while(ptr < end && *ptr != ch)
        ++ptr;

    return ptr < end;
}

inline bool skipUntil(const char*& ptr, const char* end, const char* data)
{
    while(ptr < end)
    {
        auto start = ptr;
        if(skipDesc(start, end, data))
            break;
        ++ptr;
    }

    return ptr < end;
}

inline bool readUntil(const char*& ptr, const char* end, const char ch, std::string& value)
{
    auto start = ptr;
    if(!skipUntil(ptr, end, ch))
        return false;

    value.assign(start, ptr);
    return true;
}

inline bool readUntil(const char*& ptr, const char* end, const char* data, std::string& value)
{
    auto start = ptr;
    if(!skipUntil(ptr, end, data))
        return false;

    value.assign(start, ptr);
    return true;
}

inline bool skipWs(const char*& ptr, const char* end)
{
    while(ptr < end && IS_WS(*ptr))
       ++ptr;

    return ptr < end;
}

inline bool skipWsDelimiter(const char*& ptr, const char* end, const char delimiter)
{
    if(ptr < end && !IS_WS(*ptr) && *ptr != delimiter)
        return false;

    if(skipWs(ptr, end))
    {
        if(ptr < end && *ptr == delimiter)
        {
            ++ptr;
            skipWs(ptr, end);
        }
    }

    return ptr < end;
}

inline bool skipWsComma(const char*& ptr, const char* end)
{
    return skipWsDelimiter(ptr, end, ',');
}

inline bool isIntegralDigit(char ch, int base)
{
    if(IS_NUM(ch))
        return ch - '0' < base;

    if(IS_ALPHA(ch))
        return (ch >= 'a' && ch < 'a' + std::min(base, 36) - 10) || (ch >= 'A' && ch < 'A' + std::min(base, 36) - 10);

    return false;
}

template<typename T>
inline bool parseInteger(const char*& ptr, const char* end, T& integer, int base = 10)
{
    bool isNegative = 0;
    T value = 0;

    static const T intMax = std::numeric_limits<T>::max();
    static const bool isSigned = std::numeric_limits<T>::is_signed;
    using signed_t = typename std::make_signed<T>::type;
    const T maxMultiplier = intMax / static_cast<T>(base);

    if(ptr < end && *ptr == '+')
        ++ptr;
    else if(ptr < end && isSigned && *ptr == '-')
    {
        ++ptr;
        isNegative = true;
    }

    if(ptr >= end || !isIntegralDigit(*ptr, base))
        return false;

    do {
        const char ch = *ptr++;
        int digitValue;
        if(IS_NUM(ch))
            digitValue = ch - '0';
        else if(ch >= 'a')
            digitValue = ch - 'a' + 10;
        else
            digitValue = ch - 'A' + 10;

        if(value > maxMultiplier || (value == maxMultiplier && static_cast<T>(digitValue) > (intMax % static_cast<T>(base)) + isNegative))
            return false;
        value = static_cast<T>(base) * value + static_cast<T>(digitValue);
    } while(ptr < end && isIntegralDigit(*ptr, base));

    if(isNegative)
        integer = -static_cast<signed_t>(value);
    else
        integer = value;

    return true;
}

template<typename T>
inline bool parseNumber(const char*& ptr, const char* end, T& number)
{
    T integer, fraction;
    int sign, expsign, exponent;

    static const T numberMax = std::numeric_limits<T>::max();
    fraction = 0;
    integer = 0;
    exponent = 0;
    sign = 1;
    expsign = 1;

    if(ptr < end && *ptr == '+')
        ++ptr;
    else if(ptr < end && *ptr == '-')
    {
        ++ptr;
        sign = -1;
    }

    if(ptr >= end || !(IS_NUM(*ptr) || *ptr == '.'))
        return false;

    if(*ptr != '.')
    {
        do {
            integer = static_cast<T>(10) * integer + (*ptr - '0');
            ++ptr;
        } while(ptr < end && IS_NUM(*ptr));
    }

    if(ptr < end && *ptr == '.')
    {
        ++ptr;
        if(ptr >= end || !IS_NUM(*ptr))
            return false;

        T divisor = 1;
        do {
            fraction = static_cast<T>(10) * fraction + (*ptr - '0');
            divisor *= static_cast<T>(10);
            ++ptr;
        } while(ptr < end && IS_NUM(*ptr));
        fraction /= divisor;
    }

    if(ptr < end && (*ptr == 'e' || *ptr == 'E')
       && (ptr[1] != 'x' && ptr[1] != 'm'))
    {
        ++ptr;
        if(ptr < end && *ptr == '+')
            ++ptr;
        else if(ptr < end && *ptr == '-')
        {
            ++ptr;
            expsign = -1;
        }

        if(ptr >= end || !IS_NUM(*ptr))
            return false;

        do {
            exponent = 10 * exponent + (*ptr - '0');
            ++ptr;
        } while(ptr < end && IS_NUM(*ptr));
    }

    number = sign * (integer + fraction);
    if(exponent)
        number *= static_cast<T>(pow(10.0, expsign*exponent));

    return number >= -numberMax && number <= numberMax;
}

} // namespace Utils

class ParserString {
public:
    explicit ParserString(const std::string_view& value)
        : ParserString(value.data(), value.length())
    {}

    ParserString(const char* begin, size_t length)
        : ParserString(begin, begin + length)
    {}

    ParserString(const char* begin, const char* end)
        : ParserString(begin, begin, end)
    {}

    ParserString(const char* current, const char* begin, const char* end)
        : m_current(current), m_begin(begin), m_end(end)
    {}

    ParserString operator+(size_t count) const {
        auto current = m_current + count;
        assert(m_end >= current);
        return ParserString(current, m_begin, m_end);
    }

    ParserString operator-(size_t count) const {
        auto current = m_current - count;
        assert(current >= m_begin);
        return ParserString(current, m_begin, m_end);
    }

    ParserString& operator+=(size_t count) {
        *this = *this + count;
        return *this;
    }

    ParserString& operator-=(size_t count) {
        *this = *this - count;
        return *this;
    }

    const char& operator*() const {
        assert(m_current < m_end);
        return *m_current;
    }

    char peek(size_t count = 0) const {
        auto current = m_current + count;
        assert(m_end >= current);
        if(current == m_end)
            return 0;
        return *current;
    }

    char advance(size_t count = 1) {
        m_current += count;
        assert(m_end >= m_current);
        if(m_current == m_end)
            return 0;
        return *m_current;
    }

    char get() const {
        assert(m_end >= m_current);
        if(m_current == m_end)
            return 0;
        return *m_current;
    }

    std::string_view string(size_t offset, size_t count) const { return string().substr(offset, count); }
    std::string_view substring(size_t offset, size_t count) const { return substring().substr(offset, count); }

    std::string_view string() const { return std::string_view(m_begin, length()); }
    std::string_view substring() const { return std::string_view(m_current, sublength()); }

    size_t offset() const { return m_current - m_begin; }
    size_t length() const { return m_end - m_begin; }
    size_t sublength() const { return m_end - m_current; }

    const char* current() const { return m_current; }
    const char* begin() const { return m_begin; }
    const char* end() const { return m_end; }

    bool empty() const { return m_current == m_end; }

private:
    const char* m_current;
    const char* m_begin;
    const char* m_end;
};

constexpr bool isspace(int cc) { return (cc == ' ' || cc == '\n' || cc == '\t' || cc == '\r' || cc == '\f'); }
constexpr bool isdigit(int cc) { return (cc >= '0' && cc <= '9'); }
constexpr bool isupper(int cc) { return (cc >= 'A' && cc <= 'Z'); }
constexpr bool islower(int cc) { return (cc >= 'a' && cc <= 'z'); }
constexpr bool isalpha(int cc) { return isupper(cc) || islower(cc); }

constexpr bool isxupper(int cc) { return (cc >= 'A' && cc <= 'F'); }
constexpr bool isxlower(int cc) { return (cc >= 'a' && cc <= 'f'); }
constexpr bool isxdigit(int cc) { return isdigit(cc) || isxupper(cc) || isxlower(cc); }

constexpr int xdigit(int cc) {
    if(isdigit(cc))
        return cc - '0';
    if(isxupper(cc))
        return 10 + cc - 'A';
    if(isxlower(cc))
        return 10 + cc - 'a';
    return 0;
}

constexpr char tolower(int cc) {
    if(isupper(cc))
        return cc + 0x20;
    return cc;
}

constexpr bool equals(int a, int b, bool caseSensitive) {
    if(caseSensitive)
        return a == b;
    return tolower(a) == tolower(b);
}

constexpr bool equals(const char* aData, size_t aLength, const char* bData, size_t bLength, bool caseSensitive) {
    if(aLength != bLength)
        return false;

    auto aEnd = aData + aLength;
    while(aData != aEnd) {
        if(!equals(*aData, *bData, caseSensitive))
            return false;
        ++aData;
        ++bData;
    }

    return true;
}

constexpr bool equals(const std::string_view& a, const std::string_view& b, bool caseSensitive) {
    return equals(a.data(), a.length(), b.data(), b.length(), caseSensitive);
}

constexpr bool contains(const std::string_view& value, const std::string_view& subvalue, bool caseSensitive) {
    if(subvalue.empty() || subvalue.length() > value.length())
        return false;
    auto it = value.data();
    auto end = it + value.length();
    while(it < end) {
        size_t count = 0;
        do {
            if(!equals(*it, subvalue[count], caseSensitive))
                break;
            ++count;
            ++it;
        } while(it < end && count < subvalue.length());
        if(count == subvalue.length())
            return true;
        ++it;
    }

    return false;
}

constexpr bool includes(const std::string_view& value, const std::string_view& subvalue, bool caseSensitive) {
    if(subvalue.empty() || subvalue.length() > value.length())
        return false;
    auto it = value.data();
    auto end = it + value.length();
    while(true) {
        while(it < end && isspace(*it))
            ++it;
        if(it >= end)
            return false;
        size_t count = 0;
        auto begin = it;
        do {
            ++count;
            ++it;
        } while(it < end && !isspace(*it));
        if(equals(begin, count, subvalue.data(), subvalue.length(), caseSensitive))
            return true;
        ++it;
    }

    return false;
}

constexpr bool startswith(const std::string_view& value, const std::string_view& subvalue, bool caseSensitive) {
    if(subvalue.empty() || subvalue.length() > value.length())
        return false;
    return equals(value.substr(0, subvalue.size()), subvalue, caseSensitive);
}

constexpr bool endswith(const std::string_view& value, const std::string_view& subvalue, bool caseSensitive) {
    if(subvalue.empty() || subvalue.length() > value.length())
        return false;
    return equals(value.substr(value.size() - subvalue.size(), subvalue.size()), subvalue, caseSensitive);
}

constexpr bool dashequals(const std::string_view& value, const std::string_view& subvalue, bool caseSensitive) {
    if(!startswith(value, subvalue, caseSensitive))
        return false;
    return (value.length() == subvalue.length() || value.at(subvalue.length()) == '-');
}

inline void appendCodepoint(std::string& output, uint32_t cp) {
    char c[5] = {0, 0, 0, 0, 0};
    if(cp < 0x80) {
        c[1] = 0;
        c[0] = cp;
    } else if(cp < 0x800) {
        c[2] = 0;
        c[1] = (cp & 0x3F) | 0x80;
        cp >>= 6;
        c[0] = cp | 0xC0;
    } else if(cp < 0x10000) {
        c[3] = 0;
        c[2] = (cp & 0x3F) | 0x80;
        cp >>= 6;
        c[1] = (cp & 0x3F) | 0x80;
        cp >>= 6;
        c[0] = cp | 0xE0;
    } else if(cp < 0x110000) {
        c[4] = 0;
        c[3] = (cp & 0x3F) | 0x80;
        cp >>= 6;
        c[2] = (cp & 0x3F) | 0x80;
        cp >>= 6;
        c[1] = (cp & 0x3F) | 0x80;
        cp >>= 6;
        c[0] = cp | 0xF0;
    }

    output.append(c);
}

} // namespace lunasvg

#endif // PARSERUTILS_H
