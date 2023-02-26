//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition (ES-DE) is a frontend for browsing
//  and launching games from your multi-platform game collection.
//
//  Originally created by Alec Lofquist.
//  Improved and extended by the RetroPie community.
//  Desktop Edition fork by Leon Styhre.
//
//  The column limit is 100 characters.
//  All ES-DE C++ source code is formatted using clang-format.
//
//  main.cpp
//
//  Main program loop. Interprets command-line arguments, checks for the
//  home folder and es_settings.xml configuration file, sets up the application
//  environment and starts listening to SDL events.
//

#include "ApplicationUpdater.h"
#include "AudioManager.h"
#include "CollectionSystemsManager.h"
#include "EmulationStation.h"
#include "InputManager.h"
#include "Log.h"
#include "MameNames.h"
#include "MediaViewer.h"
#include "Screensaver.h"
#include "Scripting.h"
#include "Settings.h"
#include "Sound.h"
#include "SystemData.h"
#include "guis/GuiDetectDevice.h"
#include "guis/GuiLaunchScreen.h"
#include "guis/GuiMsgBox.h"
#include "utils/FileSystemUtil.h"
#include "utils/PlatformUtil.h"
#include "utils/StringUtil.h"
#include "views/ViewController.h"

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_main.h>
#include <SDL2/SDL_timer.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#if defined(_WIN64)
#include <cstring>
#include <windows.h>
#endif

#include <FreeImage.h>
#include <fstream>
#include <iostream>
#include <time.h>

namespace
{
    SDL_Event event {};
    Renderer* renderer {nullptr};
    Window* window {nullptr};
    int lastTime {0};

#if defined(APPLICATION_UPDATER)
    bool noUpdateCheck {false};
#endif
    bool forceInputConfig {false};
    bool createSystemDirectories {false};
    bool settingsNeedSaving {false};
    bool portableMode {false};

    enum loadSystemsReturnCode {
        LOADING_OK,
        INVALID_FILE,
        NO_ROMS
    };

#if defined(_WIN64)
    enum win64ConsoleType {
        NO_CONSOLE,
        PARENT_CONSOLE,
        ALLOCATED_CONSOLE
    };
#endif

} // namespace

#if defined(_WIN64)
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
    HANDLE outputHandle {nullptr};
    HWND consoleWindow {nullptr};
    win64ConsoleType consoleType {NO_CONSOLE};

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
        FILE* fp {nullptr};
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
        outputToConsole(false);
