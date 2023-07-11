#ifndef COMPONENTS_CRASH_WINDOWSCRASHDUMPPATHHELPERS_H
#include "windows_crashshm.hpp"

#include <filesystem>

namespace Crash
{
    std::filesystem::path getCrashDumpPath(const CrashSHM& crashShm);

    std::filesystem::path getFreezeDumpPath(const CrashSHM& crashShm);
}

#endif
