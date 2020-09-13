//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition, an emulator front-end
//  with controller navigation and theming support.
//
//  Originally created by Alec "Aloshi" Lofquist.
//  http://www.aloshi.com
//  Improved and extended by the RetroPie community.
//  Desktop Edition fork by Leon Styhre.
//
//  The line length limit is 100 characters and the indentations are 4 spaces.
//
//  main.cpp
//
//  Main program loop. Interprets command-line arguments, checks for the
//  home folder and es_settings.cfg configuration file, sets up the application
//  environment and starts listening to SDL events.
//

#include "guis/GuiDetectDevice.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiComplexTextEditPopup.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "EmulationStation.h"
#include "InputManager.h"
#include "Log.h"
#include "MameNames.h"
#include "Platform.h"
#include "PowerSaver.h"
#include "Settings.h"
#include "SystemData.h"
#include "SystemScreenSaver.h"

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_main.h>
#include <SDL2/SDL_timer.h>

#if defined(_WIN64)
#include <cstring>
#include <windows.h>
#endif

#include <FreeImage.h>
#include <iostream>
#include <time.h>

bool forceInputConfig = false;

enum returnCode {
    NO_LOADING_ERROR,
    NO_SYSTEMS_FILE,
    NO_ROMS
};

#if defined(_WIN64)
enum win64ConsoleType {
    NO_CONSOLE,
    PARENT_CONSOLE,
    ALLOCATED_CONSOLE
};

// Console output for Windows. The handling of consoles is a mess on this operating system,
// and this is the best solution I could find. EmulationStation is built using the WINDOWS
// subsystem (using the -mwindows compiler flag). The idea is to attach to or allocate a new
// console as needed. However some console types such as the 'Git Bash' shell simply doesn't
// work properly. Windows thinks it's attaching to a console but is unable to redirect the
// standard input and output. Output also can't be redirected or piped by the user for any
// console type and PowerShell behaves quite strange. Still, it works well enough to be
// somewhat usable, at least for the moment. If the allocConsole argument is set to true
// and there is no console available, a new console window will be spawned.
win64ConsoleType outputToConsole(bool allocConsole)
{
    HANDLE outputHandle = nullptr;
    HWND consoleWindow = nullptr;
    win64ConsoleType consoleType = NO_CONSOLE;

    // Try to attach to a parent console process.
    if (AttachConsole(ATTACH_PARENT_PROCESS))
            outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    // If there is a parent console process, then attempt to retrieve its handle.
    if (outputHandle != INVALID_HANDLE_VALUE && outputHandle != nullptr) {
        consoleWindow = GetConsoleWindow();
        consoleType = PARENT_CONSOLE;
    }

    // If we couldn't retrieve the handle, it means we need to allocate a new console window.
    if (!consoleWindow && allocConsole) {
        AllocConsole();
        consoleType = ALLOCATED_CONSOLE;
    }

    // If we are attached to the parent console or we have opened a new console window,
    // then redirect stdin, stdout and stderr accordingly.
    if (consoleType == PARENT_CONSOLE || consoleType == ALLOCATED_CONSOLE) {

        FILE* fp = nullptr;
        freopen_s(&fp, "CONIN$", "rb", stdin);
        freopen_s(&fp, "CONOUT$", "wb", stdout);
        setvbuf(stdout, NULL, _IONBF, 0);
        freopen_s(&fp, "CONOUT$", "wb", stderr);
        setvbuf(stderr, NULL, _IONBF, 0);

        // Point the standard streams to the console.
        std::ios::sync_with_stdio(true);

        // Clear the error state for each standard stream.
        std::wcout.clear();
        std::cout.clear();
        std::wcerr.clear();
        std::cerr.clear();
        std::wcin.clear();
        std::cin.clear();

        std::cout << "\n";
    }

    return consoleType;
}

void closeConsole()
{
    FILE* fp;

    // Redirect stdin, stdout and stderr to NUL.
    freopen_s(&fp, "NUL:", "r", stdin);
    freopen_s(&fp, "NUL:", "w", stdout);
    freopen_s(&fp, "NUL:", "w", stderr);

    FreeConsole();
}
#endif

