
#include "soundmanager.hpp"

#include <iostream>
using namespace std;

#include <openengine/sound/sndmanager.hpp>
#include <mangle/sound/clients/ogre_listener_mover.hpp>
#include <mangle/sound/clients/ogre_output_updater.hpp>

#include <components/esm_store/store.hpp>
#include <algorithm>

#include <OgreRoot.h>

/* Set up the sound manager to use Audiere, FFMPEG or MPG123 for
   input. The OPENMW_USE_x macros are set in CMakeLists.txt.
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
static void setPos(SoundPtr &snd, const MWWorld::Ptr ref)
{
    // Get sound position from the reference
    const float *pos = ref.getCellRef().pos.pos;

    // Move the sound, converting from MW coordinates to Ogre
    // coordinates.
    snd->setPos(pos[0], pos[2], -pos[1]);
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

        const ESMS::ESMStore &store;
        std::string dir;

        SoundImpl(Ogre::Root *root, Ogre::Camera *camera,
                  const ESMS::ESMStore &str,
                  const std::string &soundDir)
          : mgr(new OEManager(SoundFactoryPtr(new SOUND_FACTORY)))
          , updater(mgr)
          , cameraTracker(mgr)
          , store(str)
        {
          // Attach the camera to the camera tracker
          cameraTracker.followCamera(camera);

          // Tell Ogre to update the sound system each frame
          root->addFrameListener(&updater);

          dir = soundDir + "/";
        }

      // Convert a soundId to file name, and modify the volume
      // according to the sounds local volume setting, minRange and
      // maxRange.
      std::string lookup(const std::string &soundId,
                         float &volume, float &min, float &max)
      {
        const ESM::Sound *snd = store.sounds.search(soundId);
        if(snd == NULL) return "";

        volume *= snd->data.volume / 255.0;
        // These factors are not very fine tuned.
        min = snd->data.minRange * 4;
        max = snd->data.maxRange * 1000;
        std::string file = dir + snd->sound;
        std::replace(file.begin(), file.end(), '\\', '/');
        return file;
      }

      // Add a sound to the list and play it
      void add(const std::string &file,
               MWWorld::Ptr reference,
               const std::string &id,
               float volume, float pitch,
               float min, float max,
               bool loop)
      {
        SoundPtr snd = mgr->load(file);
        snd->setRepeat(loop);
        snd->setVolume(volume);
        snd->setPitch(pitch);
        snd->setRange(min,max);
        setPos(snd, reference);
        snd->play();
      }

      // Stop a sound and remove it from the list. If id="" then
      // remove the entire object and stop all its sounds.
      void remove(MWWorld::Ptr reference, const std::string &id = "")
      {
      }

      bool isPlaying(MWWorld::Ptr reference, const std::string &id) const
      {
        return true;
      }

      void removeCell(const MWWorld::Ptr::CellStore *cell)
      {
        // Note to Nico: You can get the cell of a Ptr via the getCell
        // function. Just iterate over all sounds and remove those
        // with matching cell.
      }

      void updatePositions(MWWorld::Ptr reference)
      {
      }
    };

    SoundManager::SoundManager(Ogre::Root *root, Ogre::Camera *camera,
                               const ESMS::ESMStore &store,
                               const std::string &soundDir)
    {
      mData = new SoundImpl(root, camera, store, soundDir);
    }

    SoundManager::~SoundManager()
    {
        delete mData;
    }

    void SoundManager::say (MWWorld::Ptr reference, const std::string& filename)
    {
      // The range values are not tested
      mData->add(filename, reference, "_say_sound", 1, 1, 100, 10000, false);
    }

    bool SoundManager::sayDone (MWWorld::Ptr reference) const
    {
      return !mData->isPlaying(reference, "_say_sound");
    }

    void SoundManager::streamMusic (const std::string& filename)
    {
      // Play the sound and tell it to stream, if possible. TODO:
      // Store the reference, the jukebox will need to check status,
      // control volume etc.
      SoundPtr music = mData->mgr->play(filename);
      music->setStreaming(true);
      music->setVolume(0.6);
    }

    void SoundManager::playSound (const std::string& soundId, float volume, float pitch)
    {
      // Play and forget
      float min, max;
      const std::string &file = mData->lookup(soundId, volume, min, max);
      if(file != "")
        {
          SoundPtr snd = mData->mgr->play(file);
          snd->setVolume(volume);
          snd->setRange(min,max);
          snd->setPitch(pitch);
        }
    }

    void SoundManager::playSound3D (MWWorld::Ptr reference, const std::string& soundId,
        float volume, float pitch, bool loop)
    {
      // Look up the sound in the ESM data
      float min, max;
      const std::string &file = mData->lookup(soundId, volume, min, max);
      if(file != "")
        mData->add(file, reference, soundId, volume, pitch, min, max, loop);
    }

    void SoundManager::stopSound3D (MWWorld::Ptr reference, const std::string& soundId)
    {
      mData->remove(reference, soundId);
    }

    void SoundManager::stopSound (MWWorld::Ptr::CellStore *cell)
    {
      mData->removeCell(cell);
    }

    bool SoundManager::getSoundPlaying (MWWorld::Ptr reference, const std::string& soundId) const
    {
      return mData->isPlaying(reference, soundId);
    }

    void SoundManager::updateObject(MWWorld::Ptr reference)
    {
      mData->updatePositions(reference);
    }
}
