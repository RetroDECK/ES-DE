//
//  TimeUtil.cpp
//
//  Low-level date and time functions.
//  Set and get time, format to string formats, count days and months etc.
//

#include "utils/TimeUtil.h"

#include <time.h>

namespace Utils
{
    namespace Time
    {
        DateTime::DateTime()
        {
            mTime = 0;
            mTimeStruct = { 0, 0, 0, 1, 0, 0, 0, 0, -1 };
            mIsoString = "00000000T000000";
        }

        DateTime::DateTime(const time_t& _time)
        {
            setTime(_time);
        }

        DateTime::DateTime(const tm& _timeStruct)
        {
            setTimeStruct(_timeStruct);
        }

        DateTime::DateTime(const std::string& _isoString)
        {
            setIsoString(_isoString);
        }

        DateTime::~DateTime()
        {
        }

        void DateTime::setTime(const time_t& _time)
        {
            mTime = (_time < 0) ? 0 : _time;
            mTimeStruct = *localtime(&mTime);
            mIsoString = timeToString(mTime);
        }

        void DateTime::setTimeStruct(const tm& _timeStruct)
        {
            setTime(mktime((tm*)&_timeStruct));
        }

        void DateTime::setIsoString(const std::string& _isoString)
        {
            setTime(stringToTime(_isoString));
        }

        Duration::Duration(const time_t& _time)
        {
            mTotalSeconds = (unsigned int)_time;
            mDays = (mTotalSeconds - (mTotalSeconds % (60*60*24))) / (60*60*24);
            mHours = ((mTotalSeconds % (60*60*24)) - (mTotalSeconds % (60*60))) / (60*60);
            mMinutes = ((mTotalSeconds % (60*60)) - (mTotalSeconds % (60))) / 60;
            mSeconds = mTotalSeconds % 60;
        }

        Duration::~Duration()
        {
        }

        time_t now()
        {
            time_t time;
            ::time(&time);
            return time;
        }

        time_t stringToTime(const std::string& _string, const std::string& _format)
        {
            const char* s = _string.c_str();
            const char* f = _format.c_str();
            tm timeStruct  = { 0, 0, 0, 1, 0, 0, 0, 0, -1 };
            size_t parsedChars = 0;

            if (_string == "19700101T010000")
                return mktime(&timeStruct);

            while (*f && (parsedChars < _string.length())) {
                if (*f == '%') {
                    ++f;

                    switch (*f++) {
                    // The year [1970,xxxx]
                    case 'Y': {
                        if ((parsedChars + 4) <= _string.length()) {
                            timeStruct.tm_year  = (*s++ - '0') * 1000;
                            timeStruct.tm_year += (*s++ - '0') * 100;
                            timeStruct.tm_year += (*s++ - '0') * 10;
                            timeStruct.tm_year += (*s++ - '0');
                            if (timeStruct.tm_year >= 1900)
                                timeStruct.tm_year -= 1900;
                        }
                        parsedChars += 4;
                    }
                    break;

                    // The month number [01,12]
                    case 'm': {
                        if ((parsedChars + 2) <= _string.length()) {
                            timeStruct.tm_mon  = (*s++ - '0') * 10;
                            timeStruct.tm_mon += (*s++ - '0');
                            if (timeStruct.tm_mon >= 1)
                                timeStruct.tm_mon -= 1;
                        }
                        parsedChars += 2;
                    }
                    break;

                    // The day of the month [01,31]
                    case 'd': {
                        if ((parsedChars + 2) <= _string.length()) {
                            timeStruct.tm_mday  = (*s++ - '0') * 10;
                            timeStruct.tm_mday += (*s++ - '0');
                        }
                        parsedChars += 2;
                    }
                    break;

                    // The hour (24-hour clock) [00,23]
                    case 'H': {
                        if ((parsedChars + 2) <= _string.length()) {
                            timeStruct.tm_hour  = (*s++ - '0') * 10;
                            timeStruct.tm_hour += (*s++ - '0');
                        }
                        parsedChars += 2;
                    }
                    break;

                    // The minute [00,59]
                    case 'M': {
                        if ((parsedChars + 2) <= _string.length()) {
                            timeStruct.tm_min  = (*s++ - '0') * 10;
                            timeStruct.tm_min += (*s++ - '0');
                        }
                        parsedChars += 2;
                    }
                    break;

                    // The second [00,59]
                    case 'S': {
                        if ((parsedChars + 2) <= _string.length()) {
                            timeStruct.tm_sec  = (*s++ - '0') * 10;
                            timeStruct.tm_sec += (*s++ - '0');
                        }
                        parsedChars += 2;
                    }
                    break;
                    }
                }
                else {
                    ++s;
                    ++f;
                }
            }
            return mktime(&timeStruct);
        }

        std::string timeToString(const time_t& _time, const std::string& _format)
        {
            const char* f = _format.c_str();
            const tm timeStruct = *localtime(&_time);
            char buf[256] = { '\0' };
            char* s = buf;

            while (*f) {
                if (*f == '%') {
                    ++f;

                    switch (*f++) {
                    // The year, including the century (1900)
                    case 'Y': {
                        const int year = timeStruct.tm_year + 1900;
                        *s++ = (char)((year - (year % 1000)) / 1000) + '0';
                        *s++ = (char)(((year % 1000) - (year % 100)) / 100) + '0';
                        *s++ = (char)(((year % 100) - (year % 10)) / 10) + '0';
                        *s++ = (char)(year % 10) + '0';
                    }
                    break;

                    // The month number [00,11]
                    case 'm': {
                        const int mon = timeStruct.tm_mon + 1;
                        *s++ = (char)(mon / 10) + '0';
                        *s++ = (char)(mon % 10) + '0';
                    }
                    break;

                    // The day of the month [01,31]
                    case 'd': {
                        *s++ = (char)(timeStruct.tm_mday / 10) + '0';
                        *s++ = (char)(timeStruct.tm_mday % 10) + '0';
                    }
                    break;

                    // The hour (24-hour clock) [00,23]
                    case 'H': {
                        *s++ = (char)(timeStruct.tm_hour / 10) + '0';
                        *s++ = (char)(timeStruct.tm_hour % 10) + '0';
                    }
                    break;

                    // The minute [00,59]
                    case 'M': {
                        *s++ = (char)(timeStruct.tm_min / 10) + '0';
                        *s++ = (char)(timeStruct.tm_min % 10) + '0';
                    }
                    break;

                    // The second [00,59]
                    case 'S': {
                        *s++ = (char)(timeStruct.tm_sec / 10) + '0';
                        *s++ = (char)(timeStruct.tm_sec % 10) + '0';
                    }
                    break;
                    }
                }
                else {
                    *s++ = *f++;
                }
                *s = '\0';
            }
            return std::string(buf);
        }

        int daysInMonth(const int _year, const int _month)
        {
            tm timeStruct = { 0, 0, 0, 0, _month, _year - 1900, 0, 0, -1 };
            mktime(&timeStruct);

            return timeStruct.tm_mday;
        }

        int daysInYear(const int _year)
        {
            tm timeStruct = { 0, 0, 0, 0, 0, _year - 1900 + 1, 0, 0, -1 };
            mktime(&timeStruct);

            return timeStruct.tm_yday + 1;
        }

    } // Time::

} // Utils::
