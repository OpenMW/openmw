#include "aiescort.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/timestamp.hpp"

#include "steering.hpp"
#include "movement.hpp"

/*
    TODO: Test vanilla behavior on passing x0, y0, and z0 with duration of anything including 0.
    TODO: Different behavior for AIEscort a d x y z and AIEscortCell a c d x y z.
    TODO: Take account for actors being in different cells.
*/

namespace MWMechanics
{
    AiEscort::AiEscort(const std::string &actorId, int duration, float x, float y, float z)
    : mActorId(actorId), mX(x), mY(y), mZ(z), mDuration(duration)
    , mCellX(std::numeric_limits<int>::max())
    , mCellY(std::numeric_limits<int>::max())
    {
        mMaxDist = 470;

        // The CS Help File states that if a duration is given, the AI package will run for that long
        // BUT if a location is givin, it "trumps" the duration so it will simply escort to that location.
        if(mX != 0 || mY != 0 || mZ != 0)
            mDuration = 0;

        else
        {
            MWWorld::TimeStamp startTime = MWBase::Environment::get().getWorld()->getTimeStamp();
            mStartingSecond = ((startTime.getHour() - int(startTime.getHour())) * 100);
        }
    }

    AiEscort::AiEscort(const std::string &actorId, const std::string &cellId,int duration, float x, float y, float z)
    : mActorId(actorId), mCellId(cellId), mX(x), mY(y), mZ(z), mDuration(duration)
    , mCellX(std::numeric_limits<int>::max())
    , mCellY(std::numeric_limits<int>::max())
    {
        mMaxDist = 470;

        // The CS Help File states that if a duration is given, the AI package will run for that long
        // BUT if a location is givin, it "trumps" the duration so it will simply escort to that location.
        if(mX != 0 || mY != 0 || mZ != 0)
            mDuration = 0;

        else
        {
            MWWorld::TimeStamp startTime = MWBase::Environment::get().getWorld()->getTimeStamp();
            mStartingSecond = ((startTime.getHour() - int(startTime.getHour())) * 100);
        }
    }


    AiEscort *MWMechanics::AiEscort::clone() const
    {
        return new AiEscort(*this);
    }

    bool AiEscort::execute (const MWWorld::Ptr& actor,float duration)
    {
        // If AiEscort has ran for as long or longer then the duration specified
        // and the duration is not infinite, the package is complete.
        if(mDuration != 0)
        {
            MWWorld::TimeStamp current = MWBase::Environment::get().getWorld()->getTimeStamp();
            unsigned int currentSecond = ((current.getHour() - int(current.getHour())) * 100);
            if(currentSecond - mStartingSecond >= mDuration)
                return true;
        }

        const MWWorld::Ptr follower = MWBase::Environment::get().getWorld()->getPtr(mActorId, false);
        const float* const leaderPos = actor.getRefData().getPosition().pos;
        const float* const followerPos = follower.getRefData().getPosition().pos;
        double differenceBetween[3];

        for (short counter = 0; counter < 3; counter++)
            differenceBetween[counter] = (leaderPos[counter] - followerPos[counter]);

        float distanceBetweenResult =
            (differenceBetween[0] * differenceBetween[0]) + (differenceBetween[1] * differenceBetween[1]) + (differenceBetween[2] *
                differenceBetween[2]);

        if(distanceBetweenResult <= mMaxDist * mMaxDist)
        {
            if(pathTo(actor,ESM::Pathgrid::Point(mX,mY,mZ),duration)) //Returns true on path complete
                return true;
            mMaxDist = 470;
        }
        else
        {
            // Stop moving if the player is to far away
            MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(actor, "idle3", 0, 1);
            MWWorld::Class::get(actor).getMovementSettings(actor).mPosition[1] = 0;
            mMaxDist = 330;
        }

        return false;
    }

    int AiEscort::getTypeId() const
    {
        return TypeIdEscort;
    }
}

