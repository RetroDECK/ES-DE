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
//  Line breaks are Unix-style (line feed only).
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
#include "AudioManager.h"
#include "CollectionSystemsManager.h"
#include "EmulationStation.h"
#include "InputManager.h"
#include "Log.h"
#include "MameNames.h"
#include "Platform.h"
#include "Settings.h"
#include "SystemData.h"
#include "SystemScreensaver.h"

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
bool settingsNeedSaving = false;

enum loadSystemsReturnCode {
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
        setvbuf(stdout, 0, _IONBF, 0);
        freopen_s(&fp, "CONOUT$", "wb", stderr);
        setvbuf(stderr, 0, _IONBF, 0);

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
            i++; // Skip the argument value.
            continue;
        }
        if (strcmp(argv[i], "--display") == 0) {
            if (i >= argc - 1 || atoi(argv[i + 1]) < 1 || atoi(argv[i + 1]) > 4) {
                std::cerr << "Error: Invalid display index supplied.\n";
                return false;
            }
            int DisplayIndex = atoi(argv[i + 1]);
            Settings::getInstance()->setInt("DisplayIndex", DisplayIndex);
            settingsNeedSaving = true;
            i++;
        }
        else if (strcmp(argv[i], "--resolution") == 0) {
            if (i >= argc - 2) {
                std::cerr << "Error: Invalid resolution values supplied.\n";
                return false;
            }
            int width = atoi(argv[i + 1]);
            int height = atoi(argv[i + 2]);
            if (width < 640 || height < 480 || width > 7680 || height > 4320 ||
                    height < width / 4 || width < height / 2) {
                std::cerr << "Error: Unsupported resolution "
                        << width << "x" << height << " supplied.\n";
                return false;
            }
            Settings::getInstance()->setInt("WindowWidth", width);
            Settings::getInstance()->setInt("WindowHeight", height);
            i += 2;
        }
        else if (strcmp(argv[i], "--screensize") == 0) {
            if (i >= argc - 2) {
                std::cerr << "Error: Invalid screensize values supplied.\n";
                return false;
            }
            int width = atoi(argv[i + 1]);
            int height = atoi(argv[i + 2]);
            Settings::getInstance()->setInt("ScreenWidth", width);
            Settings::getInstance()->setInt("ScreenHeight", height);
            i += 2;
        }
        else if (strcmp(argv[i], "--screenoffset") == 0) {
            if (i >= argc - 2) {
                std::cerr << "Error: Invalid screenoffset values supplied.\n";
                return false;
            }
            int x = atoi(argv[i + 1]);
            int y = atoi(argv[i + 2]);
            Settings::getInstance()->setInt("ScreenOffsetX", x);
            Settings::getInstance()->setInt("ScreenOffsetY", y);
            i += 2;
        }
        else if (strcmp(argv[i], "--screenrotate") == 0) {
            if (i >= argc - 1) {
                std::cerr << "Error: Invalid screenrotate value supplied.\n";
                return false;
            }
            int rotate = atoi(argv[i + 1]);
            Settings::getInstance()->setInt("ScreenRotate", rotate);
            i++;
        }
        // On Unix, enable settings for the fullscreen mode.
        // On macOS and Windows only windowed mode is supported.
        #if defined(__unix__)
        else if (strcmp(argv[i], "--windowed") == 0) {
            Settings::getInstance()->setBool("Windowed", true);
        }
        else if (strcmp(argv[i], "--fullscreen-normal") == 0) {
            Settings::getInstance()->setString("FullscreenMode", "normal");
            settingsNeedSaving = true;
        }
        else if (strcmp(argv[i], "--fullscreen-borderless") == 0) {
            Settings::getInstance()->setString("FullscreenMode", "borderless");
            settingsNeedSaving = true;
        }
        #endif
        else if (strcmp(argv[i], "--vsync") == 0) {
            if (i >= argc - 1) {
                std::cerr << "Error: No VSync value supplied.\n";
                return false;
            }
            std::string vSyncValue = argv[i + 1];
            if (vSyncValue != "on" && vSyncValue != "off" && vSyncValue != "1" &&
                    vSyncValue != "0") {
                std::cerr << "Error: Invalid VSync value supplied.\n";
                return false;
            }
            bool vSync = (vSyncValue == "on" || vSyncValue == "1") ? true : false;
            Settings::getInstance()->setBool("VSync", vSync);
            i++;
        }
        else if (strcmp(argv[i], "--max-vram") == 0) {
            if (i >= argc - 1) {
                std::cerr << "Error: Invalid VRAM value supplied.\n";
                return false;
            }
            int maxVRAM = atoi(argv[i + 1]);
            Settings::getInstance()->setInt("MaxVRAM", maxVRAM);
            settingsNeedSaving = true;
            i++;
        }
        else if (strcmp(argv[i], "--no-splash") == 0) {
            Settings::getInstance()->setBool("SplashScreen", false);
        }
        else if (strcmp(argv[i], "--gamelist-only") == 0) {
            Settings::getInstance()->setBool("ParseGamelistOnly", true);
        }
        else if (strcmp(argv[i], "--ignore-gamelist") == 0) {
            Settings::getInstance()->setBool("IgnoreGamelist", true);
        }
        else if (strcmp(argv[i], "--show-hidden-files") == 0) {
            Settings::getInstance()->setBool("ShowHiddenFiles", true);
            settingsNeedSaving = true;
        }
        else if (strcmp(argv[i], "--show-hidden-games") == 0) {
            Settings::getInstance()->setBool("ShowHiddenGames", true);
            settingsNeedSaving = true;
        }
        else if (strcmp(argv[i], "--force-full") == 0) {
            Settings::getInstance()->setString("UIMode", "full");
            Settings::getInstance()->setBool("ForceFull", true);
        }
        else if (strcmp(argv[i], "--force-kiosk") == 0) {
            Settings::getInstance()->setBool("ForceKiosk", true);
        }
        else if (strcmp(argv[i], "--force-kid") == 0) {
            Settings::getInstance()->setBool("ForceKid", true);
        }
        else if (strcmp(argv[i], "--force-input-config") == 0) {
            forceInputConfig = true;
        }
        else if (strcmp(argv[i], "--debug") == 0) {
            Settings::getInstance()->setBool("Debug", true);
            Log::setReportingLevel(LogDebug);
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
"  --display [index 1-4]           Display/monitor to use\n"
"  --resolution [width] [height]   Application resolution\n"
#if defined(__unix__)
"  --windowed                      Windowed mode, should be combined with --resolution\n"
"  --fullscreen-normal             Normal fullscreen mode\n"
"  --fullscreen-borderless         Borderless fullscreen mode (always on top)\n"
#endif
"  --vsync [1/on or 0/off]         Turn VSync on or off (default is on)\n"
"  --max-vram [size]               Max VRAM to use (in mebibytes) before swapping\n"
"  --no-splash                     Don't show the splash screen during startup\n"
"  --gamelist-only                 Skip automatic game ROM search, only read from gamelist.xml\n"
"  --ignore-gamelist               Ignore the gamelist files (useful for troubleshooting)\n"
"  --show-hidden-files             Show hidden files and folders\n"
"  --show-hidden-games             Show hidden games\n"
"  --force-full                    Force the UI mode to Full\n"
"  --force-kiosk                   Force the UI mode to Kiosk\n"
"  --force-kid                     Force the UI mode to Kid\n"
"  --force-input-config            Force configuration of input device\n"
"  --home [path]                   Directory to use as home path\n"
"  --debug                         Print debug information\n"
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

loadSystemsReturnCode loadSystemConfigFile()
{
    if (SystemData::loadConfig()) {
        LOG(LogError) << "Could not parse systems configuration file (es_systems.cfg)";
        return NO_SYSTEMS_FILE;
    }

    if (SystemData::sSystemVector.size() == 0) {
        LOG(LogError) << "No game files were found, make sure that the system directories are "
                "setup correctly and that the file extensions are supported";
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
    const auto applicationStartTime = std::chrono::system_clock::now();

    std::locale::global(std::locale("C"));

    #if defined(__APPLE__)
    // This is a workaround to disable the incredibly annoying save state functionality in
    // macOS which forces a restore of the previous window state. The problem is that this
    // removes the splash screen on startup and it may have other adverse effects as well.
    std::string saveStateDir = Utils::FileSystem::expandHomePath(
                "~/Library/Saved Application State/org.es-de.EmulationStation.savedState");
    // Deletion of the state files should normally not be required as there shouldn't be any
    // files to begin with. But maybe the files can still be created for unknown reasons
    // as macOS really really loves to restore windows. Let's therefore include this deletion
    // step as an extra precaution.
    if (Utils::FileSystem::exists(saveStateDir)) {
        for (std::string stateFile : Utils::FileSystem::getDirContent(saveStateDir)) {
            Utils::FileSystem::removeFile(stateFile);
        }
    }
    else {
        Utils::FileSystem::createDirectory(saveStateDir);
    }
    // Removing the write permission from the save state directory effectively disables
    // the functionality.
    std::string chmodCommand = "chmod 500 \"" + saveStateDir + "\"";
    system(chmodCommand.c_str());
    #endif

    if (!parseArgs(argc, argv)) {
        #if defined(_WIN64)
        FreeConsole();
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
    else if (settingsNeedSaving) {
        LOG(LogInfo) << "Saving settings that were modified by the passed command line options...";
        Settings::getInstance()->saveFile();
    }

    Window window;
    SystemScreensaver screensaver(&window);
    ViewController::init(&window);
    CollectionSystemsManager::init(&window);
    window.pushGui(ViewController::get());

    bool splashScreen = Settings::getInstance()->getBool("SplashScreen");
    bool splashScreenProgress = Settings::getInstance()->getBool("SplashScreenProgress");
    SDL_Event event {};

    if (!window.init()) {
        LOG(LogError) << "Window failed to initialize";
        return 1;
    }

    InputManager::getInstance()->parseEvent(event, &window);
    if (event.type == SDL_QUIT)
        return 1;

    // This hides the mouse cursor during startup, i.e. before we have begun to capture SDL events.
    // On macOS this causes the mouse cursor to jump back to the Dock so don't do it on this OS.
    #if !defined(__APPLE__)
    SDL_SetRelativeMouseMode(SDL_TRUE);
    #endif

    AudioManager::getInstance();
    MameNames::init();

    if (splashScreen) {
        std::string progressText = "Loading...";
        if (splashScreenProgress)
            progressText = "Loading system config...";
        window.renderLoadingScreen(progressText);
    }

    loadSystemsReturnCode loadSystemsStatus = loadSystemConfigFile();

    if (loadSystemsStatus) {
        // If there was an issue with installing the es_systems.cfg file from the
        // template directory, then display an error message and let the user quit.
        // If there are no game files found, give the option to the user to quit or
        // to configure a different ROM directory as well as to generate the systems
        // directory structure.
        if (loadSystemsStatus == NO_SYSTEMS_FILE) {
            ViewController::get()->noSystemsFileDialog();
        }
        else if (loadSystemsStatus == NO_ROMS) {
            ViewController::get()->noGamesDialog();
        }
    }

    // Dont generate joystick events while we're loading.
    // (Hopefully fixes "automatically started emulator" bug.)
    SDL_JoystickEventState(SDL_DISABLE);

    // Preload what we can right away instead of waiting for the user to select it.
    // This makes for no delays when accessing content, but a longer startup time.
    ViewController::get()->preload();

    if (splashScreen && splashScreenProgress)
        window.renderLoadingScreen("Done");

    // Choose which GUI to open depending on if an input configuration already exists and
    // whether the flag to force the input configuration was passed from the command line.
    if (!loadSystemsStatus) {
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
    const auto applicationEndTime = std::chrono::system_clock::now();

    LOG(LogInfo) << "Application startup time: " <<
            std::chrono::duration_cast<std::chrono::milliseconds>
            (applicationEndTime - applicationStartTime).count() << " ms";

    bool running = true;
    // Now that we've finished loading, disable the relative mouse mode or otherwise mouse
    // input wouldn't work in any games that are launched.
    #if !defined(__APPLE__)
    SDL_SetRelativeMouseMode(SDL_FALSE);
    #endif

    while (running) {
        if (SDL_PollEvent(&event)) {
            do {
                InputManager::getInstance()->parseEvent(event, &window);

                if (event.type == SDL_QUIT)
                    running = false;
            }
            while (SDL_PollEvent(&event));
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
    CollectionSystemsManager::deinit();
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

    LOG(LogInfo) << "EmulationStation cleanly shutting down";

    #if defined(_WIN64)
    FreeConsole();
    #endif

    return 0;
}
