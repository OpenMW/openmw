#include "filter.hpp"

#include <components/compiler/locals.hpp>
#include <components/esm/refid.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadfact.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadskil.hpp>

#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/magiceffects.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "selectwrapper.hpp"

namespace
{
    bool matchesStaticFilters(const MWDialogue::SelectWrapper& select, const MWWorld::Ptr& actor)
    {
        const ESM::RefId selectId = select.getId();
        if (select.getFunction() == ESM::DialogueCondition::Function_NotId)
            return actor.getCellRef().getRefId() != selectId;
        if (actor.getClass().isNpc())
        {
            if (select.getFunction() == ESM::DialogueCondition::Function_NotFaction)
                return actor.getClass().getPrimaryFaction(actor) != selectId;
            else if (select.getFunction() == ESM::DialogueCondition::Function_NotClass)
                return actor.get<ESM::NPC>()->mBase->mClass != selectId;
            else if (select.getFunction() == ESM::DialogueCondition::Function_NotRace)
                return actor.get<ESM::NPC>()->mBase->mRace != selectId;
        }
        return true;
    }

    bool matchesStaticFilters(const ESM::DialInfo& info, const MWWorld::Ptr& actor)
    {
        for (const auto& select : info.mSelects)
        {
            MWDialogue::SelectWrapper wrapper = select;
            if (wrapper.getType() == MWDialogue::SelectWrapper::Type_Boolean)
            {
                if (!wrapper.selectCompare(matchesStaticFilters(wrapper, actor)))
                    return false;
            }
            else if (wrapper.getType() == MWDialogue::SelectWrapper::Type_Inverted)
            {
                if (!matchesStaticFilters(wrapper, actor))
                    return false;
            }
            else if (wrapper.getType() == MWDialogue::SelectWrapper::Type_Numeric)
            {
                if (wrapper.getFunction() == ESM::DialogueCondition::Function_Local)
                {
                    const ESM::RefId& scriptName = actor.getClass().getScript(actor);
                    if (scriptName.empty())
                        return false;
                    const Compiler::Locals& localDefs
                        = MWBase::Environment::get().getScriptManager()->getLocals(scriptName);
                    char type = localDefs.getType(wrapper.getName());
                    if (type == ' ')
                        return false; // script does not have a variable of this name.
                }
            }
        }
        return true;
    }
}

bool MWDialogue::Filter::testActor(const ESM::DialInfo& info) const
{
    bool isCreature = (mActor.getType() != ESM::NPC::sRecordId);

    // actor id
    if (!info.mActor.empty())
    {
        if (info.mActor != mActor.getCellRef().getRefId())
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
            return true;

        MWWorld::LiveCellRef<ESM::NPC>* cellRef = mActor.get<ESM::NPC>();

        if (!(info.mRace == cellRef->mBase->mRace))
            return false;
    }

    // NPC class
    if (!info.mClass.empty())
    {
        if (isCreature)
            return true;

        MWWorld::LiveCellRef<ESM::NPC>* cellRef = mActor.get<ESM::NPC>();

        if (!(info.mClass == cellRef->mBase->mClass))
            return false;
    }

    // NPC faction
    if (info.mFactionLess)
    {
        if (isCreature)
            return true;

        if (!mActor.getClass().getPrimaryFaction(mActor).empty())
            return false;
    }
    else if (!info.mFaction.empty())
    {
        if (isCreature)
            return true;

        if (!(mActor.getClass().getPrimaryFaction(mActor) == info.mFaction))
            return false;

        // check rank
        if (mActor.getClass().getPrimaryFactionRank(mActor) < info.mData.mRank)
            return false;
    }
    else if (info.mData.mRank != -1)
    {
        if (isCreature)
            return true;

        // Rank requirement, but no faction given. Use the actor's faction, if there is one.
        // check rank
        if (mActor.getClass().getPrimaryFactionRank(mActor) < info.mData.mRank)
            return false;
    }

    // Gender
    if (!isCreature)
    {
        MWWorld::LiveCellRef<ESM::NPC>* npc = mActor.get<ESM::NPC>();
        if (info.mData.mGender == ((npc->mBase->mFlags & ESM::NPC::Female) ? 0 : 1))
            return false;
    }

    return true;
}

