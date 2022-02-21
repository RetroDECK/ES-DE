//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  FileSystemUtil.h
//
//  Low-level filesystem functions.
//  Resolve relative paths, resolve symlinks, create directories,
//  remove files etc.
//

#ifndef ES_CORE_UTILS_FILE_SYSTEM_UTIL_H
#define ES_CORE_UTILS_FILE_SYSTEM_UTIL_H

#include <list>
#include <string>

namespace Utils
{
    namespace FileSystem
    {
        using StringList = std::list<std::string>;

        StringList getDirContent(const std::string& path, const bool recursive = false);
        StringList getMatchingFiles(const std::string& pattern);
        StringList getPathList(const std::string& path);
        void setHomePath(const std::string& path);
        std::string getHomePath();
        std::string getCWDPath();
        std::string getPathToBinary(const std::string& executable);
        void setExePath(const std::string& path);
        std::string getExePath();
        std::string getProgramDataPath();
        std::string getPreferredPath(const std::string& path);
        std::string getGenericPath(const std::string& path);
        std::string getEscapedPath(const std::string& path);
        std::string getCanonicalPath(const std::string& path);
        std::string getAbsolutePath(const std::string& path,
                                    const std::string& base = getCWDPath());
        std::string getParent(const std::string& path);
        std::string getFileName(const std::string& path);
        std::string getStem(const std::string& path);
        std::string getExtension(const std::string& path);
        std::string expandHomePath(const std::string& path);
        std::string resolveRelativePath(const std::string& path,
                                        const std::string& relativeTo,
                                        const bool allowHome);
        std::string createRelativePath(const std::string& path,
                                       const std::string& relativeTo,
                                       const bool allowHome);
        std::string removeCommonPath(const std::string& path,
                                     const std::string& commonArg,
                                     bool& contains);
        std::string resolveSymlink(const std::string& path);
        bool copyFile(const std::string& sourcePath,
                      const std::string& destinationPath,
                      bool overwrite);
        bool renameFile(const std::string& sourcePath,
                        const std::string& destinationPath,
                        bool overwrite);
        bool removeFile(const std::string& path);
        bool removeDirectory(const std::string& path);
        bool createDirectory(const std::string& path);
        bool exists(const std::string& path);
        bool driveExists(const std::string& path);
        bool isAbsolute(const std::string& path);
        bool isRegularFile(const std::string& path);
        bool isDirectory(const std::string& path);
        bool isSymlink(const std::string& path);
        bool isHidden(const std::string& path);

    } // namespace FileSystem

} // namespace Utils

#endif // ES_CORE_UTILS_FILE_SYSTEM_UTIL_H
