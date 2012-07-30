
#include "interpretercontext.hpp"

#include <cmath>
#include <stdexcept>

#include <components/interpreter/types.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"

#include "../mwgui/window_manager.hpp"

#include "../mwinput/inputmanager.hpp"

#include "locals.hpp"
#include "globalscripts.hpp"
#include "scriptmanager.hpp"

namespace MWScript
{
    MWWorld::Ptr InterpreterContext::getReference (
        const std::string& id, bool activeOnly)
    {
        if (!id.empty())
        {
            return MWBase::Environment::get().getWorld()->getPtr (id, activeOnly);
        }
        else
        {
            if (mReference.isEmpty())
                throw std::runtime_error ("no implicit reference");

            return mReference;
        }
    }

    const MWWorld::Ptr InterpreterContext::getReference (
        const std::string& id, bool activeOnly) const
    {
        if (!id.empty())
        {
            return MWBase::Environment::get().getWorld()->getPtr (id, activeOnly);
        }
        else
        {
            if (mReference.isEmpty())
                throw std::runtime_error ("no implicit reference");

            return mReference;
        }
    }

    InterpreterContext::InterpreterContext (
        MWScript::Locals *locals, MWWorld::Ptr reference)
    : mLocals (locals), mReference (reference),
      mActivationHandled (false)
    {}

    int InterpreterContext::getLocalShort (int index) const
    {
        if (!mLocals)
            throw std::runtime_error ("local variables not available in this context");

        return mLocals->mShorts.at (index);
    }

    int InterpreterContext::getLocalLong (int index) const
    {
        if (!mLocals)
            throw std::runtime_error ("local variables not available in this context");

        return mLocals->mLongs.at (index);
    }

    float InterpreterContext::getLocalFloat (int index) const
    {
        if (!mLocals)
            throw std::runtime_error ("local variables not available in this context");

        return mLocals->mFloats.at (index);
    }

    void InterpreterContext::setLocalShort (int index, int value)
    {
        if (!mLocals)
            throw std::runtime_error ("local variables not available in this context");

        mLocals->mShorts.at (index) = value;
    }

    void InterpreterContext::setLocalLong (int index, int value)
    {
        if (!mLocals)
            throw std::runtime_error ("local variables not available in this context");

        mLocals->mLongs.at (index) = value;
    }

    void InterpreterContext::setLocalFloat (int index, float value)
    {
        if (!mLocals)
            throw std::runtime_error ("local variables not available in this context");

        mLocals->mFloats.at (index) = value;
    }

    void InterpreterContext::messageBox (const std::string& message,
        const std::vector<std::string>& buttons)
    {
        MWBase::Environment::get().getWindowManager()->messageBox (message, buttons);
    }

    void InterpreterContext::report (const std::string& message)
    {
        messageBox (message);
    }

    bool InterpreterContext::menuMode()
    {
        return MWBase::Environment::get().getWindowManager()->isGuiMode();
    }

    int InterpreterContext::getGlobalShort (const std::string& name) const
    {
        return MWBase::Environment::get().getWorld()->getGlobalVariable (name).mShort;
    }

    int InterpreterContext::getGlobalLong (const std::string& name) const
    {
        // a global long is internally a float.
        return MWBase::Environment::get().getWorld()->getGlobalVariable (name).mLong;
    }

    float InterpreterContext::getGlobalFloat (const std::string& name) const
    {
        return MWBase::Environment::get().getWorld()->getGlobalVariable (name).mFloat;
    }

    void InterpreterContext::setGlobalShort (const std::string& name, int value)
    {
        if (name=="gamehour")
            MWBase::Environment::get().getWorld()->setHour (value);
        else if (name=="day")
            MWBase::Environment::get().getWorld()->setDay (value);
        else if (name=="month")
            MWBase::Environment::get().getWorld()->setMonth (value);
        else
            MWBase::Environment::get().getWorld()->getGlobalVariable (name).mShort = value;
    }

    void InterpreterContext::setGlobalLong (const std::string& name, int value)
    {
        if (name=="gamehour")
            MWBase::Environment::get().getWorld()->setHour (value);
        else if (name=="day")
            MWBase::Environment::get().getWorld()->setDay (value);
        else if (name=="month")
            MWBase::Environment::get().getWorld()->setMonth (value);
        else
            MWBase::Environment::get().getWorld()->getGlobalVariable (name).mLong = value;
    }

    void InterpreterContext::setGlobalFloat (const std::string& name, float value)
    {
        if (name=="gamehour")
            MWBase::Environment::get().getWorld()->setHour (value);
        else if (name=="day")
            MWBase::Environment::get().getWorld()->setDay (value);
        else if (name=="month")
            MWBase::Environment::get().getWorld()->setMonth (value);
        else
            MWBase::Environment::get().getWorld()->getGlobalVariable (name).mFloat = value;
    }

    bool InterpreterContext::isScriptRunning (const std::string& name) const
    {
        return MWBase::Environment::get().getScriptManager()->getGlobalScripts().isRunning (name);
    }

    void InterpreterContext::startScript (const std::string& name)
    {
        MWBase::Environment::get().getScriptManager()->getGlobalScripts().addScript (name);
    }

    void InterpreterContext::stopScript (const std::string& name)
    {
        MWBase::Environment::get().getScriptManager()->getGlobalScripts().removeScript (name);
    }

