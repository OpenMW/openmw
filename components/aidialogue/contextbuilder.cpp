#include "contextbuilder.hpp"

#include <components/esm/attr.hpp>
#include <components/esm3/loadclas.hpp>
#include <components/esm3/loadfact.hpp>
#include <components/esm3/loadinfo.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadrace.hpp>
#include <components/esm3/loadregn.hpp>

#include <apps/openmw/mwbase/environment.hpp>
#include <apps/openmw/mwbase/journal.hpp>
#include <apps/openmw/mwbase/world.hpp>

#include <apps/openmw/mwmechanics/creaturestats.hpp>
#include <apps/openmw/mwmechanics/npcstats.hpp>

#include <apps/openmw/mwworld/class.hpp>
#include <apps/openmw/mwworld/containerstore.hpp>
#include <apps/openmw/mwworld/esmstore.hpp>
#include <apps/openmw/mwworld/ptr.hpp>

#include <sstream>

namespace AIDialogue
{
    struct ContextBuilder::Impl
    {
        std::vector<std::pair<std::string, std::string>> conversationHistory;
        int maxJournalEntries = 20;
        int maxHistoryExchanges = 5;
    };

    ContextBuilder::ContextBuilder()
        : mImpl(std::make_unique<Impl>())
    {
    }

    ContextBuilder::~ContextBuilder() = default;

    void ContextBuilder::addExchange(const std::string& playerInput, const std::string& npcResponse)
    {
        mImpl->conversationHistory.emplace_back(playerInput, npcResponse);

        // Keep only recent exchanges to manage token limits
        if (mImpl->conversationHistory.size() > static_cast<size_t>(mImpl->maxHistoryExchanges))
        {
            mImpl->conversationHistory.erase(mImpl->conversationHistory.begin());
        }
    }

    void ContextBuilder::clearHistory()
    {
        mImpl->conversationHistory.clear();
    }

    void ContextBuilder::setMaxJournalEntries(int max)
    {
        mImpl->maxJournalEntries = max;
    }

