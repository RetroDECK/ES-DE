//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  FileSystemUtil.cpp
//
//  Low-level filesystem functions.
//  Resolve relative paths, resolve symlinks, create directories,
//  remove files etc.
//

#if defined(__ANDROID__)
#include "utils/PlatformUtilAndroid.h"
#include <SDL2/SDL_system.h>
#endif

#if defined(__APPLE__)
#define _DARWIN_USE_64_BIT_INODE
#endif

#include "utils/FileSystemUtil.h"

#include "Log.h"
#include "utils/PlatformUtil.h"
#include "utils/StringUtil.h"

#include <fstream>
#include <regex>
#include <string>
#include <sys/stat.h>

#if defined(_WIN64)
#include <Windows.h>
#include <direct.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

// For Unix systems, set the install prefix as defined via CMAKE_INSTALL_PREFIX when CMake was run.
// If not defined, the default prefix "/usr" will be used on Linux and "/usr/local" on FreeBSD.
// This fallback should not be required though unless the build environment is broken.
#if defined(__unix__)
#if defined(ES_INSTALL_PREFIX)
const std::string installPrefix {ES_INSTALL_PREFIX};
#else
#if defined(__linux__)
const std::string installPrefix {"/usr"};
#else
const std::string installPrefix {"/usr/local"};
#endif
#endif
#endif

namespace Utils
{
    namespace FileSystem
    {
        static std::string homePath;
        static std::string exePath;
        static std::string esBinary;

