#ifndef GAME_SOUND_SOUNDMANAGER_H
#define GAME_SOUND_SOUNDMANAGER_H

#include <string>
#include <utility>
#include <map>

#include <boost/shared_ptr.hpp>

#include <OgreResourceGroupManager.h>

#include <components/settings/settings.hpp>

#include "../mwworld/ptr.hpp"


namespace Ogre
{
    class Root;
    class Camera;
}

namespace MWSound
{
    class Sound_Output;
    struct Sound_Decoder;
    class Sound;

    typedef boost::shared_ptr<Sound_Decoder> DecoderPtr;
    typedef boost::shared_ptr<Sound> SoundPtr;

    enum PlayMode {
        Play_Normal  = 0, /* tracked, non-looping, multi-instance, environment */
        Play_Loop    = 1<<0, /* Sound will continually loop until explicitly stopped */
        Play_NoEnv   = 1<<1, /* Do not apply environment effects (eg, underwater filters) */
        Play_NoTrack = 1<<2, /* (3D only) Play the sound at the given object's position
                              * but do not keep it updated (the sound will not move with
                              * the object and will not stop when the object is deleted. */
    };
    static inline int operator|(const PlayMode &a, const PlayMode &b)
    { return (int)a | (int)b; }
    static inline int operator&(const PlayMode &a, const PlayMode &b)
    { return (int)a & (int)b; }

    enum Environment {
        Env_Normal,
        Env_Underwater,
    };

    class SoundManager
    {
        Ogre::ResourceGroupManager& mResourceMgr;

        std::auto_ptr<Sound_Output> mOutput;

        float mMasterVolume;
        float mSFXVolume;
        float mMusicVolume;
        float mVoiceVolume;

        // not implemented
        float mFootstepsVolume;

        boost::shared_ptr<Sound> mMusic;
        std::string mCurrentPlaylist;

        typedef std::pair<MWWorld::Ptr,std::string> PtrIDPair;
        typedef std::map<SoundPtr,PtrIDPair> SoundMap;
        SoundMap mActiveSounds;

        std::string lookup(const std::string &soundId,
                  float &volume, float &min, float &max);
        void streamMusicFull(const std::string& filename);
        bool isPlaying(MWWorld::Ptr ptr, const std::string &id) const;
        void updateSounds(float duration);
        void updateRegionSound(float duration);

        SoundManager(const SoundManager &rhs);
        SoundManager& operator=(const SoundManager &rhs);

    protected:
        DecoderPtr getDecoder();
        friend class OpenAL_Output;

    public:
        SoundManager(bool useSound);
        ~SoundManager();

        void processChangedSettings(const Settings::CategorySettingVector& settings);

        void stopMusic();
        ///< Stops music if it's playing

        void streamMusic(const std::string& filename);
        ///< Play a soundifle
        /// \param filename name of a sound file in "Music/" in the data directory.

        void startRandomTitle();
        ///< Starts a random track from the current playlist

        bool isMusicPlaying();
        ///< Returns true if music is playing

        void playPlaylist(const std::string &playlist);
        ///< Start playing music from the selected folder
        /// \param name of the folder that contains the playlist

        void say(MWWorld::Ptr reference, const std::string& filename);
        ///< Make an actor say some text.
        /// \param filename name of a sound file in "Sound/" in the data directory.

        void say(const std::string& filename);
        ///< Say some text, without an actor ref
        /// \param filename name of a sound file in "Sound/" in the data directory.

        bool sayDone(MWWorld::Ptr reference=MWWorld::Ptr()) const;
        ///< Is actor not speaking?

        void stopSay(MWWorld::Ptr reference=MWWorld::Ptr());
        ///< Stop an actor speaking

        SoundPtr playSound(const std::string& soundId, float volume, float pitch, int mode=Play_Normal);
        ///< Play a sound, independently of 3D-position

        SoundPtr playSound3D(MWWorld::Ptr reference, const std::string& soundId,
                             float volume, float pitch, int mode=Play_Normal);
        ///< Play a sound from an object

        void stopSound3D(MWWorld::Ptr reference, const std::string& soundId);
        ///< Stop the given object from playing the given sound,

        void stopSound3D(MWWorld::Ptr reference);
        ///< Stop the given object from playing all sounds.

        void stopSound(const MWWorld::CellStore *cell);
        ///< Stop all sounds for the given cell.

        void stopSound(const std::string& soundId);
        ///< Stop a non-3d looping sound

        bool getSoundPlaying(MWWorld::Ptr reference, const std::string& soundId) const;
        ///< Is the given sound currently playing on the given object?

        void updateObject(MWWorld::Ptr reference);
        ///< Update the position of all sounds connected to the given object.

        void update(float duration);
    };
}

#endif
