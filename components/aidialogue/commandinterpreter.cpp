#include "commandinterpreter.hpp"

#include <apps/openmw/mwbase/dialoguemanager.hpp>
#include <apps/openmw/mwbase/environment.hpp>
#include <apps/openmw/mwbase/journal.hpp>
#include <apps/openmw/mwbase/world.hpp>

#include <apps/openmw/mwmechanics/creaturestats.hpp>
#include <apps/openmw/mwworld/class.hpp>
#include <apps/openmw/mwworld/containerstore.hpp>
#include <apps/openmw/mwworld/ptr.hpp>

#include <set>

namespace AIDialogue
{
    struct CommandInterpreter::Impl
    {
        std::set<GameCommand::Type> enabledCommands;
        std::vector<GameCommand> executionHistory;
        bool dryRun = false;

        Impl()
        {
            // Enable safe commands by default
            enabledCommands.insert(GameCommand::Type::AddJournal);
            enabledCommands.insert(GameCommand::Type::SetQuestIndex);
            enabledCommands.insert(GameCommand::Type::AddTopic);
            enabledCommands.insert(GameCommand::Type::ModifyDisposition);

            // Disable potentially dangerous commands by default
            // enabledCommands.insert(GameCommand::Type::GiveItem);
            // enabledCommands.insert(GameCommand::Type::TakeItem);
            // enabledCommands.insert(GameCommand::Type::SetGlobal);
            // enabledCommands.insert(GameCommand::Type::StartCombat);
        }
    };

    CommandInterpreter::CommandInterpreter()
        : mImpl(std::make_unique<Impl>())
    {
    }

    CommandInterpreter::~CommandInterpreter() = default;

    void CommandInterpreter::setCommandEnabled(GameCommand::Type type, bool enabled)
    {
        if (enabled)
            mImpl->enabledCommands.insert(type);
        else
            mImpl->enabledCommands.erase(type);
    }

    bool CommandInterpreter::isCommandEnabled(GameCommand::Type type) const
    {
        return mImpl->enabledCommands.count(type) > 0;
    }

    void CommandInterpreter::setDryRun(bool dryRun)
    {
        mImpl->dryRun = dryRun;
    }

    const std::vector<GameCommand>& CommandInterpreter::getExecutionHistory() const
    {
        return mImpl->executionHistory;
    }

    void CommandInterpreter::clearHistory()
    {
        mImpl->executionHistory.clear();
    }

    ValidationResult CommandInterpreter::validateCommand(const GameCommand& command)
    {
        ValidationResult result;

        // Check if command type is enabled
        if (!isCommandEnabled(command.type))
        {
            result.errorMessage = "Command type is not enabled";
            return result;
        }

        // Type-specific validation
        switch (command.type)
        {
            case GameCommand::Type::AddJournal:
                result = validateAddJournal(command);
                break;

            case GameCommand::Type::SetQuestIndex:
                result = validateSetQuestIndex(command);
                break;

            case GameCommand::Type::ModifyDisposition:
                result = validateModifyDisposition(command);
                break;

            case GameCommand::Type::GiveItem:
            case GameCommand::Type::TakeItem:
                result = validateItemCommand(command);
                break;

            default:
                result.isValid = true; // Other commands validated during execution
                break;
        }

        return result;
    }

