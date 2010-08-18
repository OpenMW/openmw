
#include "soundmanager.hpp"

#include <iostream>
using namespace std;

#include <openengine/sound/sndmanager.hpp>
#include <mangle/sound/clients/ogre_listener_mover.hpp>
#include <mangle/sound/clients/ogre_output_updater.hpp>

#include <components/esm_store/store.hpp>
#include <algorithm>

#include <OgreRoot.h>

/* Set up the sound manager to use Audiere, FFMPEG or
   MPG123/libsndfile for input. The OPENMW_USE_x macros are set in
   CMakeLists.txt.
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
#include <mangle/sound/filters/openal_sndfile_mpg123.hpp>
#define SOUND_FACTORY OpenAL_SndFile_Mpg123_Factory
#endif

using namespace Mangle::Sound;
typedef OEngine::Sound::SoundManager OEManager;
typedef OEngine::Sound::SoundManagerPtr OEManagerPtr;

// Set the position on a sound based on a Ptr.
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
      min = snd->data.minRange * 7;
      max = snd->data.maxRange * 2000;
      std::string file = dir + snd->sound;
      std::replace(file.begin(), file.end(), '\\', '/');
      return file;
    }

    // Add a sound to the list and play it
    void add(const std::string &file,
             MWWorld::Ptr ptr,
             const std::string &id,
             float volume, float pitch,
             float min, float max,
             bool loop)
    {
      try
        {
          SoundPtr snd = mgr->load(file);
          snd->setRepeat(loop);
          snd->setVolume(volume);
          snd->setPitch(pitch);
          snd->setRange(min,max);
          setPos(snd, ptr);
          snd->play();
        }
      catch(...)
        {
          cout << "Error loading " << file << ", skipping.\n";
        }
    }

    // Stop a sound and remove it from the list. If id="" then
    // remove the entire object and stop all its sounds.
    void remove(MWWorld::Ptr ptr, const std::string &id = "")
    {
    }

    bool isPlaying(MWWorld::Ptr ptr, const std::string &id) const
    {
      return true;
    }

    void removeCell(const MWWorld::Ptr::CellStore *cell)
    {
    }

    void updatePositions(MWWorld::Ptr ptr)
    {
    }
  };

  SoundManager::SoundManager(Ogre::Root *root, Ogre::Camera *camera,
                             const ESMS::ESMStore &store,
                             const std::string &soundDir,
                             bool useSound)
    : mData(NULL)
  {
    if(useSound)
      mData = new SoundImpl(root, camera, store, soundDir);
  }

  SoundManager::~SoundManager()
  {
    if(mData)
      delete mData;
  }

  void SoundManager::say (MWWorld::Ptr ptr, const std::string& filename)
  {
    // The range values are not tested
    if(!mData) return;
    mData->add(filename, ptr, "_say_sound", 1, 1, 100, 10000, false);
  }

  bool SoundManager::sayDone (MWWorld::Ptr ptr) const
  {
    if(!mData) return false;
    return !mData->isPlaying(ptr, "_say_sound");
  }

  void SoundManager::streamMusic (const std::string& filename)
  {
    if(!mData) return;

    // Play the sound and tell it to stream, if possible. TODO:
    // Store the reference, the jukebox will need to check status,
    // control volume etc.
    SoundPtr music = mData->mgr->load(filename);
    music->setStreaming(true);
    music->setVolume(0.4);
    music->play();
  }

  void SoundManager::playSound (const std::string& soundId, float volume, float pitch)
  {
    if(!mData) return;

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

  void SoundManager::playSound3D (MWWorld::Ptr ptr, const std::string& soundId,
                                  float volume, float pitch, bool loop)
  {
    if(!mData) return;

    // Look up the sound in the ESM data
    float min, max;
    const std::string &file = mData->lookup(soundId, volume, min, max);
    if(file != "")
      mData->add(file, ptr, soundId, volume, pitch, min, max, loop);
  }

  void SoundManager::stopSound3D (MWWorld::Ptr ptr, const std::string& soundId)
  {
    if(!mData) return;
    mData->remove(ptr, soundId);
  }

  void SoundManager::stopSound (MWWorld::Ptr::CellStore *cell)
  {
    if(!mData) return;
    mData->removeCell(cell);
  }

  bool SoundManager::getSoundPlaying (MWWorld::Ptr ptr, const std::string& soundId) const
  {
    // Mark all sounds as playing, otherwise the scripts will just
    // keep trying to play them every frame.
    if(!mData) return true;

    return mData->isPlaying(ptr, soundId);
  }

  void SoundManager::updateObject(MWWorld::Ptr ptr)
  {
    if(!mData) return;
    mData->updatePositions(ptr);
  }
}
