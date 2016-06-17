#include "aiactivate.hpp"

#include <components/esm/aisequence.hpp>

#include "apps/openmw/mwbase/world.hpp"
#include "apps/openmw/mwbase/environment.hpp"

#include "apps/openmw/mwworld/class.hpp"

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

    bool AiActivate::execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
    {
        ESM::Position pos = actor.getRefData().getPosition(); //position of the actor
        const MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtr(mObjectId, false); //The target to follow

        actor.getClass().getCreatureStats(actor).setDrawState(DrawState_Nothing);

        if(target == MWWorld::Ptr() ||
            !target.getRefData().getCount() || !target.getRefData().isEnabled()  // Really we should be checking whether the target is currently registered
                                                                                // with the MechanicsManager
                )
            return true;   //Target doesn't exist

        //Set the target desition from the actor
        ESM::Pathgrid::Point dest = target.getRefData().getPosition().pos;

        if(distance(dest, pos.pos[0], pos.pos[1], pos.pos[2]) < 200) { //Stop when you get close
            actor.getClass().getMovementSettings(actor).mPosition[1] = 0;
            MWWorld::Ptr activatetarget = MWBase::Environment::get().getWorld()->getPtr(mObjectId,false);
            MWBase::Environment::get().getWorld()->activate(activatetarget, actor);
            return true;
        }
        else {
            pathTo(actor, dest, duration); //Go to the destination
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
