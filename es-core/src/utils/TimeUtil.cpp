//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  TimeUtil.cpp
//
//  Low-level date and time functions.
//  Set and get time, format to string formats, count days and months etc.
//

#include "utils/TimeUtil.h"

namespace Utils
{
    namespace Time
    {
        DateTime::DateTime()
        {
            mTime = 0;
#if defined(_WIN64)
            mTimeStruct = {0, 0, 0, 1, 0, 0, 0, 0, -1};
#else
            mTimeStruct = {0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0};
#endif
            mIsoString = "19000101T000000";
        }

        DateTime::DateTime(const time_t& time)
        {
            // Set time.
            setTime(time);
        }

        DateTime::DateTime(const tm& timeStruct)
        {
            // Set time struct.
            setTimeStruct(timeStruct);
        }

        DateTime::DateTime(const std::string& isoString)
        {
            // Set ISO string.
            setIsoString(isoString);
        }

        void DateTime::setTime(const time_t& time)
        {
            // Workaround to handle Unix epoch for different time zones.
            if (time < 82800)
                mTime = 0;
            else
                mTime = time;

#if defined(_WIN64)
            localtime_s(&mTimeStruct, &mTime);
#else
            localtime_r(&mTime, &mTimeStruct);
#endif
            mIsoString = timeToString(mTime);
        }

        void DateTime::setTimeStruct(const tm& timeStruct)
        {
            // Set time based on struct.
            setTime(mktime(const_cast<tm*>(&timeStruct)));
        }

        void DateTime::setIsoString(const std::string& isoString)
        {
            // Set time based on ISO string.
            setTime(stringToTime(isoString));
        }

        Duration::Duration(const time_t& time)
        {
            mTotalSeconds = static_cast<unsigned int>(time);
            mDays = (mTotalSeconds - (mTotalSeconds % (60 * 60 * 24))) / (60 * 60 * 24);
            mHours = ((mTotalSeconds % (60 * 60 * 24)) - (mTotalSeconds % (60 * 60))) / (60 * 60);
            mMinutes = ((mTotalSeconds % (60 * 60)) - (mTotalSeconds % (60))) / 60;
            mSeconds = mTotalSeconds % 60;
        }

        time_t now()
        {
            time_t time;
            ::time(&time);
            return time;
        }

        time_t stringToTime(const std::string& string, const std::string& format)
        {
            const char* s = string.c_str();
            const char* f = format.c_str();
#if defined(_WIN64)
            tm timeStruct = {0, 0, 0, 1, 0, 0, 0, 0, -1};
#else
            tm timeStruct = {0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0};
#endif
            size_t parsedChars = 0;

            if (string == "19700101T000000")
                return 0;

            while (*f && (parsedChars < string.length())) {
                if (*f == '%') {
                    f++;

                    switch (*f++) {
                        // Year, including century [1970,xxxx]
                        case 'Y': {
                            if ((parsedChars + 4) <= string.length()) {
                                timeStruct.tm_year = (*s++ - '0') * 1000;
                                timeStruct.tm_year += (*s++ - '0') * 100;
                                timeStruct.tm_year += (*s++ - '0') * 10;
                                timeStruct.tm_year += (*s++ - '0');
                                if (timeStruct.tm_year >= 1900)
                                    timeStruct.tm_year -= 1900;
                            }
                            parsedChars += 4;
                        } break;

                        // Month number [01,12]
                        case 'm': {
                            if ((parsedChars + 2) <= string.length()) {
                                timeStruct.tm_mon = (*s++ - '0') * 10;
                                timeStruct.tm_mon += (*s++ - '0');
                                if (timeStruct.tm_mon >= 1)
                                    timeStruct.tm_mon -= 1;
                            }
                            parsedChars += 2;
                        } break;

                        // Day of the month [01,31]
                        case 'd': {
                            if ((parsedChars + 2) <= string.length()) {
                                timeStruct.tm_mday = (*s++ - '0') * 10;
                                timeStruct.tm_mday += (*s++ - '0');
                            }
                            parsedChars += 2;
                        } break;

                        // Hour (24-hour clock) [00,23]
                        case 'H': {
                            if ((parsedChars + 2) <= string.length()) {
                                timeStruct.tm_hour = (*s++ - '0') * 10;
                                timeStruct.tm_hour += (*s++ - '0');
                            }
                            parsedChars += 2;
                        } break;

                        // Minute [00,59]
                        case 'M': {
                            if ((parsedChars + 2) <= string.length()) {
                                timeStruct.tm_min = (*s++ - '0') * 10;
                                timeStruct.tm_min += (*s++ - '0');
                            }
                            parsedChars += 2;
                        } break;

                        // Second [00,59]
                        case 'S': {
                            if ((parsedChars + 2) <= string.length()) {
                                timeStruct.tm_sec = (*s++ - '0') * 10;
                                timeStruct.tm_sec += (*s++ - '0');
                            }
                            parsedChars += 2;
                        } break;
                    }
                }
                else {
                    s++;
                    f++;
                }
            }
            return mktime(&timeStruct);
        }

