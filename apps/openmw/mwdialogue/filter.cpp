
#include "filter.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/containerstore.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "selectwrapper.hpp"

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

bool MWDialogue::Filter::testSelectStructs (const ESM::DialInfo& info) const
{
    for (std::vector<ESM::DialInfo::SelectStruct>::const_iterator iter (info.mSelects.begin());
        iter != info.mSelects.end(); ++iter)
        if (!testSelectStruct (*iter))
            return false;
            
    return true;
}

bool MWDialogue::Filter::testSelectStruct (const SelectWrapper& select) const
{
    if (select.isNpcOnly() && mActor.getTypeName()!=typeid (ESM::NPC).name())
        return select.isInverted();
    
    switch (select.getType())
    {
        case SelectWrapper::Type_None: return true;
        case SelectWrapper::Type_Integer: return select.selectCompare (getSelectStructInteger (select));
        case SelectWrapper::Type_Numeric: return testSelectStructNumeric (select);
        case SelectWrapper::Type_Boolean: return select.selectCompare (getSelectStructBoolean (select));
    }
    
    return true;
}

bool MWDialogue::Filter::testSelectStructNumeric (const SelectWrapper& select) const
{
    switch (select.getFunction())
    {
        default:
        
            throw std::runtime_error ("unknown numeric select function");
    }
}

int MWDialogue::Filter::getSelectStructInteger (const SelectWrapper& select) const
{
    switch (select.getFunction())
    {
        case SelectWrapper::Function_Journal:
        
            return MWBase::Environment::get().getJournal()->getJournalIndex (select.getName());
        
        case SelectWrapper::Function_Item: 
        {        
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
            MWWorld::ContainerStore& store = MWWorld::Class::get (player).getContainerStore (player);

            int sum = 0;

            std::string name = select.getName();

            for (MWWorld::ContainerStoreIterator iter (store.begin()); iter!=store.end(); ++iter)
                if (toLower(iter->getCellRef().mRefID) == name)
                    sum += iter->getRefData().getCount();
                    
            return sum;
        }

        case SelectWrapper::Function_Dead:

            return MWBase::Environment::get().getMechanicsManager()->countDeaths (select.getName());
        
        default:

            throw std::runtime_error ("unknown integer select function");
    }
}

bool MWDialogue::Filter::getSelectStructBoolean (const SelectWrapper& select) const
{
    switch (select.getFunction())
    {
        case SelectWrapper::Function_Id:
        
            return select.getName()==toLower (MWWorld::Class::get (mActor).getId (mActor));
        
        case SelectWrapper::Function_Faction:

            return toLower (mActor.get<ESM::NPC>()->mBase->mFaction)==select.getName();

        case SelectWrapper::Function_Class:

            return toLower (mActor.get<ESM::NPC>()->mBase->mClass)==select.getName();

        case SelectWrapper::Function_Race:

            return toLower (mActor.get<ESM::NPC>()->mBase->mRace)==select.getName();

        case SelectWrapper::Function_Cell:
    
            return toLower (mActor.getCell()->mCell->mName)==select.getName();
    
        default:

            throw std::runtime_error ("unknown boolean select function");
    }
}

MWDialogue::Filter::Filter (const MWWorld::Ptr& actor) : mActor (actor) {}

bool MWDialogue::Filter::operator() (const ESM::DialInfo& info) const
{
    return testActor (info) && testPlayer (info) && testSelectStructs (info);
}
