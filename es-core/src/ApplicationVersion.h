//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  ApplicationVersion.h
//

#ifndef ES_CORE_APPLICATION_VERSION_H
#define ES_CORE_APPLICATION_VERSION_H

// These numbers and strings need to be manually updated for a new version.
// Do this version number update as the very last commit for the new release version.
// clang-format off
#define PROGRAM_VERSION_MAJOR        3
#define PROGRAM_VERSION_MINOR        0
#define PROGRAM_VERSION_MAINTENANCE  3
#define PROGRAM_RELEASE_NUMBER       44
// clang-format on
#define PROGRAM_VERSION_STRING "3.0.3"

#define PROGRAM_BUILT_STRING __DATE__ " - " __TIME__

#define RESOURCE_VERSION_STRING "3,0,3\0"
#define RESOURCE_VERSION PROGRAM_VERSION_MAJOR, PROGRAM_VERSION_MINOR, PROGRAM_VERSION_MAINTENANCE

#endif // ES_CORE_APPLICATION_VERSION_H