bool MWDialogue::Filter::testPlayer(const ESM::DialInfo& info) const
{
    const MWWorld::Ptr player = MWMechanics::getPlayer();
    MWMechanics::NpcStats& stats = player.getClass().getNpcStats(player);

    // check player faction and rank
    if (!info.mPcFaction.empty())
    {
        std::map<ESM::RefId, int>::const_iterator iter = stats.getFactionRanks().find(info.mPcFaction);

        if (iter == stats.getFactionRanks().end())
            return false;

        // check rank
        if (iter->second < info.mData.mPCrank)
            return false;
    }
    else if (info.mData.mPCrank != -1)
    {
        // required PC faction is not specified but PC rank is; use speaker's faction
        std::map<ESM::RefId, int>::const_iterator iter
            = stats.getFactionRanks().find(mActor.getClass().getPrimaryFaction(mActor));

        if (iter == stats.getFactionRanks().end())
            return false;

        // check rank
        if (iter->second < info.mData.mPCrank)
            return false;
    }

    // check cell
    if (!info.mCell.empty())
    {
        // supports partial matches, just like getPcCell
        std::string_view playerCell = MWBase::Environment::get().getWorld()->getCellName(player.getCell());
        if (!Misc::StringUtils::ciStartsWith(playerCell, info.mCell.getRefIdString()))
            return false;
    }

    return true;
}

bool MWDialogue::Filter::testSelectStructs(const ESM::DialInfo& info) const
{
    for (const auto& select : info.mSelects)
        if (!testSelectStruct(select))
            return false;

    return true;
}

bool MWDialogue::Filter::testDisposition(const ESM::DialInfo& info, bool invert) const
{
    bool isCreature = (mActor.getType() != ESM::NPC::sRecordId);

    if (isCreature)
        return true;

    int actorDisposition = MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mActor);
    // For service refusal, the disposition check is inverted. However, a value of 0 still means "always succeed".
    return invert ? (info.mData.mDisposition == 0 || actorDisposition < info.mData.mDisposition)
                  : (actorDisposition >= info.mData.mDisposition);
}

bool MWDialogue::Filter::testFunctionLocal(const MWDialogue::SelectWrapper& select) const
{
    const ESM::RefId& scriptName = mActor.getClass().getScript(mActor);

    if (scriptName.empty())
        return false; // no script

    std::string name = select.getName();

    const Compiler::Locals& localDefs = MWBase::Environment::get().getScriptManager()->getLocals(scriptName);

    char type = localDefs.getType(name);

    if (type == ' ')
        return false; // script does not have a variable of this name.

    int index = localDefs.getIndex(name);
    if (index < 0)
        return false; // shouldn't happen, we checked that variable has a type above, so must exist

    const MWScript::Locals& locals = mActor.getRefData().getLocals();
    if (locals.isEmpty())
        return select.selectCompare(0);
    switch (type)
    {
        case 's':
            return select.selectCompare(static_cast<int>(locals.mShorts[index]));
        case 'l':
            return select.selectCompare(locals.mLongs[index]);
        case 'f':
            return select.selectCompare(locals.mFloats[index]);
    }

    throw std::logic_error("unknown local variable type in dialogue filter");
}

bool MWDialogue::Filter::testSelectStruct(const SelectWrapper& select) const
{
    if (select.getFunction() == ESM::DialogueCondition::Function_Choice && mChoice == -1)
        // If not currently in a choice, we reject all conditions that test against choices.
        return false;

    if (select.getFunction() == ESM::DialogueCondition::Function_Weather
        && !(MWBase::Environment::get().getWorld()->isCellExterior()
            || MWBase::Environment::get().getWorld()->isCellQuasiExterior()))
        // Reject weather conditions in interior cells
        // Note that the original engine doesn't include the "|| isCellQuasiExterior()" check, which could be considered
        // a bug.
        return false;

    switch (select.getType())
    {
        case SelectWrapper::Type_None:
            return true;
        case SelectWrapper::Type_Integer:
            return select.selectCompare(getSelectStructInteger(select));
        case SelectWrapper::Type_Numeric:
            return testSelectStructNumeric(select);
        case SelectWrapper::Type_Boolean:
            return select.selectCompare(getSelectStructBoolean(select));

        // We must not do the comparison for inverted functions (eg. Function_NotClass)
        case SelectWrapper::Type_Inverted:
            return getSelectStructBoolean(select);
    }

    return true;
}

