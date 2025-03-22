#include "recastcontext.hpp"
#include "debug.hpp"

#include <components/debug/debuglog.hpp>

#include <sstream>

namespace DetourNavigator
{
    namespace
    {
        Debug::Level getLogLevel(rcLogCategory category)
        {
            switch (category)
            {
                case RC_LOG_PROGRESS:
                    return Debug::Verbose;
                case RC_LOG_WARNING:
                    return Debug::Warning;
                case RC_LOG_ERROR:
                    return Debug::Error;
            }
            return Debug::Debug;
        }

        std::string formatPrefix(ESM::RefId worldspace, const TilePosition& tilePosition,
            const AgentBounds& agentBounds, const Version& version)
        {
            std::ostringstream stream;
            stream << "Worldspace: " << worldspace << "; tile position: " << tilePosition.x() << ", "
                   << tilePosition.y() << "; agent bounds: " << agentBounds << "; version: " << version << "; ";
            return stream.str();
        }
    }

    RecastContext::RecastContext(ESM::RefId worldspace, const TilePosition& tilePosition,
        const AgentBounds& agentBounds, const Version& version, Debug::Level maxLogLevel)
        : mMaxLogLevel(maxLogLevel)
        , mPrefix(formatPrefix(worldspace, tilePosition, agentBounds, version))
    {
    }

    void RecastContext::doLog(const rcLogCategory category, const char* msg, const int len)
    {
        if (msg == nullptr || len <= 0)
            return;
        const Debug::Level level = getLogLevel(category);
        if (level > mMaxLogLevel)
            return;
        Log(level) << mPrefix << std::string_view(msg, static_cast<std::size_t>(len));
    }
}
