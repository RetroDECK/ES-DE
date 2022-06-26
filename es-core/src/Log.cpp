//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Log.cpp
//
//  Log output.
//

#include "Log.h"
#include "utils/StringUtil.h"

LogLevel Log::getReportingLevel()
{
    // Static Log functions need to grab the lock.
    std::unique_lock<std::mutex> lock {sLogMutex};
    return sReportingLevel;
}

void Log::setReportingLevel(LogLevel level)
{
    // Static Log functions need to grab the lock.
    std::unique_lock<std::mutex> lock {sLogMutex};
    sReportingLevel = level;
}

std::string Log::getLogPath()
{
    // No attempt is made to make this thread-safe.
    // Currently getLogPath is public, and called in contexts with
    // and without sLogMutex locked.

    // getHomePath() currently does not generate any Log messages.
    return Utils::FileSystem::getHomePath() + "/.emulationstation/es_log.txt";
}

void Log::init()
{
    // No attempt is made to make this thread-safe.
    // It is unlikely to be called across multiple threads.
    // Both removeFile and renameFile might generate log messages,
    // so they might try to grab the lock.

    Utils::FileSystem::removeFile(getLogPath() + ".bak");
    // Rename the previous log file.
    Utils::FileSystem::renameFile(getLogPath(), getLogPath() + ".bak", true);
    return;
}

void Log::open()
{
    // Static Log functions need to grab the lock.
    std::unique_lock<std::mutex> lock {sLogMutex};
#if defined(_WIN64)
    sFile.open(Utils::String::stringToWideString(getLogPath()).c_str());
#else
    sFile.open(getLogPath().c_str());
#endif
}

void Log::flush()
{
    // Flush file. Static Log functions need to grab the lock.
    std::unique_lock<std::mutex> lock {sLogMutex};
    sFile.flush();
}

void Log::close()
{
    // Static Log functions need to grab the lock.
    std::unique_lock<std::mutex> lock {sLogMutex};
    if (sFile.is_open())
        sFile.close();
}

std::ostringstream& Log::get(LogLevel level)
{
    // This function is not-static, but only touches instance member variables.
    // The lock is not needed.
    time_t t {time(nullptr)};
    struct tm tm;
#if defined(_WIN64)
    // Of course Windows does not follow standards and puts the parameters the other way
    // around compared to POSIX.
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    mOutStringStream << std::put_time(&tm, "%b %d %T ") << mLogLevelMap[level] << ":\t";
    mMessageLevel = level;

    return mOutStringStream;
}

Log::~Log()
{
    mOutStringStream << std::endl;

    // Log() constructor does not need the lock.
    // Log::get() does not need the lock.
    // get(..) << msg << msg also does not need the lock,
    // since get() returns the instance member mOutStringStream.

    // It is only now that we need the lock, to guard access to
    // both std::cerr and sFile.
    std::unique_lock<std::mutex> lock {sLogMutex};

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