    float InterpreterContext::getDistance (const std::string& name, const std::string& id) const
    {
        // TODO handle exterior cells (when ref and ref2 are located in different cells)
        const MWWorld::Ptr ref2 = getReference (id, false);

        const MWWorld::Ptr ref = MWBase::Environment::get().getWorld()->getPtr (name, true);

        double diff[3];

        const float* const pos1 = ref.getRefData().getPosition().pos;
        const float* const pos2 = ref2.getRefData().getPosition().pos;
        for (int i=0; i<3; ++i)
            diff[i] = pos1[i] - pos2[i];

        return std::sqrt (diff[0]*diff[0] + diff[1]*diff[1] + diff[2]*diff[2]);
    }

    bool InterpreterContext::hasBeenActivated (const MWWorld::Ptr& ptr)
    {
        if (!mActivated.isEmpty() && mActivated==ptr)
        {
            mActivationHandled = true;
            return true;
        }

        return false;
    }

    bool InterpreterContext::hasActivationBeenHandled() const
    {
        return mActivationHandled;
    }

    void InterpreterContext::activate (const MWWorld::Ptr& ptr,
        boost::shared_ptr<MWWorld::Action> action)
    {
        mActivated = ptr;
        mActivationHandled = false;
        mAction = action;
    }

    void InterpreterContext::executeActivation()
    {
        if (!mAction.get())
            throw std::runtime_error ("activation failed, because no action to perform");

        mAction->execute (MWBase::Environment::get().getWorld()->getPlayer().getPlayer());
        mActivationHandled = true;
    }

    void InterpreterContext::clearActivation()
    {
        mActivated = MWWorld::Ptr();
        mActivationHandled = false;
        mAction.reset();
    }

    float InterpreterContext::getSecondsPassed() const
    {
        return MWBase::Environment::get().getFrameDuration();
    }

    bool InterpreterContext::isDisabled (const std::string& id) const
    {
        const MWWorld::Ptr ref = getReference (id, false);
        return !ref.getRefData().isEnabled();
    }

    void InterpreterContext::enable (const std::string& id)
    {
        MWWorld::Ptr ref = getReference (id, false);
        MWBase::Environment::get().getWorld()->enable (ref);
    }

    void InterpreterContext::disable (const std::string& id)
    {
        MWWorld::Ptr ref = getReference (id, false);
        MWBase::Environment::get().getWorld()->disable (ref);
    }

    int InterpreterContext::getMemberShort (const std::string& id, const std::string& name) const
    {
        const MWWorld::Ptr ptr = getReference (id, false);

        std::string scriptId = MWWorld::Class::get (ptr).getScript (ptr);

        int index = MWBase::Environment::get().getScriptManager()->getLocalIndex (scriptId, name, 's');

        ptr.getRefData().setLocals (
            *MWBase::Environment::get().getWorld()->getStore().scripts.find (scriptId));
        return ptr.getRefData().getLocals().mShorts[index];
    }

    int InterpreterContext::getMemberLong (const std::string& id, const std::string& name) const
    {
        const MWWorld::Ptr ptr = getReference (id, false);

        std::string scriptId = MWWorld::Class::get (ptr).getScript (ptr);

        int index = MWBase::Environment::get().getScriptManager()->getLocalIndex (scriptId, name, 'l');

        ptr.getRefData().setLocals (
            *MWBase::Environment::get().getWorld()->getStore().scripts.find (scriptId));
        return ptr.getRefData().getLocals().mLongs[index];
    }

    float InterpreterContext::getMemberFloat (const std::string& id, const std::string& name) const
    {
        const MWWorld::Ptr ptr = getReference (id, false);

        std::string scriptId = MWWorld::Class::get (ptr).getScript (ptr);

        int index = MWBase::Environment::get().getScriptManager()->getLocalIndex (scriptId, name, 'f');

        ptr.getRefData().setLocals (
            *MWBase::Environment::get().getWorld()->getStore().scripts.find (scriptId));
        return ptr.getRefData().getLocals().mFloats[index];
    }

    void InterpreterContext::setMemberShort (const std::string& id, const std::string& name, int value)
    {
        const MWWorld::Ptr ptr = getReference (id, false);

        std::string scriptId = MWWorld::Class::get (ptr).getScript (ptr);

        int index = MWBase::Environment::get().getScriptManager()->getLocalIndex (scriptId, name, 's');

        ptr.getRefData().setLocals (
            *MWBase::Environment::get().getWorld()->getStore().scripts.find (scriptId));
        ptr.getRefData().getLocals().mShorts[index] = value;
    }

    void InterpreterContext::setMemberLong (const std::string& id, const std::string& name, int value)
    {
        const MWWorld::Ptr ptr = getReference (id, false);

        std::string scriptId = MWWorld::Class::get (ptr).getScript (ptr);

        int index = MWBase::Environment::get().getScriptManager()->getLocalIndex (scriptId, name, 'l');

        ptr.getRefData().setLocals (
            *MWBase::Environment::get().getWorld()->getStore().scripts.find (scriptId));
        ptr.getRefData().getLocals().mLongs[index] = value;
    }

    void InterpreterContext::setMemberFloat (const std::string& id, const std::string& name, float value)
    {
        const MWWorld::Ptr ptr = getReference (id, false);

        std::string scriptId = MWWorld::Class::get (ptr).getScript (ptr);

        int index = MWBase::Environment::get().getScriptManager()->getLocalIndex (scriptId, name, 'f');

        ptr.getRefData().setLocals (
            *MWBase::Environment::get().getWorld()->getStore().scripts.find (scriptId));
        ptr.getRefData().getLocals().mFloats[index] = value;
    }

    MWWorld::Ptr InterpreterContext::getReference()
    {
        return getReference ("", true);
    }
}
