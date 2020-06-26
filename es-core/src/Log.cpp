//
//  Log.cpp
//
//  Log handling.
//

#include "Log.h"

#include "utils/FileSystemUtil.h"
#include "Platform.h"

#include <iostream>
#include <iomanip>

LogLevel Log::reportingLevel = LogInfo;
FILE* Log::file = nullptr; // fopen(getLogPath().c_str(), "w");

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
    remove((getLogPath() + ".bak").c_str());
    // Rename previous log file.
    rename(getLogPath().c_str(), (getLogPath() + ".bak").c_str());
    return;
}

void Log::open()
{
    file = fopen(getLogPath().c_str(), "w");
}

std::ostringstream& Log::get(LogLevel level)
{
    time_t t = time(nullptr);
    os << std::put_time(localtime(&t), "%b %d %T ") << "lvl" << level << ": \t";
    messageLevel = level;

    return os;
}

void Log::flush()
{
    fflush(getOutput());
}

void Log::close()
{
    fclose(file);
    file = nullptr;
}

FILE* Log::getOutput()
{
    return file;
}

Log::~Log()
{
    os << std::endl;

    if (getOutput() == nullptr) {
        // not open yet, print to stdout
        std::cerr << "ERROR - tried to write to log file before it was open! "
                "The following won't be logged:\n";
        std::cerr << os.str();
        return;
    }

    fprintf(getOutput(), "%s", os.str().c_str());

    // If it's an error, also print to console.
    // Print all messages if using --debug.
    if (messageLevel == LogError || reportingLevel >= LogDebug)
        fprintf(stderr, "%s", os.str().c_str());
}