bool MWDialogue::Filter::testSelectStructNumeric(const SelectWrapper& select) const
{
    switch (select.getFunction())
    {
        case ESM::DialogueCondition::Function_Global:
        {
            const auto& world = MWBase::Environment::get().getWorld();
            if (world->getGlobalVariableType(select.getName()) == ' ')
                return true; // ignore this filter if the global doesn't exist
            // internally all globals are float :(
            return select.selectCompare(world->getGlobalFloat(select.getName()));
        }

        case ESM::DialogueCondition::Function_Local:
        {
            return testFunctionLocal(select);
        }

        case ESM::DialogueCondition::Function_NotLocal:
        {
            return !testFunctionLocal(select);
        }

        case ESM::DialogueCondition::Function_PcHealthPercent:
        {
            MWWorld::Ptr player = MWMechanics::getPlayer();
            return select.selectCompare(
                static_cast<int>(player.getClass().getCreatureStats(player).getHealth().getRatio() * 100));
        }

        case ESM::DialogueCondition::Function_PcMagicka:
        case ESM::DialogueCondition::Function_PcFatigue:
        case ESM::DialogueCondition::Function_PcHealth:
        {
            MWWorld::Ptr player = MWMechanics::getPlayer();

            float value = player.getClass().getCreatureStats(player).getDynamic(select.getArgument()).getCurrent();

            return select.selectCompare(value);
        }

        case ESM::DialogueCondition::Function_Health_Percent:
        {
            return select.selectCompare(
                static_cast<int>(mActor.getClass().getCreatureStats(mActor).getHealth().getRatio() * 100));
        }

        default:

            throw std::runtime_error("unknown numeric select function");
    }
}

