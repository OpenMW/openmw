#ifndef OENGINE_SOUND_MANAGER_H
#define OENGINE_SOUND_MANAGER_H

#include <mangle/sound/filters/pure_filter.hpp>

namespace OEngine
{
  namespace Sound
  {
    using namespace Mangle::Sound;

    class ManagedSound;

    /** A manager of Mangle::Sounds.

        The sound manager is a wrapper around the more low-level
        SoundFactory - although it is also itself an implementation of
        SoundFactory. It will:
        - keep a list of all created sounds
        - let you iterate the list
        - keep references to playing sounds so you don't have to
        - auto-release references to sounds that are finished playing
          (ie. deleting them if you're not referencing them)
     */
    class SoundManager : public FactoryFilter
    {
      // Shove the implementation details into the cpp file.
      struct SoundManagerList;
      SoundManagerList *list;

      // Create a new sound wrapper based on the given source sound.
      SoundPtr wrap(SoundPtr snd);

      /** Internal function. Will completely disconnect the given
          sound from this manager. Called from ManagedSound.
       */
      friend class ManagedSound;
      void detach(ManagedSound *sound);
    public:
      SoundManager(SoundFactoryPtr fact);
      ~SoundManager();
      void update();

      /// Get number of sounds currently managed by this manager.
      int numSounds();

      SoundPtr loadRaw(SampleSourcePtr input)
      { return wrap(client->loadRaw(input)); }

      SoundPtr load(Mangle::Stream::StreamPtr input)
      { return wrap(client->load(input)); }

      SoundPtr load(const std::string &file)
      { return wrap(client->load(file)); }

      // Play a sound immediately, and release when done unless you
      // keep the returned SoundPtr.
      SoundPtr play(Mangle::Stream::StreamPtr sound)
      {
        SoundPtr snd = load(sound);
        snd->play();
        return snd;
      }

      SoundPtr play(const std::string &sound)
      {
        SoundPtr snd = load(sound);
        snd->play();
        return snd;
      }

      // Ditto for 3D sounds
      SoundPtr play3D(Mangle::Stream::StreamPtr sound, float x, float y, float z)
      {
        SoundPtr snd = load(sound);
        snd->setPos(x,y,z);
        snd->play();
        return snd;
      }

      SoundPtr play3D(const std::string &sound, float x, float y, float z)
      {
        SoundPtr snd = load(sound);
        snd->setPos(x,y,z);
        snd->play();
        return snd;
      }
    };

    typedef boost::shared_ptr<SoundManager> SoundManagerPtr;
  }
}
#endif
