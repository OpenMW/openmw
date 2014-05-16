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

    ESM::Position pos = actor.getRefData().getPosition(); //position of the actor
    const MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtr(mObjectId, false); //The target to follow

    if(target == MWWorld::Ptr())
        return true;   //Target doesn't exist

    //Set the target desition from the actor
    ESM::Pathgrid::Point dest = target.getRefData().getPosition().pos;

    if(distance(dest, pos.pos[0], pos.pos[1], pos.pos[2]) < 100) { //Stop when you get close
        actor.getClass().getMovementSettings(actor).mPosition[1] = 0;
        MWWorld::Ptr target = MWBase::Environment::get().getWorld()->getPtr(mObjectId,false);
        MWWorld::Class::get(target).activate(target,actor).get()->execute(actor); //Arrest player
        return true;
    }
    else {
        pathTo(actor, dest, duration); //Go to the destination
    }

    actor.getClass().getCreatureStats(actor).setMovementFlag(MWMechanics::CreatureStats::Flag_Run, true); //Make NPC run

    return false;
}

int MWMechanics::AiPursue::getTypeId() const
{
    return TypeIdPursue;
}
