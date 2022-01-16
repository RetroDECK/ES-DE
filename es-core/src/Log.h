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

    static LogLevel getReportingLevel() { return sReportingLevel; }
    static void setReportingLevel(LogLevel level) { sReportingLevel = level; }
    static std::string getLogPath()
    {
        return Utils::FileSystem::getHomePath() + "/.emulationstation/es_log.txt";
    }

    static void flush();
    static void init();
    static void open();
    static void close();

protected:
    std::ostringstream mOutStringStream;

private:
    std::map<LogLevel, std::string> mLogLevelMap {// Log level indicators.
                                                  {LogError, "Error"},
                                                  {LogWarning, "Warn"},
                                                  {LogInfo, "Info"},
                                                  {LogDebug, "Debug"}};
    inline static std::ofstream sFile;
    inline static LogLevel sReportingLevel = LogInfo;
    LogLevel mMessageLevel;
};

#endif // ES_CORE_LOG_H
