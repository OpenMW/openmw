#ifndef GAME_SOUND_SOUNDMANAGER_H
#define GAME_SOUND_SOUNDMANAGER_H

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include <components/fallback/fallback.hpp>
#include <components/misc/objectpool.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/settings/settings.hpp>

#include "../mwbase/soundmanager.hpp"

#include "regionsoundselector.hpp"
#include "soundbuffer.hpp"
#include "type.hpp"
#include "watersoundupdater.hpp"

namespace VFS
{
    class Manager;
}

namespace ESM
{
    struct Sound;
    struct Cell;
}

namespace MWWorld
{
    class Cell;
}

namespace MWSound
{
    class SoundOutput;
    struct SoundDecoder;
    class SoundBase;
    class Sound;
    class Stream;

    using SoundPtr = Misc::ObjectPtr<Sound>;
    using StreamPtr = Misc::ObjectPtr<Stream>;

    class SoundManager : public MWBase::SoundManager
    {
        const VFS::Manager* mVFS;

        std::unique_ptr<SoundOutput> mOutput;

        WaterSoundUpdater mWaterSoundUpdater;

        SoundBufferPool mSoundBuffers;

        Misc::ObjectPool<Sound> mSounds;

        Misc::ObjectPool<Stream> mStreams;

        typedef std::pair<SoundPtr, SoundBuffer*> SoundBufferRefPair;
        typedef std::vector<SoundBufferRefPair> SoundBufferRefPairList;

        struct ActiveSound
        {
            const MWWorld::CellStore* mCell = nullptr;
            SoundBufferRefPairList mList;
        };

        typedef std::map<const MWWorld::LiveCellRefBase*, ActiveSound> SoundMap;
        SoundMap mActiveSounds;

        struct SaySound
        {
            const MWWorld::CellStore* mCell;
            StreamPtr mStream;
        };

        typedef std::map<const MWWorld::LiveCellRefBase*, SaySound> SaySoundMap;
        SaySoundMap mSaySoundsQueue;
        SaySoundMap mActiveSaySounds;

        typedef std::vector<StreamPtr> TrackList;
        TrackList mActiveTracks;

        StreamPtr mMusic;
        MusicType mMusicType;

        bool mListenerUnderwater;
        osg::Vec3f mListenerPos;
        osg::Vec3f mListenerDir;
        osg::Vec3f mListenerUp;

        int mPausedSoundTypes[BlockerType::MaxCount] = {};

        Sound* mUnderwaterSound;
        Sound* mNearWaterSound;

        VFS::Path::Normalized mNextMusic;
        bool mPlaybackPaused;

        RegionSoundSelector mRegionSoundSelector;

        float mTimePassed;

        const MWWorld::Cell* mLastCell;

        Sound* mCurrentRegionSound;

        SoundBuffer* insertSound(const std::string& soundId, const ESM::Sound* sound);

        // returns a decoder to start streaming, or nullptr if the sound was not found
        DecoderPtr loadVoice(VFS::Path::NormalizedView voicefile);

        SoundPtr getSoundRef();
        StreamPtr getStreamRef();

        StreamPtr playVoice(DecoderPtr decoder, const osg::Vec3f& pos, bool playlocal);

        void streamMusicFull(VFS::Path::NormalizedView filename);
        void advanceMusic(VFS::Path::NormalizedView filename, float fadeOut = 1.f);

        void cull3DSound(SoundBase* sound);

        bool remove3DSoundAtDistance(PlayMode mode, const MWWorld::ConstPtr& ptr) const;

        Sound* playSound(SoundBuffer* sfx, float volume, float pitch, Type type = Type::Sfx,
            PlayMode mode = PlayMode::Normal, float offset = 0);
        Sound* playSound3D(const MWWorld::ConstPtr& ptr, SoundBuffer* sfx, float volume, float pitch, Type type,
            PlayMode mode, float offset);

        void updateSounds(float duration);
        void updateRegionSound(float duration);
        void updateWaterSound();
        void updateMusic(float duration);

        enum class WaterSoundAction
        {
            DoNothing,
            SetVolume,
            FinishSound,
            PlaySound,
        };

        std::pair<WaterSoundAction, SoundBuffer*> getWaterSoundAction(
            const WaterSoundUpdate& update, const MWWorld::Cell* cell) const;

        SoundManager(const SoundManager& rhs);
        SoundManager& operator=(const SoundManager& rhs);

    protected:
        DecoderPtr getDecoder();
        friend class OpenALOutput;

        void stopSound(SoundBuffer* sfx, const MWWorld::ConstPtr& ptr);
        ///< Stop the given object from playing given sound buffer.

    public:
        SoundManager(const VFS::Manager* vfs, bool useSound);
        ~SoundManager() override;

        void processChangedSettings(const Settings::CategorySettingVector& settings) override;

        bool isEnabled() const override { return mOutput->isInitialized(); }
        ///< Returns true if sound system is enabled

        void stopMusic() override;
        ///< Stops music if it's playing

        MWSound::MusicType getMusicType() const override { return mMusicType; }

        void streamMusic(VFS::Path::NormalizedView filename, MWSound::MusicType type, float fade = 1.f) override;
        ///< Play a soundifle
        /// \param filename name of a sound file in the data directory.
        /// \param type music type.
        /// \param fade time in seconds to fade out current track before start this one.