bool parseArgs(int argc, char* argv[])
{
    Utils::FileSystem::setExePath(argv[0]);

    #if defined(_WIN64)
    // Print any command line output to the console.
    if (argc > 1)
        win64ConsoleType consoleType = outputToConsole(false);
    #endif

    // We need to process --home before any call to Settings::getInstance(),
    // because settings are loaded from the home path.
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--home") == 0) {
            if (i >= argc - 1) {
                std::cerr << "Error: No home path supplied with \'--home'.\n";
                return false;
            }
            #if defined(_WIN64)
            if (!Utils::FileSystem::exists(argv[i + 1]) &&
                    (!Utils::FileSystem::driveExists(argv[i + 1]))) {
            #else
            if (!Utils::FileSystem::exists(argv[i + 1])) {
            #endif
                std::cerr << "Error: Home path \'" << argv[i + 1] << "\' does not exist.\n";
                return false;
            }
            if (Utils::FileSystem::isRegularFile(argv[i + 1])) {
                std::cerr << "Error: Home path \'" << argv[i + 1] <<
                        "\' is a file and not a directory.\n";
                return false;
            }
            Utils::FileSystem::setHomePath(argv[i + 1]);
            break;
        }
    }

    for (int i = 1; i < argc; i++) {
        // Skip past --home flag as we already processed it.
        if (strcmp(argv[i], "--home") == 0) {
            i++;
            continue;
        }
        if (strcmp(argv[i], "--resolution") == 0) {
            if (i >= argc - 2) {
                std::cerr << "Error: Invalid resolution values supplied.\n";
                return false;
            }

            int width = atoi(argv[i + 1]);
            int height = atoi(argv[i + 2]);
            i += 2; // skip the argument value
            Settings::getInstance()->setInt("WindowWidth", width);
            Settings::getInstance()->setInt("WindowHeight", height);
        }
        else if (strcmp(argv[i], "--screensize") == 0) {
            if (i >= argc - 2) {
                std::cerr << "Error: Invalid screensize values supplied.\n";
                return false;
            }

            int width = atoi(argv[i + 1]);
            int height = atoi(argv[i + 2]);
            i += 2; // skip the argument value
            Settings::getInstance()->setInt("ScreenWidth", width);
            Settings::getInstance()->setInt("ScreenHeight", height);
        }
        else if (strcmp(argv[i], "--screenoffset") == 0) {
            if (i >= argc - 2) {
                std::cerr << "Error: Invalid screenoffset values supplied.\n";
                return false;
            }

            int x = atoi(argv[i + 1]);
            int y = atoi(argv[i + 2]);
            i += 2; // skip the argument value
            Settings::getInstance()->setInt("ScreenOffsetX", x);
            Settings::getInstance()->setInt("ScreenOffsetY", y);
        }
        else if (strcmp(argv[i], "--screenrotate") == 0) {
            if (i >= argc - 1) {
                std::cerr << "Error: Invalid screenrotate value supplied.\n";
                return false;
            }

            int rotate = atoi(argv[i + 1]);
            ++i; // skip the argument value
            Settings::getInstance()->setInt("ScreenRotate", rotate);
        }
        else if (strcmp(argv[i], "--max-vram") == 0) {
            if (i >= argc - 1) {
                std::cerr << "Error: Invalid VRAM value supplied.\n";
                return false;
            }
            int maxVRAM = atoi(argv[i + 1]);
            Settings::getInstance()->setInt("MaxVRAM", maxVRAM);
            ++i; // skip the argument value
        }
        else if (strcmp(argv[i], "--gamelist-only") == 0) {
            Settings::getInstance()->setBool("ParseGamelistOnly", true);
        }
        else if (strcmp(argv[i], "--ignore-gamelist") == 0) {
            Settings::getInstance()->setBool("IgnoreGamelist", true);
        }
        else if (strcmp(argv[i], "--show-hidden-files") == 0) {
            Settings::getInstance()->setBool("ShowHiddenFiles", true);
        }
        else if (strcmp(argv[i], "--show-hidden-games") == 0) {
            Settings::getInstance()->setBool("ShowHiddenGames", true);
        }
        else if (strcmp(argv[i], "--no-exit") == 0) {
            Settings::getInstance()->setBool("ShowExit", false);
        }
        else if (strcmp(argv[i], "--no-splash") == 0) {
            Settings::getInstance()->setBool("SplashScreen", false);
        }
        else if (strcmp(argv[i], "--debug") == 0) {
            Settings::getInstance()->setBool("Debug", true);
            Log::setReportingLevel(LogDebug);
        }
        // On Unix, enable settings for the fullscreen mode.
        // On macOS and Windows only windowed mode is supported.
        #if defined(__unix__)
        else if (strcmp(argv[i], "--fullscreen-normal") == 0) {
            Settings::getInstance()->setString("FullscreenMode", "normal");
        }
        else if (strcmp(argv[i], "--fullscreen-borderless") == 0) {
            Settings::getInstance()->setString("FullscreenMode", "borderless");
        }
        else if (strcmp(argv[i], "--windowed") == 0) {
            Settings::getInstance()->setBool("Windowed", true);
        }
        #endif
        else if (strcmp(argv[i], "--vsync") == 0) {
            bool vsync = (strcmp(argv[i + 1], "on") == 0 ||
                    strcmp(argv[i + 1], "1") == 0) ? true : false;
            Settings::getInstance()->setBool("VSync", vsync);
            i++; // Skip vsync value.
        }
        else if (strcmp(argv[i], "--gpu-statistics") == 0) {
            Settings::getInstance()->setBool("DisplayGPUStatistics", "true");
        }
        else if (strcmp(argv[i], "--force-full") == 0) {
            Settings::getInstance()->setString("UIMode", "full");
        }
        else if (strcmp(argv[i], "--force-kiosk") == 0) {
            Settings::getInstance()->setBool("ForceKiosk", true);
        }
        else if (strcmp(argv[i], "--force-kid") == 0) {
            Settings::getInstance()->setBool("ForceKid", true);
        }
        else if (strcmp(argv[i], "--force-disable-filters") == 0) {
            Settings::getInstance()->setBool("ForceDisableFilters", true);
        }
        else if (strcmp(argv[i], "--force-input-config") == 0) {
            forceInputConfig = true;
        }
        else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            std::cout <<
            "EmulationStation Desktop Edition v" << PROGRAM_VERSION_STRING << "\n";
            return false;
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            std::cout <<
"Usage: emulationstation [options]\n"
"EmulationStation Desktop Edition, Emulator Front-end\n\n"
"Options:\n"
"  --resolution [width] [height]   Try to force a particular resolution\n"
"  --gamelist-only                 Skip automatic game ROM search, only read from gamelist.xml\n"
"  --ignore-gamelist               Ignore the gamelist files (useful for troubleshooting)\n"
"  --show-hidden-files             Show hidden files and folders\n"
"  --show-hidden-games             Show hidden games\n"
"  --no-exit                       Don't show the exit option in the menu\n"
"  --no-splash                     Don't show the splash screen\n"
"  --debug                         Print debug information\n"
#if defined(__unix__)
"  --windowed                      Windowed mode, should be combined with --resolution\n"
"  --fullscreen-normal             Normal fullscreen mode\n"
"  --fullscreen-borderless         Borderless fullscreen mode (always on top)\n"
#endif
"  --vsync [1/on or 0/off]         Turn vsync on or off (default is on)\n"
"  --max-vram [size]               Max VRAM to use (in mebibytes) before swapping\n"
"  --gpu-statistics                Display framerate and VRAM usage overlay\n"
"  --force-full                    Force the UI mode to Full\n"
"  --force-kid                     Force the UI mode to Kid\n"
"  --force-kiosk                   Force the UI mode to Kiosk\n"
"  --force-disable-filters         Force the UI to ignore applied filters in gamelist\n"
"  --force-input-config            Force configuration of input device\n"
"  --home [path]                   Directory to use as home path\n"
"  --version, -v                   Display version information\n"
"  --help, -h                      Summon a sentient, angry tuba\n";
            return false; // Exit after printing help.
        }
        else {
            std::string argv_unknown = argv[i];
            std::cout << "Unknown option '" << argv_unknown << "'.\n";
            std::cout << "Try 'emulationstation --help' for more information.\n";
            return false; // Exit after printing message.
        }
    }

    return true;
}

