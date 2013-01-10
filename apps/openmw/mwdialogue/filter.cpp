
#include "filter.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/magiceffects.hpp"

#include "selectwrapper.hpp"

bool MWDialogue::Filter::testActor (const ESM::DialInfo& info) const
{
    // actor id
    if (!info.mActor.empty())
        if ( Misc::StringUtils::lowerCase (info.mActor)!=MWWorld::Class::get (mActor).getId (mActor))
            return false;

    bool isCreature = (mActor.getTypeName() != typeid (ESM::NPC).name());

    // NPC race
    if (!info.mRace.empty())
    {
        if (isCreature)
            return false;

        MWWorld::LiveCellRef<ESM::NPC> *cellRef = mActor.get<ESM::NPC>();

        if (Misc::StringUtils::lowerCase (info.mRace)!= Misc::StringUtils::lowerCase (cellRef->mBase->mRace))
            return false;
    }

    // NPC class
    if (!info.mClass.empty())
    {
        if (isCreature)
            return false;

        MWWorld::LiveCellRef<ESM::NPC> *cellRef = mActor.get<ESM::NPC>();

        if ( Misc::StringUtils::lowerCase (info.mClass)!= Misc::StringUtils::lowerCase (cellRef->mBase->mClass))
            return false;
    }

    // NPC faction
    if (!info.mNpcFaction.empty())
    {
        if (isCreature)
            return false;

        MWMechanics::NpcStats& stats = MWWorld::Class::get (mActor).getNpcStats (mActor);
        std::map<std::string, int>::iterator iter = stats.getFactionRanks().find ( Misc::StringUtils::lowerCase (info.mNpcFaction));

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
        std::map<std::string,int>::iterator iter = stats.getFactionRanks().find (Misc::StringUtils::lowerCase (info.mPcFaction));

        if(iter==stats.getFactionRanks().end())
            return false;

        // check rank
        if (iter->second < info.mData.mPCrank)
            return false;
    }

    // check cell
    if (!info.mCell.empty())
        if (Misc::StringUtils::lowerCase (player.getCell()->mCell->mName) != Misc::StringUtils::lowerCase (info.mCell))
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
        case SelectWrapper::Function_Global:

            // internally all globals are float :(
            return select.selectCompare (
                MWBase::Environment::get().getWorld()->getGlobalVariable (select.getName()).mFloat);

        case SelectWrapper::Function_Local:
        {
            std::string scriptName = MWWorld::Class::get (mActor).getScript (mActor);

            if (scriptName.empty())
                return false; // no script

            const ESM::Script *script =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Script>().find (scriptName);

            std::string name = select.getName();

            int i = 0;

            for (; i<static_cast<int> (script->mVarNames.size()); ++i)
                if (script->mVarNames[i]==name)
                    break;

            if (i>=static_cast<int> (script->mVarNames.size()))
                return false; // script does not have a variable of this name

            const MWScript::Locals& locals = mActor.getRefData().getLocals();

            if (i<script->mData.mNumShorts)
                return select.selectCompare (static_cast<int> (locals.mShorts[i]));

            i -= script->mData.mNumShorts;

            if (i<script->mData.mNumLongs)
                return select.selectCompare (locals.mLongs[i]);

            i -= script->mData.mNumShorts;

            return select.selectCompare (locals.mFloats.at (i));
        }

        case SelectWrapper::Function_PcHealthPercent:
        {
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();

            float ratio = MWWorld::Class::get (player).getCreatureStats (player).getHealth().getCurrent() /
                MWWorld::Class::get (player).getCreatureStats (player).getHealth().getModified();

            return select.selectCompare (ratio);
        }

        case SelectWrapper::Function_PcDynamicStat:
        {
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();

            float value = MWWorld::Class::get (player).getCreatureStats (player).
                getDynamic (select.getArgument()).getCurrent();

            return select.selectCompare (value);
        }

        case SelectWrapper::Function_HealthPercent:
        {
            float ratio = MWWorld::Class::get (mActor).getCreatureStats (mActor).getHealth().getCurrent() /
                MWWorld::Class::get (mActor).getCreatureStats (mActor).getHealth().getModified();

            return select.selectCompare (ratio);
        }

        default:

            throw std::runtime_error ("unknown numeric select function");
    }
}

