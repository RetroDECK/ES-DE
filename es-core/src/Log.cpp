//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Log.cpp
//
//  Log output.
//  This class is thread safe.
//

#include "Log.h"
#include "utils/StringUtil.h"

LogLevel Log::getReportingLevel()
{
    std::unique_lock<std::mutex> lock {sLogMutex};
    return sReportingLevel;
}

void Log::setReportingLevel(LogLevel level)
{
    std::unique_lock<std::mutex> lock {sLogMutex};
    sReportingLevel = level;
}

std::string Log::getLogPath()
{
    return Utils::FileSystem::getHomePath() + "/.emulationstation/es_log.txt";
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
    std::unique_lock<std::mutex> lock {sLogMutex};
#if defined(_WIN64)
    sFile.open(Utils::String::stringToWideString(getLogPath()).c_str());
#else
    sFile.open(getLogPath().c_str());
#endif
}

void Log::flush()
{
    std::unique_lock<std::mutex> lock {sLogMutex};
    sFile.flush();
}

void Log::close()
{
    std::unique_lock<std::mutex> lock {sLogMutex};
    if (sFile.is_open())
        sFile.close();
}

std::ostringstream& Log::get(LogLevel level)
{
    time_t t {time(nullptr)};
    struct tm tm;
#if defined(_WIN64)
    // Of course Windows does not follow standards and puts the parameters the other way
    // around compared to POSIX.
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::unique_lock<std::mutex> lock {sLogMutex};
    mOutStringStream << std::put_time(&tm, "%b %d %T ") << mLogLevelMap[level] << ":\t";
    mMessageLevel = level;

    return mOutStringStream;
}

Log::~Log()
{
    std::unique_lock<std::mutex> lock {sLogMutex};
    mOutStringStream << std::endl;

    if (!sFile.is_open()) {
        // Not open yet, print to stdout.
        std::cerr << "Error: Tried to write to log file before it was open, "
                     "the following won't be logged:\n";
        std::cerr << mOutStringStream.str();
        return;
    }

    sFile << mOutStringStream.str();

    // If it's an error or the --debug flag has been set, then print to the console as well.
    if (mMessageLevel == LogError || sReportingLevel >= LogDebug)
        std::cerr << mOutStringStream.str();
}