bool verifyHomeFolderExists()
{
    // Make sure the config directory exists.
    std::string home = Utils::FileSystem::getHomePath();
    std::string configDir = home + "/.emulationstation";
    if (!Utils::FileSystem::exists(configDir)) {
        std::cout << "Creating config directory \"" << configDir << "\"\n";
        Utils::FileSystem::createDirectory(configDir);
        if (!Utils::FileSystem::exists(configDir)) {
            std::cerr << "Config directory could not be created!\n";
            return false;
        }
    }

    return true;
}

// Returns NO_LOADING_ERROR if everything is OK.
// Otherwise returns either NO_SYSTEMS_FILE or NO_ROMS.
returnCode loadSystemConfigFile(std::string& errorMsg)
{
    if (!SystemData::loadConfig()) {
        LOG(LogError) << "Could not parse systems configuration file.";
        errorMsg = "COULDN'T FIND THE SYSTEMS CONFIGURATION FILE.\n"
                "ATTEMPTED TO COPY A TEMPLATE ES_SYSTEMS.CFG FILE\n"
                "FROM THE EMULATIONSTATION RESOURCES DIRECTORY,\n"
                "BUT THIS FAILED. HAS EMULATIONSTATION BEEN PROPERLY\n"
                "INSTALLED AND DO YOU HAVE WRITE PERMISSIONS TO \n"
                "YOUR HOME DIRECTORY?";
        return NO_SYSTEMS_FILE;
    }

    if (SystemData::sSystemVector.size() == 0) {
        LOG(LogError) << "No systems found, does at least one system have a game present? "
                "(Check that the file extensions are supported.)";
        errorMsg = "THE SYSTEMS CONFIGURATION FILE EXISTS, BUT NO\n"
                "GAME FILES WERE FOUND. EITHER PLACE YOUR GAMES\n"
                "IN THE CURRENTLY CONFIGURED ROM DIRECTORY OR\n"
                "CHANGE IT USING THE BUTTON BELOW. MAKE SURE\n"
                "THAT YOUR FILE EXTENSIONS AND SYSTEMS DIRECTORY\n"
                "NAMES ARE SUPPORTED BY EMULATIONSTATION-DE.\n"
                "THIS IS THE CURRENTLY CONFIGURED ROM DIRECTORY:\n";
        #if defined(_WIN64)
        errorMsg += Utils::String::replace(FileData::getROMDirectory(), "/", "\\");
        #else
        errorMsg += FileData::getROMDirectory();
        #endif

        return NO_ROMS;
    }

    return NO_LOADING_ERROR;
}

