#ifndef GAME_SCRIPT_INTERPRETERCONTEXT_H
#define GAME_SCRIPT_INTERPRETERCONTEXT_H

#include <components/interpreter/context.hpp>

#include "../mwworld/ptr.hpp"

namespace MWSound
{
    class SoundManager;
}

namespace MWInput
{
    struct MWInputManager;
}

namespace MWScript
{
    class Locals;

    class InterpreterContext : public Interpreter::Context
    {
            Locals *mLocals;
            mutable MWWorld::Ptr mReference;

            std::string mTargetId;

            /// If \a id is empty, a reference the script is run from is returned or in case
            /// of a non-local script the reference derived from the target ID.
            MWWorld::Ptr getReferenceImp (const std::string& id = "", bool activeOnly = false,
                bool doThrow=true);

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

            InterpreterContext (MWScript::Locals *locals, const MWWorld::Ptr& reference,
                const std::string& targetId = "");
            ///< The ownership of \a locals is not transferred. 0-pointer allowed.

            virtual int getLocalShort (int index) const;

            virtual int getLocalLong (int index) const;

            virtual float getLocalFloat (int index) const;

            virtual void setLocalShort (int index, int value);

            virtual void setLocalLong (int index, int value);

            virtual void setLocalFloat (int index, float value);

            using Interpreter::Context::messageBox;

            virtual void messageBox (const std::string& message,
                const std::vector<std::string>& buttons);

            virtual void report (const std::string& message);
            ///< By default, do nothing.

            virtual bool menuMode();

            virtual int getGlobalShort (const std::string& name) const;

            virtual int getGlobalLong (const std::string& name) const;

            virtual float getGlobalFloat (const std::string& name) const;

            virtual void setGlobalShort (const std::string& name, int value);

            virtual void setGlobalLong (const std::string& name, int value);

            virtual void setGlobalFloat (const std::string& name, float value);

            virtual std::vector<std::string> getGlobals () const;

            virtual char getGlobalType (const std::string& name) const;

            virtual std::string getActionBinding(const std::string& action) const;

            virtual std::string getActorName() const;

            virtual std::string getNPCRace() const;

            virtual std::string getNPCClass() const;

            virtual std::string getNPCFaction() const;

            virtual std::string getNPCRank() const;

            virtual std::string getPCName() const;

            virtual std::string getPCRace() const;

            virtual std::string getPCClass() const;

            virtual std::string getPCRank() const;

            virtual std::string getPCNextRank() const;

            virtual int getPCBounty() const;

            virtual std::string getCurrentCellName() const;

            virtual bool isScriptRunning (const std::string& name) const;

            virtual void startScript (const std::string& name, const std::string& targetId = "");

            virtual void stopScript (const std::string& name);

            virtual float getDistance (const std::string& name, const std::string& id = "") const;
            ///< @note if \a id is empty, assumes an implicit reference

            void executeActivation(MWWorld::Ptr ptr, MWWorld::Ptr actor);
            ///< Execute the activation action for this ptr. If ptr is mActivated, mark activation as handled.

            virtual float getSecondsPassed() const;

            virtual bool isDisabled (const std::string& id = "") const;

            virtual void enable (const std::string& id = "");

            virtual void disable (const std::string& id = "");

            virtual int getMemberShort (const std::string& id, const std::string& name, bool global) const;

            virtual int getMemberLong (const std::string& id, const std::string& name, bool global) const;

            virtual float getMemberFloat (const std::string& id, const std::string& name, bool global) const;

            virtual void setMemberShort (const std::string& id, const std::string& name, int value, bool global);

            virtual void setMemberLong (const std::string& id, const std::string& name, int value, bool global);

            virtual void setMemberFloat (const std::string& id, const std::string& name, float value, bool global);

            MWWorld::Ptr getReference(bool required=true);
            ///< Reference, that the script is running from (can be empty)

            void updatePtr(const MWWorld::Ptr& base, const MWWorld::Ptr& updated);
            ///< Update the Ptr stored in mReference, if there is one stored there. Should be called after the reference has been moved to a new cell.

            virtual std::string getTargetId() const;
    };
}

#endif
