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
            const MWWorld::Ptr getReferenceImp (const std::string& id = "",
                bool activeOnly = false, bool doThrow=true) const;

            const Locals& getMemberLocals (std::string& id, bool global) const;
            ///< \a id is changed to the respective script ID, if \a id wasn't a script ID before

            Locals& getMemberLocals (std::string& id, bool global);
            ///< \a id is changed to the respective script ID, if \a id wasn't a script ID before

            /// Throws an exception if local variable can't be found.
            int findLocalVariableIndex (const std::string& scriptId, const std::string& name,
                char type) const;

        public:
            InterpreterContext (std::shared_ptr<GlobalScriptDesc> globalScriptDesc);

            InterpreterContext (MWScript::Locals *locals, const MWWorld::Ptr& reference);
            ///< The ownership of \a locals is not transferred. 0-pointer allowed.

            std::string getTarget() const override;

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

            int getGlobalShort (const std::string& name) const override;

            int getGlobalLong (const std::string& name) const override;

            float getGlobalFloat (const std::string& name) const override;

            void setGlobalShort (const std::string& name, int value) override;

            void setGlobalLong (const std::string& name, int value) override;

            void setGlobalFloat (const std::string& name, float value) override;

            std::vector<std::string> getGlobals () const override;

            char getGlobalType (const std::string& name) const override;

            std::string getActionBinding(const std::string& action) const override;

            std::string getActorName() const override;

            std::string getNPCRace() const override;

            std::string getNPCClass() const override;

            std::string getNPCFaction() const override;

            std::string getNPCRank() const override;

            std::string getPCName() const override;

            std::string getPCRace() const override;

            std::string getPCClass() const override;

            std::string getPCRank() const override;

            std::string getPCNextRank() const override;

            int getPCBounty() const override;

            std::string getCurrentCellName() const override;

            void executeActivation(const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor);
            ///< Execute the activation action for this ptr. If ptr is mActivated, mark activation as handled.

            int getMemberShort (const std::string& id, const std::string& name, bool global) const override;

            int getMemberLong (const std::string& id, const std::string& name, bool global) const override;

            float getMemberFloat (const std::string& id, const std::string& name, bool global) const override;

            void setMemberShort (const std::string& id, const std::string& name, int value, bool global) override;

            void setMemberLong (const std::string& id, const std::string& name, int value, bool global) override;

            void setMemberFloat (const std::string& id, const std::string& name, float value, bool global) override;

            MWWorld::Ptr getReference(bool required=true) const;
            ///< Reference, that the script is running from (can be empty)

            void updatePtr(const MWWorld::Ptr& base, const MWWorld::Ptr& updated);
            ///< Update the Ptr stored in mReference, if there is one stored there. Should be called after the reference has been moved to a new cell.
    };
}

#endif
