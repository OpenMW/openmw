
#include "interpretercontext.hpp"

#include <stdexcept>
#include <iostream>

#include <components/interpreter/types.hpp>

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
        Interpreter::Type_Data value = mEnvironment.mWorld->getGlobalVariable (name);       
        return static_cast<Interpreter::Type_Short> (
            *reinterpret_cast<Interpreter::Type_Integer *> (&value));
    }

    int InterpreterContext::getGlobalLong (const std::string& name) const
    {
        // a global long is internally a float.
        Interpreter::Type_Data value = mEnvironment.mWorld->getGlobalVariable (name);       
        return static_cast<Interpreter::Type_Integer> (
            *reinterpret_cast<Interpreter::Type_Float *> (&value));
    }

    float InterpreterContext::getGlobalFloat (const std::string& name) const
    {
        Interpreter::Type_Data value = mEnvironment.mWorld->getGlobalVariable (name);       
        return *reinterpret_cast<Interpreter::Type_Float *> (&value);
    }

    void InterpreterContext::setGlobalShort (const std::string& name, int value)
    {
         mEnvironment.mWorld->getGlobalVariable (name) =
            *reinterpret_cast<Interpreter::Type_Data *> (&value);
    }

    void InterpreterContext::setGlobalLong (const std::string& name, int value)
    {
        // a global long is internally a float.
        float value2 = value;

         mEnvironment.mWorld->getGlobalVariable (name) =
            *reinterpret_cast<Interpreter::Type_Data *> (&value2);    
    }

    void InterpreterContext::setGlobalFloat (const std::string& name, float value)
    {
         mEnvironment.mWorld->getGlobalVariable (name) =
            *reinterpret_cast<Interpreter::Type_Data *> (&value);    
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

