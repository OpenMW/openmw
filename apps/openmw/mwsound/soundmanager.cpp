#include "soundmanager.hpp"

#include <iostream>
using namespace std;

#include <openengine/sound/sndmanager.hpp>
#include <mangle/sound/clients/ogre_listener_mover.hpp>
#include <mangle/sound/clients/ogre_output_updater.hpp>

#include <components/file_finder/file_finder.hpp>
#include <components/esm_store/store.hpp>
#include <algorithm>
#include <map>

#include <OgreRoot.h>

/* Set up the sound manager to use Audiere, FFMPEG or
   MPG123/libsndfile for input. The OPENMW_USE_x macros are set in
   CMakeLists.txt.
*/
#ifdef OPENMW_USE_AUDIERE
#include <mangle/sound/filters/openal_audiere.hpp>
#define SOUND_FACTORY OpenAL_Audiere_Factory
#define SOUND_OUT "OpenAL"
#define SOUND_IN "Audiere"
#endif

#ifdef OPENMW_USE_FFMPEG
#include <mangle/sound/filters/openal_ffmpeg.hpp>
#define SOUND_FACTORY OpenAL_FFMpeg_Factory
#define SOUND_OUT "OpenAL"
#define SOUND_IN "FFmpeg"
#endif

#ifdef OPENMW_USE_MPG123
#include <mangle/sound/filters/openal_sndfile_mpg123.hpp>
#define SOUND_FACTORY OpenAL_SndFile_Mpg123_Factory
#define SOUND_OUT "OpenAL"
#define SOUND_IN "mpg123,sndfile"
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

    typedef std::map<std::string,Mangle::Sound::WSoundPtr> IDMap;
    typedef std::map<MWWorld::Ptr,IDMap> PtrMap;
    PtrMap sounds;

    // This is used for case insensitive and slash-type agnostic file
    // finding. It takes DOS paths (any case, \\ slashes or / slashes)
    // relative to the sound dir, and translates them into full paths
    // of existing files in the filesystem, if they exist.
    FileFinder::FileFinder files;

    SoundImpl(Ogre::Root *root, Ogre::Camera *camera,
              const ESMS::ESMStore &str,
              const std::string &soundDir)
      : mgr(new OEManager(SoundFactoryPtr(new SOUND_FACTORY)))
      , updater(mgr)
      , cameraTracker(mgr)
      , store(str)
      , files(soundDir)
    {
      cout << "Sound output:  " << SOUND_OUT << endl;
      cout << "Sound decoder: " << SOUND_IN << endl;

      // Attach the camera to the camera tracker
      cameraTracker.followCamera(camera);

      // Tell Ogre to update the sound system each frame
      root->addFrameListener(&updater);
    }

    ~SoundImpl()
    {
        Ogre::Root::getSingleton().removeFrameListener(&updater);
        cameraTracker.unfollowCamera();
    }

    std::string toMp3(const std::string &str)
    {
      std::string wav = str;
      int i = str.size()-3;
      wav[i++] = 'm';
      wav[i++] = 'p';
      wav[i++] = '3';
      return wav;
    }

    bool hasFile(const std::string &str)
    {
      if(files.has(str)) return true;
      // Not found? Try exchanging .wav with .mp3
      return files.has(toMp3(str));
    }

    // Convert a Morrowind sound path (eg. Fx\funny.wav) to full path
    // with proper slash conversion (eg. datadir/Sound/Fx/funny.wav)
    std::string convertPath(const std::string &str)
    {
      // Search and return
      if(files.has(str))
        return files.lookup(str);

      // Try mp3 if the wav wasn't found
      std::string mp3 = toMp3(str);
      if(files.has(mp3))
        return files.lookup(mp3);

      // Give up
      return "";
    }

    // Convert a soundId to file name, and modify the volume
    // according to the sounds local volume setting, minRange and
    // maxRange.
    std::string lookup(const std::string &soundId,
                       float &volume, float &min, float &max)
    {
      const ESM::Sound *snd = store.sounds.search(soundId);
      if(snd == NULL) return "";

      volume *= snd->data.volume / 255.0f;
      // These factors are not very fine tuned.
      min = snd->data.minRange * 7.0f;
      max = snd->data.maxRange * 2000.0f;
      return convertPath(snd->sound);
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

          sounds[ptr][id] = WSoundPtr(snd);
        }
      catch(...)
        {
          cout << "Error loading " << file << ", skipping.\n";
        }
    }

    // Clears all the sub-elements of a given iterator, and then
    // removes it from 'sounds'.
    void clearAll(PtrMap::iterator it)
    {
      IDMap::iterator sit = it->second.begin();

      while(sit != it->second.end())
        {
          // Get sound pointer, if any
          SoundPtr snd = sit->second.lock();

          // Stop the sound
          if(snd) snd->stop();

          sit++;
        }

      // Remove the ptr reference
      sounds.erase(it);
    }

    // Stop a sound and remove it from the list. If id="" then
    // remove the entire object and stop all its sounds.
    void remove(MWWorld::Ptr ptr, const std::string &id = "")
    {
      PtrMap::iterator it = sounds.find(ptr);
      if(it != sounds.end())
        {
          if(id == "")
            // Kill all references to 'ptr'
            clearAll(it);
          else
            {
              // Only find the id we're looking for
              IDMap::iterator it2 = it->second.find(id);
              if(it2 != it->second.end())
                {
                  // Stop the sound and remove it from the list
                  SoundPtr snd = it2->second.lock();
                  if(snd) snd->stop();
                  it->second.erase(it2);
                }
            }
        }
    }

    bool isPlaying(MWWorld::Ptr ptr, const std::string &id) const
    {
      PtrMap::const_iterator it = sounds.find(ptr);
      if(it != sounds.end())
        {
          IDMap::const_iterator it2 = it->second.find(id);
          if(it2 != it->second.end())
            {
              // Get a shared_ptr from the weak_ptr
              SoundPtr snd = it2->second.lock();;

              // Is it still alive?
              if(snd)
                {
                  // Then return its status!
                  return snd->isPlaying();
                }
            }
        }
      // Nothing found, sound is not playing
      return false;
    }

    // Remove all references to objects belonging to a given cell
    void removeCell(const MWWorld::Ptr::CellStore *cell)
    {
      PtrMap::iterator it2, it = sounds.begin();
      while(it != sounds.end())
        {
          // Make sure to increase the iterator before we erase it.
          it2 = it++;
          if(it2->first.getCell() == cell)
            clearAll(it2);
        }
    }

    void updatePositions(MWWorld::Ptr ptr)
    {
      // Find the reference (if any)
      PtrMap::iterator it = sounds.find(ptr);
      if(it != sounds.end())
        {
          // Then find all sounds in it (if any)
          IDMap::iterator it2 = it->second.begin();
          for(;it2 != it->second.end(); it2++)
            {
              // Get the sound (if it still exists)
              SoundPtr snd = it2->second.lock();
              if(snd)
                // Update position
                setPos(snd, ptr);
            }
        }
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
    if(mData->hasFile(filename))
      mData->add(mData->convertPath(filename), ptr, "_say_sound", 1, 1, 100, 10000, false);
    else
      cout << "Sound file " << filename << " not found, skipping.\n";
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
