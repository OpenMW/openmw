#include "aiavoiddoor.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwworld/class.hpp"
#include "creaturestats.hpp"
#include "movement.hpp"
#include "actorutil.hpp"


#include "steering.hpp"

MWMechanics::AiAvoidDoor::AiAvoidDoor(const MWWorld::ConstPtr& doorPtr)
: AiPackage(), mDuration(1), mDoorPtr(doorPtr), mAdjAngle(0)
{

}

bool MWMechanics::AiAvoidDoor::execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
{

    ESM::Position pos = actor.getRefData().getPosition();
    if(mDuration == 1) //If it just started, get the actor position as the stuck detection thing
        mLastPos = pos;

    mDuration -= duration; //Update timer

    if(mDuration < 0) {
        float x = pos.pos[0] - mLastPos.pos[0];
        float y = pos.pos[1] - mLastPos.pos[1];
        float z = pos.pos[2] - mLastPos.pos[2];
        float distance = x * x + y * y + z * z;
        if(distance < 10 * 10) { //Got stuck, didn't move
            if(mAdjAngle == 0) //Try going in various directions
                mAdjAngle = 1.57079632679f; //pi/2
            else if (mAdjAngle == 1.57079632679f)
                mAdjAngle = -1.57079632679f;
            else
                mAdjAngle = 0;
            mDuration = 1; //reset timer
        }
        else //Not stuck
            return true; // We have tried backing up for more than one second, we've probably cleared it
    }

    if (!mDoorPtr.getClass().getDoorState(mDoorPtr))
        return true; //Door is no longer opening

    ESM::Position tPos = mDoorPtr.getRefData().getPosition(); //Position of the door
    float x = pos.pos[0] - tPos.pos[0];
    float y = pos.pos[1] - tPos.pos[1];

    actor.getClass().getCreatureStats(actor).setMovementFlag(CreatureStats::Flag_Run, true);

    // Turn away from the door and move when turn completed
    if (zTurn(actor, std::atan2(x,y) + mAdjAngle, osg::DegreesToRadians(5.f)))
        actor.getClass().getMovementSettings(actor).mPosition[1] = 1;
    else
        actor.getClass().getMovementSettings(actor).mPosition[1] = 0;
    actor.getClass().getMovementSettings(actor).mPosition[0] = 0;

    // Make all nearby actors also avoid the door
    std::vector<MWWorld::Ptr> actors;
    MWBase::Environment::get().getMechanicsManager()->getActorsInRange(pos.asVec3(),100,actors);
    for(std::vector<MWWorld::Ptr>::iterator it = actors.begin(); it != actors.end(); ++it) {
        if(*it != getPlayer()) { //Not the player
            MWMechanics::AiSequence& seq = it->getClass().getCreatureStats(*it).getAiSequence();
            if(seq.getTypeId() != MWMechanics::AiPackage::TypeIdAvoidDoor) { //Only add it once
                seq.stack(MWMechanics::AiAvoidDoor(mDoorPtr),*it);
            }
        }
    }

    return false;
}

MWMechanics::AiAvoidDoor *MWMechanics::AiAvoidDoor::clone() const
{
    return new AiAvoidDoor(*this);
}

int MWMechanics::AiAvoidDoor::getTypeId() const
{
    return TypeIdAvoidDoor;
}

unsigned int MWMechanics::AiAvoidDoor::getPriority() const
{
 return 2;
}


