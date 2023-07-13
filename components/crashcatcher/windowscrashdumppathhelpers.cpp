#include "windowscrashdumppathhelpers.hpp"

#include <components/misc/strings/conversion.hpp>

namespace Crash
{
    std::filesystem::path getCrashDumpPath(const CrashSHM& crashShm)
    {
        return (std::filesystem::path(Misc::StringUtils::stringToU8String(crashShm.mStartup.mDumpDirectoryPath))
            / Misc::StringUtils::stringToU8String(crashShm.mStartup.mCrashDumpFileName))
            .make_preferred();
    }

    std::filesystem::path getFreezeDumpPath(const CrashSHM& crashShm)
    {
        return (std::filesystem::path(Misc::StringUtils::stringToU8String(crashShm.mStartup.mDumpDirectoryPath))
            / Misc::StringUtils::stringToU8String(crashShm.mStartup.mFreezeDumpFileName))
            .make_preferred();
    }
}
