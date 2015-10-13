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
#include "coordinateconverter.hpp"

MWMechanics::AiPackage::~AiPackage() {}

MWMechanics::AiPackage::AiPackage() : mTimer(0.26f) { //mTimer starts at .26 to force initial pathbuild

}


bool MWMechanics::AiPackage::pathTo(const MWWorld::Ptr& actor, ESM::Pathgrid::Point dest, float duration)
{
    //Update various Timers
    mTimer += duration; //Update timer

    ESM::Position pos = actor.getRefData().getPosition(); //position of the actor

    /// Stops the actor when it gets too close to a unloaded cell
    //... At current time, this test is unnecessary. AI shuts down when actor is more than 7168
    //... units from player, and exterior cells are 8192 units long and wide.
    //... But AI processing distance may increase in the future.
    if (isNearInactiveCell(pos))
    {
        actor.getClass().getMovementSettings(actor).mPosition[1] = 0;
        return false;
    }

    //***********************
    /// Checks if you can't get to the end position at all, adds end position to end of path
    /// Rebuilds path every quarter of a second, in case the target has moved
    //***********************
    if(mTimer > 0.25)
    {
        const ESM::Cell *cell = actor.getCell()->getCell();
        if (doesPathNeedRecalc(dest, cell)) { //Only rebuild path if it's moved
            mPathFinder.buildSyncedPath(pos.pos, dest, actor.getCell(), true); //Rebuild path, in case the target has moved
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
        evadeObstacles(actor, duration, pos);
    }
    return false;
}

void MWMechanics::AiPackage::evadeObstacles(const MWWorld::Ptr& actor, float duration, const ESM::Position& pos)
{
    zTurn(actor, mPathFinder.getZAngleToNext(pos.pos[0], pos.pos[1]));

    MWMechanics::Movement& movement = actor.getClass().getMovementSettings(actor);
    if (mObstacleCheck.check(actor, duration))
    {
        // first check if we're walking into a door
        MWWorld::Ptr door = getNearbyDoor(actor);
        if (door != MWWorld::Ptr()) // NOTE: checks interior cells only
        {
            if (!door.getCellRef().getTeleport() && door.getCellRef().getTrap().empty()
                    && door.getCellRef().getLockLevel() <= 0 && door.getClass().getDoorState(door) == 0) {
                MWBase::Environment::get().getWorld()->activateDoor(door, 1);
            }
        }
        else // probably walking into another NPC
        {
            mObstacleCheck.takeEvasiveAction(movement);
        }
    }
    else { //Not stuck, so reset things
        movement.mPosition[1] = 1; //Just run forward
    }
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

bool MWMechanics::AiPackage::isNearInactiveCell(const ESM::Position& actorPos)
{
    const ESM::Cell* playerCell(getPlayer().getCell()->getCell());
    if (playerCell->isExterior())
    {
        // get actor's distance from origin of center cell
        osg::Vec3f actorOffset(actorPos.asVec3());
        CoordinateConverter(playerCell).toLocal(actorOffset);

        // currently assumes 3 x 3 grid for exterior cells, with player at center cell.
        // ToDo: (Maybe) use "exterior cell load distance" setting to get count of actual active cells
        // While AI Process distance is 7168, AI shuts down actors before they reach edges of 3 x 3 grid.
        const float distanceFromEdge = 200.0;
        float minThreshold = (-1.0f * ESM::Land::REAL_SIZE) + distanceFromEdge;
        float maxThreshold = (2.0f * ESM::Land::REAL_SIZE) - distanceFromEdge;
        return (actorOffset[0] < minThreshold) || (maxThreshold < actorOffset[0])
            || (actorOffset[1] < minThreshold) || (maxThreshold < actorOffset[1]);
    }
    else
    {
        return false;
    }
}