        StringList getDirContent(const std::string& path, const bool recursive)
        {
            const std::string& genericPath {getGenericPath(path)};
            StringList contentList;

            if (!isDirectory(genericPath))
                return contentList;

            try {
                if (recursive) {
#if defined(_WIN64)
                    for (auto& entry : std::filesystem::recursive_directory_iterator(
                             Utils::String::stringToWideString(genericPath))) {
                        contentList.emplace_back(
                            Utils::String::wideStringToString(entry.path().generic_wstring()));
#else
                    for (auto& entry : std::filesystem::recursive_directory_iterator(genericPath)) {
                        contentList.emplace_back(entry.path().generic_string());
#endif
                    }
                }
                else {
#if defined(_WIN64)
                    for (auto& entry : std::filesystem::directory_iterator(
                             Utils::String::stringToWideString(genericPath))) {
                        contentList.emplace_back(
                            Utils::String::wideStringToString(entry.path().generic_wstring()));
#else
                    for (auto& entry : std::filesystem::directory_iterator(genericPath)) {
                        contentList.emplace_back(entry.path().generic_string());
#endif
                    }
                }
            }
            catch (...) {
#if defined(_WIN64)
                LOG(LogError) << "FileSystemUtil::getDirContent(): Couldn't read directory " << "\""
                              << Utils::String::replace(path, "/", "\\")
                              << "\", permission problems?";
#else
                LOG(LogError) << "FileSystemUtil::getDirContent(): Couldn't read directory " << "\""
                              << path << "\", permission problems?";
#endif
                return contentList;
            }

            contentList.sort();
            return contentList;
        }

        StringList getMatchingFiles(const std::string& pattern)
        {
            StringList files;
            size_t entry {pattern.find('*')};

            if (entry == std::string::npos)
                return files;

            const std::string& parent {getParent(pattern)};

            // Don't allow wildcard matching for the parent directory.
            if (entry <= parent.size())
                return files;

            const StringList& dirContent {getDirContent(parent)};

            if (dirContent.size() == 0)
                return files;

            // Some characters need to be escaped in order for the regular expression to work.
            std::string escPattern {Utils::String::replace(pattern, "*", ".*")};
            escPattern = Utils::String::replace(escPattern, ")", "\\)");
            escPattern = Utils::String::replace(escPattern, "(", "\\(");
            escPattern = Utils::String::replace(escPattern, "]", "\\]");
            escPattern = Utils::String::replace(escPattern, "[", "\\[");

            std::regex expression;

            try {
                expression = escPattern;
            }
            catch (...) {
                LOG(LogError) << "FileSystemUtil::getMatchingFiles(): Invalid regular expression "
                              << "\"" << pattern << "\"";
                return files;
            }

            for (auto& entry : dirContent) {
                if (std::regex_match(entry, expression))
                    files.emplace_back(entry);
            }

            return files;
        }

        StringList getPathList(const std::string& path)
        {
            StringList pathList;
            const std::string& genericPath {getGenericPath(path)};
            size_t start {0};
            size_t end {0};

            // Split at '/'
            while ((end = genericPath.find("/", start)) != std::string::npos) {
                if (end != start)
                    pathList.emplace_back(std::string {genericPath, start, end - start});
                start = end + 1;
            }
            // Add last folder / file to pathList.
            if (start != genericPath.size())
                pathList.emplace_back(std::string {genericPath, start, genericPath.size() - start});

            return pathList;
        }

        void setHomePath(const std::string& path)
        {
            // Set home path.
            homePath = getGenericPath(path);
        }

        std::string getHomePath()
        {
            // Only construct the homepath once.
            if (homePath.length())
                return homePath;

#if defined(__ANDROID__)
            homePath = FileSystemVariables::sAppDataDirectory;
            return homePath;
#endif

#if defined(_WIN64)
            // On Windows we need to check HOMEDRIVE and HOMEPATH.
            if (!homePath.length()) {
                std::wstring envHomeDrive;
                std::wstring envHomePath;
#if defined(_MSC_VER) // MSVC compiler.
                wchar_t* buffer;
                if (!_wdupenv_s(&buffer, nullptr, L"HOMEDRIVE"))
                    envHomeDrive = buffer;
                if (!_wdupenv_s(&buffer, nullptr, L"HOMEPATH"))
                    envHomePath = buffer;
#else
                envHomeDrive = _wgetenv(L"HOMEDRIVE");
                envHomePath = _wgetenv(L"HOMEPATH");
#endif
                if (envHomeDrive.length() && envHomePath.length())
                    homePath = getGenericPath(Utils::String::wideStringToString(envHomeDrive) +
                                              "/" + Utils::String::wideStringToString(envHomePath));
            }
#else

            if (!homePath.length()) {
                std::string envHome;
                if (getenv("HOME") != nullptr)
                    envHome = getenv("HOME");
                if (envHome.length())
                    homePath = getGenericPath(envHome);
            }
#endif

            // No homepath found, fall back to current working directory.
            if (!homePath.length())
                homePath = getCWDPath();

            return homePath;
        }

        std::string getSystemHomeDirectory()
        {
#if defined(_WIN64)
            // On Windows we need to check HOMEDRIVE and HOMEPATH.
            std::wstring envHomeDrive;
            std::wstring envHomePath;
#if defined(_MSC_VER) // MSVC compiler.
            wchar_t* buffer;
            if (!_wdupenv_s(&buffer, nullptr, L"HOMEDRIVE"))
                envHomeDrive = buffer;
            if (!_wdupenv_s(&buffer, nullptr, L"HOMEPATH"))
                envHomePath = buffer;
#else
            envHomeDrive = _wgetenv(L"HOMEDRIVE");
            envHomePath = _wgetenv(L"HOMEPATH");
#endif
            if (envHomeDrive.length() && envHomePath.length())
                return getGenericPath(Utils::String::wideStringToString(envHomeDrive) + "/" +
                                      Utils::String::wideStringToString(envHomePath));
#else
            std::string envHome;
            if (getenv("HOME") != nullptr)
                envHome = getenv("HOME");
            return envHome;
#endif
            return "";
        }

        std::string getAppDataDirectory()
        {
#if defined(__ANDROID__)
            return getHomePath();
#else
            if (FileSystemVariables::sAppDataDirectory.empty()) {
#if !defined(_WIN64)
                if (getenv("ESDE_APPDATA_DIR") != nullptr) {
                    const std::string envAppDataDir {getenv("ESDE_APPDATA_DIR")};
                    FileSystemVariables::sAppDataDirectory = expandHomePath(envAppDataDir);
                }
                else if (Utils::FileSystem::exists(getHomePath() + "/ES-DE")) {
#else
                if (Utils::FileSystem::exists(getHomePath() + "/ES-DE")) {
#endif
                    FileSystemVariables::sAppDataDirectory = getHomePath() + "/ES-DE";
                }
                else if (Utils::FileSystem::exists(getHomePath() + "/.emulationstation")) {
                    FileSystemVariables::sAppDataDirectory = getHomePath() + "/.emulationstation";
                }
                else {
                    FileSystemVariables::sAppDataDirectory = getHomePath() + "/ES-DE";
                }
            }

            return FileSystemVariables::sAppDataDirectory;
#endif
        }

        std::string getInternalAppDataDirectory()
        {
#if defined(__ANDROID__)
            return AndroidVariables::sExternalDataDirectory;
#else
            return "";
#endif
        }

        std::string getCWDPath()
        {
            // Return current working directory.

#if defined(_WIN64)
            wchar_t tempWide[512];
            return (_wgetcwd(tempWide, 512) ?
                        getGenericPath(Utils::String::wideStringToString(tempWide)) :
                        "");
#else
            char temp[512];
            return (getcwd(temp, 512) ? getGenericPath(temp) : "");
#endif
        }

        std::string getPathToBinary(const std::string& executable)
        {
#if defined(_WIN64)
            return "";
#else
#if defined(FLATPAK_BUILD)
            // Ugly hack to compensate for the Flatpak sandbox restrictions. We traverse
            // this hardcoded list of paths and use the "which" command to check outside the
            // sandbox if the emulator binary exists.
            const std::string& pathVariable {
                "/var/lib/flatpak/exports/bin:/usr/bin:/usr/local/"
                "bin:/usr/local/sbin:/usr/sbin:/sbin:/bin:/usr/games:/usr/"
                "local/games:/snap/bin:/var/lib/snapd/snap/bin"};

            const std::vector<std::string>& pathList {
                Utils::String::delimitedStringToVector(pathVariable, ":")};

            // Using a temporary file is the only viable solution I've found to communicate
            // between the sandbox and the outside world.
            const std::string& tempFile {Utils::FileSystem::getAppDataDirectory() +
                                         "/.flatpak_emulator_binary_path.tmp"};

            std::string emulatorPath;

            for (auto it = pathList.cbegin(); it != pathList.cend(); ++it) {
                Utils::Platform::runSystemCommand("flatpak-spawn --host which " + *it + "/" +
                                                  executable + " > " + tempFile + " 2>/dev/null");
                std::ifstream tempFileStream;
                tempFileStream.open(tempFile, std::ios::binary);
                getline(tempFileStream, emulatorPath);
                tempFileStream.close();

                if (emulatorPath != "") {
                    emulatorPath = getParent(emulatorPath);
                    break;
                }
            }

            if (exists(tempFile))
                removeFile(tempFile);

            return emulatorPath;
#else
            std::string envPath;
            if (getenv("PATH") != nullptr)
                envPath = getenv("PATH");

            const std::vector<std::string>& pathList {
                Utils::String::delimitedStringToVector(envPath, ":")};

            std::string pathTest;

            for (auto it = pathList.cbegin(); it != pathList.cend(); ++it) {
                pathTest = it->c_str() + ("/" + executable);
                if (Utils::FileSystem::isRegularFile(pathTest) ||
                    Utils::FileSystem::isSymlink(pathTest))
                    return it->c_str();
            }
            return "";
#endif
#endif
        }

        void setExePath(const std::string& path)
        {
            constexpr int pathMax {32767};
#if defined(_WIN64)
            std::wstring result(pathMax, 0);
            if (GetModuleFileNameW(nullptr, &result[0], pathMax) != 0)
                exePath = Utils::String::wideStringToString(result);
#else
            std::string result(pathMax, 0);
            if (readlink("/proc/self/exe", &result[0], pathMax) != -1)
                exePath = result;
#endif
            exePath.erase(std::find(exePath.begin(), exePath.end(), '\0'), exePath.end());
            esBinary = exePath;
            exePath = getCanonicalPath(exePath);

#if defined(__FreeBSD__) || defined(__HAIKU__)
            // Fallback to getPathToBinary(argv[0]), needed as FreeBSD and Haiku do not
            // provide /proc/self/exe.
            if (exePath.empty()) {
                esBinary = getPathToBinary(path);
                exePath = getCanonicalPath(esBinary);
            }
#endif
            // Fallback to argv[0] if everything else fails, which is always the case on macOS.
            if (exePath.empty()) {
                esBinary = path;
                exePath = getCanonicalPath(path);
            }
            if (isRegularFile(exePath))
                exePath = getParent(exePath);

#if defined(APPIMAGE_BUILD)
            // We need to check that the APPIMAGE variable is available as the APPIMAGE_BUILD
            // build flag could have been passed without running as an actual AppImage.
            if (getenv("APPIMAGE") != nullptr)
                esBinary = getenv("APPIMAGE");
#endif
        }

        std::string getExePath()
        {
            // Return executable path.
            return exePath;
        }

        std::string getEsBinary()
        {
            // Return the absolute path to the ES-DE binary.
            return esBinary;
        }

        std::string getProgramDataPath()
        {
#if defined(__ANDROID__)
            return AndroidVariables::sInternalDataDirectory;
#elif defined(__HAIKU__)
            return "/boot/system/data/es-de";
#elif defined(__unix__)
    return installPrefix + "/share/es-de";
#else
    return "";
#endif
        }

        std::string getPreferredPath(const std::string& path)
        {
#if defined(_WIN64)
            std::string preferredPath {path};
            size_t offset {std::string::npos};
            // Convert '/' to '\\'
            while ((offset = preferredPath.find('/')) != std::string::npos)
                preferredPath.replace(offset, 1, "\\");
#else
            const std::string& preferredPath {path};
#endif
            return preferredPath;
        }

        std::string getGenericPath(const std::string& path)
        {
            std::string genericPath {path};
            size_t offset {std::string::npos};

            // Remove "\\\\?\\"
            if ((genericPath.find("\\\\?\\")) == 0)
                genericPath.erase(0, 4);

            // Convert '\\' to '/'
            while ((offset = genericPath.find('\\')) != std::string::npos)
                genericPath.replace(offset, 1, "/");

            // Remove double '/'
            while ((offset = genericPath.find("//")) != std::string::npos)
                genericPath.erase(offset, 1);

            // Remove trailing '/' when the path is more than a simple '/'
            while (genericPath.length() > 1 &&
                   ((offset = genericPath.find_last_of('/')) == (genericPath.length() - 1)))
                genericPath.erase(offset, 1);

            return genericPath;
        }

        std::string getEscapedPath(const std::string& path)
        {
            std::string escapedPath {getGenericPath(path)};

#if defined(_WIN64)
            // Windows escapes stuff by just putting everything in quotes.
            if (escapedPath.find(" ") != std::string::npos)
                return '"' + getPreferredPath(escapedPath) + '"';
            else
                return getPreferredPath(escapedPath);
#else
            // Insert a backslash before most characters that would mess up a bash path.
            const char* invalidChars {"\\ '\"!$^&*(){}[]?;<>"};
            const char* invalidChar {invalidChars};

            while (*invalidChar) {
                size_t start {0};
                size_t offset {0};

                while ((offset = escapedPath.find(*invalidChar, start)) != std::string::npos) {
                    start = offset + 1;

                    if ((offset == 0) || (escapedPath[offset - 1] != '\\')) {
                        escapedPath.insert(offset, 1, '\\');
                        ++start;
                    }
                }
                ++invalidChar;
            }
            return escapedPath;
#endif
        }

        std::string getCanonicalPath(const std::string& path)
        {
            // Hack for builtin resources.
            if ((path[0] == ':') && (path[1] == '/'))
                return path;

            std::string canonicalPath {exists(path) ? getAbsolutePath(path) : getGenericPath(path)};

            // Cleanup path.
            bool scan {true};
            while (scan) {
                const StringList& pathList {getPathList(canonicalPath)};

                canonicalPath.clear();
                scan = false;

                for (StringList::const_iterator it {pathList.cbegin()}; it != pathList.cend();
                     ++it) {
                    // Ignore empty.
                    if ((*it).empty())
                        continue;

                    // Remove "/./"
                    if ((*it) == ".")
                        continue;

                    // Resolve "/../"
                    if ((*it) == "..") {
                        canonicalPath = getParent(canonicalPath);
                        continue;
                    }

#if defined(_WIN64)
                    // Append folder to path.
                    canonicalPath += (canonicalPath.size() == 0) ? (*it) : ("/" + (*it));
#else
                    // Append folder to path.
                    canonicalPath += ("/" + (*it));
#endif

                    if (isSymlink(canonicalPath)) {
                        const std::string& resolved {resolveSymlink(canonicalPath)};

                        if (resolved.empty())
                            return "";

                        if (isAbsolute(resolved))
                            canonicalPath = resolved;
                        else
                            canonicalPath = getParent(canonicalPath) + "/" + resolved;

                        for (++it; it != pathList.cend(); ++it)
                            canonicalPath += (canonicalPath.size() == 0) ? (*it) : ("/" + (*it));

                        scan = true;
                        break;
                    }
                }
            }
            return canonicalPath;
        }

        std::string getAbsolutePath(const std::string& path, const std::string& base)
        {
            const std::string& absolutePath {getGenericPath(path)};
            const std::string& baseVar {isAbsolute(base) ? getGenericPath(base) :
                                                           getAbsolutePath(base)};

            return isAbsolute(absolutePath) ? absolutePath :
                                              getGenericPath(baseVar + "/" + absolutePath);
        }

        std::string getParent(const std::string& path)
        {
            std::string genericPath {getGenericPath(path)};
            size_t offset {std::string::npos};

            // Find last '/' and erase it.
            if ((offset = genericPath.find_last_of('/')) != std::string::npos)
                return genericPath.erase(offset);

            // No parent found.
            return genericPath;
        }

        std::string getFileName(const std::string& path)
        {
            const std::string& genericPath {getGenericPath(path)};
            size_t offset {std::string::npos};

            // Find last '/' and return the filename.
            if ((offset = genericPath.find_last_of('/')) != std::string::npos)
                return ((genericPath[offset + 1] == 0) ? "." :
                                                         std::string {genericPath, offset + 1});

            // No '/' found, entire path is a filename.
            return genericPath;
        }

        std::string getStem(const std::string& path)
        {
            std::string fileName {getFileName(path)};
            size_t offset {std::string::npos};

            // Empty fileName.
            if (fileName == ".")
                return fileName;

            if (!Utils::FileSystem::isDirectory(path)) {
                // Find last '.' and erase the extension.
                if ((offset = fileName.find_last_of('.')) != std::string::npos)
                    return fileName.erase(offset);
            }

            // No '.' found, filename has no extension.
            return fileName;
        }

        std::string getExtension(const std::string& path)
        {
            const std::string& fileName {getFileName(path)};
            size_t offset {std::string::npos};

            // Empty fileName.
            if (fileName == ".")
                return fileName;

            // Find last '.' and return the extension.
            if ((offset = fileName.find_last_of('.')) != std::string::npos)
                return std::string {fileName, offset};

            // No '.' found, filename has no extension.
            return ".";
        }

        long getFileSize(const std::filesystem::path& path)
        {
            try {
#if defined(_WIN64)
                return static_cast<long>(std::filesystem::file_size(
                    Utils::String::stringToWideString(path.generic_string())));
#else
                return static_cast<long>(std::filesystem::file_size(path));
#endif
            }
            catch (std::filesystem::filesystem_error& error) {
                LOG(LogError) << "FileSystemUtil::getFileSize(): " << error.what();
                return -1;
            }
        }

        std::string expandHomePath(const std::string& path)
        {
            // Expand home path if ~ is used.
            return Utils::String::replace(path, "~", Utils::FileSystem::getHomePath());
        }

        std::string resolveRelativePath(const std::string& path,
                                        const std::string& relativeTo,
                                        const bool allowHome)
        {
            const std::string& genericPath {getGenericPath(path)};
            const std::string& relativeToVar {isDirectory(relativeTo) ? getGenericPath(relativeTo) :
                                                                        getParent(relativeTo)};

            // Nothing to resolve.
            if (!genericPath.length())
                return genericPath;

            // Replace '.' with relativeToVar.
            if ((genericPath[0] == '.') && (genericPath[1] == '/'))
                return (relativeToVar + &(genericPath[1]));

            // Replace '~' with homePath.
            if (allowHome && (genericPath[0] == '~') && (genericPath[1] == '/'))
                return (getHomePath() + &(genericPath[1]));

            // Nothing to resolve.
            return genericPath;
        }

        std::string createRelativePath(const std::string& path,
                                       const std::string& relativeTo,
                                       const bool allowHome)
        {
            bool contains {false};
            std::string relativePath {removeCommonPath(path, relativeTo, contains)};

            if (contains)
                return ("./" + relativePath);

            if (allowHome) {
                relativePath = removeCommonPath(path, getHomePath(), contains);

                if (contains)
                    return ("~/" + relativePath);
            }
            return relativePath;
        }

        std::string removeCommonPath(const std::string& path,
                                     const std::string& commonArg,
                                     bool& contains)
        {
            const std::string& genericPath {getGenericPath(path)};
            const std::string& common {isDirectory(commonArg) ? getGenericPath(commonArg) :
                                                                getParent(commonArg)};

            if (genericPath.find(common) == 0) {
                contains = true;
                return genericPath.substr(common.length() + 1);
            }

            contains = false;
            return genericPath;
        }

        std::string resolveSymlink(const std::string& path)
        {
            const std::string& genericPath {getGenericPath(path)};
            std::string resolved;

#if defined(_WIN64)
            std::wstring resolvedW;
            HANDLE hFile = CreateFileW(Utils::String::stringToWideString(genericPath).c_str(),
                                       FILE_READ_ATTRIBUTES, FILE_SHARE_READ, 0, OPEN_EXISTING,
                                       FILE_FLAG_BACKUP_SEMANTICS, 0);
            if (hFile != INVALID_HANDLE_VALUE) {
                resolvedW.resize(
                    GetFinalPathNameByHandleW(hFile, nullptr, 0, FILE_NAME_NORMALIZED) + 1);
                if (GetFinalPathNameByHandleW(hFile, const_cast<LPWSTR>(resolvedW.data()),
                                              static_cast<DWORD>(resolvedW.size()),
                                              FILE_NAME_NORMALIZED) > 0) {
                    resolvedW.resize(resolvedW.size() - 2);
                    resolved = getGenericPath(Utils::String::wideStringToString(resolvedW));
                }
                CloseHandle(hFile);
            }
#else
            struct stat info;

            // Check if lstat succeeded.
            if (lstat(genericPath.c_str(), &info) == 0) {
                resolved.resize(info.st_size);
                if (readlink(genericPath.c_str(), const_cast<char*>(resolved.data()),
                             resolved.size()) > 0)
                    resolved = getGenericPath(resolved);
            }
#endif
            return resolved;
        }

        bool copyFile(const std::string& sourcePath,
                      const std::string& destinationPath,
                      bool overwrite)
        {
            if (!exists(sourcePath)) {
                LOG(LogError) << "Can't copy file, source file does not exist:";
                LOG(LogError) << sourcePath;
                return true;
            }

            if (isDirectory(destinationPath)) {
                LOG(LogError) << "Destination file is actually a directory:";
                LOG(LogError) << destinationPath;
                return true;
            }

            if (!overwrite && exists(destinationPath)) {
                LOG(LogError) << "Destination file exists and the overwrite flag "
                                 "has not been set";
                return true;
            }

#if defined(_WIN64)
            std::ifstream sourceFile {Utils::String::stringToWideString(sourcePath).c_str(),
                                      std::ios::binary};
#else
            std::ifstream sourceFile {sourcePath, std::ios::binary};
#endif

            if (sourceFile.fail()) {
                LOG(LogError) << "Couldn't read from source file \"" << sourcePath
                              << "\", permission problems?";
                sourceFile.close();
                return true;
            }

#if defined(_WIN64)
            std::ofstream targetFile {Utils::String::stringToWideString(destinationPath).c_str(),
                                      std::ios::binary};
#else
            std::ofstream targetFile {destinationPath, std::ios::binary};
#endif

            if (targetFile.fail()) {
                LOG(LogError) << "Couldn't write to target file \"" << destinationPath
                              << "\", permission problems?";
                targetFile.close();
                return true;
            }

            targetFile << sourceFile.rdbuf();

            sourceFile.close();
            targetFile.close();

            return false;
        }

        bool renameFile(const std::string& sourcePath,
                        const std::string& destinationPath,
                        bool overwrite)
        {
            // Don't print any error message for a missing source file as Log will use this
            // function when initializing the logging. It would always generate an error in
            // case it's the first application start (as an old log file would then not exist).
            if (!exists(sourcePath)) {
                return true;
            }

            if (isDirectory(destinationPath)) {
                LOG(LogError) << "Destination file is actually a directory:";
                LOG(LogError) << destinationPath;
                return true;
            }

            if (!overwrite && exists(destinationPath)) {
                LOG(LogError) << "Destination file exists and the overwrite flag has not been set";
                return true;
            }

#if defined(_WIN64)
            return _wrename(Utils::String::stringToWideString(sourcePath).c_str(),
                            Utils::String::stringToWideString(destinationPath).c_str());
#else
            return std::rename(sourcePath.c_str(), destinationPath.c_str());
#endif
        }

        bool createEmptyFile(const std::filesystem::path& path)
        {
            const std::filesystem::path cleanPath {path.lexically_normal().make_preferred()};
            if (exists(path)) {
                LOG(LogError) << "Couldn't create target file \"" << cleanPath.string()
                              << "\" as it already exists";
                return false;
            }
#if defined(_WIN64)
            std::ofstream targetFile {Utils::String::stringToWideString(cleanPath.string()).c_str(),
                                      std::ios::binary};
#else
            std::ofstream targetFile {cleanPath, std::ios::binary};
#endif
            if (targetFile.fail()) {
                LOG(LogError) << "Couldn't create target file \"" << cleanPath.string()
                              << "\", permission problems?";
                targetFile.close();
                return false;
            }

            targetFile.close();
            return true;
        }

        bool removeFile(const std::string& path)
        {
            const std::string& genericPath {getGenericPath(path)};
            try {
#if defined(_WIN64)
                return std::filesystem::remove(Utils::String::stringToWideString(genericPath));
#else
                return std::filesystem::remove(genericPath);
#endif
            }
            catch (std::filesystem::filesystem_error& error) {
                LOG(LogError) << "FileSystemUtil::removeFile(): " << error.what();
                return false;
            }
        }

        bool removeDirectory(const std::string& path, bool recursive)
        {
            const std::string& genericPath {getGenericPath(path)};
            try {
#if defined(_WIN64)
                if (recursive)
                    return std::filesystem::remove_all(
                        Utils::String::stringToWideString(genericPath));
                else
                    return std::filesystem::remove(Utils::String::stringToWideString(genericPath));
#else
                if (recursive)
                    return std::filesystem::remove_all(genericPath);
                else
                    return std::filesystem::remove(genericPath);
#endif
            }
            catch (std::filesystem::filesystem_error& error) {
                LOG(LogError) << "FileSystemUtil::removeDirectory(): " << error.what();
                return false;
            }
        }

        bool createDirectory(const std::string& path)
        {
            const std::string& genericPath {getGenericPath(path)};

            if (exists(genericPath))
                return true;

#if defined(_WIN64)
            if (_wmkdir(Utils::String::stringToWideString(genericPath).c_str()) == 0)
                return true;
#else
            if (mkdir(genericPath.c_str(), 0755) == 0)
                return true;
#endif

            // Failed to create directory, try to create the parent.
            const std::string& parent {getParent(genericPath)};

            // Only try to create parent if it's not identical to genericPath.
            if (parent != genericPath)
                createDirectory(parent);

                // Try to create directory again now that the parent should exist.

#if defined(_WIN64)
            return (_wmkdir(Utils::String::stringToWideString(genericPath).c_str()) == 0);
#else
            return (mkdir(genericPath.c_str(), 0755) == 0);
#endif
        }

        bool exists(const std::string& path)
        {
            const std::string& genericPath {getGenericPath(path)};
            try {
#if defined(_WIN64)
                return std::filesystem::exists(Utils::String::stringToWideString(genericPath));
#else
                return std::filesystem::exists(genericPath);
#endif
            }
            catch (std::filesystem::filesystem_error& error) {
                LOG(LogError) << "FileSystemUtil::exists(): " << error.what();
                return false;
            }
        }

        bool driveExists(const std::string& path)
        {
#if defined(_WIN64)
            std::string genericPath {getGenericPath(path)};
            // Try to add a dot or a backslash and a dot depending on how the drive
            // letter was defined by the user.
            if (genericPath.length() == 2 && genericPath.at(1) == ':')
                genericPath += "\\.";
            else if (genericPath.length() == 3 && genericPath.at(1) == ':')
                genericPath += ".";

            return exists(genericPath);
#else
            return false;
#endif
        }

        bool isAbsolute(const std::string& path)
        {
            const std::string& genericPath {getGenericPath(path)};
            try {
#if defined(_WIN64)
                return ((genericPath.size() > 1) && (genericPath[1] == ':'));
#else
                return ((genericPath.size() > 0) && (genericPath[0] == '/'));
#endif
            }
            catch (...) {
                return false;
            }
        }

        bool isRegularFile(const std::string& path)
        {
            const std::string& genericPath {getGenericPath(path)};
            try {
#if defined(_WIN64)
                return std::filesystem::is_regular_file(
                    Utils::String::stringToWideString(genericPath));
#else
                return std::filesystem::is_regular_file(genericPath);
#endif
            }
            catch (std::filesystem::filesystem_error& error) {
                LOG(LogError) << "FileSystemUtil::isRegularFile(): " << error.what();
                return false;
            }
        }

        bool isDirectory(const std::string& path)
        {
            const std::string& genericPath {getGenericPath(path)};
            try {
#if defined(_WIN64)
                return std::filesystem::is_directory(
                    Utils::String::stringToWideString(genericPath));
#else
                return std::filesystem::is_directory(genericPath);
#endif
            }
            catch (std::filesystem::filesystem_error& error) {
                LOG(LogError) << "FileSystemUtil::isDirectory(): " << error.what();
                return false;
            }
        }

        bool isSymlink(const std::string& path)
        {
#if defined(__ANDROID__)
            // Symlinks are generally not supported on Android due to the Storage Access Framework
            // and the use of FAT/exFAT and NTFS filesystems.
            return false;
#endif
            const std::string& genericPath {getGenericPath(path)};
            try {
#if defined(_WIN64)
                return std::filesystem::is_symlink(Utils::String::stringToWideString(genericPath));
#else
                return std::filesystem::is_symlink(genericPath);
#endif
            }
            catch (std::filesystem::filesystem_error& error) {
                LOG(LogError) << "FileSystemUtil::isSymlink(): " << error.what();
                return false;
            }
        }

        bool isHidden(const std::string& path)
        {
            const std::string& genericPath {getGenericPath(path)};
#if defined(_WIN64)
            // Check for hidden attribute.
            const DWORD Attributes {
                GetFileAttributesW(Utils::String::stringToWideString(genericPath).c_str())};
            if ((Attributes != INVALID_FILE_ATTRIBUTES) && (Attributes & FILE_ATTRIBUTE_HIDDEN))
                return true;
#endif

            // Filenames starting with . are hidden in Linux, but
            // we do this check for windows as well.
            if (getFileName(genericPath)[0] == '.')
                return true;

            return false;
        }

    } // namespace FileSystem

} // namespace Utils
