#include "aicombat.hpp"

#include "movement.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/timestamp.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"


namespace MWMechanics
{

    AiCombat::AiCombat(const std::string &targetId)
        :mTargetId(targetId)
    {
    }

    bool AiCombat::execute (const MWWorld::Ptr& actor)
    {
        const MWWorld::Ptr target = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();//MWBase::Environment::get().getWorld()->getPtr(mTargetId, false);

        MWMechanics::DrawState_ state = MWWorld::Class::get(actor).getNpcStats(actor).getDrawState();
        if (state == MWMechanics::DrawState_Spell || state == MWMechanics::DrawState_Nothing)
            MWWorld::Class::get(actor).getNpcStats(actor).setDrawState(MWMechanics::DrawState_Weapon);    
        MWWorld::Class::get(actor).getCreatureStats(actor).setAttackingOrSpell(true);

        ESM::Position pos = actor.getRefData().getPosition();
        const ESM::Pathgrid *pathgrid =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Pathgrid>().search(*actor.getCell()->mCell);

        int cellX = actor.getCell()->mCell->mData.mX;
        int cellY = actor.getCell()->mCell->mData.mY;
        float xCell = 0;
        float yCell = 0;

        if (actor.getCell()->mCell->isExterior())
        {
            xCell = actor.getCell()->mCell->mData.mX * ESM::Land::REAL_SIZE;
            yCell = actor.getCell()->mCell->mData.mY * ESM::Land::REAL_SIZE;
        }

        ESM::Pathgrid::Point dest;
        dest.mX = target.getRefData().getPosition().pos[0];
        dest.mY = target.getRefData().getPosition().pos[1];
        dest.mZ = target.getRefData().getPosition().pos[2];

        ESM::Pathgrid::Point start;
        start.mX = pos.pos[0];
        start.mY = pos.pos[1];
        start.mZ = pos.pos[2];

        std::cout << start.mX << " " << dest.mX << "\n";

        mPathFinder.buildPath(start, dest, pathgrid, xCell, yCell, true);

        if(mPathFinder.checkPathCompleted(pos.pos[0],pos.pos[1],pos.pos[2]))
        {
            MWWorld::Class::get(actor).getMovementSettings(actor).mPosition[1] = 0;
        }

        float zAngle = mPathFinder.getZAngleToNext(pos.pos[0], pos.pos[1]);
        std::cout << zAngle;
        MWBase::Environment::get().getWorld()->rotateObject(actor, 0, 0, zAngle, false);
        MWWorld::Class::get(actor).getMovementSettings(actor).mPosition[1] = 1;

        if(dest.mX - start.mX < 100)
        {
            //MWWorld::Class::get(actor).getCreatureStats(actor).setAttackingOrSpell(false);
        }

        return false;
    }

    int AiCombat::getTypeId() const
    {
        return 2;
    }

    AiCombat *MWMechanics::AiCombat::clone() const
    {
        return new AiCombat(*this);
    }
}

