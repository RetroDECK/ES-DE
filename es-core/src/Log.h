//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  Log.h
//
//  Log output.
//  This class is thread safe.
//

#ifndef ES_CORE_LOG_H
#define ES_CORE_LOG_H

#include "utils/FileSystemUtil.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>

#if defined(__ANDROID__)
#include <android/log.h>
#endif

#define LOG(level)                                                                                 \
    if (level > Log::getReportingLevel())                                                          \
        ;                                                                                          \
    else                                                                                           \
        Log().get(level)

enum LogLevel {
    LogError,
    LogWarning,
    LogInfo,
    LogDebug
};

class Log
{
public:
    ~Log();

    std::ostringstream& get(LogLevel level = LogInfo);

    static LogLevel getReportingLevel();
    static void setReportingLevel(LogLevel level);

    // These functions are not thread safe.
    static void init();
    static void open();

    static void flush();
    static void close();

protected:
    std::ostringstream mOutStringStream;

private:
    std::map<LogLevel, std::string> mLogLevelMap {// Log level indicators.
                                                  {LogError, "Error"},
                                                  {LogWarning, "Warn"},
                                                  {LogInfo, "Info"},
                                                  {LogDebug, "Debug"}};
    static inline std::ofstream sFile;
    static inline LogLevel sReportingLevel = LogInfo;
    static inline std::mutex sLogMutex;
    static inline std::string sLogPath;
    LogLevel mMessageLevel;
};

#endif // ES_CORE_LOG_H
