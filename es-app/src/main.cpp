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
#include "utils/FileSystemUtil.h"
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
#include <SDL_events.h>
#include <SDL_main.h>
#include <SDL_timer.h>
#include <iostream>
#include <time.h>
#ifdef WIN32
#include <Windows.h>
#endif

#include <FreeImage.h>

enum eErrorCodes {
    NO_ERRORS,
    NO_SYSTEMS_FILE,
    NO_ROMS
};

bool parseArgs(int argc, char* argv[])
{
    Utils::FileSystem::setExePath(argv[0]);

    // We need to process --home before any call to Settings::getInstance(),
    // because settings are loaded from the home path.
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--home") == 0) {
            if (i >= argc - 1) {
                std::cerr << "Error: No home path supplied with \'--home'.\n";
                return false;
            }
            if(!Utils::FileSystem::exists(argv[i + 1])) {
                std::cerr << "Error: Home path \'" << argv[i + 1] << "\' does not exist.\n";
                return false;
            }
            if(Utils::FileSystem::isRegularFile(argv[i + 1])) {
                std::cerr << "Error: Home path \'" << argv[i + 1] <<
                        "\' is a file and not a directory.\n";
                return false;
            }
            Utils::FileSystem::setHomePath(argv[i + 1]);
            break;
        }
    }

    for(int i = 1; i < argc; i++) {
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
        else if (strcmp(argv[i], "--draw-framerate") == 0) {
            Settings::getInstance()->setBool("DrawFramerate", true);
        }
        else if (strcmp(argv[i], "--no-exit") == 0) {
            Settings::getInstance()->setBool("ShowExit", false);
        }
        else if (strcmp(argv[i], "--no-splash") == 0) {
            Settings::getInstance()->setBool("SplashScreen", false);
        }
        else if (strcmp(argv[i], "--debug") == 0) {
            Settings::getInstance()->setBool("Debug", true);
            Settings::getInstance()->setBool("HideConsole", false);
            Log::setReportingLevel(LogDebug);
        }
        else if (strcmp(argv[i], "--fullscreen-normal") == 0) {
            Settings::getInstance()->setString("FullscreenMode", "normal");
        }
        else if (strcmp(argv[i], "--fullscreen-borderless") == 0) {
            Settings::getInstance()->setString("FullscreenMode", "borderless");
        }
        else if (strcmp(argv[i], "--windowed") == 0) {
            Settings::getInstance()->setBool("Windowed", true);
        }
        else if (strcmp(argv[i], "--vsync") == 0) {
            bool vsync = (strcmp(argv[i + 1], "on") == 0 ||
                    strcmp(argv[i + 1], "1") == 0) ? true : false;
            Settings::getInstance()->setBool("VSync", vsync);
            i++; // Skip vsync value.
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
        else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            std::cout <<
            "EmulationStation Desktop Edition v" << PROGRAM_VERSION_STRING << "\n";
            return false;
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
#ifdef WIN32
            // This is a bit of a hack, but otherwise output will go to nowhere when the
            // application is compiled with the "WINDOWS" subsystem (which we usually are).
            // If you're an experienced Windows programmer and know how to do this
            // the right way, please submit a pull request!
            AttachConsole(ATTACH_PARENT_PROCESS);
            freopen("CONOUT$", "wb", stdout);
#endif
            std::cout <<
"EmulationStation Desktop Edition\n"
"An Emulator Front-end\n\n"
"Options:\n"
"--resolution [width] [height]   Try to force a particular resolution\n"
"--gamelist-only                 Skip automatic game ROM search, only read from gamelist.xml\n"
"--ignore-gamelist               Ignore the gamelist files (useful for troubleshooting)\n"
"--draw-framerate                Display the framerate\n"
"--no-exit                       Don't show the exit option in the menu\n"
"--no-splash                     Don't show the splash screen\n"
#ifdef WIN32
"--debug                         Show console and print debug information\n"
#else
"--debug                         Print debug information\n"
#endif
"--windowed                      Windowed mode, should be combined with --resolution\n"
"--fullscreen-normal             Normal fullscreen mode\n"
"--fullscreen-borderless         Borderless fullscreen mode (always on top)\n"
"--vsync [1/on or 0/off]         Turn vsync on or off (default is on)\n"
"--max-vram [size]               Max VRAM to use in Mb before swapping\n"
"                                Set to at least 20 to avoid unpredictable behavior\n"
"--force-kid                     Force the UI mode to Kid\n"
"--force-kiosk                   Force the UI mode to Kiosk\n"
"--force-disable-filters         Force the UI to ignore applied filters in gamelist\n"
"--home [path]                   Directory to use as home path\n"
"--version, -v                   Displays version information\n"
"--help, -h                      Summon a sentient, angry tuba\n";
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

// Returns NO_ERRORS if everything is OK.
// Otherwise returns either NO_SYSTEMS_FILE or NO_ROMS.
unsigned int loadSystemConfigFile(std::string& errorMsg)
{
    if (!SystemData::loadConfig()) {
        LOG(LogError) << "Error while parsing systems configuration file!";
        errorMsg = "COULDN'T FIND THE SYSTEMS CONFIGURATION FILE.\n"
                "WILL ATTEMPT TO INSTALL A TEMPLATE ES_SYSTEMS.CFG FILE FROM "
                "THE EMULATIONSTATION RESOURCES DIRECTORY.\n"
                "PLEASE RESTART THE APPLICATION.";
        return NO_SYSTEMS_FILE;
    }

    if (SystemData::sSystemVector.size() == 0)
    {
        LOG(LogError) << "No systems found! Does at least one system have a game present? (check "
        "that extensions match!)\n";
        errorMsg = "THE SYSTEMS CONFIGURATION FILE EXISTS BUT NO GAME "
                "ROM FILES WERE FOUND. PLEASE MAKE SURE THAT THE 'ROMDIRECTORY' "
                "SETTING IN ES_SYSTEMS.CFG IS POINTING TO YOUR ROM DIRECTORY "
                "AND THAT YOUR GAME ROMS ARE USING SUPPORTED FILE EXTENSIONS. "
                "THIS IS THE CURRENTLY CONFIGURED ROM DIRECTORY:\n";
        errorMsg += FileData::getROMDirectory();
        return NO_ROMS;
    }

    return NO_ERRORS;
}

// Called on exit, assuming we get far enough to have the log initialized.
void onExit()
{
    Log::close();
}

int main(int argc, char* argv[])
{
    srand((unsigned int)time(NULL));

    std::locale::global(std::locale("C"));

    if (!parseArgs(argc, argv))
        return 0;

    // Only show the console on Windows if HideConsole is false.
#ifdef WIN32
    // MSVC has a "SubSystem" option, with two primary options: "WINDOWS" and "CONSOLE".
    // In "WINDOWS" mode, no console is automatically created for us.  This is good,
    // because we can choose to only create the console window if the user explicitly
    // asks for it, preventing it from flashing open and then closing.
    // In "CONSOLE" mode, a console is always automatically created for us before we
    // enter main. In this case, we can only hide the console after the fact, which
    // will leave a brief flash.
    // TL;DR: You should compile ES under the "WINDOWS" subsystem.
    // I have no idea how this works with non-MSVC compilers.
    if (!Settings::getInstance()->getBool("HideConsole")) {
        // We want to show the console.
        // If we're compiled in "CONSOLE" mode, this is already done.
        // If we're compiled in "WINDOWS" mode, no console is created for us automatically;
        // the user asked for one, so make one and then hook stdin/stdout/sterr up to it.
        if (AllocConsole()) { // Should only pass in "WINDOWS" mode.
            freopen("CONIN$", "r", stdin);
            freopen("CONOUT$", "wb", stdout);
            freopen("CONOUT$", "wb", stderr);
        }
    }
    else {
        // We want to hide the console.
        // If we're compiled with the "WINDOWS" subsystem, this is already done.
        // If we're compiled with the "CONSOLE" subsystem, a console is already created;
        // it'll flash open, but we hide it nearly immediately.
        if (GetConsoleWindow()) // Should only pass in "CONSOLE" mode.
            ShowWindow(GetConsoleWindow(), SW_HIDE);
    }
#endif

    // Call this ONLY when linking with FreeImage as a static library.
#ifdef FREEIMAGE_LIB
    FreeImage_Initialise();
#endif

    // If ~/.emulationstation doesn't exist and cannot be created, bail.
    if (!verifyHomeFolderExists())
        return 1;

    // Start the logger.
    Log::init();
    Log::open();
    LOG(LogInfo) << "EmulationStation - v" << PROGRAM_VERSION_STRING <<
            ", built " << PROGRAM_BUILT_STRING;

    // Always close the log on exit.
    atexit(&onExit);

    Window window;
    SystemScreenSaver screensaver(&window);
    PowerSaver::init();
    ViewController::init(&window);
    CollectionSystemManager::init(&window);
    MameNames::init();
    window.pushGui(ViewController::get());

    bool splashScreen = Settings::getInstance()->getBool("SplashScreen");
    bool splashScreenProgress = Settings::getInstance()->getBool("SplashScreenProgress");

    if (!window.init()) {
        LOG(LogError) << "Window failed to initialize!";
        return 1;
    }

    if (splashScreen) {
        std::string progressText = "Loading...";
        if (splashScreenProgress)
            progressText = "Loading system config...";
        window.renderLoadingScreen(progressText);
    }

    std::string errorMsg;

    if (loadSystemConfigFile(errorMsg) != NO_ERRORS) {
        // Something went terribly wrong.
        if (errorMsg == "")
        {
            LOG(LogError) << "Unknown error occured while parsing system config file.";
            Renderer::deinit();
            return 1;
        }

        HelpStyle helpStyle = HelpStyle();

        if (errorMsg == "")
            helpStyle.applyTheme(ViewController::get()->
                    getState().getSystem()->getTheme(), "system");

        // We can't handle es_systems.cfg file problems inside ES itself,
        // so display the error message and then quit.
        window.pushGui(new GuiMsgBox(&window, helpStyle,
            errorMsg.c_str(),
            "QUIT", [] {
                SDL_Event* quit = new SDL_Event();
                quit->type = SDL_QUIT;
                SDL_PushEvent(quit);
            }));
    }

    std::vector<HelpPrompt> prompts;
    prompts.push_back(HelpPrompt("a", "Quit"));
    window.setHelpPrompts(prompts, HelpStyle());

    // Dont generate joystick events while we're loading.
    // (Hopefully fixes "automatically started emulator" bug.)
    SDL_JoystickEventState(SDL_DISABLE);

    // Preload what we can right away instead of waiting for the user to select it.
    // This makes for no delays when accessing content, but a longer startup time.
    ViewController::get()->preload();

    if (splashScreen && splashScreenProgress)
        window.renderLoadingScreen("Done.");

    // Choose which GUI to open depending on if an input configuration already exists.
    if (errorMsg == "") {
        if (Utils::FileSystem::exists(InputManager::getConfigPath()) &&
                InputManager::getInstance()->getNumConfiguredDevices() > 0) {
            ViewController::get()->goToStart();
        }
        else {
            window.pushGui(new GuiDetectDevice(&window, true, [] {
                    ViewController::get()->goToStart(); }));
        }
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
#ifdef FREEIMAGE_LIB
    FreeImage_DeInitialise();
#endif

    processQuitMode();

    LOG(LogInfo) << "EmulationStation cleanly shutting down.";

    return 0;
}
