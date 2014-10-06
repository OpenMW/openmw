
#include "filter.hpp"

#include <components/compiler/locals.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/scriptmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/magiceffects.hpp"

#include "selectwrapper.hpp"

bool MWDialogue::Filter::testActor (const ESM::DialInfo& info) const
{
    bool isCreature = (mActor.getTypeName() != typeid (ESM::NPC).name());

    // actor id
    if (!info.mActor.empty())
    {
        if ( !Misc::StringUtils::ciEqual(info.mActor, mActor.getClass().getId (mActor)))
            return false;
    }
    else if (isCreature)
    {
        // Creatures must not have topics aside of those specific to their id
        return false;
    }

    // NPC race
    if (!info.mRace.empty())
    {
        if (isCreature)
            return false;

        MWWorld::LiveCellRef<ESM::NPC> *cellRef = mActor.get<ESM::NPC>();

        if (!Misc::StringUtils::ciEqual(info.mRace, cellRef->mBase->mRace))
            return false;
    }

    // NPC class
    if (!info.mClass.empty())
    {
        if (isCreature)
            return false;

        MWWorld::LiveCellRef<ESM::NPC> *cellRef = mActor.get<ESM::NPC>();

        if ( !Misc::StringUtils::ciEqual(info.mClass, cellRef->mBase->mClass))
            return false;
    }

    // NPC faction
    if (!info.mFaction.empty())
    {
        if (isCreature)
            return false;

        MWMechanics::NpcStats& stats = mActor.getClass().getNpcStats (mActor);
        std::map<std::string, int>::const_iterator iter = stats.getFactionRanks().find ( Misc::StringUtils::lowerCase (info.mFaction));

        if (iter==stats.getFactionRanks().end())
            return false;

        // check rank
        if (iter->second < info.mData.mRank)
            return false;
    }
    else if (info.mData.mRank != -1)
    {
        if (isCreature)
            return false;

        // Rank requirement, but no faction given. Use the actor's faction, if there is one.
        MWMechanics::NpcStats& stats = mActor.getClass().getNpcStats (mActor);

        if (!stats.getFactionRanks().size())
            return false;

        // check rank
        if (stats.getFactionRanks().begin()->second < info.mData.mRank)
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
    const MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();

    // check player faction
    if (!info.mPcFaction.empty())
    {
        MWMechanics::NpcStats& stats = player.getClass().getNpcStats (player);
        std::map<std::string,int>::const_iterator iter = stats.getFactionRanks().find (Misc::StringUtils::lowerCase (info.mPcFaction));

        if(iter==stats.getFactionRanks().end())
            return false;

        // check rank
        if (iter->second < info.mData.mPCrank)
            return false;
    }

    // check cell
    if (!info.mCell.empty())
    {
        // supports partial matches, just like getPcCell
        const std::string& playerCell = player.getCell()->getCell()->mName;
        bool match = playerCell.length()>=info.mCell.length() &&
            Misc::StringUtils::ciEqual(playerCell.substr (0, info.mCell.length()), info.mCell);
        if (!match)
            return false;
    }

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

bool MWDialogue::Filter::testDisposition (const ESM::DialInfo& info, bool invert) const
{
    bool isCreature = (mActor.getTypeName() != typeid (ESM::NPC).name());

    if (isCreature)
        return true;

    int actorDisposition = MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mActor)
            + MWBase::Environment::get().getDialogueManager()->getTemporaryDispositionChange();
    // For service refusal, the disposition check is inverted. However, a value of 0 still means "always succeed".
    return invert ? (info.mData.mDisposition == 0 || actorDisposition < info.mData.mDisposition)
                  : (actorDisposition >= info.mData.mDisposition);
}

bool MWDialogue::Filter::testSelectStruct (const SelectWrapper& select) const
{
    if (select.isNpcOnly() && (mActor.getTypeName() != typeid (ESM::NPC).name()))
        // If the actor is a creature, we do not test the conditions applicable
        // only to NPCs. Such conditions can never be satisfied, apart
        // inverted ones (NotClass, NotRace, NotFaction return true
        // because creatures are not of any race, class or faction).
        return select.getType() == SelectWrapper::Type_Inverted;

    switch (select.getType())
    {
        case SelectWrapper::Type_None: return true;
        case SelectWrapper::Type_Integer: return select.selectCompare (getSelectStructInteger (select));
        case SelectWrapper::Type_Numeric: return testSelectStructNumeric (select);
        case SelectWrapper::Type_Boolean: return select.selectCompare (getSelectStructBoolean (select));

        // We must not do the comparison for inverted functions (eg. Function_NotClass)
        case SelectWrapper::Type_Inverted: return getSelectStructBoolean (select);
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
                MWBase::Environment::get().getWorld()->getGlobalFloat (select.getName()));

        case SelectWrapper::Function_Local:
        {
            std::string scriptName = mActor.getClass().getScript (mActor);

            if (scriptName.empty())
                return false; // no script

            std::string name = Misc::StringUtils::lowerCase (select.getName());

            const Compiler::Locals& localDefs =
                MWBase::Environment::get().getScriptManager()->getLocals (scriptName);

            char type = localDefs.getType (name);

            if (type==' ')
                return false; // script does not have a variable of this name.

            int index = localDefs.getIndex (name);

            const MWScript::Locals& locals = mActor.getRefData().getLocals();

            switch (type)
            {
                case 's': return select.selectCompare (static_cast<int> (locals.mShorts[index]));
                case 'l': return select.selectCompare (locals.mLongs[index]);
                case 'f': return select.selectCompare (locals.mFloats[index]);
            }

            throw std::logic_error ("unknown local variable type in dialogue filter");
        }

        case SelectWrapper::Function_PcHealthPercent:
        {
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();

            float ratio = player.getClass().getCreatureStats (player).getHealth().getCurrent() /
                player.getClass().getCreatureStats (player).getHealth().getModified();

            return select.selectCompare (static_cast<int>(ratio*100));
        }

        case SelectWrapper::Function_PcDynamicStat:
        {
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();

            float value = player.getClass().getCreatureStats (player).
                getDynamic (select.getArgument()).getCurrent();

            return select.selectCompare (value);
        }

        case SelectWrapper::Function_HealthPercent:
        {
            float ratio = mActor.getClass().getCreatureStats (mActor).getHealth().getCurrent() /
                mActor.getClass().getCreatureStats (mActor).getHealth().getModified();

            return select.selectCompare (static_cast<int>(ratio*100));
        }

        default:

            throw std::runtime_error ("unknown numeric select function");
    }
}

