#include "aitravel.hpp"

#include <OgreVector3.h>

#include <components/esm/aisequence.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"

#include "steering.hpp"
#include "movement.hpp"
#include "creaturestats.hpp"

namespace
{

bool isWithinMaxRange(const Ogre::Vector3& pos1, const Ogre::Vector3& pos2)
{
    // Maximum travel distance for vanilla compatibility.
    // Was likely meant to prevent NPCs walking into non-loaded exterior cells, but for some reason is used in interior cells as well.
    // We can make this configurable at some point, but the default *must* be the below value. Anything else will break shoddily-written content (*cough* MW *cough*) in bizarre ways.
    return (pos1.squaredDistance(pos2) <= 7168*7168);
}

}

namespace MWMechanics
{
    AiTravel::AiTravel(float x, float y, float z)
    : mX(x),mY(y),mZ(z)
    , mCellX(std::numeric_limits<int>::max())
    , mCellY(std::numeric_limits<int>::max())
    {
    }

    AiTravel::AiTravel(const ESM::AiSequence::AiTravel *travel)
        : mX(travel->mData.mX), mY(travel->mData.mY), mZ(travel->mData.mZ)
        , mCellX(std::numeric_limits<int>::max())
        , mCellY(std::numeric_limits<int>::max())
    {

    }

    AiTravel *MWMechanics::AiTravel::clone() const
    {
        return new AiTravel(*this);
    }

    bool AiTravel::execute (const MWWorld::Ptr& actor, AiState& state, float duration)
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

        if (!isWithinMaxRange(Ogre::Vector3(mX, mY, mZ), Ogre::Vector3(pos.pos)))
            return false;

        bool cellChange = cell->mData.mX != mCellX || cell->mData.mY != mCellY;
        if(!mPathFinder.isPathConstructed() || cellChange)
        {
            mCellX = cell->mData.mX;
            mCellY = cell->mData.mY;

            ESM::Pathgrid::Point dest(static_cast<int>(mX), static_cast<int>(mY), static_cast<int>(mZ));

            ESM::Pathgrid::Point start(PathFinder::MakePathgridPoint(pos));

            mPathFinder.buildPath(start, dest, actor.getCell(), true);
        }

        if(mPathFinder.checkPathCompleted(pos.pos[0], pos.pos[1]))
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

    void AiTravel::fastForward(const MWWorld::Ptr& actor, AiState& state)
    {
        if (!isWithinMaxRange(Ogre::Vector3(mX, mY, mZ), Ogre::Vector3(actor.getRefData().getPosition().pos)))
            return;
        // does not do any validation on the travel target (whether it's in air, inside collision geometry, etc),
        // that is the user's responsibility
        MWBase::Environment::get().getWorld()->moveObject(actor, mX, mY, mZ);
        actor.getClass().adjustPosition(actor, false);
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

