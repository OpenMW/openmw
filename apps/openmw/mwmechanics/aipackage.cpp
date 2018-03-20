#include "aipackage.hpp"

#include <cmath>

#include <components/esm/loadcell.hpp>
#include <components/esm/loadland.hpp>
#include <components/esm/loadmgef.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/action.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "pathgrid.hpp"
#include "creaturestats.hpp"
#include "movement.hpp"
#include "steering.hpp"
#include "actorutil.hpp"
#include "coordinateconverter.hpp"

#include <osg/Quat>

MWMechanics::AiPackage::~AiPackage() {}

MWMechanics::AiPackage::AiPackage() : 
    mTimer(AI_REACTION_TIME + 1.0f), // to force initial pathbuild
    mRotateOnTheRunChecks(0),
    mIsShortcutting(false),
    mShortcutProhibited(false), mShortcutFailPos()
{
}

MWWorld::Ptr MWMechanics::AiPackage::getTarget() const
{
    return MWWorld::Ptr();
}

bool MWMechanics::AiPackage::sideWithTarget() const
{
    return false;
}

bool MWMechanics::AiPackage::followTargetThroughDoors() const
{
    return false;
}

bool MWMechanics::AiPackage::canCancel() const
{
    return true;
}

bool MWMechanics::AiPackage::shouldCancelPreviousAi() const
{
    return true;
}

bool MWMechanics::AiPackage::getRepeat() const
{
    return false;
}

void MWMechanics::AiPackage::reset()
{
    // reset all members
    mTimer = AI_REACTION_TIME + 1.0f;
    mIsShortcutting = false;
    mShortcutProhibited = false;
    mShortcutFailPos = ESM::Pathgrid::Point();

    mPathFinder.clearPath();
    mObstacleCheck.clear();
}

