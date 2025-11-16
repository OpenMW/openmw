#ifndef COMPONENTS_AIDIALOGUE_COMMANDINTERPRETER_HPP
#define COMPONENTS_AIDIALOGUE_COMMANDINTERPRETER_HPP

#include "dialoguegenerator.hpp"

#include <memory>
#include <set>
#include <string>
#include <vector>

namespace MWWorld
{
    class Ptr;
}

namespace MWBase
{
    class Journal;
    class World;
}

namespace AIDialogue
{
    /// \brief Validation result for a command
    struct ValidationResult
    {
        bool isValid = false;
        std::string errorMessage;
        std::string warningMessage;
    };

    /// \brief Executes AI-generated commands safely
    /// Only executes whitelisted commands with validation
    class CommandInterpreter
    {
    public:
        CommandInterpreter();
        ~CommandInterpreter();

        /// Execute a single command
        /// \param command Command to execute
        /// \param actor NPC issuing the command
        /// \param journal Journal system
        /// \param world World system
        /// \return true if executed successfully
        bool executeCommand(
            const GameCommand& command, const MWWorld::Ptr& actor, MWBase::Journal& journal, MWBase::World& world);

        /// Execute multiple commands in sequence
        /// \param commands List of commands
        /// \param actor NPC issuing commands
        /// \param journal Journal system
        /// \param world World system
        /// \return Number of commands executed successfully
        int executeCommands(const std::vector<GameCommand>& commands, const MWWorld::Ptr& actor,
            MWBase::Journal& journal, MWBase::World& world);

        /// Validate a command without executing it
        /// \param command Command to validate
        /// \return Validation result with error/warning messages
        ValidationResult validateCommand(const GameCommand& command);

        /// Enable/disable specific command types
        void setCommandEnabled(GameCommand::Type type, bool enabled);

        /// Check if a command type is enabled
        bool isCommandEnabled(GameCommand::Type type) const;

        /// Enable dry-run mode (validate but don't execute)
        void setDryRun(bool dryRun);

        /// Get list of commands executed in current session (for logging/debugging)
        const std::vector<GameCommand>& getExecutionHistory() const;

        /// Clear execution history
        void clearHistory();

    private:
        struct Impl;
        std::unique_ptr<Impl> mImpl;

        bool executeAddJournal(const GameCommand& command, const MWWorld::Ptr& actor, MWBase::Journal& journal);
        bool executeSetQuestIndex(const GameCommand& command, MWBase::Journal& journal);
        bool executeAddTopic(const GameCommand& command);
        bool executeModifyDisposition(const GameCommand& command, const MWWorld::Ptr& actor);
        bool executeGiveItem(const GameCommand& command, MWBase::World& world);
        bool executeTakeItem(const GameCommand& command, MWBase::World& world);
        bool executeSetGlobal(const GameCommand& command, MWBase::World& world);

        ValidationResult validateAddJournal(const GameCommand& command);
        ValidationResult validateSetQuestIndex(const GameCommand& command);
        ValidationResult validateModifyDisposition(const GameCommand& command);
        ValidationResult validateItemCommand(const GameCommand& command);
    };
}

#endif // COMPONENTS_AIDIALOGUE_COMMANDINTERPRETER_HPP
