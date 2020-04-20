#include "aipackage.hpp"

#include <components/esm/loadcell.hpp>
#include <components/esm/loadland.hpp>
#include <components/esm/loadmgef.hpp>
#include <components/detournavigator/navigator.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/action.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwphysics/collisiontype.hpp"

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
    mTargetActorRefId(""),
    mTargetActorId(-1),
    mRotateOnTheRunChecks(0),
    mIsShortcutting(false),
    mShortcutProhibited(false),
    mShortcutFailPos()
{
}

MWWorld::Ptr MWMechanics::AiPackage::getTarget() const
{
    if (mTargetActorId == -2)
        return MWWorld::Ptr();

    if (mTargetActorId == -1)
    {
        MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtr(mTargetActorRefId, false);
        if (target.isEmpty())
        {
            mTargetActorId = -2;
            return target;
        }
        else
            mTargetActorId = target.getClass().getCreatureStats(target).getActorId();
    }

    if (mTargetActorId != -1)
        return MWBase::Environment::get().getWorld()->searchPtrViaActorId(mTargetActorId);
    else
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
    mShortcutFailPos = osg::Vec3f();

    mPathFinder.clearPath();
    mObstacleCheck.clear();
}

bool MWMechanics::AiPackage::pathTo(const MWWorld::Ptr& actor, const osg::Vec3f& dest, float duration, float destTolerance)
{
    mTimer += duration; //Update timer

    const osg::Vec3f position = actor.getRefData().getPosition().asVec3(); //position of the actor
    MWBase::World* world = MWBase::Environment::get().getWorld();

    const osg::Vec3f halfExtents = world->getHalfExtents(actor);

    /// Stops the actor when it gets too close to a unloaded cell
    //... At current time, this test is unnecessary. AI shuts down when actor is more than "actors processing range" setting value
    //... units from player, and exterior cells are 8192 units long and wide.
    //... But AI processing distance may increase in the future.
    if (isNearInactiveCell(position))
    {
        actor.getClass().getMovementSettings(actor).mPosition[1] = 0;
        world->updateActorPath(actor, mPathFinder.getPath(), halfExtents, position, dest);
        return false;
    }

    const float distToTarget = distance(position, dest);
    const bool isDestReached = (distToTarget <= destTolerance);

    if (!isDestReached && mTimer > AI_REACTION_TIME)
    {
        if (actor.getClass().isBipedal(actor))
            openDoors(actor);

        const bool wasShortcutting = mIsShortcutting;
        bool destInLOS = false;
        const bool actorCanMoveByZ = canActorMoveByZAxis(actor);

        // Prohibit shortcuts for AiWander, if the actor can not move in 3 dimensions.
        mIsShortcutting = actorCanMoveByZ
            && shortcutPath(position, dest, actor, &destInLOS, actorCanMoveByZ); // try to shortcut first

        if (!mIsShortcutting)
        {
            if (wasShortcutting || doesPathNeedRecalc(dest, actor)) // if need to rebuild path
            {
                const auto pathfindingHalfExtents = world->getPathfindingHalfExtents(actor);
                mPathFinder.buildPath(actor, position, dest, actor.getCell(), getPathGridGraph(actor.getCell()),
                    pathfindingHalfExtents, getNavigatorFlags(actor));
                mRotateOnTheRunChecks = 3;

                // give priority to go directly on target if there is minimal opportunity
                if (destInLOS && mPathFinder.getPath().size() > 1)
                {
                    // get point just before dest
                    auto pPointBeforeDest = mPathFinder.getPath().rbegin() + 1;

                    // if start point is closer to the target then last point of path (excluding target itself) then go straight on the target
                    if (distance(position, dest) <= distance(dest, *pPointBeforeDest))
                    {
                        mPathFinder.clearPath();
                        mPathFinder.addPointToPath(dest);
                    }
                }
            }

            if (!mPathFinder.getPath().empty()) //Path has points in it
            {
                const osg::Vec3f& lastPos = mPathFinder.getPath().back(); //Get the end of the proposed path

                if(distance(dest, lastPos) > 100) //End of the path is far from the destination
                    mPathFinder.addPointToPath(dest); //Adds the final destination to the path, to try to get to where you want to go
            }
        }

        mTimer = 0;
    }

    const float actorTolerance = 2 * actor.getClass().getSpeed(actor) * duration
            + 1.2 * std::max(halfExtents.x(), halfExtents.y());
    const float pointTolerance = std::max(MIN_TOLERANCE, actorTolerance);

    mPathFinder.update(position, pointTolerance, DEFAULT_TOLERANCE);

    if (isDestReached || mPathFinder.checkPathCompleted()) // if path is finished
    {
        // turn to destination point
        zTurn(actor, getZAngleToPoint(position, dest));
        smoothTurn(actor, getXAngleToPoint(position, dest), 0);
        world->removeActorPath(actor);
        return true;
    }

    world->updateActorPath(actor, mPathFinder.getPath(), halfExtents, position, dest);

    if (mRotateOnTheRunChecks == 0
        || isReachableRotatingOnTheRun(actor, *mPathFinder.getPath().begin())) // to prevent circling around a path point
    {
        actor.getClass().getMovementSettings(actor).mPosition[1] = 1; // move to the target
        if (mRotateOnTheRunChecks > 0) mRotateOnTheRunChecks--;
    }

    // turn to next path point by X,Z axes
    zTurn(actor, mPathFinder.getZAngleToNext(position.x(), position.y()));
    smoothTurn(actor, mPathFinder.getXAngleToNext(position.x(), position.y(), position.z()), 0);

    const auto destination = mPathFinder.getPath().empty() ? dest : mPathFinder.getPath().front();
    mObstacleCheck.update(actor, destination, duration);

    // handle obstacles on the way
    evadeObstacles(actor);

    return false;
}