// Called on exit, assuming we get far enough to have the log initialized.
void onExit()
{
    Log::close();
}

int main(int argc, char* argv[])
{
    srand((unsigned int)time(nullptr));

    std::locale::global(std::locale("C"));

    if (!parseArgs(argc, argv)) {
        #if defined(_WIN64)
        closeConsole();
        #endif
        return 0;
    }

    #if defined(_WIN64)
    // Send debug output to the console..
    if (Settings::getInstance()->getBool("Debug"))
        outputToConsole(true);
    #endif

    #if defined(_WIN64)
    // Hide taskbar if the setting for this is enabled.
    bool taskbarStateChanged = false;
    unsigned int taskbarState;

    if (Settings::getInstance()->getBool("HideTaskbar")) {
        taskbarStateChanged = true;
        taskbarState = getTaskbarState();
        hideTaskbar();
    }
    #endif

    // Call this ONLY when linking with FreeImage as a static library.
    #if defined(FREEIMAGE_LIB)
    FreeImage_Initialise();
    #endif

    // If ~/.emulationstation doesn't exist and cannot be created, bail.
    if (!verifyHomeFolderExists())
        return 1;

    // Start the logger.
    Log::init();
    Log::open();
    LOG(LogInfo) << "EmulationStation Desktop Edition v" << PROGRAM_VERSION_STRING <<
            ", built " << PROGRAM_BUILT_STRING;

    // Always close the log on exit.
    atexit(&onExit);

    // Check if the configuration file exists, and if not, create it.
    // This should only happen on first application startup.
    if (!Utils::FileSystem::exists(Utils::FileSystem::getHomePath() +
            "/.emulationstation/es_settings.cfg")) {
        LOG(LogInfo) << "Settings file es_settings.cfg does not exist, creating it...";
        Settings::getInstance()->saveFile();
    }

    Window window;
    SystemScreenSaver screensaver(&window);
    PowerSaver::init();
    ViewController::init(&window);
    CollectionSystemManager::init(&window);
    MameNames::init();
    window.pushGui(ViewController::get());

    bool splashScreen = Settings::getInstance()->getBool("SplashScreen");
    bool splashScreenProgress = Settings::getInstance()->getBool("SplashScreenProgress");
    SDL_Event event;

    if (!window.init()) {
        LOG(LogError) << "Window failed to initialize.";
        return 1;
    }

    InputManager::getInstance()->parseEvent(event, &window);
    if (event.type == SDL_QUIT)
        return 1;


    if (splashScreen) {
        std::string progressText = "Loading...";
        if (splashScreenProgress)
            progressText = "Loading system config...";
        window.renderLoadingScreen(progressText);
    }

    std::string errorMsg;
    returnCode returnCodeValue = loadSystemConfigFile(errorMsg);

    if (returnCodeValue) {
        // Something went terribly wrong.
        if (errorMsg == "") {
            LOG(LogError) << "Unknown error occured while parsing system config file.";
            Renderer::deinit();
            return 1;
        }

        HelpStyle helpStyle = HelpStyle();

        if (errorMsg == "")
            helpStyle.applyTheme(ViewController::get()->
                    getState().getSystem()->getTheme(), "system");

        // If there was an issue with installing the es_systems.cfg file from the
        // template directory, then display an error message and let the user quit.
        // If there are no game files found, give the option to the user to quit or
        // to configure a different ROM directory. The application will need to be
        // restarted though, to activate any new ROM directory setting.
        if (returnCodeValue == NO_SYSTEMS_FILE) {
            window.pushGui(new GuiMsgBox(&window, helpStyle,
                errorMsg.c_str(),
                "QUIT", [] {
                    SDL_Event* quit = new SDL_Event();
                    quit->type = SDL_QUIT;
                    SDL_PushEvent(quit);
                }, "", nullptr, "", nullptr, true));
        }
        else if (returnCodeValue == NO_ROMS) {
            auto updateVal = [](const std::string& newROMDirectory) {
                Settings::getInstance()->setString("ROMDirectory", newROMDirectory);
                Settings::getInstance()->saveFile();
                SDL_Event* quit = new SDL_Event();
                quit->type = SDL_QUIT;
                SDL_PushEvent(quit);
            };

            window.pushGui(new GuiMsgBox(&window, helpStyle, errorMsg.c_str(),
                    "CHANGE ROM DIRECTORY", [&window, &helpStyle, updateVal] {
                std::string currentROMDirectory;
                #if defined(_WIN64)
                currentROMDirectory =
                        Utils::String::replace(FileData::getROMDirectory(), "/", "\\");
                #else
                currentROMDirectory = FileData::getROMDirectory();
                #endif

                window.pushGui(new GuiComplexTextEditPopup(
                        &window,
                        helpStyle,
                        "ENTER ROM DIRECTORY",
                        "Currently configured directory:",
                        currentROMDirectory,
                        currentROMDirectory,
                        updateVal,
                        false,
                        "SAVE AND QUIT",
                        "SAVE CHANGES?",
                        "LOAD CURRENT",
                        "LOAD CURRENTLY CONFIGURED VALUE",
                        "CLEAR",
                        "CLEAR (LEAVE BLANK TO RESET TO DEFAULT DIRECTORY)",
                        true));
            },
            "QUIT", [] {
                SDL_Event* quit = new SDL_Event();
                quit->type = SDL_QUIT;
                SDL_PushEvent(quit);
            }, "", nullptr, true));
        }
    }

    // Dont generate joystick events while we're loading.
    // (Hopefully fixes "automatically started emulator" bug.)
    SDL_JoystickEventState(SDL_DISABLE);

    // Preload what we can right away instead of waiting for the user to select it.
    // This makes for no delays when accessing content, but a longer startup time.
    ViewController::get()->preload();

    if (splashScreen && splashScreenProgress)
        window.renderLoadingScreen("Done.");

    // Choose which GUI to open depending on if an input configuration already exists and
    // whether the flag to force the input configuration was passed from the command line.
    if (errorMsg == "") {
        if (!forceInputConfig && Utils::FileSystem::exists(InputManager::getConfigPath()) &&
                InputManager::getInstance()->getNumConfiguredDevices() > 0) {
            ViewController::get()->goToStart();
        }
        else if (forceInputConfig) {
            window.pushGui(new GuiDetectDevice(&window, true, true, [] {
                    ViewController::get()->goToStart(); }));
        }
        else {
            if (InputManager::getInstance()->getNumJoysticks() > 0)
                window.pushGui(new GuiDetectDevice(&window, true, false, [] {
                        ViewController::get()->goToStart(); }));
            else
                ViewController::get()->goToStart();
        }
    }

    // Check if the media directory exists, otherwise log an information entry.
    if (!Utils::FileSystem::isDirectory(FileData::getMediaDirectory()) ||
            Utils::FileSystem::isSymlink(FileData::getMediaDirectory())) {
        LOG(LogInfo) << "Game media directory does not exist "
                "(or is not a directory or a symlink):";
        LOG(LogInfo) << FileData::getMediaDirectory();
    }

    // Generate joystick events since we're done loading.
    SDL_JoystickEventState(SDL_ENABLE);

    int lastTime = SDL_GetTicks();
    int ps_time = SDL_GetTicks();

    bool running = true;

    while (running) {
        SDL_Event event;
        bool ps_standby = PowerSaver::getState() && (int) SDL_GetTicks() -
                ps_time > PowerSaver::getMode();

        if (ps_standby ? SDL_WaitEventTimeout(&event, PowerSaver::getTimeout())
                : SDL_PollEvent(&event)) {
            do {
                InputManager::getInstance()->parseEvent(event, &window);

                if (event.type == SDL_QUIT)
                    running = false;
            }
            while (SDL_PollEvent(&event));

            // Triggered if exiting from SDL_WaitEvent due to event.
            if (ps_standby)
                // Show as if continuing from last event.
                lastTime = SDL_GetTicks();

            // Reset counter.
            ps_time = SDL_GetTicks();
        }
        else if (ps_standby) {
            // If exiting SDL_WaitEventTimeout due to timeout.
            // Trail considering timeout as an event.
            ps_time = SDL_GetTicks();
        }

        if (window.isSleeping()) {
            lastTime = SDL_GetTicks();
            // This doesn't need to be accurate, we're just giving up
            // our CPU time until something wakes us up.
            continue;
            SDL_Delay(1);
        }

        int curTime = SDL_GetTicks();
        int deltaTime = curTime - lastTime;
        lastTime = curTime;

        // Cap deltaTime if it ever goes negative.
        if (deltaTime < 0)
            deltaTime = 1000;

        window.update(deltaTime);
        window.render();
        Renderer::swapBuffers();

        Log::flush();
    }

    while (window.peekGui() != ViewController::get())
        delete window.peekGui();
    window.deinit();

    MameNames::deinit();
    CollectionSystemManager::deinit();
    SystemData::deleteSystems();

    // Call this ONLY when linking with FreeImage as a static library.
    #if defined(FREEIMAGE_LIB)
    FreeImage_DeInitialise();
    #endif

    #if defined(_WIN64)
    // If the taskbar state was changed (taskbar was hidden), then revert it.
    if (taskbarStateChanged)
        revertTaskbarState(taskbarState);
    #endif

    processQuitMode();

    LOG(LogInfo) << "EmulationStation cleanly shutting down.";

    #if defined(_WIN64)
    closeConsole();
    #endif

    return 0;
}
