
#include "interpretercontext.hpp"

#include <stdexcept>
#include <iostream>

#include "locals.hpp"
#include "../mwworld/world.hpp"

namespace MWScript
{
    InterpreterContext::InterpreterContext (const MWWorld::Environment& environment,
        MWScript::Locals *locals, MWWorld::Ptr reference)
    : mEnvironment (environment), mLocals (locals), mReference (reference)
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
        std::cout << "message box: " << message << std::endl;
        
        if (!buttons.empty())    
            std::cerr << "error: message box buttons not supported" << std::endl;
    }
    
    bool InterpreterContext::menuMode()
    {
        return false;
    }
    
    int InterpreterContext::getGlobalShort (const std::string& name) const
    {
        return 0;
    }

    int InterpreterContext::getGlobalLong (const std::string& name) const
    {
        return 0;
    }

    float InterpreterContext::getGlobalFloat (const std::string& name) const
    {
        return 0;
    }

    void InterpreterContext::setGlobalShort (const std::string& name, int value)
    {
    
    }

    void InterpreterContext::setGlobalLong (const std::string& name, int value)
    {
    
    }

    void InterpreterContext::setGlobalFloat (const std::string& name, float value)
    {
    
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
        return mReference;
    }
}

