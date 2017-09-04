#include "aipursue.hpp"

#include <components/esm/aisequence.hpp>
#include <components/esm/loadmgef.hpp>

#include "../mwbase/environment.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/action.hpp"

/*
    Start of tes3mp addition

    Include additional headers for multiplayer purposes
*/
#include <components/openmw-mp/Log.hpp>
#include "../mwgui/windowmanagerimp.hpp"
#include "../mwmp/Main.hpp"
#include "../mwmp/LocalPlayer.hpp"
/*
    End of tes3mp addition
*/

#include "movement.hpp"
#include "creaturestats.hpp"

namespace MWMechanics
{

AiPursue::AiPursue(const MWWorld::Ptr& actor)
    : mTargetActorId(actor.getClass().getCreatureStats(actor).getActorId())
{
}

AiPursue::AiPursue(const ESM::AiSequence::AiPursue *pursue)
    : mTargetActorId(pursue->mTargetActorId)
{
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

    if(target == MWWorld::Ptr() || !target.getRefData().getCount() || !target.getRefData().isEnabled()  // Really we should be checking whether the target is currently registered
                                                                                                        // with the MechanicsManager
            )
        return true; //Target doesn't exist

    if (isTargetMagicallyHidden(target))
        return true;

    if(target.getClass().getCreatureStats(target).isDead())
        return true;

    /*
        Start of tes3mp addition

        Because multiplayer does not pause the game, prevent infinite arrest loops by ignoring
        players already engaged in dialogue
    */
    if (target == MWBase::Environment::get().getWorld()->getPlayerPtr())
    {
        if (MWBase::Environment::get().getWindowManager()->containsMode(MWGui::GM_Dialogue))
        {
            return true;
        }
    }
    /*
        End of tes3mp addition
    */

    actor.getClass().getCreatureStats(actor).setDrawState(DrawState_Nothing);

    //Set the target desition from the actor
    ESM::Pathgrid::Point dest = target.getRefData().getPosition().pos;


    if (pathTo(actor, dest, duration, 100)) {
        target.getClass().activate(target,actor).get()->execute(actor); //Arrest player when reached

        /*
            Start of tes3mp addition

            Record that the player has not died since the last attempt to arrest them
        */
        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "After being pursued by %s, diedSinceArrestAttempt is now false", actor.getCellRef().getRefId().c_str());
        mwmp::Main::get().getLocalPlayer()->diedSinceArrestAttempt = false;
        /*
            End of tes3mp addition
        */

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