void MWMechanics::AiPackage::evadeObstacles(const MWWorld::Ptr& actor)
{
    // check if stuck due to obstacles
    if (!mObstacleCheck.isEvading()) return;

    // first check if obstacle is a door
    static float distance = MWBase::Environment::get().getWorld()->getMaxActivationDistance();

    const MWWorld::Ptr door = getNearbyDoor(actor, distance);
    if (!door.isEmpty() && actor.getClass().isBipedal(actor))
    {
        openDoors(actor);
    }
    else
    {
        mObstacleCheck.takeEvasiveAction(actor.getClass().getMovementSettings(actor));
    }
}

namespace
{
    bool isDoorOnTheWay(const MWWorld::Ptr& actor, const MWWorld::Ptr& door, const osg::Vec3f& nextPathPoint)
    {
        const auto world = MWBase::Environment::get().getWorld();
        const auto halfExtents = world->getHalfExtents(actor);
        const auto position = actor.getRefData().getPosition().asVec3() + osg::Vec3f(0, 0, halfExtents.z());
        const auto destination = nextPathPoint + osg::Vec3f(0, 0, halfExtents.z());

        return world->hasCollisionWithDoor(door, position, destination);
    }
}

void MWMechanics::AiPackage::openDoors(const MWWorld::Ptr& actor)
{
    // note: AiWander currently does not open doors
    if (getTypeId() == TypeIdWander)
        return;

    if (mPathFinder.getPathSize() == 0)
        return;

    MWBase::World* world = MWBase::Environment::get().getWorld();
    static float distance = world->getMaxActivationDistance();

    const MWWorld::Ptr door = getNearbyDoor(actor, distance);
    if (door == MWWorld::Ptr())
        return;

    if (!door.getCellRef().getTeleport() && door.getClass().getDoorState(door) == MWWorld::DoorState::Idle)
    {
        if (!isDoorOnTheWay(actor, door, mPathFinder.getPath().front()))
            return;

        if ((door.getCellRef().getTrap().empty() && door.getCellRef().getLockLevel() <= 0 ))
        {
            world->activate(door, actor);
            return;
        }

        const std::string keyId = door.getCellRef().getKey();
        if (keyId.empty())
            return;

        MWWorld::ContainerStore &invStore = actor.getClass().getContainerStore(actor);
        MWWorld::Ptr keyPtr = invStore.search(keyId);

        if (!keyPtr.isEmpty())
            world->activate(door, actor);
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
        cache.insert(std::make_pair(id, std::make_unique<MWMechanics::PathgridGraph>(MWMechanics::PathgridGraph(cell))));
    }
    return *cache[id].get();
}