int MWDialogue::Filter::getSelectStructInteger(const SelectWrapper& select) const
{
    MWWorld::Ptr player = MWMechanics::getPlayer();

    switch (select.getFunction())
    {
        case ESM::DialogueCondition::Function_Journal:

            return MWBase::Environment::get().getJournal()->getJournalIndex(select.getId());

        case ESM::DialogueCondition::Function_Item:
        {
            MWWorld::ContainerStore& store = player.getClass().getContainerStore(player);

            return store.count(select.getId());
        }

        case ESM::DialogueCondition::Function_Dead:

            return MWBase::Environment::get().getMechanicsManager()->countDeaths(select.getId());

        case ESM::DialogueCondition::Function_Choice:

            return mChoice;

        case ESM::DialogueCondition::Function_Fight:
        case ESM::DialogueCondition::Function_Hello:
        case ESM::DialogueCondition::Function_Alarm:
        case ESM::DialogueCondition::Function_Flee:
        {
            int argument = select.getArgument();
            if (argument < 0 || argument > 3)
            {
                throw std::runtime_error("AiSetting index is out of range");
            }

            return mActor.getClass()
                .getCreatureStats(mActor)
                .getAiSetting(static_cast<MWMechanics::AiSetting>(argument))
                .getModified(false);
        }
        case ESM::DialogueCondition::Function_PcStrength:
        case ESM::DialogueCondition::Function_PcIntelligence:
        case ESM::DialogueCondition::Function_PcWillpower:
        case ESM::DialogueCondition::Function_PcAgility:
        case ESM::DialogueCondition::Function_PcSpeed:
        case ESM::DialogueCondition::Function_PcEndurance:
        case ESM::DialogueCondition::Function_PcPersonality:
        case ESM::DialogueCondition::Function_PcLuck:
        {
            ESM::RefId attribute = ESM::Attribute::indexToRefId(select.getArgument());
            return player.getClass().getCreatureStats(player).getAttribute(attribute).getModified();
        }
        case ESM::DialogueCondition::Function_PcBlock:
        case ESM::DialogueCondition::Function_PcArmorer:
        case ESM::DialogueCondition::Function_PcMediumArmor:
        case ESM::DialogueCondition::Function_PcHeavyArmor:
        case ESM::DialogueCondition::Function_PcBluntWeapon:
        case ESM::DialogueCondition::Function_PcLongBlade:
        case ESM::DialogueCondition::Function_PcAxe:
        case ESM::DialogueCondition::Function_PcSpear:
        case ESM::DialogueCondition::Function_PcAthletics:
        case ESM::DialogueCondition::Function_PcEnchant:
        case ESM::DialogueCondition::Function_PcDestruction:
        case ESM::DialogueCondition::Function_PcAlteration:
        case ESM::DialogueCondition::Function_PcIllusion:
        case ESM::DialogueCondition::Function_PcConjuration:
        case ESM::DialogueCondition::Function_PcMysticism:
        case ESM::DialogueCondition::Function_PcRestoration:
        case ESM::DialogueCondition::Function_PcAlchemy:
        case ESM::DialogueCondition::Function_PcUnarmored:
        case ESM::DialogueCondition::Function_PcSecurity:
        case ESM::DialogueCondition::Function_PcSneak:
        case ESM::DialogueCondition::Function_PcAcrobatics:
        case ESM::DialogueCondition::Function_PcLightArmor:
        case ESM::DialogueCondition::Function_PcShortBlade:
        case ESM::DialogueCondition::Function_PcMarksman:
        case ESM::DialogueCondition::Function_PcMerchantile:
        case ESM::DialogueCondition::Function_PcSpeechcraft:
        case ESM::DialogueCondition::Function_PcHandToHand:
        {
            ESM::RefId skill = ESM::Skill::indexToRefId(select.getArgument());
            return static_cast<int>(player.getClass().getNpcStats(player).getSkill(skill).getModified());
        }
        case ESM::DialogueCondition::Function_FriendHit:
        {
            int hits = mActor.getClass().getCreatureStats(mActor).getFriendlyHits();

            return hits > 4 ? 4 : hits;
        }

        case ESM::DialogueCondition::Function_PcLevel:

            return player.getClass().getCreatureStats(player).getLevel();

        case ESM::DialogueCondition::Function_PcGender:

            return player.get<ESM::NPC>()->mBase->isMale() ? 0 : 1;

        case ESM::DialogueCondition::Function_PcClothingModifier:
        {
            const MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);

            int value = 0;

            for (int i = 0; i <= 15; ++i) // everything except things held in hands and ammunition
            {
                MWWorld::ConstContainerStoreIterator slot = store.getSlot(i);

                if (slot != store.end())
                    value += slot->getClass().getValue(*slot);
            }

            return value;
        }

        case ESM::DialogueCondition::Function_PcCrimeLevel:

            return player.getClass().getNpcStats(player).getBounty();

        case ESM::DialogueCondition::Function_RankRequirement:
        {
            const ESM::RefId& faction = mActor.getClass().getPrimaryFaction(mActor);
            if (faction.empty())
                return 0;

            int rank = getFactionRank(player, faction);

            if (rank >= 9)
                return 0; // max rank

            int result = 0;

            if (hasFactionRankSkillRequirements(player, faction, rank + 1))
                result += 1;

            if (hasFactionRankReputationRequirements(player, faction, rank + 1))
                result += 2;

            return result;
        }

        case ESM::DialogueCondition::Function_Level:

            return mActor.getClass().getCreatureStats(mActor).getLevel();

        case ESM::DialogueCondition::Function_PcReputation:

            return player.getClass().getNpcStats(player).getReputation();

        case ESM::DialogueCondition::Function_Weather:

            return MWBase::Environment::get().getWorld()->getCurrentWeather();

        case ESM::DialogueCondition::Function_Reputation:
            if (!mActor.getClass().isNpc())
                return 0;
            return mActor.getClass().getNpcStats(mActor).getReputation();

        case ESM::DialogueCondition::Function_FactionRankDifference:
        {
            const ESM::RefId& faction = mActor.getClass().getPrimaryFaction(mActor);

            if (faction.empty())
                return 0;

            int rank = getFactionRank(player, faction);
            int npcRank = mActor.getClass().getPrimaryFactionRank(mActor);
            return rank - npcRank;
        }

        case ESM::DialogueCondition::Function_PcWerewolfKills:

            return player.getClass().getNpcStats(player).getWerewolfKills();

        case ESM::DialogueCondition::Function_FacReactionLowest:
        case ESM::DialogueCondition::Function_FacReactionHighest:
        {
            bool low = select.getFunction() == ESM::DialogueCondition::Function_FacReactionLowest;

            const ESM::RefId& factionId = mActor.getClass().getPrimaryFaction(mActor);

            if (factionId.empty())
                return 0;

            int value = 0;

            MWMechanics::NpcStats& playerStats = player.getClass().getNpcStats(player);

            std::map<ESM::RefId, int>::const_iterator playerFactionIt = playerStats.getFactionRanks().begin();
            for (; playerFactionIt != playerStats.getFactionRanks().end(); ++playerFactionIt)
            {
                int reaction = MWBase::Environment::get().getDialogueManager()->getFactionReaction(
                    factionId, playerFactionIt->first);
                if (low ? reaction < value : reaction > value)
                    value = reaction;
            }

            return value;
        }

        case ESM::DialogueCondition::Function_CreatureTarget:

        {
            MWWorld::Ptr target;
            mActor.getClass().getCreatureStats(mActor).getAiSequence().getCombatTarget(target);
            if (!target.isEmpty())
            {
                if (target.getClass().isNpc() && target.getClass().getNpcStats(target).isWerewolf())
                    return 2;
                if (target.getType() == ESM::Creature::sRecordId)
                    return 1;
            }
        }
            return 0;

        default:

            throw std::runtime_error("unknown integer select function");
    }
}

