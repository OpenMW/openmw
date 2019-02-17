#include "debuglog.hpp"

#include <boost/filesystem.hpp>

#include <fstream>

namespace Debug
{
    Level CurrentDebugLevel = Level::NoLevel;
}

std::mutex Log::sLock;

Profiler::Profiler()
{
    mStarts.reserve(100);
    mRecords.reserve(30000000);
    mRecords.resize(30000000);
    mRecords.resize(0);
}

void Profiler::save(const std::string& path)
{
    {
        std::ofstream file(path);
        for (const auto& record : mRecords)
            file << record.mDepth << " " << record.mName << " " << record.mDuration.count() << '\n';
    }
    Log(Debug::Verbose) << "profile: " << mRecords.size() << " " << boost::filesystem::canonical(path);
}
