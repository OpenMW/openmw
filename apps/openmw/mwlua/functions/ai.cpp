#include "ai.hpp"

#include <extern/sol2/sol.hpp>

#include "../luamanager.hpp"
#include "../util.hpp"

#include "../../mwbase/environment.hpp"

#include "../../mwmechanics/actorutil.hpp"
#include "../../mwmechanics/aiactivate.hpp"
#include "../../mwmechanics/aiescort.hpp"
#include "../../mwmechanics/aifollow.hpp"
#include "../../mwmechanics/aitravel.hpp"
#include "../../mwmechanics/aiwander.hpp"
#include "../../mwmechanics/creaturestats.hpp"

#include "../../mwworld/class.hpp"

#include <components/debug/debuglog.hpp>

namespace MWLua
{
    void bindTES3AIFunctions()
    {
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        state["omw"]["getCurrentAIPackageId"] = [](sol::table params)
        {
            MWWorld::Ptr ptr = getOptionalParamReference(params, "reference");

            if (!ptr.isEmpty())
            {
                if (ptr == MWMechanics::getPlayer())
                    return -1;

                return ptr.getClass().getCreatureStats (ptr).getAiSequence().getLastRunTypeId();
            }
            else
            {
                throw std::invalid_argument("Invalid reference parameter provided.");
            }

            return -1;
        };

        state["omw"]["setAIActivate"] = [](sol::table params)
        {
            MWWorld::Ptr ptr = getOptionalParamReference(params, "reference");
            if (!ptr.getClass().isActor())
                throw std::invalid_argument("Invalid reference parameter provided.");

            // FIXME: reference target support
            sol::optional<std::string> target = params["target"];
            if (target.value().empty())
                throw std::invalid_argument("Invalid target parameter provided.");

            // FIXME: actual reset flag support
            //bool reset = getOptionalParam<bool>(params, "reset", true);

            MWMechanics::AiActivate activatePackage(target.value());
            ptr.getClass().getCreatureStats (ptr).getAiSequence().stack(activatePackage, ptr);
            Log(Debug::Info) << "AiActivate";
        };

        state["omw"]["setAIFollow"] = [](sol::table params)
        {
            MWWorld::Ptr ptr = getOptionalParamReference(params, "reference");
            if (!ptr.getClass().isActor())
                throw std::invalid_argument("Invalid reference parameter provided.");

            MWWorld::Ptr target = getOptionalParamReference(params, "target");
            if (target.isEmpty() || !target.getClass().isActor())
            {
                throw std::invalid_argument("Invalid target parameter provided.");
            }

            // FIXME: we do not know what this parameter is supposed to do.
            // bool reset = getOptionalParam<bool>(params, "reset", true);

            std::string cellId = params["cell"];
            double duration = getOptionalParam<double>(params, "duration", 0.0);
            auto destination = getOptionalParamVector3(params, "destination");
            // FIXME: AiEscort stores arget Id instead of reference in OpenMW
            MWMechanics::AiFollow followPackage(target, cellId, duration, destination.value().x(), destination.value().y(), destination.value().z());
            ptr.getClass().getCreatureStats (ptr).getAiSequence().stack(followPackage, ptr);

            Log(Debug::Info) << "AiFollow: " << target.getCellRef().getRefId() << ", " << destination.value().x() << ", " << destination.value().y() << ", " << destination.value().z() << ", " << duration;
        };

        state["omw"]["setAIEscort"] = [](sol::table params)
        {
            MWWorld::Ptr ptr = getOptionalParamReference(params, "reference");
            if (!ptr.getClass().isActor())
                throw std::invalid_argument("Invalid reference parameter provided.");

            MWWorld::Ptr target = getOptionalParamReference(params, "target");
            if (target.isEmpty() || !target.getClass().isActor())
            {
                throw std::invalid_argument("Invalid target parameter provided.");
            }

            // FIXME: we do not know what this parameter is supposed to do.
            // bool reset = getOptionalParam<bool>(params, "reset", true);

            std::string cellId = params["cell"];
            double duration = getOptionalParam<double>(params, "duration", 0.0);
            auto destination = getOptionalParamVector3(params, "destination");
            // FIXME: AiEscort stores arget Id instead of reference in OpenMW
            MWMechanics::AiEscort escortPackage(target.getCellRef().getRefId(), cellId, duration, destination.value().x(), destination.value().y(), destination.value().z());
            ptr.getClass().getCreatureStats (ptr).getAiSequence().stack(escortPackage, ptr);

            Log(Debug::Info) << "AiEscort: " << target.getCellRef().getRefId() << ", " << destination.value().x() << ", " << destination.value().y() << ", " << destination.value().z() << ", " << duration;
        };

        state["omw"]["setAITravel"] = [](sol::table params)
        {
            MWWorld::Ptr ptr = getOptionalParamReference(params, "reference");
            if (!ptr.getClass().isActor())
                throw std::invalid_argument("Invalid reference parameter provided.");

            auto destination = getOptionalParamVector3(params, "destination");
            if (!destination)
            {
                throw std::invalid_argument("Invalid destination parameter provided.");
            }

            // FIXME: we do not know what this parameter is supposed to do.
            // bool reset = getOptionalParam<bool>(params, "reset", true);

            MWMechanics::AiTravel travelPackage(destination.value().x(), destination.value().y(), destination.value().z());
            ptr.getClass().getCreatureStats (ptr).getAiSequence().stack(travelPackage, ptr);

            Log(Debug::Info) << "AiTravel: " << destination.value().x() << ", " << destination.value().y() << ", " << destination.value().z();
        };

        state["omw"]["setAIWander"] = [](sol::table params)
        {
            MWWorld::Ptr ptr = getOptionalParamReference(params, "reference");
            if (!ptr.getClass().isActor())
                throw std::invalid_argument("Invalid reference parameter provided.");

            sol::optional<sol::table> maybeIdles = params["idles"];
            if (!maybeIdles || maybeIdles.value().get_type() != sol::type::table)
            {
                throw std::invalid_argument("Invalid idles table provided.");
            }

            // FIXME: we do not know what this parameter is supposed to do.
            // bool reset = getOptionalParam<bool>(params, "reset", true);

            double duration = getOptionalParam<double>(params, "duration", 0.0);
            double range = getOptionalParam<double>(params, "range", 0.0);
            double time = getOptionalParam<double>(params, "time", 0.0);

            sol::table idles = maybeIdles.value();
            std::vector<unsigned char> idleList;

            for (size_t i = 0; i < 8; i++)
            {
                idleList.push_back(idles.get_or(i, 0));
            }

            MWMechanics::AiWander wanderPackage(range, duration, time, idleList, true);
            ptr.getClass().getCreatureStats (ptr).getAiSequence().stack(wanderPackage, ptr);
        };
    }
}
