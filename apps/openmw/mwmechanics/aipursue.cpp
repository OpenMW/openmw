#include "aipursue.hpp"

#include <components/esm/aisequence.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/action.hpp"

#include "movement.hpp"
#include "creaturestats.hpp"
#include "combat.hpp"

namespace MWMechanics
{

AiPursue::AiPursue(const MWWorld::Ptr& actor)
{
    mTargetActorId = actor.getClass().getCreatureStats(actor).getActorId();
}

AiPursue::AiPursue(const ESM::AiSequence::AiPursue *pursue)
{
    mTargetActorId = pursue->mTargetActorId;
}

AiPursue *MWMechanics::AiPursue::clone() const
{
    return new AiPursue(*this);
}
bool AiPursue::execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
{
    if(actor.getClass().getCreatureStats(actor).isDead())
        return true;

    const MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtrViaActorId(mTargetActorId); //The target to follow

    // Stop if the target doesn't exist
    // Really we should be checking whether the target is currently registered with the MechanicsManager
    if (target == MWWorld::Ptr() || !target.getRefData().getCount() || !target.getRefData().isEnabled())
        return true;

    if (isTargetMagicallyHidden(target))
        return true;

    if (target.getClass().getCreatureStats(target).isDead())
        return true;

    actor.getClass().getCreatureStats(actor).setDrawState(DrawState_Nothing);

    //Set the target destination
    const osg::Vec3f dest = target.getRefData().getPosition().asVec3();
    const osg::Vec3f actorPos = actor.getRefData().getPosition().asVec3();

    const float pathTolerance = 100.f;

    if (pathTo(actor, dest, duration, pathTolerance) &&
        std::abs(dest.z() - actorPos.z()) < pathTolerance) // check the true distance in case the target is far away in Z-direction
    {
        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Dialogue, actor); //Arrest player when reached
        return true;
    }

    actor.getClass().getCreatureStats(actor).setMovementFlag(MWMechanics::CreatureStats::Flag_Run, true); //Make NPC run

    return false;
}

int AiPursue::getTypeId() const
{
    return TypeIdPursue;
}

MWWorld::Ptr AiPursue::getTarget() const
{
    return MWBase::Environment::get().getWorld()->searchPtrViaActorId(mTargetActorId);
}

void AiPursue::writeState(ESM::AiSequence::AiSequence &sequence) const
{
    std::unique_ptr<ESM::AiSequence::AiPursue> pursue(new ESM::AiSequence::AiPursue());
    pursue->mTargetActorId = mTargetActorId;

    ESM::AiSequence::AiPackageContainer package;
    package.mType = ESM::AiSequence::Ai_Pursue;
    package.mPackage = pursue.release();
    sequence.mPackages.push_back(package);
}

} // namespace MWMechanics
