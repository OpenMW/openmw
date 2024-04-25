#ifndef CRASHCATCHER_H
#define CRASHCATCHER_H

#include <filesystem>

#if (defined(__APPLE__) || (defined(__linux) && !defined(ANDROID)) || (defined(__unix) && !defined(ANDROID))           \
    || defined(__posix))
void crashCatcherInstall(int argc, char** argv, const std::filesystem::path& crashLogPath);
#else
inline void crashCatcherInstall(int /*argc*/, char** /*argv*/, const std::filesystem::path& /*crashLogPath*/) {}
#endif

#endif