int MWDialogue::Filter::getSelectStructInteger (const SelectWrapper& select) const
{
    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();

    switch (select.getFunction())
    {
        case SelectWrapper::Function_Journal:

            return MWBase::Environment::get().getJournal()->getJournalIndex (select.getName());

        case SelectWrapper::Function_Item:
        {
            MWWorld::ContainerStore& store = MWWorld::Class::get (player).getContainerStore (player);

            int sum = 0;

            std::string name = select.getName();

            for (MWWorld::ContainerStoreIterator iter (store.begin()); iter!=store.end(); ++iter)
                if (Misc::StringUtils::lowerCase(iter->getCellRef().mRefID) == name)
                    sum += iter->getRefData().getCount();

            return sum;
        }

        case SelectWrapper::Function_Dead:

            return MWBase::Environment::get().getMechanicsManager()->countDeaths (select.getName());

        case SelectWrapper::Function_Choice:

            return mChoice;

        case SelectWrapper::Function_AiSetting:

            return MWWorld::Class::get (mActor).getCreatureStats (mActor).getAiSetting (select.getArgument());

        case SelectWrapper::Function_PcAttribute:

            return MWWorld::Class::get (player).getCreatureStats (player).
                getAttribute (select.getArgument()).getModified();

        case SelectWrapper::Function_PcSkill:

            return static_cast<int> (MWWorld::Class::get (player).
                getNpcStats (player).getSkill (select.getArgument()).getModified());

        case SelectWrapper::Function_FriendlyHit:
        {
            int hits = MWWorld::Class::get (mActor).getCreatureStats (mActor).getFriendlyHits();

            return hits>4 ? 4 : hits;
        }

        case SelectWrapper::Function_PcLevel:

            return MWWorld::Class::get (player).getCreatureStats (player).getLevel();

        case SelectWrapper::Function_PcGender:

            return player.get<ESM::NPC>()->mBase->mFlags & ESM::NPC::Female ? 0 : 1;

        case SelectWrapper::Function_PcClothingModifier:
        {
            MWWorld::InventoryStore& store = MWWorld::Class::get (player).getInventoryStore (player);

            int value = 0;

            for (int i=0; i<=15; ++i) // everything except thigns held in hands and amunition
            {
                MWWorld::ContainerStoreIterator slot = store.getSlot (i);

                if (slot!=store.end())
                    value += MWWorld::Class::get (*slot).getValue (*slot);
            }

            return value;
        }

        case SelectWrapper::Function_PcCrimeLevel:

            return MWWorld::Class::get (player).getNpcStats (player).getBounty();

        case SelectWrapper::Function_RankRequirement:
        {
            if (MWWorld::Class::get (mActor).getNpcStats (mActor).getFactionRanks().empty())
                return 0;

            std::string faction =
                MWWorld::Class::get (mActor).getNpcStats (mActor).getFactionRanks().begin()->first;

            int rank = getFactionRank (player, faction);

            if (rank>=9)
                return 0; // max rank

            int result = 0;

            if (hasFactionRankSkillRequirements (player, faction, rank+1))
                result += 1;

            if (hasFactionRankReputationRequirements (player, faction, rank+1))
                result += 2;

            return result;
        }

        case SelectWrapper::Function_Level:

            return MWWorld::Class::get (mActor).getCreatureStats (mActor).getLevel();

        case SelectWrapper::Function_PCReputation:

            return MWWorld::Class::get (player).getNpcStats (player).getReputation();

        case SelectWrapper::Function_Weather:

            return MWBase::Environment::get().getWorld()->getCurrentWeather();

        case SelectWrapper::Function_Reputation:

            return MWWorld::Class::get (mActor).getNpcStats (mActor).getReputation();

        case SelectWrapper::Function_FactionRankDiff:
        {
            if (MWWorld::Class::get (mActor).getNpcStats (mActor).getFactionRanks().empty())
                return 0;

            std::pair<std::string, int> faction =
                *MWWorld::Class::get (mActor).getNpcStats (mActor).getFactionRanks().begin();

            int rank = getFactionRank (player, faction.first);

            return rank-faction.second;
        }

        case SelectWrapper::Function_WerewolfKills:

            return MWWorld::Class::get (player).getNpcStats (player).getWerewolfKills();

        case SelectWrapper::Function_RankLow:
        case SelectWrapper::Function_RankHigh:
        {
            bool low = select.getFunction()==SelectWrapper::Function_RankLow;

            if (MWWorld::Class::get (mActor).getNpcStats (mActor).getFactionRanks().empty())
                return 0;

            std::string factionId =
                MWWorld::Class::get (mActor).getNpcStats (mActor).getFactionRanks().begin()->first;

            int value = 0;

            const ESM::Faction& faction =
                *MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find (factionId);

            MWMechanics::NpcStats& playerStats = MWWorld::Class::get (player).getNpcStats (player);

            for (std::vector<ESM::Faction::Reaction>::const_iterator iter (faction.mReactions.begin());
                iter!=faction.mReactions.end(); ++iter)
                if (playerStats.getFactionRanks().find (iter->mFaction)!=playerStats.getFactionRanks().end())
                    if (low ? iter->mReaction<value : iter->mReaction>value)
                        value = iter->mReaction;

            return value;
        }

        default:

            throw std::runtime_error ("unknown integer select function");
    }
}