    DialogueContext ContextBuilder::buildContext(
        const MWWorld::Ptr& actor, const ESM::RefId& topic, const MWBase::Journal& journal)
    {
        DialogueContext context;

        // Extract NPC information
        context.npcName = actor.getClass().getName(actor);

        if (actor.get<ESM::NPC>()->mBase)
        {
            const ESM::NPC* npc = actor.get<ESM::NPC>()->mBase;
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

            // Get race with description
            const ESM::Race* race = store.get<ESM::Race>().search(npc->mRace);
            if (race)
            {
                context.npcRace = race->mName;
                // Add race description if available
                if (!race->mDescription.empty())
                {
                    context.npcRace += " (" + race->mDescription.substr(0, 100) + ")";
                }
            }

            // Get class with specialization
            const ESM::Class* cls = store.get<ESM::Class>().search(npc->mClass);
            if (cls)
            {
                context.npcClass = cls->mName;
                // Add class description if available
                if (!cls->mDescription.empty())
                {
                    context.npcClass += " - " + cls->mDescription.substr(0, 100);
                }
            }

            // Get gender
            context.npcGender = npc->isMale() ? "male" : "female";

            // Get faction with rank and description
            if (!npc->mFaction.empty())
            {
                const ESM::Faction* faction = store.get<ESM::Faction>().search(npc->mFaction);
                if (faction)
                {
                    context.npcFaction = faction->mName;

                    // Get NPC's rank in faction
                    if (actor.getClass().isNpc())
                    {
                        const MWMechanics::NpcStats& stats = actor.getClass().getNpcStats(actor);
                        std::map<ESM::RefId, int> factionRanks = stats.getFactionRanks();
                        auto rankIt = factionRanks.find(npc->mFaction);
                        if (rankIt != factionRanks.end() && rankIt->second >= 0
                            && static_cast<size_t>(rankIt->second) < faction->mRanks.size())
                        {
                            context.npcFaction
                                += " (Rank: " + faction->mRanks[rankIt->second].mName + ")";
                        }
                    }
                }
            }

            // Get NPC skills and attributes for personality
            if (actor.getClass().isNpc())
            {
                const MWMechanics::NpcStats& npcStats = actor.getClass().getNpcStats(actor);
                const MWMechanics::CreatureStats& creatureStats = actor.getClass().getCreatureStats(actor);

                // Notable attributes (for character flavor)
                int personality = static_cast<int>(
                    creatureStats.getAttribute(ESM::Attribute::Personality).getModified());
                int intelligence = static_cast<int>(
                    creatureStats.getAttribute(ESM::Attribute::Intelligence).getModified());
                int strength = static_cast<int>(
                    creatureStats.getAttribute(ESM::Attribute::Strength).getModified());

                // Add character traits based on attributes
                std::string traits;
                if (personality > 60)
                    traits += "charismatic, ";
                if (intelligence > 60)
                    traits += "intelligent, ";
                if (strength > 60)
                    traits += "strong, ";

                if (!traits.empty())
                {
                    traits = traits.substr(0, traits.length() - 2); // Remove trailing comma
                    context.npcClass += " [" + traits + "]";
                }
            }
        }

        // Get NPC disposition
        if (actor.getClass().isNpc())
        {
            context.disposition = static_cast<int>(actor.getClass().getCreatureStats(actor).getAiSetting(
                MWMechanics::AiSetting::Hello).getModified());
        }

        // Extract player information
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        context.playerName = player.getClass().getName(player);
        context.playerLevel = player.getClass().getCreatureStats(player).getLevel();

        if (player.get<ESM::NPC>()->mBase)
        {
            const ESM::NPC* playerNpc = player.get<ESM::NPC>()->mBase;
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

            const ESM::Race* playerRace = store.get<ESM::Race>().search(playerNpc->mRace);
            if (playerRace)
                context.playerRace = playerRace->mName;

            const ESM::Class* playerClass = store.get<ESM::Class>().search(playerNpc->mClass);
            if (playerClass)
                context.playerClass = playerClass->mName;
        }

        // Get player skills (top skills only)
        if (player.getClass().isNpc())
        {
            const MWMechanics::NpcStats& stats = player.getClass().getNpcStats(player);
            // Get a few notable skills for context
            context.playerSkills["combat"] = static_cast<int>(stats.getSkill(ESM::Skill::LongBlade).getModified());
            context.playerSkills["magic"] = static_cast<int>(stats.getSkill(ESM::Skill::Destruction).getModified());
            context.playerSkills["stealth"] = static_cast<int>(stats.getSkill(ESM::Skill::Sneak).getModified());
        }

        // Get notable items (weapons, quest items, etc.)
        // For now, just count gold
        MWWorld::ContainerStore& store = player.getClass().getContainerStore(player);
        int gold = store.count(ESM::RefId::stringRefId("gold_001"));
        if (gold > 0)
        {
            context.notableItems.push_back("Gold: " + std::to_string(gold));
        }

        // Get relevant journal entries
        context.relevantJournalEntries = getRelevantJournalEntries(topic, context.npcFaction, journal);

        // Set current topic
        context.currentTopic = topic.toDebugString();

        // Add conversation history
        context.recentExchanges = mImpl->conversationHistory;

        // Get detailed location information
        const MWWorld::CellStore* cell = player.getCell();
        if (cell)
        {
            const ESM::Cell* cellData = cell->getCell();
            if (cellData)
            {
                // Get cell name/description
                context.location = cellData->getDescription();

                // Add region info if exterior
                if (cellData->isExterior() && !cellData->mRegion.empty())
                {
                    const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
                    const ESM::Region* region = store.get<ESM::Region>().search(cellData->mRegion);
                    if (region)
                    {
                        context.location += " (Region: " + region->mName + ")";
                    }
                }

                // Add interior/exterior context
                if (cellData->isExterior())
                {
                    // Outdoor location
                    context.location = "Outdoors in " + context.location;

                    // Get time of day
                    MWBase::World* world = MWBase::Environment::get().getWorld();
                    float hour = world->getTimeStamp().getHour();

                    if (hour >= 6.0f && hour < 8.0f)
                        context.timeOfDay = "early morning";
                    else if (hour >= 8.0f && hour < 12.0f)
                        context.timeOfDay = "morning";
                    else if (hour >= 12.0f && hour < 17.0f)
                        context.timeOfDay = "afternoon";
                    else if (hour >= 17.0f && hour < 20.0f)
                        context.timeOfDay = "evening";
                    else if (hour >= 20.0f && hour < 23.0f)
                        context.timeOfDay = "night";
                    else
                        context.timeOfDay = "late night";

                    // Get weather
                    int weatherType = world->getCurrentWeather();
                    switch (weatherType)
                    {
                        case 0:
                            context.weather = "clear";
                            break;
                        case 1:
                            context.weather = "cloudy";
                            break;
                        case 2:
                            context.weather = "foggy";
                            break;
                        case 3:
                            context.weather = "overcast";
                            break;
                        case 4:
                            context.weather = "rainy";
                            break;
                        case 5:
                            context.weather = "thunderstorm";
                            break;
                        case 6:
                            context.weather = "ashstorm";
                            break;
                        case 7:
                            context.weather = "blight";
                            break;
                        case 8:
                            context.weather = "snowy";
                            break;
                        case 9:
                            context.weather = "blizzard";
                            break;
                        default:
                            context.weather = "unknown";
                            break;
                    }
                }
                else
                {
                    // Indoor location
                    context.location = "Inside " + context.location;
                }
            }
        }

        // Get special player states
        const MWMechanics::NpcStats& playerStats = player.getClass().getNpcStats(player);
        context.playerIsVampire = playerStats.isVampire();
        context.playerIsWerewolf = playerStats.isWerewolf();

        // Get player bounty
        context.playerBounty = playerStats.getBounty();

        return context;
    }