bool MWMechanics::AiPackage::pathTo(const MWWorld::Ptr& actor, const ESM::Pathgrid::Point& dest, float duration, float destTolerance)
{
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

    // handle path building and shortcutting
    ESM::Pathgrid::Point start = pos.pos;

    float distToTarget = distance(start, dest);
    bool isDestReached = (distToTarget <= destTolerance);

    if (!isDestReached && mTimer > AI_REACTION_TIME)
    {
        bool wasShortcutting = mIsShortcutting;
        bool destInLOS = false;

        const MWWorld::Class& actorClass = actor.getClass();
        MWBase::World* world = MWBase::Environment::get().getWorld();

        // check if actor can move along z-axis
        bool actorCanMoveByZ = (actorClass.canSwim(actor) && MWBase::Environment::get().getWorld()->isSwimming(actor))
            || world->isFlying(actor);

        // Prohibit shortcuts for AiWander, if the actor can not move in 3 dimensions.
        if (actorCanMoveByZ || getTypeId() != TypeIdWander)
            mIsShortcutting = shortcutPath(start, dest, actor, &destInLOS, actorCanMoveByZ); // try to shortcut first

        if (!mIsShortcutting)
        {
            if (wasShortcutting || doesPathNeedRecalc(dest, actor.getCell())) // if need to rebuild path
            {
                mPathFinder.buildSyncedPath(start, dest, actor.getCell(), getPathGridGraph(actor.getCell()));
                mRotateOnTheRunChecks = 3;

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

            if (!mPathFinder.getPath().empty()) //Path has points in it
            {
                ESM::Pathgrid::Point lastPos = mPathFinder.getPath().back(); //Get the end of the proposed path

                if(distance(dest, lastPos) > 100) //End of the path is far from the destination
                    mPathFinder.addPointToPath(dest); //Adds the final destination to the path, to try to get to where you want to go
            }
        }

        mTimer = 0;
    }

    if (isDestReached || mPathFinder.checkPathCompleted(pos.pos[0], pos.pos[1])) // if path is finished
    {
        // turn to destination point
        zTurn(actor, getZAngleToPoint(start, dest));
        smoothTurn(actor, getXAngleToPoint(start, dest), 0);
        return true;
    }
    else
    {
        if (mRotateOnTheRunChecks == 0
            || isReachableRotatingOnTheRun(actor, *mPathFinder.getPath().begin())) // to prevent circling around a path point
        {
            actor.getClass().getMovementSettings(actor).mPosition[1] = 1; // move to the target
            if (mRotateOnTheRunChecks > 0) mRotateOnTheRunChecks--;
        }

        // handle obstacles on the way
        evadeObstacles(actor, duration, pos);
    }

    // turn to next path point by X,Z axes
    zTurn(actor, mPathFinder.getZAngleToNext(pos.pos[0], pos.pos[1]));
    smoothTurn(actor, mPathFinder.getXAngleToNext(pos.pos[0], pos.pos[1], pos.pos[2]), 0);

    return false;
}

void MWMechanics::AiPackage::evadeObstacles(const MWWorld::Ptr& actor, float duration, const ESM::Position& pos)
{
    zTurn(actor, mPathFinder.getZAngleToNext(pos.pos[0], pos.pos[1]));

    MWMechanics::Movement& movement = actor.getClass().getMovementSettings(actor);

    // check if stuck due to obstacles
    if (!mObstacleCheck.check(actor, duration)) return;

    // first check if obstacle is a door
    static float distance = MWBase::Environment::get().getWorld()->getMaxActivationDistance();

    MWWorld::Ptr door = getNearbyDoor(actor, distance);
    if (door != MWWorld::Ptr() && actor.getClass().isBipedal(actor))
    {
        // note: AiWander currently does not open doors
        if (getTypeId() != TypeIdWander && !door.getCellRef().getTeleport() && door.getClass().getDoorState(door) == 0)
        {
            if ((door.getCellRef().getTrap().empty() && door.getCellRef().getLockLevel() <= 0 ))
            {
                MWBase::Environment::get().getWorld()->activate(door, actor);
                return;
            }

            std::string keyId = door.getCellRef().getKey();
            if (keyId.empty())
                return;

            bool hasKey = false;
            const MWWorld::ContainerStore &invStore = actor.getClass().getContainerStore(actor);

            // make key id lowercase
            Misc::StringUtils::lowerCaseInPlace(keyId);
            for (MWWorld::ConstContainerStoreIterator it = invStore.cbegin(); it != invStore.cend(); ++it)
            {
                std::string refId = it->getCellRef().getRefId();
                Misc::StringUtils::lowerCaseInPlace(refId);
                if (refId == keyId)
                {
                    hasKey = true;
                    break;
                }
            }

            if (hasKey)
                MWBase::Environment::get().getWorld()->activate(door, actor);
        }
    }
    else
    {
        mObstacleCheck.takeEvasiveAction(movement);
    }
}

const MWMechanics::PathgridGraph& MWMechanics::AiPackage::getPathGridGraph(const MWWorld::CellStore *cell)
{
    const ESM::CellId& id = cell->getCell()->getCellId();
    // static cache is OK for now, pathgrids can never change during runtime
    typedef std::map<ESM::CellId, std::unique_ptr<MWMechanics::PathgridGraph> > CacheMap;
    static CacheMap cache;
    CacheMap::iterator found = cache.find(id);
    if (found == cache.end())
    {
        cache.insert(std::make_pair(id, std::unique_ptr<MWMechanics::PathgridGraph>(new MWMechanics::PathgridGraph(cell))));
    }
    return *cache[id].get();
}

bool MWMechanics::AiPackage::shortcutPath(const ESM::Pathgrid::Point& startPoint, const ESM::Pathgrid::Point& endPoint, const MWWorld::Ptr& actor, bool *destInLOS, bool isPathClear)
{
    if (!mShortcutProhibited || (PathFinder::MakeOsgVec3(mShortcutFailPos) - PathFinder::MakeOsgVec3(startPoint)).length() >= PATHFIND_SHORTCUT_RETRY_DIST)
    {
        // check if target is clearly visible
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
    float maxAvoidDist = AI_REACTION_TIME * actorSpeed + actorSpeed / MAX_VEL_ANGULAR_RADIANS * 2; // *2 - for reliability
    osg::Vec3f::value_type distToTarget = osg::Vec3f(static_cast<float>(endPoint.mX), static_cast<float>(endPoint.mY), 0).length();

    float offsetXY = distToTarget > maxAvoidDist*1.5? maxAvoidDist : maxAvoidDist/2;

    bool isClear = checkWayIsClear(PathFinder::MakeOsgVec3(startPoint), PathFinder::MakeOsgVec3(endPoint), offsetXY);

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

bool MWMechanics::AiPackage::doesPathNeedRecalc(const ESM::Pathgrid::Point& newDest, const MWWorld::CellStore* currentCell)
{
    return mPathFinder.getPath().empty() || (distance(mPathFinder.getPath().back(), newDest) > 10) || mPathFinder.getPathCell() != currentCell;
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

bool MWMechanics::AiPackage::isReachableRotatingOnTheRun(const MWWorld::Ptr& actor, const ESM::Pathgrid::Point& dest)
{
    // get actor's shortest radius for moving in circle
    float speed = actor.getClass().getSpeed(actor);
    speed += speed * 0.1f; // 10% real speed inaccuracy
    float radius = speed / MAX_VEL_ANGULAR_RADIANS;

    // get radius direction to the center
    const float* rot = actor.getRefData().getPosition().rot;
    osg::Quat quatRot(rot[0], -osg::X_AXIS, rot[1], -osg::Y_AXIS, rot[2], -osg::Z_AXIS);
    osg::Vec3f dir = quatRot * osg::Y_AXIS; // actor's orientation direction is a tangent to circle
    osg::Vec3f radiusDir = dir ^ osg::Z_AXIS; // radius is perpendicular to a tangent
    radiusDir.normalize();
    radiusDir *= radius;
    
    // pick up the nearest center candidate
    osg::Vec3f dest_ = PathFinder::MakeOsgVec3(dest);
    osg::Vec3f pos = actor.getRefData().getPosition().asVec3();
    osg::Vec3f center1 = pos - radiusDir;
    osg::Vec3f center2 = pos + radiusDir;
    osg::Vec3f center = (center1 - dest_).length2() < (center2 - dest_).length2() ? center1 : center2;

    float distToDest = (center - dest_).length();

    // if pathpoint is reachable for the actor rotating on the run:
    // no points of actor's circle should be farther from the center than destination point
    return (radius <= distToDest);
}