bool MWDialogue::Filter::getSelectStructBoolean (const SelectWrapper& select) const
{
    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();

    switch (select.getFunction())
    {
        case SelectWrapper::Function_False:

            return false;

        case SelectWrapper::Function_Id:

            return select.getName()==Misc::StringUtils::lowerCase (MWWorld::Class::get (mActor).getId (mActor));

        case SelectWrapper::Function_Faction:

            return Misc::StringUtils::lowerCase (mActor.get<ESM::NPC>()->mBase->mFaction)==select.getName();

        case SelectWrapper::Function_Class:

            return Misc::StringUtils::lowerCase (mActor.get<ESM::NPC>()->mBase->mClass)==select.getName();

        case SelectWrapper::Function_Race:

            return Misc::StringUtils::lowerCase (mActor.get<ESM::NPC>()->mBase->mRace)==select.getName();

        case SelectWrapper::Function_Cell:

            return Misc::StringUtils::lowerCase (mActor.getCell()->mCell->mName)==select.getName();

        case SelectWrapper::Function_SameGender:

            return (player.get<ESM::NPC>()->mBase->mFlags & ESM::NPC::Female)==
                (mActor.get<ESM::NPC>()->mBase->mFlags & ESM::NPC::Female);

        case SelectWrapper::Function_SameRace:

            return Misc::StringUtils::lowerCase (mActor.get<ESM::NPC>()->mBase->mRace)!=
                Misc::StringUtils::lowerCase (player.get<ESM::NPC>()->mBase->mRace);

        case SelectWrapper::Function_SameFaction:

            return MWWorld::Class::get (mActor).getNpcStats (mActor).isSameFaction (
                MWWorld::Class::get (player).getNpcStats (player));

        case SelectWrapper::Function_PcCommonDisease:

            return MWWorld::Class::get (player).getCreatureStats (player).hasCommonDisease();

        case SelectWrapper::Function_PcBlightDisease:

            return MWWorld::Class::get (player).getCreatureStats (player).hasBlightDisease();

        case SelectWrapper::Function_PcCorprus:

            return MWWorld::Class::get (player).getCreatureStats (player).
                getMagicEffects().get (132).mMagnitude!=0;

        case SelectWrapper::Function_PcExpelled:
        {
            if (MWWorld::Class::get (mActor).getNpcStats (mActor).getFactionRanks().empty())
                return false;

            std::string faction =
                MWWorld::Class::get (mActor).getNpcStats (mActor).getFactionRanks().begin()->first;

            std::set<std::string>& expelled = MWWorld::Class::get (player).getNpcStats (player).getExpelled();

            return expelled.find (faction)!=expelled.end();
        }

        case SelectWrapper::Function_PcVampire:

            return MWWorld::Class::get (player).getNpcStats (player).isVampire();

        case SelectWrapper::Function_TalkedToPc:

            return mTalkedToPlayer;

        case SelectWrapper::Function_Alarmed:

            return MWWorld::Class::get (mActor).getCreatureStats (mActor).isAlarmed();

        case SelectWrapper::Function_Detected:

            return MWWorld::Class::get (mActor).hasDetected (mActor, player);

        case SelectWrapper::Function_Attacked:

            return MWWorld::Class::get (mActor).getCreatureStats (mActor).getAttacked();

        case SelectWrapper::Function_ShouldAttack:

            return MWWorld::Class::get (mActor).getCreatureStats (mActor).isHostile();

        case SelectWrapper::Function_CreatureTargetted:

            return MWWorld::Class::get (mActor).getCreatureStats (mActor).getCreatureTargetted();

        case SelectWrapper::Function_PCWerewolf:

            return MWWorld::Class::get (player).getNpcStats (player).isWerewolf();

        default:

            throw std::runtime_error ("unknown boolean select function");
    }
}

