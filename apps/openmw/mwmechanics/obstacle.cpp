#include "obstacle.hpp"

#include <components/esm/loadcell.hpp>

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

    // Proximity check function for interior doors.  Given that most interior cells
    // do not have many doors performance shouldn't be too much of an issue.
    //
    // Limitation: there can be false detections, and does not test whether the
    // actor is facing the door.
    bool proximityToDoor(const MWWorld::Ptr& actor, float minSqr)
    {
        if(getNearbyDoor(actor, minSqr)!=MWWorld::Ptr())
            return true;
        else
            return false;
    }

    MWWorld::Ptr getNearbyDoor(const MWWorld::Ptr& actor, float minSqr)
    {
        MWWorld::CellStore *cell = actor.getCell();

        if(cell->getCell()->isExterior())
            return MWWorld::Ptr(); // check interior cells only

        // Check all the doors in this cell
        MWWorld::CellRefList<ESM::Door>& doors = cell->get<ESM::Door>();
        MWWorld::CellRefList<ESM::Door>::List& refList = doors.mList;
        MWWorld::CellRefList<ESM::Door>::List::iterator it = refList.begin();
        osg::Vec3f pos(actor.getRefData().getPosition().asVec3());

        /// TODO: How to check whether the actor is facing a door? Below code is for
        ///       the player, perhaps it can be adapted.
        //MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->getFacedObject();
        //if(!ptr.isEmpty())
            //std::cout << "faced door " << ptr.getClass().getName(ptr) << std::endl;

        /// TODO: The in-game observation of rot[2] value seems to be the
        ///       opposite of the code in World::activateDoor() ::confused::
        for (; it != refList.end(); ++it)
        {
            MWWorld::LiveCellRef<ESM::Door>& ref = *it;
            if((pos - ref.mData.getPosition().asVec3()).length2() < minSqr
                    && ref.mData.getPosition().rot[2] == ref.mRef.getPosition().rot[2])
            {
                return MWWorld::Ptr(&ref, actor.getCell()); // found, stop searching
            }
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
    bool ObstacleCheck::check(const MWWorld::Ptr& actor, float duration)
    {
        const MWWorld::Class& cls = actor.getClass();
        ESM::Position pos = actor.getRefData().getPosition();

        // actors can move at most 60 fps (the physics framerate).
        // the max() can be removed if we implement physics interpolation.
        float movementDuration = std::max(1/60.f, duration);

        if(mDistSameSpot == -1)
            mDistSameSpot = DIST_SAME_SPOT * cls.getSpeed(actor);

        float distSameSpot = mDistSameSpot * movementDuration;

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
