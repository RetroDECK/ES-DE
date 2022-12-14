//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  FileSystemUtil.cpp
//
//  Low-level filesystem functions.
//  Resolve relative paths, resolve symlinks, create directories,
//  remove files etc.
//

#if !defined(__APPLE__) && !defined(__FreeBSD__) && !defined(__OpenBSD__) &&                       \
    !defined(__NetBSD__) && !defined(__EMSCRIPTEN__)
#define _FILE_OFFSET_BITS 64
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
#if defined(_MSC_VER) // MSVC compiler.
#define stat64 _stat64
#define S_ISREG(x) (((x)&S_IFMT) == S_IFREG)
#define S_ISDIR(x) (((x)&S_IFMT) == S_IFDIR)
#endif
#else
#include <dirent.h>
#include <unistd.h>
#endif

// For Unix systems, set the install prefix as defined via CMAKE_INSTALL_PREFIX when CMake was run.
// If not defined, the default prefix '/usr' will be used on Linux, '/usr/pkg' on NetBSD and
// '/usr/local' on FreeBSD and OpenBSD. This fallback should not be required though unless the
// build environment is broken.
#if defined(__unix__)
#if defined(ES_INSTALL_PREFIX)
const std::string installPrefix {ES_INSTALL_PREFIX};
#else
#if defined(__linux__)
const std::string installPrefix {"/usr"};
#elif defined(__NetBSD__)
const std::string installPrefix {"/usr/pkg"};
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

        StringList getDirContent(const std::string& path, const bool recursive)
        {
            const std::string& genericPath {getGenericPath(path)};
            StringList contentList;

            // Only parse the directory, if it's a directory.
            if (isDirectory(genericPath)) {

#if defined(_WIN64)
                WIN32_FIND_DATAW findData;
                const std::wstring& wildcard {Utils::String::stringToWideString(genericPath) +
                                              L"/*"};
                const HANDLE hFind {FindFirstFileW(wildcard.c_str(), &findData)};

                if (hFind != INVALID_HANDLE_VALUE) {
                    // Loop over all files in the directory.
                    do {
                        const std::string& name {
                            Utils::String::wideStringToString(findData.cFileName)};
                        // Ignore "." and ".."
                        if ((name != ".") && (name != "..")) {
                            const std::string& fullName {getGenericPath(genericPath + "/" + name)};
                            contentList.emplace_back(fullName);

                            if (recursive && isDirectory(fullName)) {
                                contentList.sort();
                                contentList.merge(getDirContent(fullName, true));
                            }
                        }
                    } while (FindNextFileW(hFind, &findData));
                    FindClose(hFind);
                }
#else
                DIR* dir {opendir(genericPath.c_str())};

                if (dir != nullptr) {
                    struct dirent* entry;
                    // Loop over all files in the directory.
                    while ((entry = readdir(dir)) != nullptr) {
                        const std::string& name(entry->d_name);

                        // Ignore "." and ".."
                        if ((name != ".") && (name != "..")) {
                            const std::string& fullName {getGenericPath(genericPath + "/" + name)};
                            contentList.emplace_back(fullName);

                            if (recursive && isDirectory(fullName)) {
                                contentList.sort();
                                contentList.merge(getDirContent(fullName, true));
                            }
                        }
                    }
                    closedir(dir);
                }
#endif
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
                const std::string& envHome {getenv("HOME")};
                if (envHome.length())
                    homePath = getGenericPath(envHome);
            }
#endif

            // No homepath found, fall back to current working directory.
            if (!homePath.length())
                homePath = getCWDPath();

            return homePath;
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
            const std::string& tempFile {Utils::FileSystem::getHomePath() + "/.emulationstation/" +
                                         ".flatpak_emulator_binary_path.tmp"};

            std::string emulatorPath;

            for (auto it = pathList.cbegin(); it != pathList.cend(); ++it) {
                Utils::Platform::runSystemCommand("flatpak-spawn --host which " + *it + "/" +
                                                  executable + " > " + tempFile + " 2>/dev/null");
                std::ifstream tempFileStream;
                tempFileStream.open(tempFile);
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
            const std::string& pathVariable {std::string {getenv("PATH")}};

            const std::vector<std::string>& pathList {
                Utils::String::delimitedStringToVector(pathVariable, ":")};

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
            exePath = getCanonicalPath(exePath);

            // Fallback to argv[0] if everything else fails.
            if (exePath.empty())
                exePath = getCanonicalPath(path);
            if (isRegularFile(exePath))
                exePath = getParent(exePath);
        }

        std::string getExePath()
        {
            // Return executable path.
            return exePath;
        }

        std::string getProgramDataPath()
        {
#if defined(__unix__)
            return installPrefix + "/share/emulationstation";
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
            _wrename(Utils::String::stringToWideString(sourcePath).c_str(),
                     Utils::String::stringToWideString(destinationPath).c_str());
#else
            std::rename(sourcePath.c_str(), destinationPath.c_str());
#endif

            return false;
        }

        bool removeFile(const std::string& path)
        {
            const std::string& genericPath {getGenericPath(path)};

            // Don't remove if it doesn't exists.
            if (!exists(genericPath))
                return true;

#if defined(_WIN64)
            if (_wunlink(Utils::String::stringToWideString(genericPath).c_str()) != 0) {
                LOG(LogError) << "Couldn't delete file, permission problems?";
                LOG(LogError) << genericPath;
                return true;
            }
            else {
                return false;
            }
#else
            if (unlink(genericPath.c_str()) != 0) {
                LOG(LogError) << "Couldn't delete file, permission problems?";
                LOG(LogError) << genericPath;
                return true;
            }
            else {
                return false;
            }
            return (unlink(genericPath.c_str()) == 0);
#endif
        }

        bool removeDirectory(const std::string& path)
        {
            if (getDirContent(path).size() != 0) {
                LOG(LogError) << "Couldn't delete directory as it's not empty";
                LOG(LogError) << path;
                return false;
            }
            if (isSymlink(path)) {
                LOG(LogError) << "Couldn't delete directory as it's actually a symlink";
                LOG(LogError) << path;
                return false;
            }
#if defined(_WIN64)
            if (_wrmdir(Utils::String::stringToWideString(path).c_str()) != 0) {
#else
            if (rmdir(path.c_str()) != 0) {
#endif
                LOG(LogError) << "Couldn't delete directory, permission problems?";
                LOG(LogError) << path;
                return false;
            }
            return true;
        } // namespace FileSystem

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

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
            struct stat info;
            return (stat(genericPath.c_str(), &info) == 0);
#elif defined(_WIN64)
            struct _stat64 info;
            return (_wstat64(Utils::String::stringToWideString(genericPath).c_str(), &info) == 0);
#else
        struct stat64 info;
        return (stat64(genericPath.c_str(), &info) == 0);
#endif
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

            struct _stat64 info;
            return (_wstat64(Utils::String::stringToWideString(genericPath).c_str(), &info) == 0);

#else
            return false;
#endif
        }

        bool isAbsolute(const std::string& path)
        {
            const std::string& genericPath {getGenericPath(path)};

#if defined(_WIN64)
            return ((genericPath.size() > 1) && (genericPath[1] == ':'));
#else
            return ((genericPath.size() > 0) && (genericPath[0] == '/'));
#endif
        }

        bool isRegularFile(const std::string& path)
        {
            const std::string& genericPath {getGenericPath(path)};

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
            struct stat info;
            if (stat(genericPath.c_str(), &info) != 0)
                return false;
#elif defined(_WIN64)
            struct stat64 info;
            if (_wstat64(Utils::String::stringToWideString(genericPath).c_str(), &info) != 0)
                return false;
#else
        struct stat64 info;
        if (stat64(genericPath.c_str(), &info) != 0)
            return false;
#endif

            // Check for S_IFREG attribute.
            return (S_ISREG(info.st_mode));
        }

        bool isDirectory(const std::string& path)
        {
            const std::string& genericPath {getGenericPath(path)};

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
            struct stat info;
            if (stat(genericPath.c_str(), &info) != 0)
                return false;
#elif defined(_WIN64)
            struct stat64 info;
            if (_wstat64(Utils::String::stringToWideString(genericPath).c_str(), &info) != 0)
                return false;
#else
        struct stat64 info;
        if (stat64(genericPath.c_str(), &info) != 0)
            return false;
#endif

            // Check for S_IFDIR attribute.
            return (S_ISDIR(info.st_mode));
        }

        bool isSymlink(const std::string& path)
        {
            const std::string& genericPath {getGenericPath(path)};

#if defined(_WIN64)
            // Check for symlink attribute.
            const DWORD Attributes {
                GetFileAttributesW(Utils::String::stringToWideString(genericPath).c_str())};
            if ((Attributes != INVALID_FILE_ATTRIBUTES) &&
                (Attributes & FILE_ATTRIBUTE_REPARSE_POINT))
                return true;
#else

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
            struct stat info;

            if (lstat(genericPath.c_str(), &info) != 0)
                return false;
#else
            struct stat64 info;

            if (lstat64(genericPath.c_str(), &info) != 0)
                return false;
#endif

            // Check for S_IFLNK attribute.
            return (S_ISLNK(info.st_mode));
#endif

            // Not a symlink.
            return false;
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
