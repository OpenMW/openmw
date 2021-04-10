#include "aiavoiddoor.hpp"

#include <components/misc/rng.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/class.hpp"

#include "creaturestats.hpp"
#include "movement.hpp"
#include "actorutil.hpp"
#include "steering.hpp"

static const int MAX_DIRECTIONS = 4;

MWMechanics::AiAvoidDoor::AiAvoidDoor(const MWWorld::ConstPtr& doorPtr)
: mDuration(1), mDoorPtr(doorPtr), mDirection(0)
{

}

bool MWMechanics::AiAvoidDoor::execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
{

    ESM::Position pos = actor.getRefData().getPosition();
    if(mDuration == 1) //If it just started, get the actor position as the stuck detection thing
        mLastPos = pos.asVec3();

    mDuration -= duration; //Update timer

    if (mDuration < 0)
    {
        if (isStuck(pos.asVec3()))
        {
            adjustDirection();
            mDuration = 1; //reset timer
        }
        else
            return true; // We have tried backing up for more than one second, we've probably cleared it
    }

    if (mDoorPtr.getClass().getDoorState(mDoorPtr) == MWWorld::DoorState::Idle)
        return true; //Door is no longer opening

    ESM::Position tPos = mDoorPtr.getRefData().getPosition(); //Position of the door
    float x = pos.pos[1] - tPos.pos[1];
    float y = pos.pos[0] - tPos.pos[0];

    actor.getClass().getCreatureStats(actor).setMovementFlag(CreatureStats::Flag_Run, true);

    // Turn away from the door and move when turn completed
    if (zTurn(actor, std::atan2(y,x) + getAdjustedAngle(), osg::DegreesToRadians(5.f)))
        actor.getClass().getMovementSettings(actor).mPosition[1] = 1;
    else
        actor.getClass().getMovementSettings(actor).mPosition[1] = 0;
    actor.getClass().getMovementSettings(actor).mPosition[0] = 0;

    // Make all nearby actors also avoid the door
    std::vector<MWWorld::Ptr> actors;
    MWBase::Environment::get().getMechanicsManager()->getActorsInRange(pos.asVec3(),100,actors);
    for(auto& neighbor : actors)
    {
        if (neighbor == getPlayer())
            continue;

        MWMechanics::AiSequence& seq = neighbor.getClass().getCreatureStats(neighbor).getAiSequence();
        if (seq.getTypeId() != MWMechanics::AiPackageTypeId::AvoidDoor)
            seq.stack(MWMechanics::AiAvoidDoor(mDoorPtr), neighbor);
    }

    return false;
}

bool MWMechanics::AiAvoidDoor::isStuck(const osg::Vec3f& actorPos) const
{
    return (actorPos - mLastPos).length2() < 10 * 10;
}

void MWMechanics::AiAvoidDoor::adjustDirection()
{
    mDirection = Misc::Rng::rollDice(MAX_DIRECTIONS);
}

float MWMechanics::AiAvoidDoor::getAdjustedAngle() const
{
    return 2 * osg::PI / MAX_DIRECTIONS * mDirection;
}
