
#include "filter.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"

#include "../mwmechanics/npcstats.hpp"

namespace
{
    std::string toLower (const std::string& name)
    {
        std::string lowerCase;

        std::transform (name.begin(), name.end(), std::back_inserter (lowerCase),
            (int(*)(int)) std::tolower);

        return lowerCase;
    }
}

bool MWDialogue::Filter::testActor (const ESM::DialInfo& info) const
{
    // actor id
    if (!info.mActor.empty())
        if (toLower (info.mActor)!=MWWorld::Class::get (mActor).getId (mActor))
            return false;

    bool isCreature = (mActor.getTypeName() != typeid (ESM::NPC).name());

    // NPC race
    if (!info.mRace.empty())
    {
        if (isCreature)
            return false;

        MWWorld::LiveCellRef<ESM::NPC> *cellRef = mActor.get<ESM::NPC>();

        if (toLower (info.mRace)!=toLower (cellRef->mBase->mRace))
            return false;
    }

    // NPC class
    if (!info.mClass.empty())
    {
        if (isCreature)
            return false;

        MWWorld::LiveCellRef<ESM::NPC> *cellRef = mActor.get<ESM::NPC>();

        if (toLower (info.mClass)!=toLower (cellRef->mBase->mClass))
            return false;
    }

    // NPC faction
    if (!info.mNpcFaction.empty())
    {
        if (isCreature)
            return false;

        MWMechanics::NpcStats& stats = MWWorld::Class::get (mActor).getNpcStats (mActor);
        std::map<std::string, int>::iterator iter = stats.getFactionRanks().find (toLower (info.mNpcFaction));

        if (iter==stats.getFactionRanks().end())
            return false;
            
        // check rank
        if (iter->second < info.mData.mRank)
            return false;
    }

    // Gender
    if (!isCreature)
    {
        MWWorld::LiveCellRef<ESM::NPC>* npc = mActor.get<ESM::NPC>();
        if (info.mData.mGender==(npc->mBase->mFlags & npc->mBase->Female ? 0 : 1))
            return false;
    }
    
    return true;
}

bool MWDialogue::Filter::testPlayer (const ESM::DialInfo& info) const
{
    const MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
    
    // check player faction
    if (!info.mPcFaction.empty())
    {
        MWMechanics::NpcStats& stats = MWWorld::Class::get (player).getNpcStats (player);
        std::map<std::string,int>::iterator iter = stats.getFactionRanks().find (toLower (info.mPcFaction));

        if(iter==stats.getFactionRanks().end())
            return false;

        // check rank
        if (iter->second < info.mData.mPCrank)
            return false;
    }

    // check cell
    if (!info.mCell.empty())
        if (toLower (player.getCell()->mCell->mName) != toLower (info.mCell))
            return false;

    return true;
}

MWDialogue::Filter::Filter (const MWWorld::Ptr& actor) : mActor (actor) {}

bool MWDialogue::Filter::operator() (const ESM::DialInfo& info) const
{
    return testActor (info) && testPlayer (info);
}
