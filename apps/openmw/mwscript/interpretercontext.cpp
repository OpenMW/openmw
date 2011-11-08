
#include "interpretercontext.hpp"

#include <cmath>
#include <stdexcept>
#include <iostream>

#include <components/interpreter/types.hpp>

#include "../mwworld/world.hpp"

#include "../mwgui/window_manager.hpp"

#include "../mwinput/inputmanager.hpp"

#include "locals.hpp"
#include "globalscripts.hpp"

namespace MWScript
{
    MWWorld::Ptr InterpreterContext::getReference (
        const std::string& id, bool activeOnly)
    {
        if (!id.empty())
        {
            return mEnvironment.mWorld->getPtr (id, activeOnly);
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
            return mEnvironment.mWorld->getPtr (id, activeOnly);
        }
        else
        {
            if (mReference.isEmpty())
                throw std::runtime_error ("no implicit reference");

            return mReference;
        }
    }

    InterpreterContext::InterpreterContext (MWWorld::Environment& environment,
        MWScript::Locals *locals, MWWorld::Ptr reference)
    : mEnvironment (environment), mLocals (locals), mReference (reference),
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
        mEnvironment.mWindowManager->messageBox (message, buttons);
    }

    void InterpreterContext::report (const std::string& message)
    {
        messageBox (message);
    }

    bool InterpreterContext::menuMode()
    {
        return mEnvironment.mWindowManager->isGuiMode();
    }

    int InterpreterContext::getGlobalShort (const std::string& name) const
    {
        return mEnvironment.mWorld->getGlobalVariable (name).mShort;
    }

    int InterpreterContext::getGlobalLong (const std::string& name) const
    {
        // a global long is internally a float.
        return mEnvironment.mWorld->getGlobalVariable (name).mLong;
    }

    float InterpreterContext::getGlobalFloat (const std::string& name) const
    {
        return mEnvironment.mWorld->getGlobalVariable (name).mFloat;
    }

    void InterpreterContext::setGlobalShort (const std::string& name, int value)
    {
        if (name=="gamehour")
            mEnvironment.mWorld->setHour (value);
        else if (name=="day")
            mEnvironment.mWorld->setDay (value);
        else if (name=="month")
            mEnvironment.mWorld->setMonth (value);
        else
            mEnvironment.mWorld->getGlobalVariable (name).mShort = value;
    }

    void InterpreterContext::setGlobalLong (const std::string& name, int value)
    {
        if (name=="gamehour")
            mEnvironment.mWorld->setHour (value);
        else if (name=="day")
            mEnvironment.mWorld->setDay (value);
        else if (name=="month")
            mEnvironment.mWorld->setMonth (value);
        else
            mEnvironment.mWorld->getGlobalVariable (name).mLong = value;
    }

    void InterpreterContext::setGlobalFloat (const std::string& name, float value)
    {
        if (name=="gamehour")
            mEnvironment.mWorld->setHour (value);
        else if (name=="day")
            mEnvironment.mWorld->setDay (value);
        else if (name=="month")
            mEnvironment.mWorld->setMonth (value);
        else
            mEnvironment.mWorld->getGlobalVariable (name).mFloat = value;
    }

    bool InterpreterContext::isScriptRunning (const std::string& name) const
    {
        return mEnvironment.mGlobalScripts->isRunning (name);
    }

    void InterpreterContext::startScript (const std::string& name)
    {
        mEnvironment.mGlobalScripts->addScript (name);
    }

    void InterpreterContext::stopScript (const std::string& name)
    {
        mEnvironment.mGlobalScripts->removeScript (name);
    }

    float InterpreterContext::getDistance (const std::string& name, const std::string& id) const
    {
        // TODO handle exterior cells (when ref and ref2 are located in different cells)
        const MWWorld::Ptr ref2 = getReference (id, false);

        const MWWorld::Ptr ref = mEnvironment.mWorld->getPtr (name, true);

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

        mAction->execute (mEnvironment);
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
        return mEnvironment.mFrameDuration;
    }

    bool InterpreterContext::isDisabled (const std::string& id) const
    {
        const MWWorld::Ptr ref = getReference (id, false);
        return !ref.getRefData().isEnabled();
    }

    void InterpreterContext::enable (const std::string& id)
    {
        MWWorld::Ptr ref = getReference (id, false);
        mEnvironment.mWorld->enable (ref);
    }

    void InterpreterContext::disable (const std::string& id)
    {
        MWWorld::Ptr ref = getReference (id, false);
        mEnvironment.mWorld->disable (ref);
    }

    MWWorld::Environment& InterpreterContext::getEnvironment()
    {
        return mEnvironment;
    }

    MWGui::WindowManager& InterpreterContext::getWindowManager()
    {
        return *mEnvironment.mWindowManager;
    }

    MWInput::MWInputManager& InterpreterContext::getInputManager()
    {
        return *mEnvironment.mInputManager;
    }

    MWWorld::World& InterpreterContext::getWorld()
    {
        return *mEnvironment.mWorld;
    }

    MWSound::SoundManager& InterpreterContext::getSoundManager()
    {
        return *mEnvironment.mSoundManager;
    }

    MWWorld::Ptr InterpreterContext::getReference()
    {
        return getReference ("", true);
    }
}