#endif

    std::string portableFilePath {Utils::FileSystem::getExePath() + "/portable.txt"};

    // This is primarily intended for portable ES-DE installations on Windows (for example
    // placed on a USB memory stick) but it may be usable for other operating systems too.
    if (Utils::FileSystem::exists(portableFilePath)) {
        std::cout << "Found portable.txt in the ES-DE executable directory\n";
        std::ifstream portableFile;
        std::string homePath;
#if defined(_WIN64)
        portableFile.open(Utils::String::stringToWideString(portableFilePath).c_str());
#else
        portableFile.open(portableFilePath.c_str());
#endif
        if (!portableFile.fail()) {
            std::string relativePath;
            getline(portableFile, relativePath);
            // If the file is empty, use the ES-DE executable directory as home.
            if (relativePath == "")
                homePath = Utils::FileSystem::getExePath();
            else
                homePath = Utils::FileSystem::getExePath() + "/" + relativePath;

#if defined(_WIN64)
            homePath = Utils::String::replace(homePath, "/", "\\");
#endif
            bool homeExists {false};

            if (Utils::FileSystem::exists(homePath))
                homeExists = true;
#if defined(_WIN64)
            else if (homePath.size() == 2 && Utils::FileSystem::driveExists(homePath))
                homeExists = true;
#endif

            if (!homeExists) {
                std::cerr << "Error: Defined home path \"" << homePath << "\" does not exist\n";
            }
            else if (Utils::FileSystem::isRegularFile(homePath)) {
                std::cerr << "Error: Defined home path \"" << homePath << "\" is a file\n";
            }
            else {
                std::cout << "Setting home path to \"" << homePath << "\"\n";
                Utils::FileSystem::setHomePath(homePath);
                portableMode = true;
            }
        }
        portableFile.close();
    }

    // We need to process --home before any call to Settings::getInstance(),
    // because settings are loaded from the home path.
    for (int i {1}; i < argc; ++i) {
        if (strcmp(argv[i], "--home") == 0) {
            if (i >= argc - 1) {
                std::cerr << "Error: No home path supplied with \'--home'\n";
                return false;
            }
#if defined(_WIN64)
            if (!Utils::FileSystem::exists(argv[i + 1]) &&
                (!Utils::FileSystem::driveExists(argv[i + 1]))) {
#else
            if (!Utils::FileSystem::exists(argv[i + 1])) {
#endif
                std::cerr << "Error: Home path \'" << argv[i + 1] << "\' does not exist\n";
                return false;
            }
            if (Utils::FileSystem::isRegularFile(argv[i + 1])) {
                std::cerr << "Error: Home path \'" << argv[i + 1]
                          << "\' is a file and not a directory\n";
                return false;
            }
            Utils::FileSystem::setHomePath(argv[i + 1]);
            portableMode = false;
            break;
        }
    }

    for (int i {1}; i < argc; ++i) {
        // Skip past --home flag as we already processed it.
        if (strcmp(argv[i], "--home") == 0) {
            ++i; // Skip the argument value.
            continue;
        }
        if (strcmp(argv[i], "--display") == 0) {
            if (i >= argc - 1 || atoi(argv[i + 1]) < 1 || atoi(argv[i + 1]) > 4) {
                std::cerr << "Error: Invalid display index supplied\n";
                return false;
            }
            int DisplayIndex {atoi(argv[i + 1])};
            Settings::getInstance()->setInt("DisplayIndex", DisplayIndex);
            settingsNeedSaving = true;
            ++i;
        }
        else if (strcmp(argv[i], "--resolution") == 0) {
            if (i >= argc - 2) {
                std::cerr << "Error: Invalid resolution values supplied\n";
                return false;
            }
            std::string widthArg {argv[i + 1]};
            std::string heightArg {argv[i + 2]};
            if (widthArg.find_first_not_of("0123456789") != std::string::npos ||
                heightArg.find_first_not_of("0123456789") != std::string::npos) {
                std::cerr << "Error: Invalid resolution values supplied\n";
                return false;
            }
            int width {atoi(argv[i + 1])};
            int height {atoi(argv[i + 2])};
            if (width < 224 || height < 224 || width > 7680 || height > 7680 ||
                height < width / 4 || width < height / 2) {
                std::cerr << "Error: Unsupported resolution " << width << "x" << height
                          << " supplied\n";
                return false;
            }
            Settings::getInstance()->setInt("ScreenWidth", width);
            Settings::getInstance()->setInt("ScreenHeight", height);
            i += 2;
        }
        else if (strcmp(argv[i], "--screenoffset") == 0) {
            if (i >= argc - 2) {
                std::cerr << "Error: Invalid screenoffset values supplied\n";
                return false;
            }
            int x {atoi(argv[i + 1])};
            int y {atoi(argv[i + 2])};
            Settings::getInstance()->setInt("ScreenOffsetX", x);
            Settings::getInstance()->setInt("ScreenOffsetY", y);
            i += 2;
        }
        else if (strcmp(argv[i], "--screenrotate") == 0) {
            if (i >= argc - 1) {
                std::cerr << "Error: No screenrotate value supplied\n";
                return false;
            }
            const std::string rotateValue {argv[i + 1]};
            if (rotateValue != "0" && rotateValue != "90" && rotateValue != "180" &&
                rotateValue != "270") {
                std::cerr << "Error: Invalid screenrotate value supplied\n";
                return false;
            }
            Settings::getInstance()->setInt("ScreenRotate", atoi(argv[i + 1]));
            settingsNeedSaving = true;
            ++i;
        }
        else if (strcmp(argv[i], "--fullscreen-padding") == 0) {
            if (i >= argc - 1) {
                std::cerr << "Error: No fullscreen-padding value supplied\n";
                return false;
            }
            std::string fullscreenPaddingValue {argv[i + 1]};
            if (fullscreenPaddingValue != "on" && fullscreenPaddingValue != "off" &&
                fullscreenPaddingValue != "1" && fullscreenPaddingValue != "0") {
                std::cerr << "Error: Invalid fullscreen-padding value supplied\n";
                return false;
            }
            const bool fullscreenPadding {
                (fullscreenPaddingValue == "on" || fullscreenPaddingValue == "1") ? true : false};
            Settings::getInstance()->setBool("FullscreenPadding", fullscreenPadding);
            ++i;
        }
        else if (strcmp(argv[i], "--vsync") == 0) {
            if (i >= argc - 1) {
                std::cerr << "Error: No VSync value supplied\n";
                return false;
            }
            std::string vSyncValue {argv[i + 1]};
            if (vSyncValue != "on" && vSyncValue != "off" && vSyncValue != "1" &&
                vSyncValue != "0") {
                std::cerr << "Error: Invalid VSync value supplied\n";
                return false;
            }
            const bool vSync {(vSyncValue == "on" || vSyncValue == "1") ? true : false};
            Settings::getInstance()->setBool("VSync", vSync);
            ++i;
        }
        else if (strcmp(argv[i], "--max-vram") == 0) {
            if (i >= argc - 1) {
                std::cerr << "Error: Invalid VRAM value supplied\n";
                return false;
            }
            const int maxVRAM {atoi(argv[i + 1])};
            Settings::getInstance()->setInt("MaxVRAM", maxVRAM);
            settingsNeedSaving = true;
            ++i;
        }
#if !defined(USE_OPENGLES)
        else if (strcmp(argv[i], "--anti-aliasing") == 0) {
            bool invalidValue {false};
            int antiAlias {0};
            if (i >= argc - 1) {
                invalidValue = true;
            }
            else {
                antiAlias = atoi(argv[i + 1]);
                if (antiAlias != 0 && antiAlias != 2 && antiAlias != 4)
                    invalidValue = true;
            }
            if (invalidValue) {
                std::cerr << "Error: Invalid anti-aliasing value supplied\n";
                return false;
            }
            Settings::getInstance()->setInt("AntiAliasing", antiAlias);
            settingsNeedSaving = true;
            ++i;
        }
#endif
        else if (strcmp(argv[i], "--no-splash") == 0) {
            Settings::getInstance()->setBool("SplashScreen", false);
        }
#if defined(APPLICATION_UPDATER)
        else if (strcmp(argv[i], "--no-update-check") == 0) {
            noUpdateCheck = true;
        }
#endif
        else if (strcmp(argv[i], "--gamelist-only") == 0) {
            Settings::getInstance()->setBool("ParseGamelistOnly", true);
            settingsNeedSaving = true;
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
        else if (strcmp(argv[i], "--create-system-dirs") == 0) {
            createSystemDirectories = true;
        }
        else if (strcmp(argv[i], "--debug") == 0) {
            Settings::getInstance()->setBool("Debug", true);
            Log::setReportingLevel(LogDebug);
        }
        else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            std::cout << "EmulationStation Desktop Edition v" << PROGRAM_VERSION_STRING << " (r"
                      << PROGRAM_RELEASE_NUMBER << ")\n";
            return false;
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            std::cout <<
                // clang-format off
"Usage: emulationstation [options]\n"
"EmulationStation Desktop Edition, Emulator Frontend\n\n"
"Options:\n"
"  --display [1 to 4]                    Display/monitor to use\n"
"  --resolution [width] [height]         Application resolution\n"
"  --screenoffset [horiz.] [vert.]       Offset screen contents within application window\n"
"  --screenrotate [0, 90, 180 or 270]    Rotate screen contents within application window\n"
"  --fullscreen-padding [1/on or 0/off]  Padding if --resolution is lower than display resolution\n"
"  --vsync [1/on or 0/off]               Turn VSync on or off (default is on)\n"
"  --max-vram [size]                     Max VRAM to use (in mebibytes) before swapping\n"
#if !defined(USE_OPENGLES)
"  --anti-aliasing [0, 2 or 4]           Set MSAA anti-aliasing to disabled, 2x or 4x\n"
#endif
"  --no-splash                           Don't show the splash screen during startup\n"
#if defined(APPLICATION_UPDATER)
"  --no-update-check                     Don't check for application updates during startup\n"
#endif
"  --gamelist-only                       Skip automatic game ROM search, only read from gamelist.xml\n"
"  --ignore-gamelist                     Ignore the gamelist.xml files\n"
"  --show-hidden-files                   Show hidden files and folders\n"
"  --show-hidden-games                   Show hidden games\n"
"  --force-full                          Force the UI mode to Full\n"
"  --force-kiosk                         Force the UI mode to Kiosk\n"
"  --force-kid                           Force the UI mode to Kid\n"
"  --force-input-config                  Force configuration of input devices\n"
"  --create-system-dirs                  Create game system directories\n"
"  --home [path]                         Directory to use as home path\n"
"  --debug                               Print debug information\n"
"  --version, -v                         Display version information\n"
"  --help, -h                            Summon a sentient, angry tuba\n";
            // clang-format on
            return false; // Exit after printing help.
        }
        else {
            std::string argv_unknown = argv[i];
            std::cout << "Unknown option '" << argv_unknown << "'.\n";
            std::cout << "Try 'emulationstation --help' for more information.\n";
            return false; // Exit after printing message.
        }
    }

    if (Settings::getInstance()->getBool("IgnoreGamelist")) {
        Settings::getInstance()->setBool("ParseGamelistOnly", false);
        settingsNeedSaving = true;
    }

    return true;
}

