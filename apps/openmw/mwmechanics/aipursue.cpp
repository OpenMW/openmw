#include "aipursue.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/action.hpp"
#include "../mwworld/cellstore.hpp"

#include "steering.hpp"
#include "movement.hpp"
#include "creaturestats.hpp"

MWMechanics::AiPursue::AiPursue(const std::string &objectId)
    : mObjectId(objectId)
{
}
MWMechanics::AiPursue *MWMechanics::AiPursue::clone() const
{
    return new AiPursue(*this);
}
bool MWMechanics::AiPursue::execute (const MWWorld::Ptr& actor, float duration)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    ESM::Position pos = actor.getRefData().getPosition();
    Movement &movement = actor.getClass().getMovementSettings(actor);
    const ESM::Cell *cell = actor.getCell()->getCell();

    actor.getClass().getCreatureStats(actor).setMovementFlag(CreatureStats::Flag_Run, true);

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

    // Big TODO: Sync this with current AiFollow. Move common code to a shared base class or helpers (applies to all AI packages, way too much duplicated code)

    MWWorld::Ptr target = world->getPtr(mObjectId,false);
    ESM::Position targetPos = target.getRefData().getPosition();

    bool cellChange = cell->mData.mX != mCellX || cell->mData.mY != mCellY;
    if(!mPathFinder.isPathConstructed() || cellChange || mPathFinder.checkPathCompleted(pos.pos[0], pos.pos[1], pos.pos[2]))
    {
        mCellX = cell->mData.mX;
        mCellY = cell->mData.mY;

        ESM::Pathgrid::Point dest;
        dest.mX = targetPos.pos[0];
        dest.mY = targetPos.pos[1];
        dest.mZ = targetPos.pos[2];

        ESM::Pathgrid::Point start;
        start.mX = pos.pos[0];
        start.mY = pos.pos[1];
        start.mZ = pos.pos[2];

        mPathFinder.buildPath(start, dest, actor.getCell(), true);
    }

    if((pos.pos[0]-targetPos.pos[0])*(pos.pos[0]-targetPos.pos[0])+
        (pos.pos[1]-targetPos.pos[1])*(pos.pos[1]-targetPos.pos[1])+
        (pos.pos[2]-targetPos.pos[2])*(pos.pos[2]-targetPos.pos[2]) < 100*100)
    {
        movement.mPosition[1] = 0;
        MWWorld::Ptr target = world->getPtr(mObjectId,false);
        MWWorld::Class::get(target).activate(target,actor).get()->execute(actor);
        return true;
    }

    float zAngle = mPathFinder.getZAngleToNext(pos.pos[0], pos.pos[1]);
    zTurn(actor, Ogre::Degree(zAngle));
    MWWorld::Class::get(actor).getMovementSettings(actor).mPosition[1] = 1;
    movement.mPosition[1] = 1;

    return false;
}

int MWMechanics::AiPursue::getTypeId() const
{
    return TypeIdPursue;
}
