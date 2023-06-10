#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTCONTEXT_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTCONTEXT_H

#include "tileposition.hpp"

#include <string>

#include <Recast.h>

namespace DetourNavigator
{
    struct AgentBounds;

    class RecastContext final : public rcContext
    {
    public:
        explicit RecastContext(
            std::string_view worldspace, const TilePosition& tilePosition, const AgentBounds& agentBounds);

        const std::string& getPrefix() const { return mPrefix; }

    private:
        std::string mPrefix;

        void doLog(rcLogCategory category, const char* msg, int len) override;
    };
}

#endif
