#include "aiavoiddoor.hpp"
#include <iostream>
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"
#include "creaturestats.hpp"
#include "movement.hpp"

#include <OgreMath.h>

#include "steering.hpp"

MWMechanics::AiAvoidDoor::AiAvoidDoor(const MWWorld::Ptr& doorPtr)
: AiPackage(), mDoorPtr(doorPtr), mDuration(1)
{
}

bool MWMechanics::AiAvoidDoor::execute (const MWWorld::Ptr& actor,float duration)
{
    mDuration -= duration; //Update timer

    if(mDuration < 0)
        return true; // We have tried backing up for more than one second, we've probably cleared it

    if(!MWBase::Environment::get().getWorld()->getIsMovingDoor(mDoorPtr))
        return true; //Door is no longer opening

    ESM::Position pos = actor.getRefData().getPosition(); //position of the actor
    ESM::Position tPos = mDoorPtr.getRefData().getPosition(); //Position of the door
    float x = pos.pos[0] - tPos.pos[0];
    float y = pos.pos[1] - tPos.pos[1];
    float z = pos.pos[2] - tPos.pos[2];
    int distance = sqrt(x * x + y * y + z * z);

    if(distance > 300) //Stop backing up when you're far enough away
        return true;
/// TODO: Calculate this from door size, not have it built in

    float dirToDoor = std::atan2(x,y) + pos.rot[2]; //Calculates the direction to the door, relative to the direction of the NPC
                                                    // For example, if the NPC is directly facing the door this will be pi/2

    // Make actor move away from the door
    actor.getClass().getMovementSettings(actor).mPosition[1] = -1 * std::sin(dirToDoor); //I knew I'd use trig someday
    actor.getClass().getMovementSettings(actor).mPosition[0] = -1 * std::cos(dirToDoor);

    return false;
}

std::string MWMechanics::AiAvoidDoor::getAvoidedDoor()
{
    return mDoorPtr.getCellRef().mRefID;
}

MWMechanics::AiAvoidDoor *MWMechanics::AiAvoidDoor::clone() const
{
    return new AiAvoidDoor(*this);
}

 int MWMechanics::AiAvoidDoor::getTypeId() const
{
    return TypeIdAvoidDoor;
}

