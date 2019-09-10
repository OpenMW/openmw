#include "ai.hpp"

#include <extern/sol2/sol.hpp>

#include "../luamanager.hpp"
#include "../luautil.hpp"

#include "../../mwbase/environment.hpp"

#include "../../mwmechanics/actorutil.hpp"
#include "../../mwmechanics/aiactivate.hpp"
#include "../../mwmechanics/creaturestats.hpp"

#include "../../mwworld/class.hpp"

#include <components/debug/debuglog.hpp>

namespace MWLua
{
    void bindTES3AI()
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
    }
}