    std::vector<DialogueContext::JournalEntryInfo> ContextBuilder::getRelevantJournalEntries(
        const ESM::RefId& topic, const std::string& npcFaction, const MWBase::Journal& journal)
    {
        std::vector<DialogueContext::JournalEntryInfo> entries;

        // Get all quests
        for (auto questIter = journal.questBegin(); questIter != journal.questEnd(); ++questIter)
        {
            const MWDialogue::Quest& quest = questIter->second;
            ESM::RefId questId = questIter->first;

            DialogueContext::JournalEntryInfo info;
            info.questId = questId.toDebugString();
            info.questIndex = quest.getIndex();
            info.isFinished = quest.isFinished();
            info.isActive = !quest.isFinished();

            // Get the most recent entry for this quest
            for (auto entryIter = quest.begin(); entryIter != quest.end(); ++entryIter)
            {
                info.text = entryIter->getText();
            }

            if (!info.text.empty())
            {
                entries.push_back(info);
            }

            // Limit entries to manage token count
            if (entries.size() >= static_cast<size_t>(mImpl->maxJournalEntries))
                break;
        }

        return entries;
    }

    std::string ContextBuilder::getNPCPersonality(const DialogueContext& context)
    {
        std::ostringstream personality;

        personality << "You are " << context.npcName;

        if (!context.npcRace.empty())
            personality << ", a " << context.npcRace;

        if (!context.npcClass.empty())
            personality << " " << context.npcClass;

        personality << ".";

        if (!context.npcFaction.empty())
        {
            personality << " You are a member of " << context.npcFaction << ".";
        }

        // Add disposition-based personality hints
        if (context.disposition < 30)
        {
            personality << " You are hostile and distrustful of strangers.";
        }
        else if (context.disposition < 50)
        {
            personality << " You are cautious and reserved.";
        }
        else if (context.disposition < 70)
        {
            personality << " You are friendly but professional.";
        }
        else
        {
            personality << " You are warm and helpful.";
        }

        return personality.str();
    }

