
#include "player.hpp"

#include "../mwrender/player.hpp"

#include "../mwmechanics/movement.hpp"

#include "world.hpp"
#include "class.hpp"

namespace MWWorld
{
    Player::Player (MWRender::Player *renderer, const ESM::NPC *player, MWWorld::World& world) :
      mCellStore (0), mRenderer (renderer), mWorld (world), mClass (0),
      mAutoMove (false), mForwardBackward (0)
    {
        mPlayer.base = player;
        mPlayer.ref.refID = "player";
        mName = player->name;
        mMale = !(player->flags & ESM::NPC::Female);
        mRace = player->race;

        float* playerPos = mPlayer.mData.getPosition().pos;
        playerPos[0] = playerPos[1] = playerPos[2] = 0;

        std::cout << renderer->getHandle();

        mPlayer.mData.setBaseNode(renderer->getNode());
        /// \todo Do not make a copy of classes defined in esm/p records.
        mClass = new ESM::Class (*world.getStore().classes.find (player->cls));
    }

    Player::~Player()
    {
        delete mClass;
    }

    void Player::setPos(float x, float y, float z)
    {
      /// \todo This fcuntion should be removed during the mwrender-refactoring.
        mWorld.moveObject (getPlayer(), x, y, z);
    }

    void Player::setClass (const ESM::Class& class_)
    {
        ESM::Class *new_class = new ESM::Class (class_);
        delete mClass;
        mClass = new_class;
    }

    void Player::setAutoMove (bool enable)
    {
        MWWorld::Ptr ptr = getPlayer();

        mAutoMove = enable;

        int value = mForwardBackward;

        if (mAutoMove)
            value = 1;

        MWWorld::Class::get (ptr).getMovementSettings (ptr).mForwardBackward = value;
    }

    void Player::setLeftRight (int value)
    {
        MWWorld::Ptr ptr = getPlayer();

        MWWorld::Class::get (ptr).getMovementSettings (ptr).mLeftRight = value;
    }

    void Player::setForwardBackward (int value)
    {
        MWWorld::Ptr ptr = getPlayer();

        mForwardBackward = value;

        if (mAutoMove)
            value = 1;

        MWWorld::Class::get (ptr).getMovementSettings (ptr).mForwardBackward = value;
    }

    void Player::toggleRunning()
    {
        MWWorld::Ptr ptr = getPlayer();

        bool running = MWWorld::Class::get (ptr).getStance (ptr, MWWorld::Class::Run, true);

        MWWorld::Class::get (ptr).setStance (ptr, MWWorld::Class::Run, !running);
    }

    Player::Faction Player::getFaction(std::string factionID)
    {
        for(std::list<Player::Faction>::iterator it = mFactions.begin(); it != mFactions.end();it++)
        {
            if(it->name == factionID) return *it;
        }
        //faction was not found->dummy faction
        Player::Faction fact;
        fact.id = "not found";
        fact.name = "not found";
        fact.rank = -10;
        fact.expelled = false;
        return fact;
    }

    void Player::addFaction(std::string factionID)
    {
        if(getFaction(factionID).name == "not found")
        {
            Player::Faction fact;
            const ESM::Faction* eFact = mWorld.getStore().factions.find(factionID);
            fact.expelled = false;
            fact.rank = 0;
            fact.name = eFact->name;
            fact.id = eFact->id;
            mFactions.push_back(fact);
        }
    }

    int Player::getRank(std::string factionID)
    {
        Player::Faction fact = getFaction(factionID);
        return fact.rank;
    }

    void Player::setRank(std::string factionID,int rank)
    {
        Player::Faction fact = getFaction(factionID);
        fact.rank = rank;
    }

    void Player::raiseRank(std::string factionID)
    {
        if(getFaction(factionID).name == "not found")
        {
            addFaction(factionID);
            setRank(factionID,1);
        }
        else
        {
            setRank(factionID,getRank(factionID) + 1);
        }
    }

    void Player::lowerRank(std::string factionID)
    {
        if(getFaction(factionID).name == "not found")
        {
            std::cout << "cannot lower the rank of the player: faction no found. Faction: "<< factionID << std::endl;
        }
        else
        {
            setRank(factionID,getRank(factionID) - 1);
        }
    }

    bool Player::isExpelled(std::string factionID)
    {
        Player::Faction fact = getFaction(factionID);
        return fact.expelled;
    }

    void Player::setExpelled(std::string factionID,bool expelled)
    {
        Player::Faction fact = getFaction(factionID);
        fact.expelled = expelled;
    }
}
