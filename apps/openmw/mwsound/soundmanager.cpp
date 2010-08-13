
#include "soundmanager.hpp"

#include <openengine/sound/sndmanager.hpp>
#include <mangle/sound/clients/ogre_listener_mover.hpp>
#include <mangle/sound/clients/ogre_output_updater.hpp>

/* Set up the sound manager to use Audiere of FFMPEG for input. The
   OPENMW_USE_x macros are set in CMakeLists.txt.
 */
#ifdef OPENMW_USE_AUDIERE
#include <mangle/sound/filters/openal_audiere.hpp>
#define SOUND_FACTORY OpenAL_Audiere_Factory
#endif

#ifdef OPENMW_USE_FFMPEG
#include <mangle/sound/filters/openal_ffmpeg.hpp>
#define SOUND_FACTORY OpenAL_FFMpeg_Factory
#endif

#ifdef OPENMW_USE_MPG123
#include <mangle/sound/filters/openal_mpg123.hpp>
#define SOUND_FACTORY OpenAL_Mpg123_Factory
#endif

using namespace Mangle::Sound;
typedef OEngine::Sound::SoundManager OEManager;
typedef OEngine::Sound::SoundManagerPtr OEManagerPtr;

/* Set the position on a sound based on a Ptr. TODO: We do not support
   tracking moving objects yet, once a sound is started it stays in
   the same place until it finishes.

   This obviously has to be fixed at some point for player/npc
   footstep sounds and the like. However, updating all sounds each
   frame is expensive, so there should be a special flag for sounds
   that need to track their attached object.
 */
static void setPos(SoundPtr snd, const MWWorld::Ptr ref)
{
    // Get sound position from the reference
    const float *pos = ref.getCellRef().pos.pos;

    // Move the sound. Might need to convert coordinates, test.
    snd->setPos(pos[0], pos[1], pos[2]);
}

namespace MWSound
{
    struct SoundManager::SoundImpl
    {
        /* This is the sound manager. It loades, stores and deletes
           sounds based on the sound factory it is given.
         */
        OEManagerPtr mgr;

        /* This class calls update() on the sound manager each frame
           using and Ogre::FrameListener
         */
        Mangle::Sound::OgreOutputUpdater updater;

        /* This class tracks the movement of an Ogre::Camera and moves
           a sound listener automatically to follow it.
         */
        Mangle::Sound::OgreListenerMover cameraTracker;

        SoundImpl()
          : mgr(new OEManager(SoundFactoryPtr(new SOUND_FACTORY)))
          , updater(mgr)
          , cameraTracker(mgr)
        {}
    };

    SoundManager::SoundManager(Ogre::Root *root, Ogre::Camera *camera)
    {
        mData = new SoundImpl;

        // Attach the camera to the camera tracker
        mData->cameraTracker.followCamera(camera);
    }

    SoundManager::~SoundManager()
    {
        delete mData;
    }

    void SoundManager::say (MWWorld::Ptr reference, const std::string& filename)
    {
        // Play the sound at the correct position
        SoundPtr snd = mData->mgr->play(filename);
        setPos(snd, reference);
        // TODO: We need to attach it to the reference somehow. A weak
        // pointer is probably the best bet
    }

    bool SoundManager::sayDone (MWWorld::Ptr reference) const
    {
        return true;
        // TODO: Ask the reference to check its attached 'say' sound.
    }

    void SoundManager::streamMusic (const std::string& filename)
    {
        // Play the sound and tell it to stream, if possible.
        mData->mgr->play(filename)->setStreaming(true);
    }
        
    void SoundManager::playSound (const std::string& soundId, float volume, float pitch)
    {
    }

    void SoundManager::playSound3D (MWWorld::Ptr reference, const std::string& soundId,
        float volume, float pitch, bool loop)
    {
        // Not implemented - need both a way to find sounds by id and
        // a way to attach them to the reference
    }

    void SoundManager::stopSound3D (MWWorld::Ptr reference, const std::string& soundId)
    {
    }

    bool SoundManager::getSoundPlaying (MWWorld::Ptr reference, const std::string& soundId) const
    {
      return false;
    }
}

