//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Log.h
//
//  Log output.
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
    // Deconstructor grabs the lock.
    ~Log();

    // No lock needed for get() as it operates on and returns non-static members.
    std::ostringstream& get(LogLevel level = LogInfo);

    // Lock is grabbed for sReportingLevel.
    // This means level > Log::getReportingLevel() still requires the lock.
    static LogLevel getReportingLevel();
    static void setReportingLevel(LogLevel level);

    // getLogPath() is not thread-safe.
    static std::string getLogPath();
    // init() is not thread-safe.
    static void init();
    // open() is not fully thread-safe, as it uses getLogPath().
    static void open();

    // The following static functions are thread-safe.
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
    LogLevel mMessageLevel;
};

#endif // ES_CORE_LOG_H
