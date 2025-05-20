#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTCONTEXT_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTCONTEXT_H

#include "tileposition.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm/refid.hpp>

#include <string>

#include <Recast.h>

namespace DetourNavigator
{
    struct AgentBounds;
    struct Version;

    class RecastContext final : public rcContext
    {
    public:
        explicit RecastContext(ESM::RefId worldspace, const TilePosition& tilePosition, const AgentBounds& agentBounds,
            const Version& version, Debug::Level maxLogLevel);

        const std::string& getPrefix() const { return mPrefix; }

    private:
        Debug::Level mMaxLogLevel;
        std::string mPrefix;

        void doLog(rcLogCategory category, const char* msg, int len) override;
    };
}

#endif