int MWDialogue::Filter::getFactionRank (const MWWorld::Ptr& actor, const std::string& factionId) const
{
    MWMechanics::NpcStats& stats = MWWorld::Class::get (actor).getNpcStats (actor);

    std::map<std::string, int>::const_iterator iter = stats.getFactionRanks().find (factionId);

    if (iter==stats.getFactionRanks().end())
        return -1;

    return iter->second;
}

bool MWDialogue::Filter::hasFactionRankSkillRequirements (const MWWorld::Ptr& actor,
    const std::string& factionId, int rank) const
{
    if (rank<0 || rank>=10)
        throw std::runtime_error ("rank index out of range");

    if (!MWWorld::Class::get (actor).getNpcStats (actor).hasSkillsForRank (factionId, rank))
        return false;

    const ESM::Faction& faction =
        *MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find (factionId);

    MWMechanics::CreatureStats& stats = MWWorld::Class::get (actor).getCreatureStats (actor);

    return stats.getAttribute (faction.mData.mAttribute1).getBase()>=faction.mData.mRankData[rank].mAttribute1 &&
        stats.getAttribute (faction.mData.mAttribute2).getBase()>=faction.mData.mRankData[rank].mAttribute2;
}

bool MWDialogue::Filter::hasFactionRankReputationRequirements (const MWWorld::Ptr& actor,
    const std::string& factionId, int rank) const
{
    if (rank<0 || rank>=10)
        throw std::runtime_error ("rank index out of range");

    MWMechanics::NpcStats& stats = MWWorld::Class::get (actor).getNpcStats (actor);

    const ESM::Faction& faction =
        *MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find (factionId);

    return stats.getFactionReputation (factionId)>=faction.mData.mRankData[rank].mFactReaction;
}

MWDialogue::Filter::Filter (const MWWorld::Ptr& actor, int choice, bool talkedToPlayer)
: mActor (actor), mChoice (choice), mTalkedToPlayer (talkedToPlayer)
{}

bool MWDialogue::Filter::operator() (const ESM::DialInfo& info) const
{
    return testActor (info) && testPlayer (info) && testSelectStructs (info);
}

const ESM::DialInfo *MWDialogue::Filter::search (const ESM::Dialogue& dialogue) const
{
    for (std::vector<ESM::DialInfo>::const_iterator iter = dialogue.mInfo.begin();
        iter!=dialogue.mInfo.end(); ++iter)
        if ((*this) (*iter))
            return &*iter;

    return 0;
}

