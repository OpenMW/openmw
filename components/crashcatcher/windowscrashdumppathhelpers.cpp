#include "windowscrashdumppathhelpers.hpp"

#include <boost/filesystem/path.hpp>

std::string Crash::getCrashDumpPath(const CrashSHM& crashShm)
{
    return (boost::filesystem::path(crashShm.mStartup.mDumpDirectoryPath) / crashShm.mStartup.mCrashDumpFileName).string();
}

std::string Crash::getFreezeDumpPath(const CrashSHM& crashShm)
{
    return (boost::filesystem::path(crashShm.mStartup.mDumpDirectoryPath) / crashShm.mStartup.mFreezeDumpFileName).string();
}
