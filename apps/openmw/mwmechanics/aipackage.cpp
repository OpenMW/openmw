#include "aipackage.hpp"

#include <cmath>

#include <components/esm/loadcell.hpp>
#include <components/esm/loadland.hpp>
#include <components/esm/loadmgef.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"
#include "creaturestats.hpp"
#include "movement.hpp"
#include "../mwworld/action.hpp"

#include "steering.hpp"
#include "actorutil.hpp"

MWMechanics::AiPackage::~AiPackage() {}

MWMechanics::AiPackage::AiPackage() : mTimer(0.26f) { //mTimer starts at .26 to force initial pathbuild

}


bool MWMechanics::AiPackage::pathTo(const MWWorld::Ptr& actor, ESM::Pathgrid::Point dest, float duration)
{
    //Update various Timers
    mTimer += duration; //Update timer

    ESM::Position pos = actor.getRefData().getPosition(); //position of the actor

    /// Stops the actor when it gets too close to a unloaded cell
    const ESM::Cell *cell = actor.getCell()->getCell();
    {
        MWWorld::Ptr player = getPlayer();
        Movement &movement = actor.getClass().getMovementSettings(actor);

        //Ensure pursuer doesn't leave loaded cells
        if(cell->mData.mX != player.getCell()->getCell()->mData.mX)
        {
            int sideX = PathFinder::sgn(cell->mData.mX - player.getCell()->getCell()->mData.mX);
            //check if actor is near the border of an inactive cell. If so, stop walking.
            if(sideX * (pos.pos[0] - cell->mData.mX*ESM::Land::REAL_SIZE) > sideX * (ESM::Land::REAL_SIZE/2.0f - 200.0f))
            {
                movement.mPosition[1] = 0;
                return false;
            }
        }
        if(cell->mData.mY != player.getCell()->getCell()->mData.mY)
        {
            int sideY = PathFinder::sgn(cell->mData.mY - player.getCell()->getCell()->mData.mY);
            //check if actor is near the border of an inactive cell. If so, stop walking.
            if(sideY * (pos.pos[1] - cell->mData.mY*ESM::Land::REAL_SIZE) > sideY * (ESM::Land::REAL_SIZE/2.0f - 200.0f))
            {
                movement.mPosition[1] = 0;
                return false;
            }
        }
    }

    //Start position
    ESM::Pathgrid::Point start = pos.pos;

    //***********************
    /// Checks if you can't get to the end position at all, adds end position to end of path
    /// Rebuilds path every quarter of a second, in case the target has moved
    //***********************
    if(mTimer > 0.25)
    {
        if (doesPathNeedRecalc(dest, cell)) { //Only rebuild path if it's moved
            mPathFinder.buildSyncedPath(start, dest, actor.getCell(), true); //Rebuild path, in case the target has moved
            mPrevDest = dest;
        }

        if(!mPathFinder.getPath().empty()) //Path has points in it
        {
            ESM::Pathgrid::Point lastPos = mPathFinder.getPath().back(); //Get the end of the proposed path

            if(distance(dest, lastPos) > 100) //End of the path is far from the destination
                mPathFinder.addPointToPath(dest); //Adds the final destination to the path, to try to get to where you want to go
        }

        mTimer = 0;
    }

    //************************
    /// Checks if you aren't moving; attempts to unstick you
    //************************
    if(mPathFinder.checkPathCompleted(pos.pos[0],pos.pos[1])) //Path finished?
        return true;
    else
    {
        zTurn(actor, mPathFinder.getZAngleToNext(pos.pos[0], pos.pos[1]));

        MWMechanics::Movement& movement = actor.getClass().getMovementSettings(actor);
        if(mObstacleCheck.check(actor, duration))
        {
            // first check if we're walking into a door
            MWWorld::Ptr door = getNearbyDoor(actor);
            if(door != MWWorld::Ptr()) // NOTE: checks interior cells only
            {
                if(!door.getCellRef().getTeleport() && door.getCellRef().getTrap().empty() && door.getClass().getDoorState(door) == 0) { //Open the door if untrapped
                    MWBase::Environment::get().getWorld()->activateDoor(door, 1);
                }
            }
            else // probably walking into another NPC
            {
                movement.mPosition[0] = 1;
                movement.mPosition[1] = 1;
                // change the angle a bit, too
                zTurn(actor, mPathFinder.getZAngleToNext(pos.pos[0] + 1, pos.pos[1]));
            }
        }
        else { //Not stuck, so reset things
            movement.mPosition[1] = 1; //Just run forward
        }
    }
    return false;
}

bool MWMechanics::AiPackage::doesPathNeedRecalc(ESM::Pathgrid::Point dest, const ESM::Cell *cell)
{
    return distance(mPrevDest, dest) > 10;
}

bool MWMechanics::AiPackage::isTargetMagicallyHidden(const MWWorld::Ptr& target)
{
    const MagicEffects& magicEffects(target.getClass().getCreatureStats(target).getMagicEffects());
    return (magicEffects.get(ESM::MagicEffect::Invisibility).getMagnitude() > 0)
        || (magicEffects.get(ESM::MagicEffect::Chameleon).getMagnitude() > 75);
}