        bool isMusicPlaying() override;
        ///< Returns true if music is playing

        void say(const MWWorld::ConstPtr& reference, VFS::Path::NormalizedView filename) override;
        ///< Make an actor say some text.
        /// \param filename name of a sound file in the VFS

        void say(VFS::Path::NormalizedView filename) override;
        ///< Say some text, without an actor ref
        /// \param filename name of a sound file in the VFS

        bool sayActive(const MWWorld::ConstPtr& reference = MWWorld::ConstPtr()) const override;
        ///< Is actor not speaking?

        bool sayDone(const MWWorld::ConstPtr& reference = MWWorld::ConstPtr()) const override;
        ///< For scripting backward compatibility

        void stopSay(const MWWorld::ConstPtr& reference = MWWorld::ConstPtr()) override;
        ///< Stop an actor speaking

        float getSaySoundLoudness(const MWWorld::ConstPtr& reference) const override;
        ///< Check the currently playing say sound for this actor
        /// and get an average loudness value (scale [0,1]) at the current time position.
        /// If the actor is not saying anything, returns 0.

        Stream* playTrack(const DecoderPtr& decoder, Type type) override;
        ///< Play a 2D audio track, using a custom decoder

        void stopTrack(Stream* stream) override;
        ///< Stop the given audio track from playing

        double getTrackTimeDelay(Stream* stream) override;
        ///< Retives the time delay, in seconds, of the audio track (must be a sound
        /// returned by \ref playTrack). Only intended to be called by the track
        /// decoder's read method.

        Sound* playSound(const ESM::RefId& soundId, float volume, float pitch, Type type = Type::Sfx,
            PlayMode mode = PlayMode::Normal, float offset = 0) override;
        ///< Play a sound, independently of 3D-position
        ///< @param offset Number of seconds into the sound to start playback.

        Sound* playSound(std::string_view fileName, float volume, float pitch, Type type = Type::Sfx,
            PlayMode mode = PlayMode::Normal, float offset = 0) override;
        ///< Play a sound, independently of 3D-position
        ///< @param offset Number of seconds into the sound to start playback.

        Sound* playSound3D(const MWWorld::ConstPtr& reference, const ESM::RefId& soundId, float volume, float pitch,
            Type type = Type::Sfx, PlayMode mode = PlayMode::Normal, float offset = 0) override;
        ///< Play a 3D sound attached to an MWWorld::Ptr. Will be updated automatically with the Ptr's position, unless
        ///< Play_NoTrack is specified.
        ///< @param offset Number of seconds into the sound to start playback.

        Sound* playSound3D(const MWWorld::ConstPtr& reference, std::string_view fileName, float volume, float pitch,
            Type type = Type::Sfx, PlayMode mode = PlayMode::Normal, float offset = 0) override;
        ///< Play a 3D sound attached to an MWWorld::Ptr. Will be updated automatically with the Ptr's position, unless
        ///< Play_NoTrack is specified.
        ///< @param offset Number of seconds into the sound to start playback.

        Sound* playSound3D(const osg::Vec3f& initialPos, const ESM::RefId& soundId, float volume, float pitch,
            Type type, PlayMode mode, float offset = 0) override;
        ///< Play a 3D sound at \a initialPos. If the sound should be moving, it must be updated using
        ///< Sound::setPosition.
        ///< @param offset Number of seconds into the sound to start playback.

        void stopSound(Sound* sound) override;
        ///< Stop the given sound from playing
        /// @note no-op if \a sound is null

        void stopSound3D(const MWWorld::ConstPtr& reference, const ESM::RefId& soundId) override;
        ///< Stop the given object from playing the given sound.

        void stopSound3D(const MWWorld::ConstPtr& reference, std::string_view fileName) override;
        ///< Stop the given object from playing the given sound.

        void stopSound3D(const MWWorld::ConstPtr& reference) override;
        ///< Stop the given object from playing all sounds.

        void stopSound(const MWWorld::CellStore* cell) override;
        ///< Stop all sounds for the given cell.

        void fadeOutSound3D(const MWWorld::ConstPtr& reference, const ESM::RefId& soundId, float duration) override;
        ///< Fade out given sound (that is already playing) of given object
        ///< @param reference Reference to object, whose sound is faded out
        ///< @param soundId ID of the sound to fade out.
        ///< @param duration Time until volume reaches 0.

        bool getSoundPlaying(const MWWorld::ConstPtr& reference, const ESM::RefId& soundId) const override;
        ///< Is the given sound currently playing on the given object?

        bool getSoundPlaying(const MWWorld::ConstPtr& reference, std::string_view fileName) const override;
        ///< Is the given sound currently playing on the given object?

        void pauseSounds(MWSound::BlockerType blocker, int types = int(Type::Mask)) override;
        ///< Pauses all currently playing sounds, including music.

        void resumeSounds(MWSound::BlockerType blocker) override;
        ///< Resumes all previously paused sounds.

        void pausePlayback() override;
        void resumePlayback() override;

        void update(float duration);

        void setListenerPosDir(
            const osg::Vec3f& pos, const osg::Vec3f& dir, const osg::Vec3f& up, bool underwater) override;

        void updatePtr(const MWWorld::ConstPtr& old, const MWWorld::ConstPtr& updated) override;

        void clear() override;
    };
}

#endif
