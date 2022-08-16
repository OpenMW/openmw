#ifndef GAME_SCRIPT_INTERPRETERCONTEXT_H
#define GAME_SCRIPT_INTERPRETERCONTEXT_H

#include <memory>
#include <stdexcept>

#include <components/interpreter/context.hpp>

#include "globalscripts.hpp"

#include "../mwworld/ptr.hpp"

namespace MWScript
{
    class Locals;

    class MissingImplicitRefError : public std::runtime_error
    {
        public:
            MissingImplicitRefError();
    };

    class InterpreterContext : public Interpreter::Context
    {
            Locals *mLocals;
            mutable MWWorld::Ptr mReference;
            std::shared_ptr<GlobalScriptDesc> mGlobalScriptDesc;

            /// If \a id is empty, a reference the script is run from is returned or in case
            /// of a non-local script the reference derived from the target ID.
            const MWWorld::Ptr getReferenceImp(std::string_view id = {}, bool activeOnly = false, bool doThrow = true) const;

            const Locals& getMemberLocals(std::string_view& id, bool global) const;
            ///< \a id is changed to the respective script ID, if \a id wasn't a script ID before

            Locals& getMemberLocals(std::string_view& id, bool global);
            ///< \a id is changed to the respective script ID, if \a id wasn't a script ID before

            /// Throws an exception if local variable can't be found.
            int findLocalVariableIndex(std::string_view scriptId, std::string_view name, char type) const;

        public:
            InterpreterContext (std::shared_ptr<GlobalScriptDesc> globalScriptDesc);

            InterpreterContext (MWScript::Locals *locals, const MWWorld::Ptr& reference);
            ///< The ownership of \a locals is not transferred. 0-pointer allowed.

            std::string_view getTarget() const override;

            int getLocalShort (int index) const override;

            int getLocalLong (int index) const override;

            float getLocalFloat (int index) const override;

            void setLocalShort (int index, int value) override;

            void setLocalLong (int index, int value) override;

            void setLocalFloat (int index, float value) override;

            using Interpreter::Context::messageBox;

            void messageBox (const std::string& message,
                const std::vector<std::string>& buttons) override;

            void report (const std::string& message) override;
            ///< By default, do nothing.

            int getGlobalShort(std::string_view name) const override;

            int getGlobalLong(std::string_view name) const override;

            float getGlobalFloat(std::string_view name) const override;

            void setGlobalShort(std::string_view name, int value) override;

            void setGlobalLong(std::string_view name, int value) override;

            void setGlobalFloat(std::string_view name, float value) override;

            std::vector<std::string> getGlobals () const override;

            char getGlobalType(std::string_view name) const override;

            std::string getActionBinding(std::string_view action) const override;

            std::string_view getActorName() const override;

            std::string_view getNPCRace() const override;

            std::string_view getNPCClass() const override;

            std::string_view getNPCFaction() const override;

            std::string_view getNPCRank() const override;

            std::string_view getPCName() const override;

            std::string_view getPCRace() const override;

            std::string_view getPCClass() const override;

            std::string_view getPCRank() const override;

            std::string_view getPCNextRank() const override;

            int getPCBounty() const override;

            std::string_view getCurrentCellName() const override;

            void executeActivation(const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor);
            ///< Execute the activation action for this ptr. If ptr is mActivated, mark activation as handled.

            int getMemberShort(std::string_view id, std::string_view name, bool global) const override;

            int getMemberLong(std::string_view id, std::string_view name, bool global) const override;

            float getMemberFloat(std::string_view id, std::string_view name, bool global) const override;

            void setMemberShort(std::string_view id, std::string_view name, int value, bool global) override;

            void setMemberLong(std::string_view id, std::string_view name, int value, bool global) override;

            void setMemberFloat(std::string_view id, std::string_view name, float value, bool global) override;

            MWWorld::Ptr getReference(bool required=true) const;
            ///< Reference, that the script is running from (can be empty)

            void updatePtr(const MWWorld::Ptr& base, const MWWorld::Ptr& updated);
            ///< Update the Ptr stored in mReference, if there is one stored there. Should be called after the reference has been moved to a new cell.
    };
}

#endif
