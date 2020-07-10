//
//  FileSystemUtil.cpp
//
//  Low-level filesystem functions.
//  Resolve relative paths, resolve symlinks, create directories,
//  remove files etc.
//

#define _FILE_OFFSET_BITS 64

#include "utils/FileSystemUtil.h"

#include "utils/StringUtil.h"
#include "Log.h"

#include <sys/stat.h>
#include <fstream>
#include <string.h>

#if defined(_WIN64)
#include <direct.h>
#include <Windows.h>
//#define S_ISREG(x) (((x) & S_IFMT) == S_IFREG)
//#define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#else
#include <dirent.h>
#include <unistd.h>
#endif

// For Unix systems, try to get the install prefix as defined when CMake was run.
// The installPrefix directory is the value set for CMAKE_INSTALL_PREFIX during build.
// If not defined, the default prefix path '/usr/local' will be used.
#if defined(__unix__)
#ifdef ES_INSTALL_PREFIX
std::string installPrefix = ES_INSTALL_PREFIX;
#else
std::string installPrefix = "/usr/local";
#endif
#endif

namespace Utils
{
    namespace FileSystem
    {
        static std::string homePath = "";
        static std::string exePath = "";

        stringList getDirContent(const std::string& _path, const bool _recursive)
        {
            std::string path = getGenericPath(_path);
            stringList contentList;

            // Only parse the directory, if it's a directory.
            if (isDirectory(path)) {

                #if defined(_WIN64)
                WIN32_FIND_DATAW findData;
                std::wstring wildcard = Utils::String::stringToWideString(path) + L"/*";
                HANDLE hFind = FindFirstFileW(wildcard.c_str(), &findData);

                if (hFind != INVALID_HANDLE_VALUE) {
                    // Loop over all files in the directory.
                    do {
                        std::string name = Utils::String::wideStringToString(findData.cFileName);
                        // Ignore "." and ".."
                        if ((name != ".") && (name != "..")) {
                            std::string fullName(getGenericPath(path + "/" + name));
                            contentList.push_back(fullName);

                            if (_recursive && isDirectory(fullName))
                                contentList.merge(getDirContent(fullName, true));
                        }
                    }
                    while (FindNextFileW(hFind, &findData));
                    FindClose(hFind);
                }
                #else
                DIR* dir = opendir(path.c_str());

                if (dir != nullptr) {
                    struct dirent* entry;
                    // Loop over all files in the directory.
                    while ((entry = readdir(dir)) != nullptr) {
                        std::string name(entry->d_name);

                        // Ignore "." and ".."
                        if ((name != ".") && (name != "..")) {
                            std::string fullName(getGenericPath(path + "/" + name));
                            contentList.push_back(fullName);

                            if (_recursive && isDirectory(fullName))
                                contentList.merge(getDirContent(fullName, true));
                        }
                    }
                    closedir(dir);
                }
                #endif
            }
            contentList.sort();
            return contentList;
        }

        stringList getPathList(const std::string& _path)
        {
            stringList pathList;
            std::string path = getGenericPath(_path);
            size_t start = 0;
            size_t end = 0;

            // Split at '/'
            while ((end = path.find("/", start)) != std::string::npos) {
                if (end != start)
                    pathList.push_back(std::string(path, start, end - start));
                start = end + 1;
            }
            // Add last folder / file to pathList.
            if (start != path.size())
                pathList.push_back(std::string(path, start, path.size() - start));

            return pathList;
        }

        void setHomePath(const std::string& _path)
        {
            homePath = getGenericPath(_path);
        }

