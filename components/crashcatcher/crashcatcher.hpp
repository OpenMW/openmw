#ifndef CRASHCATCHER_H
#define CRASHCATCHER_H

#include <string>

#if (defined(__APPLE__) || (defined(__linux)  &&  !defined(ANDROID)) || (defined(__unix) &&  !defined(ANDROID)) || defined(__posix))
    #define USE_CRASH_CATCHER 1
#else
    #define USE_CRASH_CATCHER 0
#endif

#if USE_CRASH_CATCHER
extern void cc_install(int argc, char **argv, const std::string &crashLogPath);
#else
inline void cc_install(int, char **) { return 0; }
#endif

#endif
