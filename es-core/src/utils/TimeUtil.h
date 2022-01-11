//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  TimeUtil.h
//
//  Low-level date and time functions.
//  Set and get time, format to string formats, count days and months etc.
//

#ifndef ES_CORE_UTILS_TIME_UTIL_H
#define ES_CORE_UTILS_TIME_UTIL_H

#include <ctime>
#include <string>

namespace Utils
{
    namespace Time
    {
        class DateTime
        {
        public:
            DateTime();
            DateTime(const time_t& time);
            DateTime(const tm& timeStruct);
            DateTime(const std::string& isoString);
            ~DateTime() {}

            const bool operator<(const DateTime& other) const { return (mTime < other.mTime); }
            const bool operator<=(const DateTime& other) const { return (mTime <= other.mTime); }
            const bool operator>(const DateTime& other) const { return (mTime > other.mTime); }
            const bool operator>=(const DateTime& other) const { return (mTime >= other.mTime); }
            operator time_t() const { return mTime; }
            operator tm() const { return mTimeStruct; }
            operator std::string() const { return mIsoString; }

            void setTime(const time_t& time);
            const time_t& getTime() const { return mTime; }
            void setTimeStruct(const tm& timeStruct);
            const tm& getTimeStruct() const { return mTimeStruct; }
            void setIsoString(const std::string& isoString);
            const std::string& getIsoString() const { return mIsoString; }

        private:
            time_t mTime;
            tm mTimeStruct;
            std::string mIsoString;
        };

        class Duration
        {
        public:
            Duration(const time_t& time);
            ~Duration() {}

            unsigned int getDays() const { return mDays; }
            unsigned int getHours() const { return mHours; }
            unsigned int getMinutes() const { return mMinutes; }
            unsigned int getSeconds() const { return mSeconds; }

        private:
            unsigned int mTotalSeconds;
            unsigned int mDays;
            unsigned int mHours;
            unsigned int mMinutes;
            unsigned int mSeconds;
        };

        time_t now();
        time_t stringToTime(const std::string& string, const std::string& format = "%Y%m%dT%H%M%S");
        std::string timeToString(const time_t& time, const std::string& format = "%Y%m%dT%H%M%S");
        int daysInMonth(const int year, const int month);
        int daysInYear(const int year);

    } // namespace Time

} // namespace Utils

#endif // ES_CORE_UTILS_TIME_UTIL_H
