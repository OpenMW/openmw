#include "obstacle.hpp"

#include <components/esm/loadcell.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>

#include "../mwbase/world.hpp"
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

            int doorState = doorPtr.getClass().getDoorState(doorPtr);
            float doorRot = ref.mData.getPosition().rot[2] - doorPtr.getCellRef().getPosition().rot[2];

            if (doorState != 0 || doorRot != 0)
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

    ObstacleCheck::ObstacleCheck():
        mPrevX(0) // to see if the moved since last time
      , mPrevY(0)
      , mWalkState(State_Norm)
      , mStuckDuration(0)
      , mEvadeDuration(0)
      , mDistSameSpot(-1) // avoid calculating it each time
      , mEvadeDirectionIndex(0)
    {
    }

    void ObstacleCheck::clear()
    {
        mWalkState = State_Norm;
        mStuckDuration = 0;
        mEvadeDuration = 0;
    }

    bool ObstacleCheck::isNormalState() const
    {
        return mWalkState == State_Norm;
    }

    bool ObstacleCheck::isEvading() const
    {
        return mWalkState == State_Evade;
    }

    /*
     * input   - actor, duration (time since last check)
     * output  - true if evasive action needs to be taken
     *
     *  Walking state transitions (player greeting check not shown):
     *
     *  MoveNow <------------------------------------+
     *    |                                         d|
     *    |                                          |
     *    +-> State_Norm <---> State_CheckStuck --> State_Evade
     *         ^  ^   |    f      ^   |         t    ^   |  |
     *         |  |   |           |   |              |   |  |
     *         |  +---+           +---+              +---+  | u
     *         |   any             < t                < u   |
     *         +--------------------------------------------+
     *
     * f = one reaction time
     * d = proximity to a closed door
     * t = how long before considered stuck
     * u = how long to move sideways
     *
     */
    bool ObstacleCheck::check(const MWWorld::Ptr& actor, float duration, float scaleMinimumDistance)
    {
        const MWWorld::Class& cls = actor.getClass();
        ESM::Position pos = actor.getRefData().getPosition();

        if(mDistSameSpot == -1)
            mDistSameSpot = DIST_SAME_SPOT * cls.getSpeed(actor) * scaleMinimumDistance;

        float distSameSpot = mDistSameSpot * duration;

        bool samePosition =  (osg::Vec2f(pos.pos[0], pos.pos[1]) - osg::Vec2f(mPrevX, mPrevY)).length2() <  distSameSpot * distSameSpot;

        // update position
        mPrevX = pos.pos[0];
        mPrevY = pos.pos[1];

        switch(mWalkState)
        {
            case State_Norm:
            {
                if(!samePosition)
                    break;
                else
                    mWalkState = State_CheckStuck;
            }
                /* FALL THROUGH */
            case State_CheckStuck:
            {
                if(!samePosition)
                {
                    mWalkState = State_Norm;
                    mStuckDuration = 0;
                    break;
                }
                else
                {
                    mStuckDuration += duration;
                    // consider stuck only if position unchanges for a period
                    if(mStuckDuration < DURATION_SAME_SPOT)
                        break; // still checking, note duration added to timer
                    else
                    {
                        mStuckDuration = 0;
                        mWalkState = State_Evade;
                        chooseEvasionDirection();
                    }
                }
            }
                /* FALL THROUGH */
            case State_Evade:
            {
                mEvadeDuration += duration;
                if(mEvadeDuration < DURATION_TO_EVADE)
                    return true;
                else
                {
                    // tried to evade, assume all is ok and start again
                    mWalkState = State_Norm;
                    mEvadeDuration = 0;
                }
            }
            /* NO DEFAULT CASE */
        }
        return false; // no obstacles to evade (yet)
    }

    void ObstacleCheck::takeEvasiveAction(MWMechanics::Movement& actorMovement)
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