bool MWDialogue::Filter::getSelectStructBoolean(const SelectWrapper& select) const
{
    MWWorld::Ptr player = MWMechanics::getPlayer();

    switch (select.getFunction())
    {
        case ESM::DialogueCondition::Function_NotId:

            return mActor.getCellRef().getRefId() != select.getId();

        case ESM::DialogueCondition::Function_NotFaction:

            return mActor.getClass().getPrimaryFaction(mActor) != select.getId();

        case ESM::DialogueCondition::Function_NotClass:

            return !mActor.getClass().isNpc() || mActor.get<ESM::NPC>()->mBase->mClass != select.getId();

        case ESM::DialogueCondition::Function_NotRace:

            return !mActor.getClass().isNpc() || mActor.get<ESM::NPC>()->mBase->mRace != select.getId();

        case ESM::DialogueCondition::Function_NotCell:
        {
            std::string_view actorCell = MWBase::Environment::get().getWorld()->getCellName(mActor.getCell());
            return !Misc::StringUtils::ciStartsWith(actorCell, select.getCellName());
        }
        case ESM::DialogueCondition::Function_SameSex:
            if (!mActor.getClass().isNpc())
                return false;
            return (player.get<ESM::NPC>()->mBase->mFlags & ESM::NPC::Female)
                == (mActor.get<ESM::NPC>()->mBase->mFlags & ESM::NPC::Female);

        case ESM::DialogueCondition::Function_SameRace:
            if (!mActor.getClass().isNpc())
                return false;
            return mActor.get<ESM::NPC>()->mBase->mRace == player.get<ESM::NPC>()->mBase->mRace;

        case ESM::DialogueCondition::Function_SameFaction:

            return player.getClass().getNpcStats(player).isInFaction(mActor.getClass().getPrimaryFaction(mActor));

        case ESM::DialogueCondition::Function_PcCommonDisease:

            return player.getClass().getCreatureStats(player).hasCommonDisease();

        case ESM::DialogueCondition::Function_PcBlightDisease:

            return player.getClass().getCreatureStats(player).hasBlightDisease();

        case ESM::DialogueCondition::Function_PcCorprus:

            return player.getClass()
                       .getCreatureStats(player)
                       .getMagicEffects()
                       .getOrDefault(ESM::MagicEffect::Corprus)
                       .getMagnitude()
                != 0;

        case ESM::DialogueCondition::Function_PcExpelled:
        {
            const ESM::RefId& faction = mActor.getClass().getPrimaryFaction(mActor);

            if (faction.empty())
                return false;

            return player.getClass().getNpcStats(player).getExpelled(faction);
        }

        case ESM::DialogueCondition::Function_PcVampire:

            return player.getClass()
                       .getCreatureStats(player)
                       .getMagicEffects()
                       .getOrDefault(ESM::MagicEffect::Vampirism)
                       .getMagnitude()
                > 0;

        case ESM::DialogueCondition::Function_TalkedToPc:

            return mTalkedToPlayer;

        case ESM::DialogueCondition::Function_Alarmed:

            return mActor.getClass().getCreatureStats(mActor).isAlarmed();

        case ESM::DialogueCondition::Function_Detected:

            return MWBase::Environment::get().getMechanicsManager()->awarenessCheck(player, mActor);

        case ESM::DialogueCondition::Function_Attacked:

            return mActor.getClass().getCreatureStats(mActor).getAttacked();

        case ESM::DialogueCondition::Function_ShouldAttack:

            return MWBase::Environment::get().getMechanicsManager()->isAggressive(mActor, MWMechanics::getPlayer());

        case ESM::DialogueCondition::Function_Werewolf:

            return mActor.getClass().isNpc() && mActor.getClass().getNpcStats(mActor).isWerewolf();

        default:

            throw std::runtime_error("unknown boolean select function");
    }
}