bool MWMechanics::AiPackage::shortcutPath(const osg::Vec3f& startPoint, const osg::Vec3f& endPoint,
        const MWWorld::Ptr& actor, bool *destInLOS, bool isPathClear)
{
    if (!mShortcutProhibited || (mShortcutFailPos - startPoint).length() >= PATHFIND_SHORTCUT_RETRY_DIST)
    {
        // check if target is clearly visible
        isPathClear = !MWBase::Environment::get().getWorld()->castRay(
            startPoint.x(), startPoint.y(), startPoint.z(),
            endPoint.x(), endPoint.y(), endPoint.z());

        if (destInLOS != nullptr) *destInLOS = isPathClear;

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

bool MWMechanics::AiPackage::checkWayIsClearForActor(const osg::Vec3f& startPoint, const osg::Vec3f& endPoint, const MWWorld::Ptr& actor)
{
    if (canActorMoveByZAxis(actor))
        return true;

    const float actorSpeed = actor.getClass().getSpeed(actor);
    const float maxAvoidDist = AI_REACTION_TIME * actorSpeed + actorSpeed / getAngularVelocity(actorSpeed) * 2; // *2 - for reliability
    const float distToTarget = osg::Vec2f(endPoint.x(), endPoint.y()).length();

    const float offsetXY = distToTarget > maxAvoidDist*1.5? maxAvoidDist : maxAvoidDist/2;

    // update shortcut prohibit state
    if (checkWayIsClear(startPoint, endPoint, offsetXY))
    {
        if (mShortcutProhibited)
        {
            mShortcutProhibited = false;
            mShortcutFailPos = osg::Vec3f();
        }
        return true;
    }
    else
    {
        if (mShortcutFailPos == osg::Vec3f())
        {
            mShortcutProhibited = true;
            mShortcutFailPos = startPoint;
        }
    }

    return false;
}

bool MWMechanics::AiPackage::doesPathNeedRecalc(const osg::Vec3f& newDest, const MWWorld::Ptr& actor) const
{
    return mPathFinder.getPath().empty()
        || getPathDistance(actor, mPathFinder.getPath().back(), newDest) > 10
        || mPathFinder.getPathCell() != actor.getCell();
}

bool MWMechanics::AiPackage::isNearInactiveCell(osg::Vec3f position)
{
    const ESM::Cell* playerCell(getPlayer().getCell()->getCell());
    if (playerCell->isExterior())
    {
        // get actor's distance from origin of center cell
        CoordinateConverter(playerCell).toLocal(position);

        // currently assumes 3 x 3 grid for exterior cells, with player at center cell.
        // ToDo: (Maybe) use "exterior cell load distance" setting to get count of actual active cells
        // AI shuts down actors before they reach edges of 3 x 3 grid.
        const float distanceFromEdge = 200.0;
        float minThreshold = (-1.0f * ESM::Land::REAL_SIZE) + distanceFromEdge;
        float maxThreshold = (2.0f * ESM::Land::REAL_SIZE) - distanceFromEdge;
        return (position.x() < minThreshold) || (maxThreshold < position.x())
            || (position.y() < minThreshold) || (maxThreshold < position.y());
    }
    else
    {
        return false;
    }
}

bool MWMechanics::AiPackage::isReachableRotatingOnTheRun(const MWWorld::Ptr& actor, const osg::Vec3f& dest)
{
    // get actor's shortest radius for moving in circle
    float speed = actor.getClass().getSpeed(actor);
    speed += speed * 0.1f; // 10% real speed inaccuracy
    float radius = speed / getAngularVelocity(speed);

    // get radius direction to the center
    const float* rot = actor.getRefData().getPosition().rot;
    osg::Quat quatRot(rot[0], -osg::X_AXIS, rot[1], -osg::Y_AXIS, rot[2], -osg::Z_AXIS);
    osg::Vec3f dir = quatRot * osg::Y_AXIS; // actor's orientation direction is a tangent to circle
    osg::Vec3f radiusDir = dir ^ osg::Z_AXIS; // radius is perpendicular to a tangent
    radiusDir.normalize();
    radiusDir *= radius;

    // pick up the nearest center candidate
    osg::Vec3f pos = actor.getRefData().getPosition().asVec3();
    osg::Vec3f center1 = pos - radiusDir;
    osg::Vec3f center2 = pos + radiusDir;
    osg::Vec3f center = (center1 - dest).length2() < (center2 - dest).length2() ? center1 : center2;

    float distToDest = (center - dest).length();

    // if pathpoint is reachable for the actor rotating on the run:
    // no points of actor's circle should be farther from the center than destination point
    return (radius <= distToDest);
}

DetourNavigator::Flags MWMechanics::AiPackage::getNavigatorFlags(const MWWorld::Ptr& actor) const
{
    const MWWorld::Class& actorClass = actor.getClass();
    DetourNavigator::Flags result = DetourNavigator::Flag_none;

    if (actorClass.isPureWaterCreature(actor) || (getTypeId() != TypeIdWander && actorClass.canSwim(actor)))
        result |= DetourNavigator::Flag_swim;

    if (actorClass.canWalk(actor))
        result |= DetourNavigator::Flag_walk;

    if (actorClass.isBipedal(actor) && getTypeId() != TypeIdWander)
        result |= DetourNavigator::Flag_openDoor;

    return result;
}
