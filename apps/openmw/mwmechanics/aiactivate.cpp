#include "aiactivate.hpp"

#include <components/esm/aisequence.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwmechanics/creaturestats.hpp"

#include "../mwworld/class.hpp"

#include "steering.hpp"
#include "movement.hpp"

MWMechanics::AiActivate::AiActivate(const std::string &objectId)
    : mObjectId(objectId)
{
}
MWMechanics::AiActivate *MWMechanics::AiActivate::clone() const
{
    return new AiActivate(*this);
}
bool MWMechanics::AiActivate::execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
{
    ESM::Position pos = actor.getRefData().getPosition(); //position of the actor
    const MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtr(mObjectId, false); //The target to follow

    actor.getClass().getCreatureStats(actor).setDrawState(DrawState_Nothing);

    if(target == MWWorld::Ptr() ||
        !target.getRefData().getCount() || !target.getRefData().isEnabled()  // Really we should be checking whether the target is currently registered
                                                                            // with the MechanicsManager
            )
        return true;   //Target doesn't exist

    //Set the target destination for the actor
    ESM::Pathgrid::Point dest = target.getRefData().getPosition().pos;

    if (pathTo(actor, dest, duration, 200)) //Go to the destination
    {
        // activate when reached
        MWWorld::Ptr target = MWBase::Environment::get().getWorld()->getPtr(mObjectId,false);
        MWBase::Environment::get().getWorld()->activate(target, actor);

        return true;
    }

    return false;
}

int MWMechanics::AiActivate::getTypeId() const
{
    return TypeIdActivate;
}

void MWMechanics::AiActivate::writeState(ESM::AiSequence::AiSequence &sequence) const
{
    std::auto_ptr<ESM::AiSequence::AiActivate> activate(new ESM::AiSequence::AiActivate());
    activate->mTargetId = mObjectId;

    ESM::AiSequence::AiPackageContainer package;
    package.mType = ESM::AiSequence::Ai_Activate;
    package.mPackage = activate.release();
    sequence.mPackages.push_back(package);
}

MWMechanics::AiActivate::AiActivate(const ESM::AiSequence::AiActivate *activate)
    : mObjectId(activate->mTargetId)
{

}
