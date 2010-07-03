
#include "soundmanager.hpp"

#include <iostream> // TODO remove this line, once the real code is in place.

#include <components/interpreter/context.hpp>

namespace MWSound
{
    void SoundManager::say (const std::string& handle, const std::string& filename,
        const std::string& text, Interpreter::Context& context)
    {
        std::cout << "sound effect: " << handle << " is speaking" << std::endl;
        
        context.messageBox (text);
    }

    bool SoundManager::sayDone (const std::string& handle, Interpreter::Context& context) const
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

    void SoundManager::playSound3D (const std::string& handle, const std::string& soundId,
        float volume, float pitch, Interpreter::Context& context)
    {
        std::cout
            << "sound effect: playing sound " << soundId
            << " from " << handle
            << " at volume " << volume << ", at pitch " << pitch
            << std::endl;   
            
        mSounds[handle] = soundId;
    }

    void SoundManager::stopSound3D (const std::string& handle, const std::string& soundId,
        Interpreter::Context& context)
    {
        std::cout
            << "sound effect : stop playing sound " << soundId
            << " from " << handle << std::endl;
            
        mSounds[handle] = "";
    }

    bool SoundManager::getSoundPlaying (const std::string& handle, const std::string& soundId,
        Interpreter::Context& context) const
    {
         std::map<std::string, std::string>::const_iterator iter = mSounds.find (handle);
         
         if (iter==mSounds.end())
            return false;
            
         return iter->second==soundId;
    }
}

