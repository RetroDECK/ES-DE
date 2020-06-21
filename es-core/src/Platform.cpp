//
//  Platform.cpp
//
//  Platform-specific functions.
//

#include "Platform.h"

#include <SDL_events.h>
#ifdef WIN32
#include <codecvt>
#else
#include <unistd.h>
#endif
#include <fcntl.h>

#include "Log.h"

int runRebootCommand()
{
#ifdef WIN32 // Windows.
    return system("shutdown -r -t 0");
#else // macOS and Linux.
    return system("shutdown --reboot now");
#endif
}

int runPoweroffCommand()
{
#ifdef WIN32 // Windows.
    return system("shutdown -s -t 0");
#else // macOS and Linux.
    return system("shutdown --poweroff now");
#endif
}

int runSystemCommand(const std::string& cmd_utf8)
{
#ifdef WIN32
    // On Windows we use _wsystem to support non-ASCII paths
    // which requires converting from UTF8 to a wstring.
    typedef std::codecvt_utf8<wchar_t> convert_type;
    std::wstring_convert<convert_type, wchar_t> converter;
    std::wstring wchar_str = converter.from_bytes(cmd_utf8);
    return _wsystem(wchar_str.c_str());
#else
    return system(cmd_utf8.c_str());
#endif
}

QuitMode quitMode = QuitMode::QUIT;

int quitES(QuitMode mode)
{
    quitMode = mode;

    SDL_Event *quit = new SDL_Event();
    quit->type = SDL_QUIT;
    SDL_PushEvent(quit);
    return 0;
}

void touch(const std::string& filename)
{
#ifdef WIN32
    FILE* fp = fopen(filename.c_str(), "ab+");
    if (fp != NULL)
        fclose(fp);
#else
    int fd = open(filename.c_str(), O_CREAT|O_WRONLY, 0644);
    if (fd >= 0)
        close(fd);
#endif
}

void processQuitMode()
{
    switch (quitMode) {
    case QuitMode::REBOOT:
        LOG(LogInfo) << "Rebooting system";
        runRebootCommand();
        break;
    case QuitMode::POWEROFF:
        LOG(LogInfo) << "Powering off system";
        runPoweroffCommand();
        break;
    }
}
