#ifndef GAME_SOUND_SOUNDMANAGER_H
#define GAME_SOUND_SOUNDMANAGER_H

#include <memory>
#include <string>
#include <utility>
#include <deque>
#include <map>

#include <boost/shared_ptr.hpp>

#include <components/settings/settings.hpp>

#include "loudness.hpp"
#include "../mwbase/soundmanager.hpp"

namespace VFS
{
    class Manager;
}

namespace ESM
{
    struct Sound;
}

namespace MWSound
{
    class Sound_Output;
    struct Sound_Decoder;
    class Sound;
    class Sound_Buffer;

    enum Environment {
        Env_Normal,
        Env_Underwater
    };
    // Extra play flags, not intended for caller use
    enum PlayModeEx {
        Play_2D = 0,
        Play_3D = 1<<31
    };

    class SoundManager : public MWBase::SoundManager
    {
        const VFS::Manager* mVFS;

        std::auto_ptr<Sound_Output> mOutput;

        // Caches available music tracks by <playlist name, (sound files) >
        std::map<std::string, std::vector<std::string> > mMusicFiles;
        std::string mLastPlayedMusic; // The music file that was last played

        float mMasterVolume;
        float mSFXVolume;
        float mMusicVolume;
        float mVoiceVolume;
        float mFootstepsVolume;

        typedef std::auto_ptr<std::deque<Sound_Buffer> > SoundBufferList;
        // List of sound buffers, grown as needed. New enties are added to the
        // back, allowing existing Sound_Buffer references/pointers to remain
        // valid.
        SoundBufferList mSoundBuffers;
        size_t mBufferCacheMin;
        size_t mBufferCacheMax;
        size_t mBufferCacheSize;

        typedef std::map<std::string,Sound_Buffer*> NameBufferMap;
        NameBufferMap mBufferNameMap;

        typedef std::deque<Sound_Loudness> LoudnessList;
        LoudnessList mVoiceLipBuffers;

        typedef std::map<std::string,Sound_Loudness*> NameLoudnessRefMap;
        NameLoudnessRefMap mVoiceLipNameMap;

        // NOTE: unused buffers are stored in front-newest order.
        typedef std::deque<Sound_Buffer*> SoundList;
        SoundList mUnusedBuffers;

        typedef std::pair<MWBase::SoundPtr,Sound_Buffer*> SoundBufferRefPair;
        typedef std::vector<SoundBufferRefPair> SoundBufferRefPairList;
        typedef std::map<MWWorld::ConstPtr,SoundBufferRefPairList> SoundMap;
        SoundMap mActiveSounds;

        typedef std::pair<MWBase::SoundStreamPtr,Sound_Loudness*> SoundLoudnessPair;
        typedef std::map<MWWorld::ConstPtr,SoundLoudnessPair> SaySoundMap;
        SaySoundMap mActiveSaySounds;

        typedef std::pair<DecoderPtr,Sound_Loudness*> DecoderLoudnessPair;
        typedef std::map<MWWorld::ConstPtr,DecoderLoudnessPair> SayDecoderMap;
        SayDecoderMap mPendingSaySounds;

        typedef std::vector<MWBase::SoundStreamPtr> TrackList;
        TrackList mActiveTracks;

        MWBase::SoundStreamPtr mMusic;
        std::string mCurrentPlaylist;

        bool mListenerUnderwater;
        osg::Vec3f mListenerPos;
        osg::Vec3f mListenerDir;
        osg::Vec3f mListenerUp;

        int mPausedSoundTypes;

        MWBase::SoundPtr mUnderwaterSound;

        Sound_Buffer *insertSound(const std::string &soundId, const ESM::Sound *sound);

        Sound_Buffer *lookupSound(const std::string &soundId) const;
        Sound_Buffer *loadSound(const std::string &soundId);

        // Ensures the loudness/"lip" data gets loaded, and returns a decoder
        // to start streaming
        DecoderPtr loadVoice(const std::string &voicefile, Sound_Loudness **lipdata);

        MWBase::SoundStreamPtr playVoice(DecoderPtr decoder, const osg::Vec3f &pos, bool playlocal);

        void streamMusicFull(const std::string& filename);
        void updateSounds(float duration);
        void updateRegionSound(float duration);

        float volumeFromType(PlayType type) const;

        SoundManager(const SoundManager &rhs);
        SoundManager& operator=(const SoundManager &rhs);