    bool CommandInterpreter::executeCommand(
        const GameCommand& command, const MWWorld::Ptr& actor, MWBase::Journal& journal, MWBase::World& world)
    {
        // Validate first
        ValidationResult validation = validateCommand(command);
        if (!validation.isValid)
        {
            return false;
        }

        if (mImpl->dryRun)
        {
            // In dry-run mode, just record the command
            mImpl->executionHistory.push_back(command);
            return true;
        }

        bool success = false;

        // Execute based on type
        switch (command.type)
        {
            case GameCommand::Type::AddJournal:
                success = executeAddJournal(command, actor, journal);
                break;

            case GameCommand::Type::SetQuestIndex:
                success = executeSetQuestIndex(command, journal);
                break;

            case GameCommand::Type::AddTopic:
                success = executeAddTopic(command);
                break;

            case GameCommand::Type::ModifyDisposition:
                success = executeModifyDisposition(command, actor);
                break;

            case GameCommand::Type::GiveItem:
                success = executeGiveItem(command, world);
                break;

            case GameCommand::Type::TakeItem:
                success = executeTakeItem(command, world);
                break;

            case GameCommand::Type::SetGlobal:
                success = executeSetGlobal(command, world);
                break;

            default:
                success = false;
                break;
        }

        if (success)
        {
            mImpl->executionHistory.push_back(command);
        }

        return success;
    }

    int CommandInterpreter::executeCommands(const std::vector<GameCommand>& commands, const MWWorld::Ptr& actor,
        MWBase::Journal& journal, MWBase::World& world)
    {
        int successCount = 0;

        for (const auto& command : commands)
        {
            if (executeCommand(command, actor, journal, world))
            {
                successCount++;
            }
        }

        return successCount;
    }

    // Command execution implementations