    std::string ContextBuilder::formatAsSystemPrompt(const DialogueContext& context)
    {
        std::ostringstream prompt;

        // Core personality and role (ENHANCED)
        prompt << "=== CHARACTER YOU ARE PLAYING ===\n";
        prompt << getNPCPersonality(context) << "\n";

        // Add detailed NPC info
        if (!context.npcRace.empty())
            prompt << "Race: " << context.npcRace << "\n";
        if (!context.npcClass.empty())
            prompt << "Class: " << context.npcClass << "\n";
        if (!context.npcFaction.empty())
            prompt << "Faction: " << context.npcFaction << "\n";
        prompt << "Disposition toward player: " << context.disposition << "/100\n";
        prompt << "\n";

        // World context (ENHANCED)
        prompt << "=== CURRENT SETTING ===\n";
        prompt << "Location: " << context.location << "\n";
        if (!context.timeOfDay.empty())
            prompt << "Time of Day: " << context.timeOfDay << "\n";
        if (!context.weather.empty())
            prompt << "Weather: " << context.weather << "\n";
        prompt << "\n";

        // Player information (ENHANCED)
        prompt << "=== ABOUT THE PLAYER ===\n";
        prompt << "Name: " << context.playerName << "\n";
        if (!context.playerRace.empty())
            prompt << "Race: " << context.playerRace << "\n";
        if (!context.playerClass.empty())
            prompt << "Class: " << context.playerClass << "\n";
        prompt << "Level: " << context.playerLevel << "\n";

        // Special statuses
        if (context.playerIsVampire)
            prompt << "⚠️ WARNING: This person is a VAMPIRE\n";
        if (context.playerIsWerewolf)
            prompt << "⚠️ WARNING: This person is a WEREWOLF\n";
        if (context.playerBounty > 0)
            prompt << "⚠️ This person has a bounty: " << context.playerBounty << " gold\n";

        // Notable skills
        if (!context.playerSkills.empty())
        {
            prompt << "\nNotable Skills:\n";
            for (const auto& skill : context.playerSkills)
            {
                if (skill.second > 30)
                { // Only show notable skills
                    prompt << "- " << skill.first << ": " << skill.second << "\n";
                }
            }
        }

        // Notable items
        if (!context.notableItems.empty())
        {
            prompt << "\nCarrying:\n";
            for (const auto& item : context.notableItems)
            {
                prompt << "- " << item << "\n";
            }
        }

        prompt << "\n";

        // Journal context (KEY FEATURE! - ENHANCED)
        if (!context.relevantJournalEntries.empty())
        {
            prompt << "=== QUEST KNOWLEDGE (What you know about ongoing events) ===\n";
            for (const auto& entry : context.relevantJournalEntries)
            {
                prompt << "\n📜 " << entry.questId;
                if (entry.isFinished)
                    prompt << " [COMPLETED]";
                else if (entry.isActive)
                    prompt << " [ACTIVE - Stage " << entry.questIndex << "]";
                else
                    prompt << " [Stage " << entry.questIndex << "]";
                prompt << "\n";
                prompt << "   " << entry.text << "\n";
            }
            prompt << "\n";
        }

        // Conversation history (ENHANCED)
        if (!context.recentExchanges.empty())
        {
            prompt << "=== RECENT CONVERSATION ===\n";
            for (const auto& exchange : context.recentExchanges)
            {
                prompt << "👤 Player: " << exchange.first << "\n";
                prompt << "💬 You: " << exchange.second << "\n";
            }
            prompt << "\n";
        }

        // Current topic
        if (!context.currentTopic.empty())
        {
            prompt << "=== CURRENT TOPIC ===\n";
            prompt << context.currentTopic << "\n\n";
        }

        // Instructions for AI behavior (ENHANCED)
        prompt << "=== INSTRUCTIONS ===\n";
        prompt << "1. ✓ Stay in character as " << context.npcName << " at all times\n";
        prompt << "2. ✓ Respond naturally and concisely (2-4 sentences preferred)\n";
        prompt << "3. ✓ Reference quest knowledge and world details when relevant\n";
        prompt << "4. ✓ Maintain tone appropriate for disposition (" << context.disposition
               << "/100): ";
        if (context.disposition < 30)
            prompt << "hostile, unfriendly\n";
        else if (context.disposition < 50)
            prompt << "cautious, reserved\n";
        else if (context.disposition < 70)
            prompt << "friendly, professional\n";
        else
            prompt << "warm, helpful\n";

        prompt << "5. ✓ Consider the setting: " << context.location;
        if (!context.timeOfDay.empty())
            prompt << " at " << context.timeOfDay;
        if (!context.weather.empty())
            prompt << " (" << context.weather << ")";
        prompt << "\n";

        prompt << "6. ✓ React to player's special status (vampire/werewolf/bounty) if appropriate\n";
        prompt << "7. ✓ Use game state commands when needed:\n";
        prompt << "   • [COMMAND:ADD_JOURNAL:quest_id:index:text] - Add/update quest\n";
        prompt << "   • [COMMAND:ADD_TOPIC:topic_id] - Unlock new topic\n";
        prompt << "   • [COMMAND:MODIFY_DISPOSITION:±amount] - Change relationship\n";
        prompt << "\n8. ✓ Respond in JSON: {\"dialogue\": \"your response\", \"commands\": []}\n";

        return prompt.str();
    }
}
