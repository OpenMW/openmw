#ifndef CRASHCATCHER_H
#define CRASHCATCHER_H

#include <filesystem>

void crashCatcherInstall(int argc, char** argv, const std::filesystem::path& crashLogPath);

#endif
