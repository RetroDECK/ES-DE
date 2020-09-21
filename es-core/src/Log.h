//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Log.h
//
//  Log handling.
//

#ifndef ES_CORE_LOG_H
#define ES_CORE_LOG_H

#include <map>
#include <sstream>

#define LOG(level) \
if (level > Log::getReportingLevel()); \
else Log().get(level)

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

    static std::string getLogPath();

    static void flush();
    static void init();
    static void open();
    static void close();

protected:
    std::ostringstream os;

private:
    std::map<LogLevel, std::string> logLevelMap {
        { LogError, "Error" },
        { LogWarning, "Warn" },
        { LogInfo, "Info" },
        { LogDebug, "Debug" }
    };

    static LogLevel reportingLevel;
    LogLevel messageLevel;
};

#endif // ES_CORE_LOG_H
