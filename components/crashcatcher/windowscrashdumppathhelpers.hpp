#ifndef COMPONENTS_CRASH_WINDOWSCRASHDUMPPATHHELPERS_H
#include "windows_crashshm.hpp"

#include <string>

namespace Crash
{
    std::string getCrashDumpPath(const CrashSHM& crashShm);

    std::string getFreezeDumpPath(const CrashSHM& crashShm);
}

#endif
