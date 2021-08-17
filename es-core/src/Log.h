//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Log.h
//
//  Log handling.
//

#ifndef ES_CORE_LOG_H
#define ES_CORE_LOG_H

#include "utils/FileSystemUtil.h"

#include <map>
#include <sstream>

#define LOG(level)                                                                                 \
    if (level > Log::getReportingLevel())                                                          \
        ;                                                                                          \
    else                                                                                           \
        Log().get(level)

enum LogLevel {
    LogError, // Replace with AllowShortEnumsOnASingleLine: false (clang-format >=11.0).
    LogWarning,
    LogInfo,
    LogDebug
};

class Log
{
public:
    ~Log();
    std::ostringstream& get(LogLevel level = LogInfo);

    static LogLevel getReportingLevel() { return reportingLevel; }
    static void setReportingLevel(LogLevel level) { reportingLevel = level; }
    static std::string getLogPath()
    {
        return Utils::FileSystem::getHomePath() + "/.emulationstation/es_log.txt";
    }

    static void flush();
    static void init();
    static void open();
    static void close();

protected:
    std::ostringstream os;

private:
    std::map<LogLevel, std::string> logLevelMap{// Log level indicators.
                                                {LogError, "Error"},
                                                {LogWarning, "Warn"},
                                                {LogInfo, "Info"},
                                                {LogDebug, "Debug"}};

    static LogLevel reportingLevel;
    LogLevel messageLevel;
};

#endif // ES_CORE_LOG_H
