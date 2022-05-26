#include "actionteleport.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwmechanics/creaturestats.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellutils.hpp"

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
            getFollowers(actor, followers, mCellName.empty(), true);

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
                const osg::Vec2i index = positionToCellIndex(mPosition.pos[0], mPosition.pos[1]);
                world->moveObject(actor, world->getExterior(index.x(), index.y()), mPosition.asVec3(), true, true);
            }
            else
                world->moveObject(actor,world->getInterior(mCellName),mPosition.asVec3(), true, true);
        }
    }

    void ActionTeleport::getFollowers(const MWWorld::Ptr& actor, std::set<MWWorld::Ptr>& out, bool toExterior, bool includeHostiles) {
        std::set<MWWorld::Ptr> followers;
        MWBase::Environment::get().getMechanicsManager()->getActorsFollowing(actor, followers);

        for(std::set<MWWorld::Ptr>::iterator it = followers.begin();it != followers.end();++it)
        {
            MWWorld::Ptr follower = *it;

            std::string script = follower.getClass().getScript(follower);

            if (!includeHostiles && follower.getClass().getCreatureStats(follower).getAiSequence().isInCombat(actor))
                continue;

            if (!toExterior && !script.empty() && follower.getRefData().getLocals().getIntVar(script, "stayoutside") == 1 && follower.getCell()->getCell()->isExterior())
                continue;

            if ((follower.getRefData().getPosition().asVec3() - actor.getRefData().getPosition().asVec3()).length2() > 800 * 800)
                continue;

            out.emplace(follower);
        }
    }
}