    protected:
        DecoderPtr getDecoder();
        friend class OpenAL_Output;

    public:
        SoundManager(const VFS::Manager* vfs, bool useSound);
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

        virtual void say(const MWWorld::ConstPtr &reference, const std::string& filename);
        ///< Make an actor say some text.
        /// \param filename name of a sound file in "Sound/" in the data directory.

        virtual void say(const std::string& filename);
        ///< Say some text, without an actor ref
        /// \param filename name of a sound file in "Sound/" in the data directory.

        virtual bool sayDone(const MWWorld::ConstPtr &reference=MWWorld::ConstPtr()) const;
        ///< Is actor not speaking?

        virtual void stopSay(const MWWorld::ConstPtr &reference=MWWorld::ConstPtr());
        ///< Stop an actor speaking

        virtual float getSaySoundLoudness(const MWWorld::ConstPtr& reference) const;
        ///< Check the currently playing say sound for this actor
        /// and get an average loudness value (scale [0,1]) at the current time position.
        /// If the actor is not saying anything, returns 0.

        virtual MWBase::SoundStreamPtr playTrack(const DecoderPtr& decoder, PlayType type);
        ///< Play a 2D audio track, using a custom decoder

        virtual void stopTrack(MWBase::SoundStreamPtr stream);
        ///< Stop the given audio track from playing

        virtual double getTrackTimeDelay(MWBase::SoundStreamPtr stream);
        ///< Retives the time delay, in seconds, of the audio track (must be a sound
        /// returned by \ref playTrack). Only intended to be called by the track
        /// decoder's read method.

        virtual MWBase::SoundPtr playSound(const std::string& soundId, float volume, float pitch, PlayType type=Play_TypeSfx, PlayMode mode=Play_Normal, float offset=0);
        ///< Play a sound, independently of 3D-position
        ///< @param offset Number of seconds into the sound to start playback.

        virtual MWBase::SoundPtr playSound3D(const MWWorld::ConstPtr &reference, const std::string& soundId,
                                             float volume, float pitch, PlayType type=Play_TypeSfx,
                                             PlayMode mode=Play_Normal, float offset=0);
        ///< Play a 3D sound attached to an MWWorld::Ptr. Will be updated automatically with the Ptr's position, unless Play_NoTrack is specified.
        ///< @param offset Number of seconds into the sound to start playback.

        virtual MWBase::SoundPtr playSound3D(const osg::Vec3f& initialPos, const std::string& soundId,
                                             float volume, float pitch, PlayType type, PlayMode mode, float offset=0);
        ///< Play a 3D sound at \a initialPos. If the sound should be moving, it must be updated using Sound::setPosition.
        ///< @param offset Number of seconds into the sound to start playback.

        virtual void stopSound(MWBase::SoundPtr sound);
        ///< Stop the given sound from playing
        /// @note no-op if \a sound is null

        virtual void stopSound3D(const MWWorld::ConstPtr &reference, const std::string& soundId);
        ///< Stop the given object from playing the given sound,

        virtual void stopSound3D(const MWWorld::ConstPtr &reference);
        ///< Stop the given object from playing all sounds.

        virtual void stopSound(const MWWorld::CellStore *cell);
        ///< Stop all sounds for the given cell.

        virtual void stopSound(const std::string& soundId);
        ///< Stop a non-3d looping sound

        virtual void fadeOutSound3D(const MWWorld::ConstPtr &reference, const std::string& soundId, float duration);
        ///< Fade out given sound (that is already playing) of given object
        ///< @param reference Reference to object, whose sound is faded out
        ///< @param soundId ID of the sound to fade out.
        ///< @param duration Time until volume reaches 0.

        virtual bool getSoundPlaying(const MWWorld::ConstPtr &reference, const std::string& soundId) const;
        ///< Is the given sound currently playing on the given object?

        virtual void pauseSounds(int types=Play_TypeMask);
        ///< Pauses all currently playing sounds, including music.

        virtual void resumeSounds(int types=Play_TypeMask);
        ///< Resumes all previously paused sounds.

        virtual void update(float duration);

        virtual void setListenerPosDir(const osg::Vec3f &pos, const osg::Vec3f &dir, const osg::Vec3f &up, bool underwater);

        virtual void updatePtr (const MWWorld::ConstPtr& old, const MWWorld::ConstPtr& updated);

        virtual void clear();
    };
}

#endif
