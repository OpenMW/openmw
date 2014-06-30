#include "aiavoiddoor.hpp"
#include <iostream>
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"
#include "creaturestats.hpp"
#include "movement.hpp"
#include "mechanicsmanagerimp.hpp"

#include <OgreMath.h>

#include "steering.hpp"

MWMechanics::AiAvoidDoor::AiAvoidDoor(const MWWorld::Ptr& doorPtr)
: AiPackage(), mDoorPtr(doorPtr), mDuration(1), mAdjAngle(0)
{

}

bool MWMechanics::AiAvoidDoor::execute (const MWWorld::Ptr& actor,float duration)
{

    ESM::Position pos = actor.getRefData().getPosition();
    if(mDuration == 1) //If it just started, get the actor position as the stuck detection thing
        mLastPos = pos;

    mDuration -= duration; //Update timer

    if(mDuration < 0) {
        float x = pos.pos[0] - mLastPos.pos[0];
        float y = pos.pos[1] - mLastPos.pos[1];
        float z = pos.pos[2] - mLastPos.pos[2];
        int distance = x * x + y * y + z * z;
        if(distance < 10 * 10) { //Got stuck, didn't move
            if(mAdjAngle == 0) //Try going in various directions
                mAdjAngle = 1.57079632679f; //pi/2
            else if (mAdjAngle == 1.57079632679f)
                mAdjAngle = -1.57079632679;
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
    float dirToDoor = std::atan2(x,y) + pos.rot[2] + mAdjAngle; //Calculates the direction to the door, relative to the direction of the NPC
                                                    // For example, if the NPC is directly facing the door this will be pi/2

    // Make actor move away from the door
    actor.getClass().getMovementSettings(actor).mPosition[1] = -1 * std::sin(dirToDoor); //I knew I'd use trig someday
    actor.getClass().getMovementSettings(actor).mPosition[0] = -1 * std::cos(dirToDoor);

    //Make all nearby actors also avoid the door
    std::vector<MWWorld::Ptr> actors;
    MWBase::Environment::get().getMechanicsManager()->getActorsInRange(Ogre::Vector3(pos.pos[0],pos.pos[1],pos.pos[2]),100,actors);
    for(std::vector<MWWorld::Ptr>::iterator it = actors.begin(); it != actors.end(); it++) {
        if(*it != MWBase::Environment::get().getWorld()->getPlayerPtr()) { //Not the player
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