bool checkApplicationHomeDirectory()
{
    // Check that the application home directory exists, otherwise create it.
    std::string home {Utils::FileSystem::getHomePath()};
    std::string applicationHome {home + "/.emulationstation"};
    if (!Utils::FileSystem::exists(applicationHome)) {
#if defined(_WIN64)
        std::cout << "First startup, creating application home directory \""
                  << Utils::String::replace(applicationHome, "/", "\\") << "\"\n";
#else
        std::cout << "First startup, creating application home directory \"" << applicationHome
                  << "\"\n";
#endif
        Utils::FileSystem::createDirectory(applicationHome);
        if (!Utils::FileSystem::exists(applicationHome)) {
            std::cerr << "Fatal error: Couldn't create directory, permission problems?\n";
            return false;
        }
    }

    return true;
}

loadSystemsReturnCode loadSystemConfigFile()
{
    if (SystemData::loadConfig())
        return INVALID_FILE;

    if (SystemData::sSystemVector.size() == 0) {
        LOG(LogInfo) << "No game files were found, make sure that the system directories are "
                        "setup correctly and that the file extensions are supported";
        return NO_ROMS;
    }

    return LOADING_OK;
}

void onExit()
{
    // Called on exit, assuming we get far enough to have the log initialized.
    Log::close();
}

