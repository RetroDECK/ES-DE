//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Log.cpp
//
//  Log handling.
//

#include "Log.h"

#include "Platform.h"
#include "utils/StringUtil.h"

#include <fstream>
#include <iomanip>
#include <iostream>

LogLevel Log::reportingLevel = LogInfo;
std::ofstream file;

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
    // This runs on application exit.
    file.flush();
}

void Log::close()
{
    if (file.is_open())
        file.close();
}

Log::~Log()
{
    os << std::endl;

    if (!file.is_open()) {
        // Not open yet, print to stdout.
        std::cerr << "Error: Tried to write to log file before it was open, "
                     "the following won't be logged:\n";
        std::cerr << os.str();
        return;
    }

    file << os.str();

    // If it's an error, also print to console.
    // Print all messages if using --debug.
    if (messageLevel == LogError || reportingLevel >= LogDebug)
        std::cerr << os.str();
}
