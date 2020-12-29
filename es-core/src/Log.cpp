//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Log.cpp
//
//  Log handling.
//

#include "Log.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "Platform.h"

#include <fstream>
#include <iostream>
#include <iomanip>

LogLevel Log::reportingLevel = LogInfo;
std::ofstream file;

LogLevel Log::getReportingLevel()
{
    return reportingLevel;
}

std::string Log::getLogPath()
{
    return Utils::FileSystem::getHomePath() + "/.emulationstation/es_log.txt";
}

void Log::setReportingLevel(LogLevel level)
{
    reportingLevel = level;
}

void Log::init()
{
    Utils::FileSystem::removeFile(getLogPath() + ".bak");
    // Rename the previous log file.
    Utils::FileSystem::renameFile(getLogPath(), getLogPath() + ".bak", true);
    return;
}

void Log::open()
{
    #if defined(_WIN64)
    file.open(Utils::String::stringToWideString(getLogPath()).c_str());
    #else
    file.open(getLogPath().c_str());
    #endif
}

std::ostringstream& Log::get(LogLevel level)
{
    time_t t = time(nullptr);
    struct tm tm;
    #if defined(_WIN64)
    // Of course Windows does not follow standards and puts the parameters the other way
    // around compared to POSIX.
    localtime_s(&tm, &t);
    #else
    localtime_r(&t, &tm);
    #endif
    os << std::put_time(&tm, "%b %d %T ") << logLevelMap[level] << ":\t";
    messageLevel = level;

    return os;
}

void Log::flush()
{
    file.flush();
}

void Log::close()
{
    file.close();
}

Log::~Log()
{
    os << std::endl;

    if (!file.is_open()) {
        // Not open yet, print to stdout.
        std::cerr << "ERROR - tried to write to log file before it was open! "
                "The following won't be logged:\n";
        std::cerr << os.str();
        return;
    }

    file << os.str();

    // If it's an error, also print to console.
    // Print all messages if using --debug.
    if (messageLevel == LogError || reportingLevel >= LogDebug)
        std::cerr << os.str();
}
