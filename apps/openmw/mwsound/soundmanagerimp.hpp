#ifndef GAME_SOUND_SOUNDMANAGER_H
#define GAME_SOUND_SOUNDMANAGER_H

#include <memory>
#include <string>
#include <utility>
#include <deque>
#include <map>
#include <unordered_map>

#include <components/settings/settings.hpp>

#include <components/fallback/fallback.hpp>

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
    class Stream;
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

        std::unique_ptr<Sound_Output> mOutput;

        // Caches available music tracks by <playlist name, (sound files) >
        std::unordered_map<std::string, std::vector<std::string>> mMusicFiles;
        std::unordered_map<std::string, std::vector<int>> mMusicToPlay; // A list with music files not yet played
        std::string mLastPlayedMusic; // The music file that was last played

        float mMasterVolume;
        float mSFXVolume;
        float mMusicVolume;
        float mVoiceVolume;
        float mFootstepsVolume;

        int mNearWaterRadius;
        int mNearWaterPoints;
        float mNearWaterIndoorTolerance;
        float mNearWaterOutdoorTolerance;
        std::string mNearWaterIndoorID;
        std::string mNearWaterOutdoorID;
        typedef std::unique_ptr<std::deque<Sound_Buffer> > SoundBufferList;
        // List of sound buffers, grown as needed. New enties are added to the
        // back, allowing existing Sound_Buffer references/pointers to remain
        // valid.
        SoundBufferList mSoundBuffers;
        size_t mBufferCacheMin;
        size_t mBufferCacheMax;
        size_t mBufferCacheSize;

        typedef std::unordered_map<std::string,Sound_Buffer*> NameBufferMap;
        NameBufferMap mBufferNameMap;

        // NOTE: unused buffers are stored in front-newest order.
        typedef std::deque<Sound_Buffer*> SoundList;
        SoundList mUnusedBuffers;

        std::unique_ptr<std::deque<Sound>> mSounds;
        std::vector<Sound*> mUnusedSounds;

        std::unique_ptr<std::deque<Stream>> mStreams;
        std::vector<Stream*> mUnusedStreams;

        typedef std::pair<MWBase::Sound*,Sound_Buffer*> SoundBufferRefPair;
        typedef std::vector<SoundBufferRefPair> SoundBufferRefPairList;
        typedef std::map<MWWorld::ConstPtr,SoundBufferRefPairList> SoundMap;
        SoundMap mActiveSounds;

        typedef std::map<MWWorld::ConstPtr,Stream*> SaySoundMap;
        SaySoundMap mSaySoundsQueue;
        SaySoundMap mActiveSaySounds;

        typedef std::vector<Stream*> TrackList;
        TrackList mActiveTracks;

        Stream *mMusic;
        std::string mCurrentPlaylist;

        bool mListenerUnderwater;
        osg::Vec3f mListenerPos;
        osg::Vec3f mListenerDir;
        osg::Vec3f mListenerUp;

        int mPausedSoundTypes[BlockerType::MaxCount] = {};

        Sound *mUnderwaterSound;
        Sound *mNearWaterSound;

        std::string mNextMusic;
        bool mPlaybackPaused;

        Sound_Buffer *insertSound(const std::string &soundId, const ESM::Sound *sound);

        Sound_Buffer *lookupSound(const std::string &soundId) const;
        Sound_Buffer *loadSound(const std::string &soundId);

        // returns a decoder to start streaming, or nullptr if the sound was not found
        DecoderPtr loadVoice(const std::string &voicefile);

        Sound *getSoundRef();
        Stream *getStreamRef();

        Stream *playVoice(DecoderPtr decoder, const osg::Vec3f &pos, bool playlocal);

        void streamMusicFull(const std::string& filename);
        void advanceMusic(const std::string& filename);
        void startRandomTitle();

        void updateSounds(float duration);
        void updateRegionSound(float duration);
        void updateWaterSound(float duration);
        void updateMusic(float duration);

        float volumeFromType(Type type) const;

        SoundManager(const SoundManager &rhs);
        SoundManager& operator=(const SoundManager &rhs);

    protected:
        DecoderPtr getDecoder();
        friend class OpenAL_Output;

        void stopSound(Sound_Buffer *sfx, const MWWorld::ConstPtr &ptr);
        ///< Stop the given object from playing given sound buffer.

    public:
        SoundManager(const VFS::Manager* vfs, bool useSound);
        virtual ~SoundManager();

        virtual void processChangedSettings(const Settings::CategorySettingVector& settings);

        virtual void stopMusic();
        ///< Stops music if it's playing

        virtual void streamMusic(const std::string& filename);
        ///< Play a soundifle
        /// \param filename name of a sound file in "Music/" in the data directory.

        virtual bool isMusicPlaying();
        ///< Returns true if music is playing

        virtual void playPlaylist(const std::string &playlist);
        ///< Start playing music from the selected folder
        /// \param name of the folder that contains the playlist

        virtual void playTitleMusic();
        ///< Start playing title music

        virtual void say(const MWWorld::ConstPtr &reference, const std::string& filename);
        ///< Make an actor say some text.
        /// \param filename name of a sound file in "Sound/" in the data directory.

        virtual void say(const std::string& filename);
        ///< Say some text, without an actor ref
        /// \param filename name of a sound file in "Sound/" in the data directory.

        virtual bool sayActive(const MWWorld::ConstPtr &reference=MWWorld::ConstPtr()) const;
        ///< Is actor not speaking?

        virtual bool sayDone(const MWWorld::ConstPtr &reference=MWWorld::ConstPtr()) const;
        ///< For scripting backward compatibility

        virtual void stopSay(const MWWorld::ConstPtr &reference=MWWorld::ConstPtr());
        ///< Stop an actor speaking

        virtual float getSaySoundLoudness(const MWWorld::ConstPtr& reference) const;
        ///< Check the currently playing say sound for this actor
        /// and get an average loudness value (scale [0,1]) at the current time position.
        /// If the actor is not saying anything, returns 0.

        virtual Stream *playTrack(const DecoderPtr& decoder, Type type);
        ///< Play a 2D audio track, using a custom decoder

        virtual void stopTrack(Stream *stream);
        ///< Stop the given audio track from playing

        virtual double getTrackTimeDelay(Stream *stream);
        ///< Retives the time delay, in seconds, of the audio track (must be a sound
        /// returned by \ref playTrack). Only intended to be called by the track
        /// decoder's read method.

        virtual Sound *playSound(const std::string& soundId, float volume, float pitch, Type type=Type::Sfx, PlayMode mode=PlayMode::Normal, float offset=0);
        ///< Play a sound, independently of 3D-position
        ///< @param offset Number of seconds into the sound to start playback.

        virtual Sound *playSound3D(const MWWorld::ConstPtr &reference, const std::string& soundId,
                                   float volume, float pitch, Type type=Type::Sfx,
                                   PlayMode mode=PlayMode::Normal, float offset=0);
        ///< Play a 3D sound attached to an MWWorld::Ptr. Will be updated automatically with the Ptr's position, unless Play_NoTrack is specified.
        ///< @param offset Number of seconds into the sound to start playback.

        virtual Sound *playSound3D(const osg::Vec3f& initialPos, const std::string& soundId,
                                   float volume, float pitch, Type type, PlayMode mode, float offset=0);
        ///< Play a 3D sound at \a initialPos. If the sound should be moving, it must be updated using Sound::setPosition.
        ///< @param offset Number of seconds into the sound to start playback.

        virtual void stopSound(Sound *sound);
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

        virtual void pauseSounds(MWSound::BlockerType blocker, int types=int(Type::Mask));
        ///< Pauses all currently playing sounds, including music.

        virtual void resumeSounds(MWSound::BlockerType blocker);
        ///< Resumes all previously paused sounds.

        virtual void pausePlayback();
        virtual void resumePlayback();

        virtual void update(float duration);

        virtual void setListenerPosDir(const osg::Vec3f &pos, const osg::Vec3f &dir, const osg::Vec3f &up, bool underwater);

        virtual void updatePtr (const MWWorld::ConstPtr& old, const MWWorld::ConstPtr& updated);

        virtual void clear();
    };
}

#endif
