
#include "soundmanager.hpp"

#include <openengine/sound/sndmanager.hpp>

/* Set up the sound manager to use Audiere for input (reading sound
   files) and OpenAL for output.
 */
#include <mangle/sound/filters/openal_audiere.hpp>
#define SOUND_FACTORY OpenAL_Audiere_Factory
/*
   We could allow several options for libraries via external #defines
   if we like, controlled through CMake. The only things that need to
   change is the include and #define above, and of course the linker
   parameters.
 */

using namespace Mangle::Sound;

/* Set the position on a sound based on a Ptr. TODO: We do not support
   tracking moving objects yet, once a sound is started it stays in
   the same place until it finishes.

   This obviously has to be fixed at some point for player/npc
   footstep sounds and the like. However, updating all sounds each
   frame is expensive, so there should be a special flag for sounds
   that need to track their attached object.
 */
static void setPos(SoundPtr snd, MWWorld::Ptr ref)
{
    // Get sound position from the reference
    float *pos = ref.getCellRef().pos.pos;

    // Move the sound. Might need to convert coordinates, test.
    snd->setPos(pos[0], pos[1], pos[2]);
}

namespace MWSound
{
    struct SoundManager::SoundImpl
    {
        OEngine::Sound::SoundManager mgr;

        SoundImpl()
            : mgr(SoundFactoryPtr(new SOUND_FACTORY))
        {}

        std::map<std::string, std::string> mSounds; // object, sound
    };

    SoundManager::SoundManager(Ogre::Root *root, Ogre::Camera *camera)
    {
        mData = new SoundImpl;

        // TODO: Set up updater and camera listener.
    }

    SoundManager::~SoundManager()
    {
        delete mData;
    }

    void SoundManager::say (MWWorld::Ptr reference, const std::string& filename,
        const std::string& text)
    {
        std::cout << "sound effect: " << reference.getRefData().getHandle() << " is speaking" << std::endl;
        
    }

    bool SoundManager::sayDone (MWWorld::Ptr reference) const
    {
        return false;
    }

    void SoundManager::streamMusic (const std::string& filename)
    {
        std::cout << "sound effect: playing music" << filename << std::endl;
    }
        
    void SoundManager::playSound (const std::string& soundId, float volume, float pitch)
    {
        std::cout
            << "sound effect: playing sound " << soundId
            << " at volume " << volume << ", at pitch " << pitch
            << std::endl;
    }

    void SoundManager::playSound3D (MWWorld::Ptr reference, const std::string& soundId,
        float volume, float pitch, bool loop)
    {
        std::cout
            << "sound effect: playing sound " << soundId
            << " from " << reference.getRefData().getHandle()
            << " at volume " << volume << ", at pitch " << pitch
            << std::endl;   
            
        mData->mSounds[reference.getRefData().getHandle()] = soundId;
    }

    void SoundManager::stopSound3D (MWWorld::Ptr reference, const std::string& soundId)
    {
        std::cout
            << "sound effect : stop playing sound " << soundId
            << " from " << reference.getRefData().getHandle() << std::endl;
            
        mData->mSounds[reference.getRefData().getHandle()] = "";
    }

    bool SoundManager::getSoundPlaying (MWWorld::Ptr reference, const std::string& soundId) const
    {
         std::map<std::string, std::string>::const_iterator iter =
            mData->mSounds.find (reference.getRefData().getHandle());
         
         if (iter==mData->mSounds.end())
            return false;
            
         return iter->second==soundId;
    }
}

