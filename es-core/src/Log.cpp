//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  Log.cpp
//
//  Log output.
//  This class is thread safe.
//

#include "Log.h"
#include "Settings.h"
#include "utils/StringUtil.h"

#if defined(RETRODECK)
#include <algorithm>
#endif

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

void Log::init()
{
#if defined(RETRODECK)
    // Check for the rd_logs_folder environment variable
    //const char* logFolder = std::getenv("rd_logs_folder");
    const char* logFolder = "/var/config/retrodeck/logs";
    if (logFolder && std::strlen(logFolder) > 0)
    {
        sLogPath = std::string(logFolder) + "/retrodeck.log";
    }
    else
    {
        // Default to the existing location if rd_logs_folder is not defined
        sLogPath = Utils::FileSystem::getAppDataDirectory() + "/retrodeck.log";
    }
    // Skip renaming to .bak for RetroDECK
#else
    if (Settings::getInstance()->getBool("LegacyAppDataDirectory"))
        sLogPath = Utils::FileSystem::getAppDataDirectory() + "/es_log.txt";
    else
        sLogPath = Utils::FileSystem::getAppDataDirectory() + "/logs/es_log.txt";

    Utils::FileSystem::removeFile(sLogPath + ".bak");
    // Rename the previous log file.
    Utils::FileSystem::renameFile(sLogPath, sLogPath + ".bak", true);
    return;
#endif
}

void Log::open()
{
    std::unique_lock<std::mutex> lock {sLogMutex};

    std::ios_base::openmode mode = std::ios::out;
#if defined(RETRODECK)
    mode |= std::ios::app; // Append to the log file if RetroDECK is defined
#endif

#if defined(_WIN64)
    sFile.open(Utils::String::stringToWideString(sLogPath).c_str(), mode);
#else
    sFile.open(sLogPath.c_str(), mode);
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

#if defined(RETRODECK)
    // Convert log level to uppercase for RetroDECK
    std::string levelUpper = mLogLevelMap[level];
    std::transform(levelUpper.begin(), levelUpper.end(), levelUpper.begin(), ::toupper);

    // Get current time with milliseconds
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto time = std::chrono::system_clock::to_time_t(now);
    tm = *std::localtime(&time);

    mOutStringStream << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
                     << "." << std::setfill('0') << std::setw(3) << ms.count() // Add milliseconds
                     << "] [" << levelUpper << "] [ES-DE] ";
#else
    // Get current time with milliseconds
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto time = std::chrono::system_clock::to_time_t(now);
    tm = *std::localtime(&time);

    mOutStringStream << "[" << std::put_time(&tm, "%b %d %H:%M:%S")
                     << "." << std::setfill('0') << std::setw(3) << ms.count() // Add milliseconds
                     << "] " << mLogLevelMap[level]
                     << (level == LogLevel::LogInfo || level == LogLevel::LogWarning ? ":   " :
                                                                                       ":  ");
#endif

    mMessageLevel = level;

    return mOutStringStream;
}

Log::~Log()
{
    std::unique_lock<std::mutex> lock {sLogMutex};
    mOutStringStream << std::endl;

    if (!sFile.is_open()) {
        // Not open yet, print to stdout.
#if defined(__ANDROID__)
        __android_log_print(
            ANDROID_LOG_ERROR, ANDROID_APPLICATION_ID,
            "Error: Tried to write to log file before it was open, the following won't be logged:");
        __android_log_print(ANDROID_LOG_ERROR, ANDROID_APPLICATION_ID, "%s",
                            mOutStringStream.str().c_str());
#else
        std::cerr << "Error: Tried to write to log file before it was open, "
                     "the following won't be logged:\n";
        std::cerr << mOutStringStream.str();
#endif
        return;
    }

    sFile << mOutStringStream.str();

    #if defined(RETRODECK)
        sFile.flush();
    #endif

#if defined(__ANDROID__)
    if (mMessageLevel == LogError) {
        __android_log_print(ANDROID_LOG_ERROR, ANDROID_APPLICATION_ID, "%s",
                            mOutStringStream.str().c_str());
    }
    else if (sReportingLevel >= LogDebug) {
        if (mMessageLevel == LogInfo)
            __android_log_print(ANDROID_LOG_INFO, ANDROID_APPLICATION_ID, "%s",
                                mOutStringStream.str().c_str());
        else if (mMessageLevel == LogWarning)
            __android_log_print(ANDROID_LOG_WARN, ANDROID_APPLICATION_ID, "%s",
                                mOutStringStream.str().c_str());
        else
            __android_log_print(ANDROID_LOG_DEBUG, ANDROID_APPLICATION_ID, "%s",
                                mOutStringStream.str().c_str());
    }
#else
    // If it's an error or the --debug flag has been set, then print to the console as well.
    if (mMessageLevel == LogError || sReportingLevel >= LogDebug)
        std::cerr << mOutStringStream.str();
#endif

#if defined(RETRODECK)
    // Always write logs to the terminal as well when RetroDECK is defined
    std::cout << mOutStringStream.str();
#endif
}

// RetroDECK specific function
#if defined(RETRODECK)
void Log::setReportingLevelFromEnv()
{
    // Check for the logging_level environment variable
    const char* logLevelEnv = std::getenv("logging_level");
    std::string logLevel = logLevelEnv ? logLevelEnv : "info";

    // Map string to LogLevel
    if (logLevel == "debug")
        sReportingLevel = LogDebug;
    else if (logLevel == "warning")
        sReportingLevel = LogWarning;
    else if (logLevel == "error")
        sReportingLevel = LogError;
    else
        sReportingLevel = LogInfo; // Default is Info
}
#endif