    bool CommandInterpreter::executeAddJournal(const GameCommand& command, const MWWorld::Ptr& actor, MWBase::Journal& journal)
    {
        auto questIdIt = command.parameters.find("quest_id");
        auto indexIt = command.parameters.find("index");

        if (questIdIt == command.parameters.end() || indexIt == command.parameters.end())
            return false;

        try
        {
            ESM::RefId questId = ESM::RefId::stringRefId(questIdIt->second);
            int index = std::stoi(indexIt->second);

            journal.addEntry(questId, index, actor);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool CommandInterpreter::executeSetQuestIndex(const GameCommand& command, MWBase::Journal& journal)
    {
        auto questIdIt = command.parameters.find("quest_id");
        auto indexIt = command.parameters.find("index");

        if (questIdIt == command.parameters.end() || indexIt == command.parameters.end())
            return false;

        try
        {
            ESM::RefId questId = ESM::RefId::stringRefId(questIdIt->second);
            int index = std::stoi(indexIt->second);

            journal.setJournalIndex(questId, index);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool CommandInterpreter::executeAddTopic(const GameCommand& command)
    {
        auto topicIdIt = command.parameters.find("topic_id");

        if (topicIdIt == command.parameters.end())
            return false;

        try
        {
            ESM::RefId topicId = ESM::RefId::stringRefId(topicIdIt->second);
            MWBase::Environment::get().getDialogueManager()->addTopic(topicId);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool CommandInterpreter::executeModifyDisposition(const GameCommand& command, const MWWorld::Ptr& actor)
    {
        auto amountIt = command.parameters.find("amount");

        if (amountIt == command.parameters.end())
            return false;

        try
        {
            int amount = std::stoi(amountIt->second);

            // Limit disposition changes to prevent abuse
            if (amount < -20 || amount > 20)
                return false;

            if (actor.getClass().isNpc())
            {
                MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);
                int currentDisposition = static_cast<int>(stats.getAiSetting(MWMechanics::AiSetting::Hello).getModified());
                int newDisposition = std::max(0, std::min(100, currentDisposition + amount));
                stats.getAiSetting(MWMechanics::AiSetting::Hello).setModified(static_cast<float>(newDisposition), 0);
                return true;
            }

            return false;
        }
        catch (...)
        {
            return false;
        }
    }

    bool CommandInterpreter::executeGiveItem(const GameCommand& command, MWBase::World& world)
    {
        auto itemIdIt = command.parameters.find("item_id");
        auto countIt = command.parameters.find("count");

        if (itemIdIt == command.parameters.end())
            return false;

        try
        {
            ESM::RefId itemId = ESM::RefId::stringRefId(itemIdIt->second);
            int count = countIt != command.parameters.end() ? std::stoi(countIt->second) : 1;

            // Limit item count to prevent abuse
            if (count < 1 || count > 100)
                return false;

            MWWorld::Ptr player = world.getPlayerPtr();
            MWWorld::ContainerStore& store = player.getClass().getContainerStore(player);

            // Create item and add to inventory
            MWWorld::Ptr item = world.createRecord(itemId);
            store.add(item, count, false);

            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool CommandInterpreter::executeTakeItem(const GameCommand& command, MWBase::World& world)
    {
        auto itemIdIt = command.parameters.find("item_id");
        auto countIt = command.parameters.find("count");

        if (itemIdIt == command.parameters.end())
            return false;

        try
        {
            ESM::RefId itemId = ESM::RefId::stringRefId(itemIdIt->second);
            int count = countIt != command.parameters.end() ? std::stoi(countIt->second) : 1;

            MWWorld::Ptr player = world.getPlayerPtr();
            MWWorld::ContainerStore& store = player.getClass().getContainerStore(player);

            int removed = store.remove(itemId, count);
            return removed > 0;
        }
        catch (...)
        {
            return false;
        }
    }

    bool CommandInterpreter::executeSetGlobal(const GameCommand& command, MWBase::World& world)
    {
        auto variableIt = command.parameters.find("variable");
        auto valueIt = command.parameters.find("value");

        if (variableIt == command.parameters.end() || valueIt == command.parameters.end())
            return false;

        try
        {
            ESM::RefId varName = ESM::RefId::stringRefId(variableIt->second);

            // Only allow setting globals that already exist (safety check)
            if (world.getGlobalVariableType(varName) == ' ')
                return false;

            float value = std::stof(valueIt->second);
            world.setGlobalFloat(varName, value);

            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    // Validation implementations

    ValidationResult CommandInterpreter::validateAddJournal(const GameCommand& command)
    {
        ValidationResult result;

        if (command.parameters.count("quest_id") == 0)
        {
            result.errorMessage = "Missing quest_id parameter";
            return result;
        }

        if (command.parameters.count("index") == 0)
        {
            result.errorMessage = "Missing index parameter";
            return result;
        }

        try
        {
            int index = std::stoi(command.parameters.at("index"));
            if (index < 0 || index > 1000)
            {
                result.errorMessage = "Quest index out of valid range (0-1000)";
                return result;
            }
        }
        catch (...)
        {
            result.errorMessage = "Invalid index value";
            return result;
        }

        result.isValid = true;
        return result;
    }

    ValidationResult CommandInterpreter::validateSetQuestIndex(const GameCommand& command)
    {
        return validateAddJournal(command); // Same validation
    }

    ValidationResult CommandInterpreter::validateModifyDisposition(const GameCommand& command)
    {
        ValidationResult result;

        if (command.parameters.count("amount") == 0)
        {
            result.errorMessage = "Missing amount parameter";
            return result;
        }

        try
        {
            int amount = std::stoi(command.parameters.at("amount"));
            if (amount < -20 || amount > 20)
            {
                result.errorMessage = "Disposition change too large (limit: -20 to +20)";
                return result;
            }
        }
        catch (...)
        {
            result.errorMessage = "Invalid amount value";
            return result;
        }

        result.isValid = true;
        return result;
    }

    ValidationResult CommandInterpreter::validateItemCommand(const GameCommand& command)
    {
        ValidationResult result;

        if (command.parameters.count("item_id") == 0)
        {
            result.errorMessage = "Missing item_id parameter";
            return result;
        }

        if (command.parameters.count("count") > 0)
        {
            try
            {
                int count = std::stoi(command.parameters.at("count"));
                if (count < 1 || count > 100)
                {
                    result.errorMessage = "Item count out of range (1-100)";
                    return result;
                }
            }
            catch (...)
            {
                result.errorMessage = "Invalid count value";
                return result;
            }
        }

        result.isValid = true;
        return result;
    }
}