void applicationLoop()
{
#if !defined(__EMSCRIPTEN__)
    while (true) {
#endif
        if (SDL_PollEvent(&event)) {
            do {
                InputManager::getInstance().parseEvent(event);

                if (event.type == SDL_QUIT)
#if !defined(__EMSCRIPTEN__)
                    return;
#else
                SDL_Quit();
#endif
            } while (SDL_PollEvent(&event));
        }

        int curTime = SDL_GetTicks();
        int deltaTime = curTime - lastTime;
        lastTime = curTime;

        // Cap deltaTime if it ever goes negative.
        if (deltaTime < 0)
            deltaTime = 1000;

        window->update(deltaTime);
        window->render();

        renderer->swapBuffers();
        Log::flush();
#if !defined(__EMSCRIPTEN__)
    }
#endif
}

int main(int argc, char* argv[])
{
    const auto applicationStartTime {std::chrono::system_clock::now()};

    std::locale::global(std::locale("C"));

#if defined(__APPLE__)
    // This is a workaround to disable the incredibly annoying save state functionality in
    // macOS which forces a restore of the previous window state. The problem is that this
    // removes the splash screen on startup and it may have other adverse effects as well.
    std::string saveStateDir {Utils::FileSystem::expandHomePath(
        "~/Library/Saved Application State/org.es-de.EmulationStation.savedState")};
    // Deletion of the state files should normally not be required as there shouldn't be any
    // files to begin with. But maybe the files can still be created for unknown reasons
    // as macOS really really loves to restore windows. Let's therefore include this deletion
    // step as an extra precaution.
    if (Utils::FileSystem::exists(saveStateDir)) {
        for (std::string stateFile : Utils::FileSystem::getDirContent(saveStateDir))
            Utils::FileSystem::removeFile(stateFile);
    }
    else {
        Utils::FileSystem::createDirectory(saveStateDir);
    }
    // Removing the write permission from the save state directory effectively disables
    // the functionality.
    std::string chmodCommand {"chmod 500 \"" + saveStateDir + "\""};
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

#if defined(FREEIMAGE_LIB)
    // Call this ONLY when linking with FreeImage as a static library.
    FreeImage_Initialise();
#endif

    // If ~/.emulationstation doesn't exist and cannot be created, bail.
    if (!checkApplicationHomeDirectory())
        return 1;

    // Start the logger.
    Log::init();
    Log::open();
    LOG(LogInfo) << "EmulationStation Desktop Edition v" << PROGRAM_VERSION_STRING << " (r"
                 << PROGRAM_RELEASE_NUMBER << "), built " << PROGRAM_BUILT_STRING;
    if (portableMode) {
        LOG(LogInfo) << "Running in portable mode";
        Settings::getInstance()->setBool("PortableMode", true);
    }
    else {
        Settings::getInstance()->setBool("PortableMode", false);
    }

    // Always close the log on exit.
    atexit(&onExit);

    if (createSystemDirectories) {
        if (!SystemData::createSystemDirectories() && !Settings::getInstance()->getBool("Debug"))
            std::cout << "System directories successfully created" << std::endl;
        LOG(LogInfo) << "EmulationStation cleanly shutting down";
#if defined(_WIN64)
        FreeConsole();
#endif
        return 0;
    }

    Scripting::fireEvent("startup");

#if defined(__EMSCRIPTEN__)
    // TODO: Remove when application window resizing has been implemented.
    Settings::getInstance()->setBool("Debug", true);
    Log::setReportingLevel(LogDebug);
    Settings::getInstance()->setInt("ScreenWidth", 1280);
    Settings::getInstance()->setInt("ScreenHeight", 720);
#endif

    // Check if the configuration file exists, and if not, create it.
    // This should only happen on first application startup.
    if (!Utils::FileSystem::exists(Utils::FileSystem::getHomePath() +
                                   "/.emulationstation/es_settings.xml")) {
        LOG(LogInfo) << "Settings file es_settings.xml does not exist, creating it...";
        Settings::getInstance()->saveFile();
    }
    else if (settingsNeedSaving) {
        LOG(LogInfo) << "Saving settings that were modified by command line options...";
        Settings::getInstance()->saveFile();
    }

    // Check if the application version has changed, which would normally mean that the
    // user has upgraded to a newer release.
    std::string applicationVersion;
    if ((applicationVersion = Settings::getInstance()->getString("ApplicationVersion")) !=
        PROGRAM_VERSION_STRING) {
        if (applicationVersion != "") {
            LOG(LogInfo) << "Application version changed from previous startup, from \""
                         << applicationVersion << "\" to \"" << PROGRAM_VERSION_STRING << "\"";
        }
        else {
            LOG(LogInfo) << "Application version setting is blank, changing it to \""
                         << PROGRAM_VERSION_STRING << "\"";
        }
        Settings::getInstance()->setString("ApplicationVersion", PROGRAM_VERSION_STRING);
        Settings::getInstance()->saveFile();
    }

    // Create the gamelists directory in the application home folder.
    const std::string gamelistsDir {Utils::FileSystem::getHomePath() +
                                    "/.emulationstation/gamelists"};
    if (!Utils::FileSystem::exists(gamelistsDir)) {
#if defined(_WIN64)
        LOG(LogInfo) << "Creating gamelists directory \""
                     << Utils::String::replace(gamelistsDir, "/", "\\") << "\"...";
#else
        LOG(LogInfo) << "Creating gamelists directory \"" << gamelistsDir << "\"...";
#endif
        Utils::FileSystem::createDirectory(gamelistsDir);
        if (!Utils::FileSystem::exists(gamelistsDir)) {
            LOG(LogWarning) << "Couldn't create directory, permission problems?\n";
        }
    }

    // Create the themes directory in the application home folder. This is not required but
    // is rather a convenience in case the user wants to add additional themes.
    const std::string themesDir {Utils::FileSystem::getHomePath() + "/.emulationstation/themes"};
    if (!Utils::FileSystem::exists(themesDir)) {
#if defined(_WIN64)
        LOG(LogInfo) << "Creating themes directory \""
                     << Utils::String::replace(themesDir, "/", "\\") << "\"...";
#else
        LOG(LogInfo) << "Creating themes directory \"" << themesDir << "\"...";
#endif
        Utils::FileSystem::createDirectory(themesDir);
        if (!Utils::FileSystem::exists(themesDir)) {
            LOG(LogWarning) << "Couldn't create directory, permission problems?\n";
        }
    }

    // Create the scripts directory in the application home folder. This is only required
    // for custom event scripts so it's also created as a convenience.
    const std::string scriptsDir {Utils::FileSystem::getHomePath() + "/.emulationstation/scripts"};
    if (!Utils::FileSystem::exists(scriptsDir)) {
#if defined(_WIN64)
        LOG(LogInfo) << "Creating scripts directory \""
                     << Utils::String::replace(scriptsDir, "/", "\\") << "\"...";
#else
        LOG(LogInfo) << "Creating scripts directory \"" << scriptsDir << "\"...";
#endif
        Utils::FileSystem::createDirectory(scriptsDir);
        if (!Utils::FileSystem::exists(scriptsDir)) {
            LOG(LogWarning) << "Couldn't create directory, permission problems?\n";
        }
    }

    renderer = Renderer::getInstance();
    window = Window::getInstance();

    ViewController::getInstance();
    CollectionSystemsManager::getInstance();
    Screensaver screensaver;
    MediaViewer mediaViewer;
    GuiLaunchScreen guiLaunchScreen;

    if (!window->init()) {
        LOG(LogError) << "Window failed to initialize";
        return 1;
    }

#if defined(APPLICATION_UPDATER)
    if (!noUpdateCheck)
        ApplicationUpdater::getInstance().checkForUpdates();
#endif

    window->pushGui(ViewController::getInstance());

    if (Settings::getInstance()->getBool("SplashScreen"))
        window->renderSplashScreen(Window::SplashScreenState::SCANNING, 0.0f);

    while (SDL_PollEvent(&event)) {};

#if defined(_WIN64)
    // Hide taskbar if the setting for this is enabled.
    bool taskbarStateChanged {false};
    unsigned int taskbarState {0};

    if (Settings::getInstance()->getBool("HideTaskbar")) {
        taskbarStateChanged = true;
        taskbarState = Utils::Platform::getTaskbarState();
        Utils::Platform::hideTaskbar();
    }
#endif

    AudioManager::getInstance();
    MameNames::getInstance();
    ThemeData::populateThemeSets();
    loadSystemsReturnCode loadSystemsStatus {loadSystemConfigFile()};

    if (!SystemData::sStartupExitSignal) {
        if (loadSystemsStatus) {
            // If there was an issue parsing the es_systems.xml file, display an error message.
            // If there were no game files found, give the option to the user to quit or to
            // configure a different ROM directory as well as to generate the game systems
            // directory structure.
            if (loadSystemsStatus == INVALID_FILE) {
                ViewController::getInstance()->invalidSystemsFileDialog();
            }
            else if (loadSystemsStatus == NO_ROMS) {
                ViewController::getInstance()->noGamesDialog();
            }
        }

        // Don't generate controller events while we're loading.
        SDL_GameControllerEventState(SDL_DISABLE);

        // Preload system view and all gamelist views.
        ViewController::getInstance()->preload();
    }

    if (!SystemData::sStartupExitSignal) {
        if (loadSystemsStatus == loadSystemsReturnCode::LOADING_OK)
            ThemeData::themeLoadedLogOutput();

        if (!loadSystemsStatus)
            ViewController::getInstance()->goToStart(true);

        // Check if any of the enabled systems have an invalid alternative emulator entry,
        // which means that a label is present in the gamelist.xml file which is not matching
        // any command tag in es_systems.xml.
        for (auto system : SystemData::sSystemVector) {
            if (system->getAlternativeEmulator().substr(0, 9) == "<INVALID>") {
                ViewController::getInstance()->invalidAlternativeEmulatorDialog();
                break;
            }
        }

        // Generate controller events since we're done loading.
        SDL_GameControllerEventState(SDL_ENABLE);

        lastTime = SDL_GetTicks();

#if defined(APPLICATION_UPDATER)
        if (ApplicationUpdater::getInstance().getResults())
            ViewController::getInstance()->updateAvailableDialog();
#endif

        LOG(LogInfo) << "Application startup time: "
                     << std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now() - applicationStartTime)
                            .count()
                     << " ms";

        // Open the input configuration GUI if the force flag was passed from the command line.
        if (forceInputConfig) {
            ViewController::getInstance()->cancelViewTransitions();
            window->pushGui(new GuiDetectDevice(false, true, nullptr));
        }

        // Main application loop.

        if (!SystemData::sStartupExitSignal) {
#if defined(__EMSCRIPTEN__)
            emscripten_set_main_loop(&applicationLoop, 0, 1);
#else
            applicationLoop();
#endif
        }
    }
    else {
        LOG(LogInfo) << "Exit signal received, aborting application startup";
    }

    while (window->peekGui() != ViewController::getInstance())
        delete window->peekGui();
    window->deinit();

    CollectionSystemsManager::getInstance()->deinit();
    SystemData::deleteSystems();
    NavigationSounds::getInstance().deinit();

#if defined(FREEIMAGE_LIB)
    // Call this ONLY when linking with FreeImage as a static library.
    FreeImage_DeInitialise();
#endif

#if defined(_WIN64)
    // If the taskbar state was changed (taskbar was hidden), then revert it.
    if (taskbarStateChanged)
        Utils::Platform::revertTaskbarState(taskbarState);
#endif

    Utils::Platform::processQuitMode();

    LOG(LogInfo) << "EmulationStation cleanly shutting down";

#if defined(_WIN64)
    FreeConsole();
#endif

    return 0;
}