        std::string getHomePath()
        {
            // Only construct the homepath once.
            if (homePath.length())
                return homePath;

            #if defined(_WIN64)
            // On Windows we need to check HOMEDRIVE and HOMEPATH.
            if (!homePath.length()) {
                #ifdef _WIN64
                std::string envHomeDrive =
                        Utils::String::wideStringToString(_wgetenv(L"HOMEDRIVE"));
                std::string envHomePath  =
                        Utils::String::wideStringToString(_wgetenv(L"HOMEPATH"));
                #else
                std::string envHomeDrive = getenv("HOMEDRIVE");
                std::string envHomePath  = getenv("HOMEPATH");
                #endif
                if (envHomeDrive.length() && envHomePath.length())
                    homePath = getGenericPath(envHomeDrive + "/" + envHomePath);
            }
            #else
            // Check for HOME environment variable.
            if (!homePath.length()) {
                std::string envHome = getenv("HOME");
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
            char temp[512];

            // Return current working directory.
            #ifdef _WIN64
            wchar_t tempWide[512];
            return (_wgetcwd(tempWide, 512) ?
                    getGenericPath(Utils::String::wideStringToString(tempWide)) : "");
            #else
            return (getcwd(temp, 512) ? getGenericPath(temp) : "");
            #endif
        }

        std::string getPathToBinary(const std::string& executable)
        {
            #ifdef _WIN64
            return "";
            #else
            std::string pathVariable = std::string(getenv("PATH"));
            std::vector<std::string> pathList =
                    Utils::String::delimitedStringToVector(pathVariable, ":");

            std::string pathTest;

            for (auto it = pathList.cbegin(); it != pathList.cend(); it++) {
                pathTest = it->c_str() + ("/" + executable);
                if (exists(pathTest))
                    return it->c_str();
            }
            return "";
            #endif
        }

        void setExePath(const std::string& _path)
        {
            constexpr int path_max = 32767;
            #if defined(_WIN64)
            std::wstring result(path_max, 0);
            if (GetModuleFileNameW(nullptr, &result[0], path_max) != 0)
                exePath = Utils::String::wideStringToString(result);
            #else
            std::string result(path_max, 0);
            if (readlink("/proc/self/exe", &result[0], path_max) != -1)
                exePath = result;
            #endif
            exePath = getCanonicalPath(exePath);

            // Fallback to argv[0] if everything else fails.
            if (exePath.empty())
                exePath = getCanonicalPath(_path);
            if (isRegularFile(exePath))
                exePath = getParent(exePath);
        }

        std::string getExePath()
        {
            return exePath;
        }

        std::string getProgramDataPath()
        {
            // For Unix systems, installPrefix should be populated by CMAKE from
            // $CMAKE_INSTALL_PREFIX. But just as a precaution, let's check if this
            // variable is blank, and if so set it to '/usr/local'.
            // Just in case some build environments won't handle this correctly.
            // For Windows it doesn't really work like that and the application could have
            // been install to an arbitrary location, so this function won't be used on that OS.
            #ifdef __unix__
            if (!installPrefix.length())
                installPrefix = "/usr/local";
            return installPrefix + "/share/emulationstation";
            #else
            return "";
            #endif
        }

        std::string getPreferredPath(const std::string& _path)
        {
            std::string path = _path;
            size_t offset = std::string::npos;
            #if defined(_WIN64)
            // Convert '/' to '\\'
            while ((offset = path.find('/')) != std::string::npos)
                path.replace(offset, 1, "\\");
            #endif
            return path;
        }

        std::string getGenericPath(const std::string& _path)
        {
            std::string path = _path;
            size_t offset = std::string::npos;

            // Remove "\\\\?\\"
            if ((path.find("\\\\?\\")) == 0)
                path.erase(0, 4);

            // Convert '\\' to '/'
            while ((offset = path.find('\\')) != std::string::npos)
                path.replace(offset, 1 ,"/");

            // Remove double '/'
            while ((offset = path.find("//")) != std::string::npos)
                path.erase(offset, 1);

            // Remove trailing '/' when the path is more than a simple '/'
            while (path.length() > 1 && ((offset = path.find_last_of('/')) == (path.length() - 1)))
                path.erase(offset, 1);

            return path;
        }

        std::string getEscapedPath(const std::string& _path)
        {
            std::string path = getGenericPath(_path);

            #if defined(_WIN64)
            // Windows escapes stuff by just putting everything in quotes.
            return '"' + getPreferredPath(path) + '"';
            #else
            // Insert a backslash before most characters that would mess up a bash path.
            const char* invalidChars = "\\ '\"!$^&*(){}[]?;<>";
            const char* invalidChar = invalidChars;

            while (*invalidChar) {
                size_t start = 0;
                size_t offset = 0;

                while ((offset = path.find(*invalidChar, start)) != std::string::npos) {
                    start = offset + 1;

                    if ((offset == 0) || (path[offset - 1] != '\\')) {
                        path.insert(offset, 1, '\\');
                        ++start;
                    }
                }
                ++invalidChar;
            }
            return path;
            #endif
        }

        std::string getCanonicalPath(const std::string& _path)
        {
            // Hack for builtin resources.
            if ((_path[0] == ':') && (_path[1] == '/'))
                return _path;

            std::string path = exists(_path) ? getAbsolutePath(_path) : getGenericPath(_path);

            // Cleanup path.
            bool scan = true;
            while (scan) {
                stringList pathList = getPathList(path);

                path.clear();
                scan = false;

                for (stringList::const_iterator it = pathList.cbegin();
                        it != pathList.cend(); ++it) {
                    // Ignore empty.
                    if ((*it).empty())
                        continue;

                    // Remove "/./"
                    if ((*it) == ".")
                        continue;

                    // Resolve "/../"
                    if ((*it) == "..") {
                        path = getParent(path);
                        continue;
                    }

                    #if defined(_WIN64)
                    // Append folder to path.
                    path += (path.size() == 0) ? (*it) : ("/" + (*it));
                    #else
                    // Append folder to path.
                    path += ("/" + (*it));
                    #endif

                    // Resolve symlink.
                    if (isSymlink(path)) {
                        std::string resolved = resolveSymlink(path);

                        if (resolved.empty())
                            return "";

                        if (isAbsolute(resolved))
                            path = resolved;
                        else
                            path = getParent(path) + "/" + resolved;

                        for (++it; it != pathList.cend(); ++it)
                            path += (path.size() == 0) ? (*it) : ("/" + (*it));

                        scan = true;
                        break;
                    }
                }
            }
            return path;
        }

        std::string getAbsolutePath(const std::string& _path, const std::string& _base)
        {
            std::string path = getGenericPath(_path);
            std::string base = isAbsolute(_base) ? getGenericPath(_base) : getAbsolutePath(_base);

            // Return absolute path.
            return isAbsolute(path) ? path : getGenericPath(base + "/" + path);

        }

        std::string getParent(const std::string& _path)
        {
            std::string path = getGenericPath(_path);
            size_t offset = std::string::npos;

            // Find last '/' and erase it.
            if ((offset = path.find_last_of('/')) != std::string::npos)
                return path.erase(offset);

            // No parent found.
            return path;
        }

        std::string getFileName(const std::string& _path)
        {
            std::string path = getGenericPath(_path);
            size_t offset = std::string::npos;

            // Find last '/' and return the filename.
            if ((offset = path.find_last_of('/')) != std::string::npos)
                return ((path[offset + 1] == 0) ? "." : std::string(path, offset + 1));

            // No '/' found, entire path is a filename.
            return path;
        }

        std::string getStem(const std::string& _path)
        {
            std::string fileName = getFileName(_path);
            size_t offset = std::string::npos;

            // Empty fileName.
            if (fileName == ".")
                return fileName;

            // Find last '.' and erase the extension.
            if ((offset = fileName.find_last_of('.')) != std::string::npos)
                return fileName.erase(offset);

            // No '.' found, filename has no extension.
            return fileName;
        }

        std::string getExtension(const std::string& _path)
        {
            std::string fileName = getFileName(_path);
            size_t offset = std::string::npos;

            // Empty fileName.
            if (fileName == ".")
                return fileName;

            // Find last '.' and return the extension.
            if ((offset = fileName.find_last_of('.')) != std::string::npos)
                return std::string(fileName, offset);

            // No '.' found, filename has no extension.
            return ".";
        }

        std::string resolveRelativePath(const std::string& _path,
                const std::string& _relativeTo, const bool _allowHome)
        {
            std::string path = getGenericPath(_path);
            std::string relativeTo = isDirectory(_relativeTo) ?
                    getGenericPath(_relativeTo) : getParent(_relativeTo);

            // Nothing to resolve.
            if (!path.length())
                return path;

            // Replace '.' with relativeTo.
            if ((path[0] == '.') && (path[1] == '/'))
                return (relativeTo + &(path[1]));

            // Replace '~' with homePath.
            if (_allowHome && (path[0] == '~') && (path[1] == '/'))
                return (getHomePath() + &(path[1]));

            // Nothing to resolve.
            return path;
        }

        std::string createRelativePath(const std::string& _path,
                const std::string& _relativeTo, const bool _allowHome)
        {
            bool contains = false;
            std::string path = removeCommonPath(_path, _relativeTo, contains);

            if (contains)
                return ("./" + path);

            if (_allowHome) {
                path = removeCommonPath(_path, getHomePath(), contains);

                if (contains)
                    return ("~/" + path);
            }
            return path;
        }

        std::string removeCommonPath(const std::string& _path,
                const std::string& _common, bool& _contains)
        {
            std::string path   = getGenericPath(_path);
            std::string common = isDirectory(_common) ?
                    getGenericPath(_common) : getParent(_common);

            // Check if path contains common.
            if (path.find(common) == 0) {
                _contains = true;
                return path.substr(common.length() + 1);
            }

            // It didn't.
            _contains = false;
            return path;
        }

        std::string resolveSymlink(const std::string& _path)
        {
            std::string path = getGenericPath(_path);
            std::string resolved;

            #if defined(_WIN64)
            HANDLE hFile = CreateFile(path.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ,
                    0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
//            TEMPORARY, will need to fix this later.
//            if (hFile != INVALID_HANDLE_VALUE) {
//                resolved.resize(GetFinalPathNameByHandle(hFile, nullptr, 0,
//                resolved.resize(GetFinalPathNameByHandle(hFile, nullptr, 0,
//                        FILE_NAME_NORMALIZED) + 1);
//                if (GetFinalPathNameByHandle(hFile, (LPSTR)resolved.data(),
//               if (GetFinalPathNameByHandle(hFile, (LPSTR)resolved.data(),
//                        (DWORD)resolved.size(), FILE_NAME_NORMALIZED) > 0) {
//                    resolved.resize(resolved.size() - 1);
//                    resolved = getGenericPath(resolved);
//                }
//                CloseHandle(hFile);
//            }
            #else
            struct stat info;

            // Check if lstat succeeded.
            if (lstat(path.c_str(), &info) == 0) {
                resolved.resize(info.st_size);
                if (readlink(path.c_str(), (char*)resolved.data(), resolved.size()) > 0)
                    resolved = getGenericPath(resolved);
            }
            #endif
            return resolved;
        }

        bool copyFile(const std::string& _source_path,
                const std::string& _destination_path, bool _overwrite)
        {
            if (!exists(_source_path)) {
                LOG(LogError) << "Error - Can't copy file, source file does not exist:";
                LOG(LogError) << _source_path;
                return true;
            }

            if(isDirectory(_destination_path)) {
                LOG(LogError) << "Error - Destination file is actually a directory:";
                LOG(LogError) << _destination_path;
                return true;
            }

            if (!_overwrite && exists(_destination_path)) {
                LOG(LogError) << "Error - Destination file exists and the overwrite flag "
                        "has not been set.";
                return true;
            }

            #ifdef _WIN64
            std::ifstream sourceFile(Utils::String::stringToWideString(_source_path).c_str(),
                    std::ios::binary);
            #else
            std::ifstream sourceFile(_source_path, std::ios::binary);
            #endif

            if (sourceFile.fail()) {
                LOG(LogError) << "Error - Couldn't read from source file (" << _source_path <<
                        "), permission problems?";
                sourceFile.close();
                return true;
            }

            #ifdef _WIN64
            std::ofstream targetFile(Utils::String::stringToWideString(_destination_path).c_str(),
                    std::ios::binary);
            #else
            std::ofstream targetFile(_destination_path, std::ios::binary);
            #endif

            if (targetFile.fail()) {
                LOG(LogError) << "Error - Couldn't write to target file (" << _destination_path <<
                        "), permission problems?";
                targetFile.close();
                return true;
            }

            targetFile << sourceFile.rdbuf();

            sourceFile.close();
            targetFile.close();

            return false;
        }

        bool renameFile(const std::string& _source_path,
                const std::string& _destination_path, bool _overwrite)
        {
            // Don't print any error message for a missing source file as Log will use this
            // function when initializing the logging. It would always generate an error in
            // case it's the first application start (as an old log file would then not exist).
            if (!exists(_source_path)) {
                return true;
            }

            if(isDirectory(_destination_path)) {
                LOG(LogError) << "Error - Destination file is actually a directory:";
                LOG(LogError) << _destination_path;
                return true;
            }

            if (!_overwrite && exists(_destination_path)) {
                LOG(LogError) << "Error - Destination file exists and the overwrite flag "
                        "has not been set.";
                return true;
            }

            #ifdef _WIN64
            _wrename(Utils::String::stringToWideString(_source_path).c_str(),
                    Utils::String::stringToWideString(_destination_path).c_str());
            #else
            std::rename(_source_path.c_str(), _destination_path.c_str());
            #endif

            return false;
        }

        bool removeFile(const std::string& _path)
        {
            std::string path = getGenericPath(_path);

            // Don't remove if it doesn't exists.
            if (!exists(path))
                return true;

            // Try to remove file.
            #ifdef _WIN64
            if (_wunlink(Utils::String::stringToWideString(path).c_str()) != 0) {
                LOG(LogError) << "Error - Couldn't delete file, permission problems?";
                LOG(LogError) << path;
                return true;
            }
            else {
                return false;
            }
            #else
            if (unlink(path.c_str()) != 0) {
                LOG(LogError) << "Error - Couldn't delete file, permission problems?";
                LOG(LogError) << path;
                return true;
            }
            else {
                return false;
            }
            return (unlink(path.c_str()) == 0);
            #endif
        }

        bool createDirectory(const std::string& _path)
        {
            std::string path = getGenericPath(_path);

            // Don't create if it already exists.
            if (exists(path))
                return true;

            // Try to create directory.
            #ifdef _WIN64
            if (_wmkdir(Utils::String::stringToWideString(path).c_str()) == 0)
                return true;
            #else
            if (mkdir(path.c_str(), 0755) == 0)
                return true;
            #endif

            // Failed to create directory, try to create the parent.
            std::string parent = getParent(path);

            // Only try to create parent if it's not identical to path.
            if (parent != path)
                createDirectory(parent);

            // Try to create directory again now that the parent should exist.
            #ifdef _WIN64
            return (_wmkdir(Utils::String::stringToWideString(path).c_str()) == 0);
            #else
            return (mkdir(path.c_str(), 0755) == 0);
            #endif
        }

        bool exists(const std::string& _path)
        {
            std::string path = getGenericPath(_path);

            #ifdef _WIN64
            struct _stat64 info;
            return (_wstat64(Utils::String::stringToWideString(path).c_str(), &info) == 0);
            #else
            struct stat64 info;
            return (stat64(path.c_str(), &info) == 0);
            #endif
        }

        bool isAbsolute(const std::string& _path)
        {
            std::string path = getGenericPath(_path);

            #if defined(_WIN64)
            return ((path.size() > 1) && (path[1] == ':'));
            #else
            return ((path.size() > 0) && (path[0] == '/'));
            #endif
        }

        bool isRegularFile(const std::string& _path)
        {
            std::string path = getGenericPath(_path);
            struct stat64 info;

            #ifdef _WIN64
            if (_wstat64(Utils::String::stringToWideString(path).c_str(), &info) != 0)
                return false;
            #else
            if (stat64(path.c_str(), &info) != 0)
                return false;
            #endif

            // Check for S_IFREG attribute.
            return (S_ISREG(info.st_mode));
        }

        bool isDirectory(const std::string& _path)
        {
            std::string path = getGenericPath(_path);
            struct stat64 info;

            #ifdef _WIN64
            if (_wstat64(Utils::String::stringToWideString(path).c_str(), &info) != 0)
                return false;
            #else
             if (stat64(path.c_str(), &info) != 0)
                return false;
            #endif

            // Check for S_IFDIR attribute.
            return (S_ISDIR(info.st_mode));
        }

        bool isSymlink(const std::string& _path)
        {
            std::string path = getGenericPath(_path);

            #if defined(_WIN64)
            // Check for symlink attribute.
            const DWORD Attributes = GetFileAttributes(path.c_str());
            if ((Attributes != INVALID_FILE_ATTRIBUTES) &&
                    (Attributes & FILE_ATTRIBUTE_REPARSE_POINT))
                return true;
            #else
            struct stat info;

            if (lstat(path.c_str(), &info) != 0)
                return false;

            // Check for S_IFLNK attribute.
            return (S_ISLNK(info.st_mode));
            #endif

            // Not a symlink.
            return false;
        }

        bool isHidden(const std::string& _path)
        {
            std::string path = getGenericPath(_path);

            #if defined(_WIN64)
            // Check for hidden attribute.
            const DWORD Attributes = GetFileAttributes(path.c_str());
            if ((Attributes != INVALID_FILE_ATTRIBUTES) && (Attributes & FILE_ATTRIBUTE_HIDDEN))
                return true;
            #endif // _WIN64

            // Filenames starting with . are hidden in Linux, but
            // we do this check for windows as well.
            if (getFileName(path)[0] == '.')
                return true;

            return false;
        }

    } // FileSystem::
} // Utils::
