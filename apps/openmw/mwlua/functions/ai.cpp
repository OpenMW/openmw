#include "ai.hpp"

#include <extern/sol2/sol.hpp>

#include "../luamanager.hpp"
#include "../util.hpp"

#include "../../mwbase/environment.hpp"

#include "../../mwmechanics/actorutil.hpp"
#include "../../mwmechanics/aiactivate.hpp"
#include "../../mwmechanics/aifollow.hpp"
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
            osg::Vec3f destination = getOptionalParamVector3(params, "destination");
            MWMechanics::AiFollow followPackage(target, cellId, duration, destination.x(), destination.y(), destination.z());
            ptr.getClass().getCreatureStats (ptr).getAiSequence().stack(followPackage, ptr);

            Log(Debug::Info) << "AiFollow: " << target.getCellRef().getRefId() << ", " << destination.x() << ", " << destination.y() << ", " << destination.z() << ", " << duration;
        };

        /*
        state["tes3"]["setAIEscort"] = [](sol::table params)
        {
            TES3::MobileActor * mobileActor = getOptionalParamMobileActor(params, "reference");
            if (mobileActor == nullptr)
            {
                throw std::invalid_argument("Invalid reference parameter provided.");
            }

            TES3::Reference * target = getOptionalParamReference(params, "target");
            if (target == nullptr || !target->baseObject->isActor())
            {
                throw std::invalid_argument("Invalid target parameter provided.");
            }

            auto destination = getOptionalParamVector3(params, "destination");
            if (!destination)
            {
                throw std::invalid_argument("Destination parameter is missing.");
            }

            auto config = tes3::_new<TES3::AIPackageEscort::Config>();
            config->type = TES3::AIPackageConfigType::Escort;
            config->destination = destination.value();
            config->duration = getOptionalParam<double>(params, "duration", 0.0);
            config->actor = static_cast<TES3::Actor*>(target->getBaseObject());
            config->cell = getOptionalParamCell(params, "cell");
            config->reset = getOptionalParam<bool>(params, "reset", true);

            auto actor = static_cast<TES3::Actor*>(mobileActor->reference->baseObject);
            actor->setAIPackage(config, mobileActor->reference);
        };

        state["tes3"]["setAITravel"] = [](sol::table params)
        {
            TES3::MobileActor * mobileActor = getOptionalParamMobileActor(params, "reference");
            if (mobileActor == nullptr)
            {
                throw std::invalid_argument("Invalid reference parameter provided.");
            }

            auto destination = getOptionalParamVector3(params, "destination");
            if (!destination)
            {
                throw std::invalid_argument("Invalid destination parameter provided.");
            }

            auto config = tes3::_new<TES3::AIPackageTravel::Config>();
            config->type = TES3::AIPackageConfigType::Travel;
            config->position = destination.value();
            config->reset = getOptionalParam<bool>(params, "reset", true);

            auto actor = static_cast<TES3::Actor*>(mobileActor->reference->baseObject);
            actor->setAIPackage(config, mobileActor->reference);
        };

        state["tes3"]["setAIWander"] = [](sol::table params)
        {
            TES3::MobileActor * mobileActor = getOptionalParamMobileActor(params, "reference");
            if (mobileActor == nullptr)
            {
                throw std::invalid_argument("Invalid reference parameter provided.");
            }

            sol::optional<sol::table> maybeIdles = params["idles"];
            if (!maybeIdles || maybeIdles.value().get_type() != sol::type::table)
            {
                throw std::invalid_argument("Invalid idles table provided.");
            }

            auto config = tes3::_new<TES3::AIPackageWander::Config>();
            config->type = TES3::AIPackageConfigType::Wander;
            config->range = getOptionalParam<double>(params, "range", 0.0);
            config->duration = getOptionalParam<double>(params, "duration", 0.0);
            config->time = getOptionalParam<double>(params, "time", 0.0);
            config->reset = getOptionalParam<bool>(params, "reset", true);

            sol::table idles = maybeIdles.value();
            for (size_t i = 0; i < 8; i++)
            {
                config->idles[i] = idles.get_or(i, 0);
            }

            auto actor = static_cast<TES3::Actor*>(mobileActor->reference->baseObject);
            actor->setAIPackage(config, mobileActor->reference);
        };
        */
    }
}
