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
#include <fstream>
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
#if defined(_WIN64)
    sFile.open(Utils::String::stringToWideString(sLogPath).c_str());
#else
#if defined(RETRODECK)
    // Append to the log file for RetroDECK builds
    sFile.open(sLogPath.c_str(), std::ios::out | std::ios::app);
#else
    sFile.open(sLogPath.c_str());
#endif
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
    time_t now_t = std::chrono::system_clock::to_time_t(now);
    // Use thread-safe localtime variants rather than std::localtime
#if defined(_WIN64)
    localtime_s(&tm, &now_t);
#else
    localtime_r(&now_t, &tm);
#endif

    mOutStringStream << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
                     << "." << std::setfill('0') << std::setw(3) << ms.count() // Add milliseconds
                     << "] [" << levelUpper << "] [ES-DE] ";

    // Ensure message level is recorded for later logging (destructor/console/android)
    mMessageLevel = level;
#else

    mOutStringStream << std::put_time(&tm, "%b %d %H:%M:%S ") << mLogLevelMap[level]
                     << (level == LogLevel::LogInfo || level == LogLevel::LogWarning ? ":   " :                                                                                       ":  ");
    mMessageLevel = level;

#endif

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
    // For regular builds (non-RetroDECK) print to stderr when it's an error
    // or the reporting level is Debug. On RetroDECK we always write logs to
    // the terminal via stdout instead, to avoid duplicate messages (stderr+stdout).
#if !defined(RETRODECK)
    if (mMessageLevel == LogError || sReportingLevel >= LogDebug)
        std::cerr << mOutStringStream.str();
#endif
#endif
#if defined(RETRODECK)
    // Write logs to the terminal (stdout) for RetroDECK builds. Avoid using
    // stderr here to prevent duplicate logging when reporting level is Debug.
    std::cout << mOutStringStream.str();
#endif
}

// RetroDECK specific function
#if defined(RETRODECK)
void Log::setReportingLevelFromRetroDeckConfig()
{
    // Try to read the logging level from RetroDECK config file
    const char* rdHomePath = std::getenv("RETRODECK_CONFIG_HOME");
    if (!rdHomePath) {
        LOG(LogError) << "setReportingLevelFromRetroDeckConfig: Failed to read rd_logging_level "
                      << "- RETRODECK_CONFIG_HOME environment variable not set. Falling back to DEBUG.";
        sReportingLevel = LogDebug;
        return;
    }

    std::string configPath = std::string(rdHomePath) + "/retrodeck.cfg";
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        LOG(LogError) << "setReportingLevelFromRetroDeckConfig: Failed to read rd_logging_level "
                      << "from '" << configPath << "' - File not found. Falling back to DEBUG.";
        sReportingLevel = LogDebug;
        return;
    }

    // Parse the JSON file looking for rd_logging_level
    std::string line;
    std::string logLevel = "debug"; // Default fallback
    bool found = false;

    while (std::getline(configFile, line)) {
        // Simple pattern matching for "rd_logging_level": "value"
        size_t pos = line.find("\"rd_logging_level\"");
        if (pos != std::string::npos) {
            // Find the value between quotes after the colon
            size_t colonPos = line.find(':', pos);
            if (colonPos != std::string::npos) {
                size_t firstQuote = line.find('"', colonPos);
                size_t secondQuote = line.find('"', firstQuote + 1);
                if (firstQuote != std::string::npos && secondQuote != std::string::npos) {
                    logLevel = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                    found = true;
                    break;
                }
            }
        }
    }

    configFile.close();

    if (!found) {
        LOG(LogError) << "setReportingLevelFromRetroDeckConfig: Failed to read rd_logging_level "
                      << "from '" << configPath << "' - Setting not found. Falling back to DEBUG.";
        sReportingLevel = LogDebug;
        return;
    }

    // Map string to LogLevel
    if (logLevel == "debug")
        sReportingLevel = LogDebug;
    else if (logLevel == "warning")
        sReportingLevel = LogWarning;
    else if (logLevel == "error")
        sReportingLevel = LogError;
    else if (logLevel == "info")
        sReportingLevel = LogInfo;
    else {
        LOG(LogError) << "setReportingLevelFromRetroDeckConfig: Invalid rd_logging_level value '"
                      << logLevel << "'. Falling back to DEBUG.";
        sReportingLevel = LogDebug;
    }
}

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