int MWDialogue::Filter::getFactionRank(const MWWorld::Ptr& actor, const ESM::RefId& factionId) const
{
    MWMechanics::NpcStats& stats = actor.getClass().getNpcStats(actor);
    return stats.getFactionRank(factionId);
}

bool MWDialogue::Filter::hasFactionRankSkillRequirements(
    const MWWorld::Ptr& actor, const ESM::RefId& factionId, int rank) const
{
    if (!actor.getClass().getNpcStats(actor).hasSkillsForRank(factionId, rank))
        return false;

    const ESM::Faction& faction = *MWBase::Environment::get().getESMStore()->get<ESM::Faction>().find(factionId);

    MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);

    return stats.getAttribute(ESM::Attribute::indexToRefId(faction.mData.mAttribute[0])).getBase()
        >= faction.mData.mRankData[rank].mAttribute1
        && stats.getAttribute(ESM::Attribute::indexToRefId(faction.mData.mAttribute[1])).getBase()
        >= faction.mData.mRankData[rank].mAttribute2;
}

bool MWDialogue::Filter::hasFactionRankReputationRequirements(
    const MWWorld::Ptr& actor, const ESM::RefId& factionId, int rank) const
{
    MWMechanics::NpcStats& stats = actor.getClass().getNpcStats(actor);

    const ESM::Faction& faction = *MWBase::Environment::get().getESMStore()->get<ESM::Faction>().find(factionId);

    return stats.getFactionReputation(factionId) >= faction.mData.mRankData.at(rank).mFactReaction;
}

MWDialogue::Filter::Filter(const MWWorld::Ptr& actor, int choice, bool talkedToPlayer)
    : mActor(actor)
    , mChoice(choice)
    , mTalkedToPlayer(talkedToPlayer)
{
}

MWDialogue::Filter::Response MWDialogue::Filter::search(
    const ESM::Dialogue& dialogue, const bool fallbackToInfoRefusal) const
{
    auto suitableInfos = list(dialogue, fallbackToInfoRefusal, false);

    if (suitableInfos.empty())
        return {};
    else
        return suitableInfos[0];
}

bool MWDialogue::Filter::couldPotentiallyMatch(const ESM::DialInfo& info) const
{
    return testActor(info) && matchesStaticFilters(info, mActor);
}

std::vector<MWDialogue::Filter::Response> MWDialogue::Filter::list(
    const ESM::Dialogue& dialogue, bool fallbackToInfoRefusal, bool searchAll, bool invertDisposition) const
{
    std::vector<MWDialogue::Filter::Response> infos;

    bool infoRefusal = false;

    // Iterate over topic responses to find a matching one
    for (const auto& info : dialogue.mInfo)
    {
        if (testActor(info) && testPlayer(info) && testSelectStructs(info))
        {
            if (testDisposition(info, invertDisposition))
            {
                infos.emplace_back(&dialogue, &info);
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

        const MWWorld::Store<ESM::Dialogue>& dialogues = MWBase::Environment::get().getESMStore()->get<ESM::Dialogue>();

        const ESM::Dialogue& infoRefusalDialogue = *dialogues.find(ESM::RefId::stringRefId("Info Refusal"));

        for (const auto& info : infoRefusalDialogue.mInfo)
            if (testActor(info) && testPlayer(info) && testSelectStructs(info)
                && testDisposition(info, invertDisposition))
            {
                infos.emplace_back(&infoRefusalDialogue, &info);
                if (!searchAll)
                    break;
            }
    }

    return infos;
}