int MWDialogue::Filter::getSelectStructInteger (const SelectWrapper& select) const
{
    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();

    switch (select.getFunction())
    {
        case SelectWrapper::Function_Journal:

            return MWBase::Environment::get().getJournal()->getJournalIndex (select.getName());

        case SelectWrapper::Function_Item:
        {
            MWWorld::ContainerStore& store = player.getClass().getContainerStore (player);

            return store.count(select.getName());
        }

        case SelectWrapper::Function_Dead:

            return MWBase::Environment::get().getMechanicsManager()->countDeaths (select.getName());

        case SelectWrapper::Function_Choice:

            return mChoice;

        case SelectWrapper::Function_AiSetting:

            return mActor.getClass().getCreatureStats (mActor).getAiSetting (
                        (MWMechanics::CreatureStats::AiSetting)select.getArgument()).getModified();

        case SelectWrapper::Function_PcAttribute:

            return player.getClass().getCreatureStats (player).
                getAttribute (select.getArgument()).getModified();

        case SelectWrapper::Function_PcSkill:

            return static_cast<int> (player.getClass().
                getNpcStats (player).getSkill (select.getArgument()).getModified());

        case SelectWrapper::Function_FriendlyHit:
        {
            int hits = mActor.getClass().getCreatureStats (mActor).getFriendlyHits();

            return hits>4 ? 4 : hits;
        }

        case SelectWrapper::Function_PcLevel:

            return player.getClass().getCreatureStats (player).getLevel();

        case SelectWrapper::Function_PcGender:

            return player.get<ESM::NPC>()->mBase->isMale() ? 0 : 1;

        case SelectWrapper::Function_PcClothingModifier:
        {
            MWWorld::InventoryStore& store = player.getClass().getInventoryStore (player);

            int value = 0;

            for (int i=0; i<=15; ++i) // everything except thigns held in hands and amunition
            {
                MWWorld::ContainerStoreIterator slot = store.getSlot (i);

                if (slot!=store.end())
                    value += slot->getClass().getValue (*slot);
            }

            return value;
        }

        case SelectWrapper::Function_PcCrimeLevel:

            return player.getClass().getNpcStats (player).getBounty();

        case SelectWrapper::Function_RankRequirement:
        {
            if (mActor.getClass().getNpcStats (mActor).getFactionRanks().empty())
                return 0;

            std::string faction =
                mActor.getClass().getNpcStats (mActor).getFactionRanks().begin()->first;

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

            return mActor.getClass().getCreatureStats (mActor).getLevel();

        case SelectWrapper::Function_PCReputation:

            return player.getClass().getNpcStats (player).getReputation();

        case SelectWrapper::Function_Weather:

            return MWBase::Environment::get().getWorld()->getCurrentWeather();

        case SelectWrapper::Function_Reputation:

            return mActor.getClass().getNpcStats (mActor).getReputation();

        case SelectWrapper::Function_FactionRankDiff:
        {
            if (mActor.getClass().getNpcStats (mActor).getFactionRanks().empty())
                return 0;

            const std::pair<std::string, int> faction =
                *mActor.getClass().getNpcStats (mActor).getFactionRanks().begin();

            int rank = getFactionRank (player, faction.first);

            return rank-faction.second;
        }

        case SelectWrapper::Function_WerewolfKills:

            return player.getClass().getNpcStats (player).getWerewolfKills();

        case SelectWrapper::Function_RankLow:
        case SelectWrapper::Function_RankHigh:
        {
            bool low = select.getFunction()==SelectWrapper::Function_RankLow;

            if (mActor.getClass().getNpcStats (mActor).getFactionRanks().empty())
                return 0;

            std::string factionId =
                mActor.getClass().getNpcStats (mActor).getFactionRanks().begin()->first;

            int value = 0;

            MWMechanics::NpcStats& playerStats = player.getClass().getNpcStats (player);

            std::map<std::string, int>::const_iterator playerFactionIt = playerStats.getFactionRanks().begin();
            for (; playerFactionIt != playerStats.getFactionRanks().end(); ++playerFactionIt)
            {
                int reaction = MWBase::Environment::get().getDialogueManager()->getFactionReaction(factionId, playerFactionIt->first);
                if (low ? reaction < value : reaction > value)
                    value = reaction;
            }

            return value;
        }

        default:

            throw std::runtime_error ("unknown integer select function");
    }
}

bool MWDialogue::Filter::getSelectStructBoolean (const SelectWrapper& select) const
{
    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();

    switch (select.getFunction())
    {
        case SelectWrapper::Function_False:

            return false;

        case SelectWrapper::Function_NotId:

            return !Misc::StringUtils::ciEqual(mActor.getClass().getId (mActor), select.getName());

        case SelectWrapper::Function_NotFaction:

            return !Misc::StringUtils::ciEqual(mActor.get<ESM::NPC>()->mBase->mFaction, select.getName());

        case SelectWrapper::Function_NotClass:

            return !Misc::StringUtils::ciEqual(mActor.get<ESM::NPC>()->mBase->mClass, select.getName());

        case SelectWrapper::Function_NotRace:

            return !Misc::StringUtils::ciEqual(mActor.get<ESM::NPC>()->mBase->mRace, select.getName());

        case SelectWrapper::Function_NotCell:

            return !Misc::StringUtils::ciEqual(mActor.getCell()->getCell()->mName, select.getName());

        case SelectWrapper::Function_NotLocal:
        {
            std::string scriptName = mActor.getClass().getScript (mActor);

            if (scriptName.empty())
                // This actor has no attached script, so there is no local variable
                return true;

            const Compiler::Locals& localDefs =
                MWBase::Environment::get().getScriptManager()->getLocals (scriptName);

            return localDefs.getIndex (Misc::StringUtils::lowerCase (select.getName()))==-1;
        }

        case SelectWrapper::Function_SameGender:

            return (player.get<ESM::NPC>()->mBase->mFlags & ESM::NPC::Female)==
                (mActor.get<ESM::NPC>()->mBase->mFlags & ESM::NPC::Female);

        case SelectWrapper::Function_SameRace:

            return !Misc::StringUtils::ciEqual(mActor.get<ESM::NPC>()->mBase->mRace, player.get<ESM::NPC>()->mBase->mRace);

        case SelectWrapper::Function_SameFaction:

            return mActor.getClass().getNpcStats (mActor).isSameFaction (
                player.getClass().getNpcStats (player));

        case SelectWrapper::Function_PcCommonDisease:

            return player.getClass().getCreatureStats (player).hasCommonDisease();

        case SelectWrapper::Function_PcBlightDisease:

            return player.getClass().getCreatureStats (player).hasBlightDisease();

        case SelectWrapper::Function_PcCorprus:

            return player.getClass().getCreatureStats (player).
                getMagicEffects().get (ESM::MagicEffect::Corprus).getMagnitude()!=0;

        case SelectWrapper::Function_PcExpelled:
        {
            if (mActor.getClass().getNpcStats (mActor).getFactionRanks().empty())
                return false;

            std::string faction =
                mActor.getClass().getNpcStats (mActor).getFactionRanks().begin()->first;

            return player.getClass().getNpcStats(player).getExpelled(faction);
        }

        case SelectWrapper::Function_PcVampire:

            return player.getClass().getCreatureStats(player).getMagicEffects().
                    get(ESM::MagicEffect::Vampirism).getMagnitude() > 0;

        case SelectWrapper::Function_TalkedToPc:

            return mTalkedToPlayer;

        case SelectWrapper::Function_Alarmed:

            return mActor.getClass().getCreatureStats (mActor).isAlarmed();

        case SelectWrapper::Function_Detected:

            return MWBase::Environment::get().getMechanicsManager()->awarenessCheck(player, mActor);

        case SelectWrapper::Function_Attacked:

            return mActor.getClass().getCreatureStats (mActor).getAttacked();

        case SelectWrapper::Function_ShouldAttack:

            return MWBase::Environment::get().getMechanicsManager()->isAggressive(mActor,
                    MWBase::Environment::get().getWorld()->getPlayerPtr());

        case SelectWrapper::Function_CreatureTargetted:

            return mActor.getClass().getCreatureStats (mActor).getCreatureTargetted();

        case SelectWrapper::Function_Werewolf:

            return mActor.getClass().getNpcStats (mActor).isWerewolf();

        default:

            throw std::runtime_error ("unknown boolean select function");
    }
}

int MWDialogue::Filter::getFactionRank (const MWWorld::Ptr& actor, const std::string& factionId) const
{
    MWMechanics::NpcStats& stats = actor.getClass().getNpcStats (actor);

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

    if (!actor.getClass().getNpcStats (actor).hasSkillsForRank (factionId, rank))
        return false;

    const ESM::Faction& faction =
        *MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find (factionId);

    MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats (actor);

    return stats.getAttribute (faction.mData.mAttribute[0]).getBase()>=faction.mData.mRankData[rank].mAttribute1 &&
        stats.getAttribute (faction.mData.mAttribute[1]).getBase()>=faction.mData.mRankData[rank].mAttribute2;
}

bool MWDialogue::Filter::hasFactionRankReputationRequirements (const MWWorld::Ptr& actor,
    const std::string& factionId, int rank) const
{
    if (rank<0 || rank>=10)
        throw std::runtime_error ("rank index out of range");

    MWMechanics::NpcStats& stats = actor.getClass().getNpcStats (actor);

    const ESM::Faction& faction =
        *MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find (factionId);

    return stats.getFactionReputation (factionId)>=faction.mData.mRankData[rank].mFactReaction;
}

MWDialogue::Filter::Filter (const MWWorld::Ptr& actor, int choice, bool talkedToPlayer)
: mActor (actor), mChoice (choice), mTalkedToPlayer (talkedToPlayer)
{}

const ESM::DialInfo* MWDialogue::Filter::search (const ESM::Dialogue& dialogue, const bool fallbackToInfoRefusal) const
{
    std::vector<const ESM::DialInfo *> suitableInfos = list (dialogue, fallbackToInfoRefusal, false);

    if (suitableInfos.empty())
        return NULL;
    else
        return suitableInfos[0];
}

std::vector<const ESM::DialInfo *> MWDialogue::Filter::list (const ESM::Dialogue& dialogue,
    bool fallbackToInfoRefusal, bool searchAll, bool invertDisposition) const
{
    std::vector<const ESM::DialInfo *> infos;

    bool infoRefusal = false;

    // Iterate over topic responses to find a matching one
    for (ESM::Dialogue::InfoContainer::const_iterator iter = dialogue.mInfo.begin();
        iter!=dialogue.mInfo.end(); ++iter)
    {
        if (testActor (*iter) && testPlayer (*iter) && testSelectStructs (*iter))
        {
            if (testDisposition (*iter, invertDisposition)) {
                infos.push_back(&*iter);
                if (!searchAll)
                    break;
            }
            else
                infoRefusal = true;
        }
    }

    if (infos.empty() && infoRefusal && fallbackToInfoRefusal)
    {
        // No response is valid because of low NPC disposition,
        // search a response in the topic "Info Refusal"

        const MWWorld::Store<ESM::Dialogue> &dialogues =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>();

        const ESM::Dialogue& infoRefusalDialogue = *dialogues.find ("Info Refusal");

        for (ESM::Dialogue::InfoContainer::const_iterator iter = infoRefusalDialogue.mInfo.begin();
            iter!=infoRefusalDialogue.mInfo.end(); ++iter)
            if (testActor (*iter) && testPlayer (*iter) && testSelectStructs (*iter) && testDisposition(*iter, invertDisposition)) {
                infos.push_back(&*iter);
                if (!searchAll)
                    break;
            }
    }

    return infos;
}

bool MWDialogue::Filter::responseAvailable (const ESM::Dialogue& dialogue) const
{
    for (ESM::Dialogue::InfoContainer::const_iterator iter = dialogue.mInfo.begin();
        iter!=dialogue.mInfo.end(); ++iter)
    {
        if (testActor (*iter) && testPlayer (*iter) && testSelectStructs (*iter))
            return true;
    }

    return false;
}
