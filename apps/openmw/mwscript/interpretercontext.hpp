#ifndef GAME_SCRIPT_INTERPRETERCONTEXT_H
#define GAME_SCRIPT_INTERPRETERCONTEXT_H

#include <boost/shared_ptr.hpp>

#include <components/interpreter/context.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"
#include "../mwworld/action.hpp"

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
    struct Locals;

    class InterpreterContext : public Interpreter::Context
    {
            MWWorld::Environment& mEnvironment;
            Locals *mLocals;
            MWWorld::Ptr mReference;

            MWWorld::Ptr mActivated;
            bool mActivationHandled;
            boost::shared_ptr<MWWorld::Action> mAction;

            MWWorld::Ptr getReference (const std::string& id, bool activeOnly);

            const MWWorld::Ptr getReference (const std::string& id, bool activeOnly) const;

        public:

            InterpreterContext (MWWorld::Environment& environment,
                MWScript::Locals *locals, MWWorld::Ptr reference);
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
            ///< By default echo via messageBox.

            virtual bool menuMode();

            virtual int getGlobalShort (const std::string& name) const;

            virtual int getGlobalLong (const std::string& name) const;

            virtual float getGlobalFloat (const std::string& name) const;

            virtual void setGlobalShort (const std::string& name, int value);

            virtual void setGlobalLong (const std::string& name, int value);

            virtual void setGlobalFloat (const std::string& name, float value);

            virtual bool isScriptRunning (const std::string& name) const;

            virtual void startScript (const std::string& name);

            virtual void stopScript (const std::string& name);

            virtual float getDistance (const std::string& name, const std::string& id = "") const;

            bool hasBeenActivated (const MWWorld::Ptr& ptr);
            ///< \attention Calling this function for the right reference will mark the action as
            /// been handled.

            bool hasActivationBeenHandled() const;

            void activate (const MWWorld::Ptr& ptr, boost::shared_ptr<MWWorld::Action> action);
            ///< Store reference acted upon and action. The actual execution of the action does not
            /// take place here.

            void executeActivation();
            ///< Execute the action defined by the last activate call.

            void clearActivation();
            ///< Discard the action defined by the last activate call.

            virtual float getSecondsPassed() const;

            virtual bool isDisabled (const std::string& id = "") const;

            virtual void enable (const std::string& id = "");

            virtual void disable (const std::string& id = "");

            MWWorld::Environment& getEnvironment();

            /// \todo remove the following functions (extentions should use getEnvironment instead)

            MWWorld::World& getWorld();

            MWSound::SoundManager& getSoundManager();

            MWGui::WindowManager& getWindowManager();

            MWInput::MWInputManager& getInputManager();

            MWWorld::Ptr getReference();
            ///< Reference, that the script is running from (can be empty)
    };
}

#endif
