
#include "interpretercontext.hpp"

#include <stdexcept>
#include <iostream>

#include "locals.hpp"
#include "../mwworld/world.hpp"

namespace MWScript
{
    InterpreterContext::InterpreterContext (MWWorld::World& world,
        MWSound::SoundManager& soundManager, MWScript::Locals *locals)
    : mWorld (world), mSoundManager (soundManager), mLocals (locals)
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
    
    MWWorld::World& InterpreterContext::getWorld()
    {
        return mWorld;
    }
    
    MWSound::SoundManager& InterpreterContext::getSoundManager()
    {
        return mSoundManager;
    }
}

