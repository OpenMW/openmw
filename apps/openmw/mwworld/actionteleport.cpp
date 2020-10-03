#include "actionteleport.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwmechanics/creaturestats.hpp"

#include "../mwworld/class.hpp"

#include "player.hpp"

namespace MWWorld
{
    ActionTeleport::ActionTeleport (const std::string& cellName,
        const ESM::Position& position, bool teleportFollowers)
    : Action (true), mCellName (cellName), mPosition (position), mTeleportFollowers(teleportFollowers)
    {
    }

    void ActionTeleport::executeImp (const Ptr& actor)
    {
        if (mTeleportFollowers)
        {
            // Find any NPCs that are following the actor and teleport them with him
            std::set<MWWorld::Ptr> followers;
            getFollowers(actor, followers, true);

            for (std::set<MWWorld::Ptr>::iterator it = followers.begin(); it != followers.end(); ++it)
                teleport(*it);
        }

        teleport(actor);
    }

    void ActionTeleport::teleport(const Ptr &actor)
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        actor.getClass().getCreatureStats(actor).land(actor == world->getPlayerPtr());
        if(actor == world->getPlayerPtr())
        {
            world->getPlayer().setTeleported(true);
            if (mCellName.empty())
                world->changeToExteriorCell (mPosition, true);
            else
                world->changeToInteriorCell (mCellName, mPosition, true);
        }
        else
        {
            if (actor.getClass().getCreatureStats(actor).getAiSequence().isInCombat(world->getPlayerPtr()))
                actor.getClass().getCreatureStats(actor).getAiSequence().stopCombat();
            else if (mCellName.empty())
            {
                int cellX;
                int cellY;
                world->positionToIndex(mPosition.pos[0],mPosition.pos[1],cellX,cellY);
                world->moveObject(actor,world->getExterior(cellX,cellY),
                    mPosition.pos[0],mPosition.pos[1],mPosition.pos[2]);
            }
            else
                world->moveObject(actor,world->getInterior(mCellName),mPosition.pos[0],mPosition.pos[1],mPosition.pos[2]);
        }
    }

    void ActionTeleport::getFollowers(const MWWorld::Ptr& actor, std::set<MWWorld::Ptr>& out, bool includeHostiles) {
        std::set<MWWorld::Ptr> followers;
        MWBase::Environment::get().getMechanicsManager()->getActorsFollowing(actor, followers);

        for(std::set<MWWorld::Ptr>::iterator it = followers.begin();it != followers.end();++it)
        {
            MWWorld::Ptr follower = *it;

            std::string script = follower.getClass().getScript(follower);

            if (!includeHostiles && follower.getClass().getCreatureStats(follower).getAiSequence().isInCombat(actor))
                continue;

            if (!script.empty() && follower.getRefData().getLocals().getIntVar(script, "stayoutside") == 1)
                continue;

            if ((follower.getRefData().getPosition().asVec3() - actor.getRefData().getPosition().asVec3()).length2() > 800 * 800)
                continue;

            out.emplace(follower);
        }
    }
}
