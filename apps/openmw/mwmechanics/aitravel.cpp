#include "aitravel.hpp"

#include "movement.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"

namespace
{
    float sgn(float a)
    {
        if(a > 0)
            return 1.0;
        return -1.0;
    }
}

namespace MWMechanics
{
    AiTravel::AiTravel(float x, float y, float z)
    : mX(x),mY(y),mZ(z),mPathFinder()
    {
    }

    AiTravel *MWMechanics::AiTravel::clone() const
    {
        return new AiTravel(*this);
    }

    bool AiTravel::execute (const MWWorld::Ptr& actor)
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        ESM::Position pos = actor.getRefData().getPosition();
        bool cellChange = actor.getCell()->mCell->mData.mX != cellX || actor.getCell()->mCell->mData.mY != cellY;
        const ESM::Pathgrid *pathgrid =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Pathgrid>().search(*actor.getCell()->mCell);

        if(actor.getCell()->mCell->mData.mX != player.getCell()->mCell->mData.mX)
        {
            int sideX = sgn(actor.getCell()->mCell->mData.mX - player.getCell()->mCell->mData.mX);
            //check if actor is near the border of an inactive cell. If so, disable aitravel.
            if(sideX * (pos.pos[0] - actor.getCell()->mCell->mData.mX * ESM::Land::REAL_SIZE) > sideX * (ESM::Land::REAL_SIZE /
                2.0 - 200)) 
            {
                MWWorld::Class::get(actor).getMovementSettings(actor).mPosition[1] = 0;
                return true;
            }
        }
        if(actor.getCell()->mCell->mData.mY != player.getCell()->mCell->mData.mY)
        {
            int sideY = sgn(actor.getCell()->mCell->mData.mY - player.getCell()->mCell->mData.mY);
            //check if actor is near the border of an inactive cell. If so, disable aitravel.
            if(sideY * (pos.pos[1] - actor.getCell()->mCell->mData.mY * ESM::Land::REAL_SIZE) > sideY * (ESM::Land::REAL_SIZE /
                2.0 - 200)) 
            {
                MWWorld::Class::get(actor).getMovementSettings(actor).mPosition[1] = 0;
                return true;
            }
        }

        if(!mPathFinder.isPathConstructed() || cellChange)
        {
            cellX = actor.getCell()->mCell->mData.mX;
            cellY = actor.getCell()->mCell->mData.mY;
            float xCell = 0;
            float yCell = 0;

            if (actor.getCell()->mCell->isExterior())
            {
                xCell = actor.getCell()->mCell->mData.mX * ESM::Land::REAL_SIZE;
                yCell = actor.getCell()->mCell->mData.mY * ESM::Land::REAL_SIZE;
            }

            ESM::Pathgrid::Point dest;
            dest.mX = mX;
            dest.mY = mY;
            dest.mZ = mZ;

            ESM::Pathgrid::Point start;
            start.mX = pos.pos[0];
            start.mY = pos.pos[1];
            start.mZ = pos.pos[2];

            mPathFinder.buildPath(start, dest, pathgrid, xCell, yCell, true);
        }

        if(mPathFinder.checkPathCompleted(pos.pos[0], pos.pos[1], pos.pos[2]))
        {
            MWWorld::Class::get(actor).getMovementSettings(actor).mPosition[1] = 0;
            return true;
        }

        float zAngle = mPathFinder.getZAngleToNext(pos.pos[0], pos.pos[1]);
        MWBase::Environment::get().getWorld()->rotateObject(actor, 0, 0, zAngle, false);
        MWWorld::Class::get(actor).getMovementSettings(actor).mPosition[1] = 1;

        return false;
    }

    int AiTravel::getTypeId() const
    {
        return 1;
    }
}

