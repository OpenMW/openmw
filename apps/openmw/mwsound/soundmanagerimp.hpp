#ifndef GAME_SOUND_SOUNDMANAGER_H
#define GAME_SOUND_SOUNDMANAGER_H

#include <string>
#include <utility>
#include <map>

#include <boost/shared_ptr.hpp>

#include <OgreVector3.h>
#include <OgreResourceGroupManager.h>

#include <components/settings/settings.hpp>

#include "../mwbase/soundmanager.hpp"

namespace MWSound
{
    class Sound_Output;
    struct Sound_Decoder;
    class Sound;

    enum Environment {
        Env_Normal,
        Env_Underwater
    };

    class SoundManager : public MWBase::SoundManager
    {
        Ogre::ResourceGroupManager& mResourceMgr;

        std::auto_ptr<Sound_Output> mOutput;

        float mMasterVolume;
        float mSFXVolume;
        float mMusicVolume;
        float mVoiceVolume;
        float mFootstepsVolume;

        boost::shared_ptr<Sound> mMusic;
        std::string mCurrentPlaylist;

        typedef std::pair<MWWorld::Ptr,std::string> PtrIDPair;
        typedef std::map<MWBase::SoundPtr,PtrIDPair> SoundMap;
        SoundMap mActiveSounds;

        MWBase::SoundPtr mUnderwaterSound;

        Ogre::Vector3 mListenerPos;
        Ogre::Vector3 mListenerDir;
        Ogre::Vector3 mListenerUp;

        int mPausedSoundTypes;

        std::string lookup(const std::string &soundId,
                  float &volume, float &min, float &max);
        void streamMusicFull(const std::string& filename);
        bool isPlaying(const MWWorld::Ptr &ptr, const std::string &id) const;
        void updateSounds(float duration);
        void updateRegionSound(float duration);

        float volumeFromType(PlayType type) const;

        SoundManager(const SoundManager &rhs);
        SoundManager& operator=(const SoundManager &rhs);

    protected:
        DecoderPtr getDecoder();
        friend class OpenAL_Output;

    public:
        SoundManager(bool useSound);
        virtual ~SoundManager();

        virtual void processChangedSettings(const Settings::CategorySettingVector& settings);

        virtual void stopMusic();
        ///< Stops music if it's playing

        virtual void streamMusic(const std::string& filename);
        ///< Play a soundifle
        /// \param filename name of a sound file in "Music/" in the data directory.

        virtual void startRandomTitle();
        ///< Starts a random track from the current playlist

        virtual bool isMusicPlaying();
        ///< Returns true if music is playing

        virtual void playPlaylist(const std::string &playlist);
        ///< Start playing music from the selected folder
        /// \param name of the folder that contains the playlist

        virtual void say(const MWWorld::Ptr &reference, const std::string& filename);
        ///< Make an actor say some text.
        /// \param filename name of a sound file in "Sound/" in the data directory.

        virtual void say(const std::string& filename);
        ///< Say some text, without an actor ref
        /// \param filename name of a sound file in "Sound/" in the data directory.

        virtual bool sayDone(const MWWorld::Ptr &reference=MWWorld::Ptr()) const;
        ///< Is actor not speaking?

        virtual void stopSay(const MWWorld::Ptr &reference=MWWorld::Ptr());
        ///< Stop an actor speaking

        virtual MWBase::SoundPtr playTrack(const DecoderPtr& decoder, PlayType type);
        ///< Play a 2D audio track, using a custom decoder

        virtual MWBase::SoundPtr playSound(const std::string& soundId, float volume, float pitch, PlayType type=Play_TypeSfx, PlayMode mode=Play_Normal, float offset=0);
        ///< Play a sound, independently of 3D-position
        ///< @param offset value from [0,1], when to start playback. 0 is beginning, 1 is end.

        virtual MWBase::SoundPtr playSound3D(const MWWorld::Ptr &reference, const std::string& soundId,
                                             float volume, float pitch, PlayType type=Play_TypeSfx,
                                             PlayMode mode=Play_Normal, float offset=0);
        ///< Play a sound from an object
        ///< @param offset value from [0,1], when to start playback. 0 is beginning, 1 is end.

        virtual void stopSound3D(const MWWorld::Ptr &reference, const std::string& soundId);
        ///< Stop the given object from playing the given sound,

        virtual void stopSound3D(const MWWorld::Ptr &reference);
        ///< Stop the given object from playing all sounds.

        virtual void stopSound(const MWWorld::CellStore *cell);
        ///< Stop all sounds for the given cell.

        virtual void stopSound(const std::string& soundId);
        ///< Stop a non-3d looping sound

        virtual void fadeOutSound3D(const MWWorld::Ptr &reference, const std::string& soundId, float duration);
        ///< Fade out given sound (that is already playing) of given object
        ///< @param reference Reference to object, whose sound is faded out
        ///< @param soundId ID of the sound to fade out.
        ///< @param duration Time until volume reaches 0.

        virtual bool getSoundPlaying(const MWWorld::Ptr &reference, const std::string& soundId) const;
        ///< Is the given sound currently playing on the given object?

        virtual void pauseSounds(int types=Play_TypeMask);
        ///< Pauses all currently playing sounds, including music.

        virtual void resumeSounds(int types=Play_TypeMask);
        ///< Resumes all previously paused sounds.

        virtual void update(float duration);

        virtual void setListenerPosDir(const Ogre::Vector3 &pos, const Ogre::Vector3 &dir, const Ogre::Vector3 &up);

        virtual void updatePtr (const MWWorld::Ptr& old, const MWWorld::Ptr& updated);
    };
}

#endif
