#include "recastcontext.hpp"
#include "debug.hpp"

#include "components/debug/debuglog.hpp"

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

        std::string formatPrefix(
            ESM::RefId worldspace, const TilePosition& tilePosition, const AgentBounds& agentBounds)
        {
            std::ostringstream stream;
            stream << "Worldspace: " << worldspace << "; tile position: " << tilePosition.x() << ", "
                   << tilePosition.y() << "; agent bounds: " << agentBounds << "; ";
            return stream.str();
        }
    }

    RecastContext::RecastContext(
        ESM::RefId worldspace, const TilePosition& tilePosition, const AgentBounds& agentBounds)
        : mPrefix(formatPrefix(worldspace, tilePosition, agentBounds))
    {
    }

    void RecastContext::doLog(const rcLogCategory category, const char* msg, const int len)
    {
        if (len > 0)
            Log(getLogLevel(category)) << mPrefix << std::string_view(msg, static_cast<std::size_t>(len));
    }
}
