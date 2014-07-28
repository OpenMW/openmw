#include "aitravel.hpp"

#include <components/esm/aisequence.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"

#include "steering.hpp"
#include "movement.hpp"
#include "creaturestats.hpp"

namespace MWMechanics
{
    AiTravel::AiTravel(float x, float y, float z)
    : mX(x),mY(y),mZ(z),mPathFinder()
    , mCellX(std::numeric_limits<int>::max())
    , mCellY(std::numeric_limits<int>::max())
    {
    }

    AiTravel::AiTravel(const ESM::AiSequence::AiTravel *travel)
        : mX(travel->mData.mX), mY(travel->mData.mY), mZ(travel->mData.mZ)
        , mPathFinder()
        , mCellX(std::numeric_limits<int>::max())
        , mCellY(std::numeric_limits<int>::max())
    {

    }

    AiTravel *MWMechanics::AiTravel::clone() const
    {
        return new AiTravel(*this);
    }

    bool AiTravel::execute (const MWWorld::Ptr& actor,float duration)
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        ESM::Position pos = actor.getRefData().getPosition();
        Movement &movement = actor.getClass().getMovementSettings(actor);
        const ESM::Cell *cell = actor.getCell()->getCell();

        actor.getClass().getCreatureStats(actor).setMovementFlag(CreatureStats::Flag_Run, false);

        actor.getClass().getCreatureStats(actor).setDrawState(DrawState_Nothing);

        MWWorld::Ptr player = world->getPlayerPtr();
        if(cell->mData.mX != player.getCell()->getCell()->mData.mX)
        {
            int sideX = PathFinder::sgn(cell->mData.mX - player.getCell()->getCell()->mData.mX);
            //check if actor is near the border of an inactive cell. If so, stop walking.
            if(sideX * (pos.pos[0] - cell->mData.mX*ESM::Land::REAL_SIZE) >
               sideX * (ESM::Land::REAL_SIZE/2.0f - 200.0f))
            {
                movement.mPosition[1] = 0;
                return false;
            }
        }
        if(cell->mData.mY != player.getCell()->getCell()->mData.mY)
        {
            int sideY = PathFinder::sgn(cell->mData.mY - player.getCell()->getCell()->mData.mY);
            //check if actor is near the border of an inactive cell. If so, stop walking.
            if(sideY * (pos.pos[1] - cell->mData.mY*ESM::Land::REAL_SIZE) >
               sideY * (ESM::Land::REAL_SIZE/2.0f - 200.0f))
            {
                movement.mPosition[1] = 0;
                return false;
            }
        }

        bool cellChange = cell->mData.mX != mCellX || cell->mData.mY != mCellY;
        if(!mPathFinder.isPathConstructed() || cellChange)
        {
            mCellX = cell->mData.mX;
            mCellY = cell->mData.mY;

            ESM::Pathgrid::Point dest;
            dest.mX = mX;
            dest.mY = mY;
            dest.mZ = mZ;

            ESM::Pathgrid::Point start;
            start.mX = pos.pos[0];
            start.mY = pos.pos[1];
            start.mZ = pos.pos[2];

            mPathFinder.buildPath(start, dest, actor.getCell(), true);
        }

        if(mPathFinder.checkPathCompleted(pos.pos[0], pos.pos[1], pos.pos[2]))
        {
            movement.mPosition[1] = 0;
            return true;
        }

        zTurn(actor, Ogre::Degree(mPathFinder.getZAngleToNext(pos.pos[0], pos.pos[1])));
        movement.mPosition[1] = 1;

        return false;
    }

    int AiTravel::getTypeId() const
    {
        return TypeIdTravel;
    }

    void AiTravel::writeState(ESM::AiSequence::AiSequence &sequence) const
    {
        std::auto_ptr<ESM::AiSequence::AiTravel> travel(new ESM::AiSequence::AiTravel());
        travel->mData.mX = mX;
        travel->mData.mY = mY;
        travel->mData.mZ = mZ;

        ESM::AiSequence::AiPackageContainer package;
        package.mType = ESM::AiSequence::Ai_Travel;
        package.mPackage = travel.release();
        sequence.mPackages.push_back(package);
    }
}