        std::string timeToString(const time_t& time, const std::string& format)
        {
            // Workaround to handle Unix epoch for different time zones.
            if (time < 82800)
                return "19700101T000000";

            const char* f = format.c_str();
            tm timeStruct;
#if defined(_WIN64)
            localtime_s(&timeStruct, &time);
#else
            localtime_r(&time, &timeStruct);
#endif
            char buf[256] = {'\0'};
            char* s = buf;

            while (*f) {
                if (*f == '%') {
                    f++;

                    switch (*f++) {
                        // Year, including century [1970,xxxx]
                        case 'Y': {
                            const int year = timeStruct.tm_year + 1900;
                            *s++ = static_cast<char>((year - (year % 1000)) / 1000) + '0';
                            *s++ = static_cast<char>(((year % 1000) - (year % 100)) / 100) + '0';
                            *s++ = static_cast<char>(((year % 100) - (year % 10)) / 10) + '0';
                            *s++ = static_cast<char>(year % 10) + '0';
                        } break;

                        // Month number [00,11]
                        case 'm': {
                            const int mon = timeStruct.tm_mon + 1;
                            *s++ = static_cast<char>(mon / 10) + '0';
                            *s++ = static_cast<char>(mon % 10) + '0';
                        } break;

                        // Day of the month [01,31]
                        case 'd': {
                            *s++ = static_cast<char>(timeStruct.tm_mday / 10) + '0';
                            *s++ = static_cast<char>(timeStruct.tm_mday % 10) + '0';
                        } break;

                        // Hour (24-hour clock) [00,23]
                        case 'H': {
                            *s++ = static_cast<char>(timeStruct.tm_hour / 10) + '0';
                            *s++ = static_cast<char>(timeStruct.tm_hour % 10) + '0';
                        } break;

                        // Minute [00,59]
                        case 'M': {
                            *s++ = static_cast<char>(timeStruct.tm_min / 10) + '0';
                            *s++ = static_cast<char>(timeStruct.tm_min % 10) + '0';
                        } break;

                        // Second [00,59]
                        case 'S': {
                            *s++ = static_cast<char>(timeStruct.tm_sec / 10) + '0';
                            *s++ = static_cast<char>(timeStruct.tm_sec % 10) + '0';
                        } break;
                    }
                }
                else {
                    *s++ = *f++;
                }
                *s = '\0';
            }
            return std::string(buf);
        }

        int daysInMonth(const int year, const int month)
        {
#if defined(_WIN64)
            tm timeStruct = {0, 0, 0, 0, month, year - 1900, 0, 0, -1};
#else
            tm timeStruct = {0, 0, 0, 0, month, year - 1900, 0, 0, -1, 0, 0};
#endif
            mktime(&timeStruct);

            return timeStruct.tm_mday;
        }

        int daysInYear(const int year)
        {
#if defined(_WIN64)
            tm timeStruct = {0, 0, 0, 0, 0, year - 1900 + 1, 0, 0, -1};
#else
            tm timeStruct = {0, 0, 0, 0, 0, year - 1900 + 1, 0, 0, -1, 0, 0};
#endif
            mktime(&timeStruct);

            return timeStruct.tm_yday + 1;
        }

    } // namespace Time

} // namespace Utils
