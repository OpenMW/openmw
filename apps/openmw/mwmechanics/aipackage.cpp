
#include "aipackage.hpp"

#include <cmath>
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"
#include "creaturestats.hpp"
#include "movement.hpp"
#include "../mwworld/action.hpp"

#include <OgreMath.h>

#include "steering.hpp"

MWMechanics::AiPackage::~AiPackage() {}

MWMechanics::AiPackage::AiPackage() : 
    mTimer(0.26f), mStuckTimer(0), //mTimer starts at .26 to force initial pathbuild
    mShortcutProhibited(false), mShortcutFailPos()
{
}


bool MWMechanics::AiPackage::pathTo(const MWWorld::Ptr& actor, const ESM::Pathgrid::Point& dest, float duration, float destTolerance)
{
    //Update various Timers
    mTimer += duration; //Update timer
    mStuckTimer += duration;   //Update stuck timer

    ESM::Position pos = actor.getRefData().getPosition(); //position of the actor

    /// Stops the actor when it gets too close to a unloaded cell
    const ESM::Cell *cell = actor.getCell()->getCell();
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
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
    /// Rebuilds path every [AI_REACTION_TIME] seconds, in case the target has moved
    //***********************

    bool isStuck = distance(start, mStuckPos.pos[0], mStuckPos.pos[1], mStuckPos.pos[2]) < actor.getClass().getSpeed(actor)*0.05 
        && distance(dest, start) > 20;

    float distToNextWaypoint = distance(start, dest);

    bool isDestReached = (distToNextWaypoint <= destTolerance);

    if (!isDestReached && mTimer > AI_REACTION_TIME)
    {
        bool needPathRecalc = doesPathNeedRecalc(dest, cell);

        bool isWayClear = true;

        if (!needPathRecalc) // TODO: add check if actor is actually shortcutting
        {
            isWayClear = checkWayIsClearForActor(start, dest, actor); // check if current shortcut is safe to follow
        }

        if (!isWayClear || needPathRecalc) // Only rebuild path if the target has moved or can't follow current shortcut
        {
            bool destInLOS = false;

            if (isStuck || !isWayClear || !shortcutPath(start, dest, actor, &destInLOS))
            {
                mPathFinder.buildSyncedPath(start, dest, actor.getCell());

                // give priority to go directly on target if there is minimal opportunity
                if (destInLOS && mPathFinder.getPath().size() > 1)
                {
                    // get point just before dest
                    std::list<ESM::Pathgrid::Point>::const_iterator pPointBeforeDest = mPathFinder.getPath().end();
                    --pPointBeforeDest;
                    --pPointBeforeDest;

                    // if start point is closer to the target then last point of path (excluding target itself) then go straight on the target
                    if (distance(start, dest) <= distance(dest, *pPointBeforeDest))
                    {
                        mPathFinder.clearPath();
                        mPathFinder.addPointToPath(dest);
                    }
                }
            }
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
    if (isDestReached || mPathFinder.checkPathCompleted(pos.pos[0],pos.pos[1])) //Path finished?
    {
        actor.getClass().getMovementSettings(actor).mPosition[0] = 0; // stop moving
        actor.getClass().getMovementSettings(actor).mPosition[1] = 0;
        actor.getClass().getMovementSettings(actor).mPosition[2] = 0;

        // turn to destination point
        zTurn(actor, Ogre::Degree(getZAngleToPoint(start, dest)));
        smoothTurn(actor, Ogre::Degree(getXAngleToPoint(start, dest)), 0);
        return true;
    }
    else if(mStuckTimer>0.5) //Every half second see if we need to take action to avoid something
    {
/// TODO (tluppi#1#): Use ObstacleCheck here. Not working for some reason
        //if(mObstacleCheck.check(actor, duration)) {
        if (isStuck) { //Actually stuck, and far enough away from destination to care
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
                actor.getClass().getMovementSettings(actor).mPosition[0] = 1;
                actor.getClass().getMovementSettings(actor).mPosition[1] = 1;
                // change the angle a bit, too
                zTurn(actor, Ogre::Degree(mPathFinder.getZAngleToNext(pos.pos[0] + 1, pos.pos[1])));
            }
        }
        else { //Not stuck, so reset things
            mStuckTimer = 0;
            mStuckPos = pos;
            actor.getClass().getMovementSettings(actor).mPosition[1] = 1; //Just run forward
        }
    }
    else {
        actor.getClass().getMovementSettings(actor).mPosition[1] = 1; //Just run forward the rest of the time
    }

    // turn to next path point by X,Z axes
    zTurn(actor, Ogre::Degree(mPathFinder.getZAngleToNext(pos.pos[0], pos.pos[1])));
    smoothTurn(actor, Ogre::Degree(mPathFinder.getXAngleToNext(pos.pos[0], pos.pos[1], pos.pos[2])), 0);

    return false;
}

bool MWMechanics::AiPackage::shortcutPath(const ESM::Pathgrid::Point& startPoint, const ESM::Pathgrid::Point& endPoint, const MWWorld::Ptr& actor, bool *destInLOS)
{
    const MWWorld::Class& actorClass = actor.getClass();
    MWBase::World* world = MWBase::Environment::get().getWorld();

    // check if actor can move along z-axis
    bool actorCanMoveByZ = (actorClass.canSwim(actor) && MWBase::Environment::get().getWorld()->isSwimming(actor))
        || world->isFlying(actor);

    // don't use pathgrid when actor can move in 3 dimensions
    bool isPathClear = actorCanMoveByZ;

    if (!isPathClear
        && (!mShortcutProhibited || (PathFinder::MakeOgreVector3(mShortcutFailPos) - PathFinder::MakeOgreVector3(startPoint)).length() >= PATHFIND_SHORTCUT_RETRY_DIST))
    {
        // take the direct path only if there aren't any obstacles
        isPathClear = !MWBase::Environment::get().getWorld()->castRay(
            static_cast<float>(startPoint.mX), static_cast<float>(startPoint.mY), static_cast<float>(startPoint.mZ),
            static_cast<float>(endPoint.mX), static_cast<float>(endPoint.mY), static_cast<float>(endPoint.mZ));

        if (destInLOS != NULL) *destInLOS = isPathClear;

        if (!isPathClear)
            return false;

        // check if an actor can move along the shortcut path
        isPathClear = checkWayIsClearForActor(startPoint, endPoint, actor);
    }

    if (isPathClear) // can shortcut the path
    {
        mPathFinder.clearPath();
        mPathFinder.addPointToPath(endPoint);
        return true;
    }

    return false;
}

bool MWMechanics::AiPackage::checkWayIsClearForActor(const ESM::Pathgrid::Point& startPoint, const ESM::Pathgrid::Point& endPoint, const MWWorld::Ptr& actor)
{
    bool actorCanMoveByZ = (actor.getClass().canSwim(actor) && MWBase::Environment::get().getWorld()->isSwimming(actor))
        || MWBase::Environment::get().getWorld()->isFlying(actor);

    if (actorCanMoveByZ)
        return true;

    float actorSpeed = actor.getClass().getSpeed(actor);
    float maxAvoidDist = AI_REACTION_TIME * actorSpeed + actorSpeed / MAX_VEL_ANGULAR.valueRadians() * 2; // *2 - for reliability
    Ogre::Real distToTarget = Ogre::Vector3(static_cast<float>(endPoint.mX), static_cast<float>(endPoint.mY), 0).length();

    float offsetXY = distToTarget > maxAvoidDist*1.5? maxAvoidDist : maxAvoidDist/2;

    bool isClear = checkWayIsClear(PathFinder::MakeOgreVector3(startPoint), PathFinder::MakeOgreVector3(endPoint), offsetXY);

    // update shortcut prohibit state
    if (isClear)
    {
        if (mShortcutProhibited)
        {
            mShortcutProhibited = false;
            mShortcutFailPos = ESM::Pathgrid::Point();
        }
    }
    if (!isClear)
    {
        if (mShortcutFailPos.mX == 0 && mShortcutFailPos.mY == 0 && mShortcutFailPos.mZ == 0)
        {
            mShortcutProhibited = true;
            mShortcutFailPos = startPoint;
        }
    }

    return isClear;
}

bool MWMechanics::AiPackage::doesPathNeedRecalc(ESM::Pathgrid::Point dest, const ESM::Cell *cell)
{
    bool needRecalc = distance(mPrevDest, dest) > 10;
    if (needRecalc) 
        mPrevDest = dest;

    return needRecalc;
}