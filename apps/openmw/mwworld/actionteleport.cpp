#include "actionteleport.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "player.hpp"

namespace
{

    void getFollowers (const MWWorld::Ptr& actor, std::set<MWWorld::Ptr>& out)
    {
        std::list<MWWorld::Ptr> followers = MWBase::Environment::get().getMechanicsManager()->getActorsFollowing(actor);
        for(std::list<MWWorld::Ptr>::iterator it = followers.begin();it != followers.end();++it)
        {
            if (out.insert(*it).second)
            {
                getFollowers(*it, out);
            }
        }
    }

}

namespace MWWorld
{
    ActionTeleport::ActionTeleport (const std::string& cellName,
        const ESM::Position& position)
    : Action (true), mCellName (cellName), mPosition (position)
    {
    }

    void ActionTeleport::executeImp (const Ptr& actor)
    {
        //find any NPC that is following the actor and teleport him too
        std::set<MWWorld::Ptr> followers;
        getFollowers(actor, followers);
        for(std::set<MWWorld::Ptr>::iterator it = followers.begin();it != followers.end();++it)
        {
            MWWorld::Ptr follower = *it;
            if (Ogre::Vector3(follower.getRefData().getPosition().pos).squaredDistance(
                        Ogre::Vector3( actor.getRefData().getPosition().pos))
                    <= 800*800)
                teleport(*it);
        }

        teleport(actor);
    }

    void ActionTeleport::teleport(const Ptr &actor)
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        if(actor == world->getPlayerPtr())
        {
            world->getPlayer().setTeleported(true);
            if (mCellName.empty())
                world->changeToExteriorCell (mPosition);
            else
                world->changeToInteriorCell (mCellName, mPosition);
        }
        else
        {
            if (mCellName.empty())
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
}
