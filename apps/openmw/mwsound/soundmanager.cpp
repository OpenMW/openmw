
#include "soundmanager.hpp"

#include <iostream> // TODO remove this line, once the real code is in place.

#include <components/interpreter/context.hpp>

namespace MWSound
{
    void SoundManager::say (MWWorld::Ptr reference, const std::string& filename,
        const std::string& text, Interpreter::Context& context)
    {
        std::cout << "sound effect: " << reference.getRefData().getHandle() << " is speaking" << std::endl;
        
        context.messageBox (text);
    }

    bool SoundManager::sayDone (MWWorld::Ptr reference, Interpreter::Context& context) const
    {
        return false;
    }

    void SoundManager::streamMusic (const std::string& filename, Interpreter::Context& context)
    {
        std::cout << "sound effect: playing music" << filename << std::endl;
    }
        
    void SoundManager::playSound (const std::string& soundId, float volume, float pitch,
        Interpreter::Context& context)
    {
        std::cout
            << "sound effect: playing sound " << soundId
            << " at volume " << volume << ", at pitch " << pitch
            << std::endl;
    }

    void SoundManager::playSound3D (MWWorld::Ptr reference, const std::string& soundId,
        float volume, float pitch, bool loop, Interpreter::Context& context)
    {
        std::cout
            << "sound effect: playing sound " << soundId
            << " from " << reference.getRefData().getHandle()
            << " at volume " << volume << ", at pitch " << pitch
            << std::endl;   
            
        mSounds[reference.getRefData().getHandle()] = soundId;
    }

    void SoundManager::stopSound3D (MWWorld::Ptr reference, const std::string& soundId,
        Interpreter::Context& context)
    {
        std::cout
            << "sound effect : stop playing sound " << soundId
            << " from " << reference.getRefData().getHandle() << std::endl;
            
        mSounds[reference.getRefData().getHandle()] = "";
    }

    bool SoundManager::getSoundPlaying (MWWorld::Ptr reference, const std::string& soundId,
        Interpreter::Context& context) const
    {
         std::map<std::string, std::string>::const_iterator iter =
            mSounds.find (reference.getRefData().getHandle());
         
         if (iter==mSounds.end())
            return false;
            
         return iter->second==soundId;
    }
}

