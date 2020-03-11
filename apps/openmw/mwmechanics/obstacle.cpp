#include "obstacle.hpp"

#include <components/sceneutil/positionattitudetransform.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"

#include "movement.hpp"

namespace MWMechanics
{
    // NOTE: determined empirically but probably need further tweaking
    static const float DIST_SAME_SPOT = 0.5f;
    static const float DURATION_SAME_SPOT = 1.5f;
    static const float DURATION_TO_EVADE = 0.4f;

    const float ObstacleCheck::evadeDirections[NUM_EVADE_DIRECTIONS][2] =
    {
        { 1.0f, 0.0f },     // move to side
        { 1.0f, -1.0f },    // move to side and backwards
        { -1.0f, 0.0f },    // move to other side
        { -1.0f, -1.0f }    // move to side and backwards
    };

    bool proximityToDoor(const MWWorld::Ptr& actor, float minDist)
    {
        if(getNearbyDoor(actor, minDist).isEmpty())
            return false;
        else
            return true;
    }

    const MWWorld::Ptr getNearbyDoor(const MWWorld::Ptr& actor, float minDist)
    {
        MWWorld::CellStore *cell = actor.getCell();

        // Check all the doors in this cell
        const MWWorld::CellRefList<ESM::Door>& doors = cell->getReadOnlyDoors();
        const MWWorld::CellRefList<ESM::Door>::List& refList = doors.mList;
        MWWorld::CellRefList<ESM::Door>::List::const_iterator it = refList.begin();
        osg::Vec3f pos(actor.getRefData().getPosition().asVec3());
        pos.z() = 0;

        osg::Vec3f actorDir = (actor.getRefData().getBaseNode()->getAttitude() * osg::Vec3f(0,1,0));

        for (; it != refList.end(); ++it)
        {
            const MWWorld::LiveCellRef<ESM::Door>& ref = *it;

            osg::Vec3f doorPos(ref.mData.getPosition().asVec3());

            // FIXME: cast
            const MWWorld::Ptr doorPtr = MWWorld::Ptr(&const_cast<MWWorld::LiveCellRef<ESM::Door> &>(ref), actor.getCell());

            const auto doorState = doorPtr.getClass().getDoorState(doorPtr);
            float doorRot = ref.mData.getPosition().rot[2] - doorPtr.getCellRef().getPosition().rot[2];

            if (doorState != MWWorld::DoorState::Idle || doorRot != 0)
                continue; // the door is already opened/opening

            doorPos.z() = 0;

            float angle = std::acos(actorDir * (doorPos - pos) / (actorDir.length() * (doorPos - pos).length()));

            // Allow 60 degrees angle between actor and door
            if (angle < -osg::PI / 3 || angle > osg::PI / 3)
                continue;

            // Door is not close enough
            if ((pos - doorPos).length2() > minDist*minDist)
                continue;

            return doorPtr; // found, stop searching
        }

        return MWWorld::Ptr(); // none found
    }

    ObstacleCheck::ObstacleCheck()
      : mWalkState(WalkState::Initial)
      , mStateDuration(0)
      , mEvadeDirectionIndex(0)
    {
    }

    void ObstacleCheck::clear()
    {
        mWalkState = WalkState::Initial;
    }

    bool ObstacleCheck::isEvading() const
    {
        return mWalkState == WalkState::Evade;
    }

    /*
     * input   - actor, duration (time since last check)
     * output  - true if evasive action needs to be taken
     *
     * Walking state transitions (player greeting check not shown):
     *
     * Initial ----> Norm  <--------> CheckStuck -------> Evade ---+
     *               ^ ^ | f             ^   |       t    ^   |    |
     *               | | |               |   |            |   |    |
     *               | +-+               +---+            +---+    | u
     *               | any                < t              < u     |
     *               +---------------------------------------------+
     *
     * f = one reaction time
     * t = how long before considered stuck
     * u = how long to move sideways
     *
     */
    void ObstacleCheck::update(const MWWorld::Ptr& actor, const osg::Vec3f& destination, float duration)
    {
        const auto position = actor.getRefData().getPosition().asVec3();

        if (mWalkState == WalkState::Initial)
        {
            mWalkState = WalkState::Norm;
            mStateDuration = 0;
            mPrev = position;
            mInitialDistance = (destination - position).length();
            return;
        }

        if (mWalkState != WalkState::Evade)
        {
            const float distSameSpot = DIST_SAME_SPOT * actor.getClass().getSpeed(actor) * duration;
            const float prevDistance = (destination - mPrev).length();
            const float currentDistance = (destination - position).length();
            const float movedDistance = prevDistance - currentDistance;
            const float movedFromInitialDistance = mInitialDistance - currentDistance;

            mPrev = position;

            if (movedDistance >= distSameSpot && movedFromInitialDistance >= distSameSpot)
            {
                mWalkState = WalkState::Norm;
                mStateDuration = 0;
                return;
            }

            if (mWalkState == WalkState::Norm)
            {
                mWalkState = WalkState::CheckStuck;
                mStateDuration = duration;
                mInitialDistance = (destination - position).length();
                return;
            }

            mStateDuration += duration;
            if (mStateDuration < DURATION_SAME_SPOT)
            {
                return;
            }

            mWalkState = WalkState::Evade;
            mStateDuration = 0;
            chooseEvasionDirection();
            return;
        }

        mStateDuration += duration;
        if(mStateDuration >= DURATION_TO_EVADE)
        {
            // tried to evade, assume all is ok and start again
            mWalkState = WalkState::Norm;
            mStateDuration = 0;
            mPrev = position;
        }
    }

    void ObstacleCheck::takeEvasiveAction(MWMechanics::Movement& actorMovement) const
    {
        actorMovement.mPosition[0] = evadeDirections[mEvadeDirectionIndex][0];
        actorMovement.mPosition[1] = evadeDirections[mEvadeDirectionIndex][1];
    }

    void ObstacleCheck::chooseEvasionDirection()
    {
        // change direction if attempt didn't work
        ++mEvadeDirectionIndex;
        if (mEvadeDirectionIndex == NUM_EVADE_DIRECTIONS)
        {
            mEvadeDirectionIndex = 0;
        }
    }

}
