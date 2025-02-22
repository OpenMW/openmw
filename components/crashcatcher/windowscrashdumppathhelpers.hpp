#ifndef OPENMW_COMPONENTS_CRASHCATCHER_WINDOWSCRASHDUMPPATHHELPERS_HPP
#define OPENMW_COMPONENTS_CRASHCATCHER_WINDOWSCRASHDUMPPATHHELPERS_HPP

#include <filesystem>

#include "windowscrashshm.hpp"

namespace Crash
{
    std::filesystem::path getCrashDumpPath(const CrashSHM& crashShm);

    std::filesystem::path getFreezeDumpPath(const CrashSHM& crashShm);
}

#endif
