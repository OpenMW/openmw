#include "aiactivate.hpp"

#include <components/esm/aisequence.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/class.hpp"

#include "creaturestats.hpp"
#include "steering.hpp"
#include "movement.hpp"

namespace MWMechanics
{
    AiActivate::AiActivate(const std::string &objectId)
        : mObjectId(objectId)
    {
    }

    AiActivate *MWMechanics::AiActivate::clone() const
    {
        return new AiActivate(*this);
    }

    bool AiActivate::execute(const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
    {
        const MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtr(mObjectId, false); //The target to follow

        actor.getClass().getCreatureStats(actor).setDrawState(DrawState_Nothing);

        if (target == MWWorld::Ptr() ||
            !target.getRefData().getCount() || !target.getRefData().isEnabled()  // Really we should check whether the target is currently registered
                                                                                // with the MechanicsManager
            )
        return true;   //Target doesn't exist

        //Set the target destination for the actor
        ESM::Pathgrid::Point dest = target.getRefData().getPosition().pos;

        if (pathTo(actor, dest, duration, MWBase::Environment::get().getWorld()->getMaxActivationDistance())) //Stop when you get in activation range
        {
            // activate when reached
            MWWorld::Ptr target = MWBase::Environment::get().getWorld()->getPtr(mObjectId,false);
            MWBase::Environment::get().getWorld()->activate(target, actor);

            return true;
        }

        return false;
    }

    int AiActivate::getTypeId() const
    {
        return TypeIdActivate;
    }

    void AiActivate::writeState(ESM::AiSequence::AiSequence &sequence) const
    {
        std::auto_ptr<ESM::AiSequence::AiActivate> activate(new ESM::AiSequence::AiActivate());
        activate->mTargetId = mObjectId;

        ESM::AiSequence::AiPackageContainer package;
        package.mType = ESM::AiSequence::Ai_Activate;
        package.mPackage = activate.release();
        sequence.mPackages.push_back(package);
    }

    AiActivate::AiActivate(const ESM::AiSequence::AiActivate *activate)
        : mObjectId(activate->mTargetId)
    {
    }
}
