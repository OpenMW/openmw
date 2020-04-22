#ifndef GAME_MWBASE_SOUNDMANAGER_H
#define GAME_MWBASE_SOUNDMANAGER_H

#include <memory>
#include <string>
#include <set>

#include "../mwworld/ptr.hpp"

namespace MWWorld
{
    class CellStore;
}

namespace MWSound
{
    // Each entry excepts of MaxCount should be used only in one place
    enum BlockerType
    {
        VideoPlayback,

        MaxCount
    };

    class Sound;
    class Stream;
    struct Sound_Decoder;
    typedef std::shared_ptr<Sound_Decoder> DecoderPtr;

    /* These must all fit together */
    enum class PlayMode {
        Normal  = 0, /* non-looping, affected by environment */
        Loop    = 1<<0, /* Sound will continually loop until explicitly stopped */
        NoEnv   = 1<<1, /* Do not apply environment effects (eg, underwater filters) */
        RemoveAtDistance = 1<<2, /* (3D only) If the listener gets further than 2000 units away
                                  * from the sound source, the sound is removed.
                                  * This is weird stuff but apparently how vanilla works for sounds
                                  * played by the PlayLoopSound family of script functions. Perhaps
                                  * we can make this cut off a more subtle fade later, but have to
                                  * be careful to not change the overall volume of areas by too
                                  * much. */
        NoPlayerLocal = 1<<3, /* (3D only) Don't play the sound local to the listener even if the
                               * player is making it. */
        LoopNoEnv = Loop | NoEnv,
        LoopRemoveAtDistance = Loop | RemoveAtDistance
    };
    enum class Type {
        Sfx   = 1<<4, /* Normal SFX sound */
        Voice = 1<<5, /* Voice sound */
        Foot  = 1<<6, /* Footstep sound */
        Music = 1<<7, /* Music track */
        Movie = 1<<8, /* Movie audio track */
        Mask  = Sfx | Voice | Foot | Music | Movie
    };
    // Used for creating a type mask for SoundManager::pauseSounds and resumeSounds
    inline int operator~(Type a) { return ~static_cast<int>(a); }
    inline int operator&(Type a, Type b) { return static_cast<int>(a) & static_cast<int>(b); }
    inline int operator&(int a, Type b) { return a & static_cast<int>(b); }
    inline int operator|(Type a, Type b) { return static_cast<int>(a) | static_cast<int>(b); }
}

namespace MWBase
{
    using Sound = MWSound::Sound;
    using SoundStream = MWSound::Stream;

    /// \brief Interface for sound manager (implemented in MWSound)
    class SoundManager
    {
            SoundManager (const SoundManager&);
            ///< not implemented

            SoundManager& operator= (const SoundManager&);
            ///< not implemented

        protected:
            using PlayMode = MWSound::PlayMode;
            using Type = MWSound::Type;

        public:
            SoundManager() {}
            virtual ~SoundManager() {}

            virtual void processChangedSettings(const std::set< std::pair<std::string, std::string> >& settings) = 0;

            virtual void stopMusic() = 0;
            ///< Stops music if it's playing

            virtual void streamMusic(const std::string& filename) = 0;
            ///< Play a soundifle
            /// \param filename name of a sound file in "Music/" in the data directory.

            virtual bool isMusicPlaying() = 0;
            ///< Returns true if music is playing

            virtual void playPlaylist(const std::string &playlist) = 0;
            ///< Start playing music from the selected folder
            /// \param name of the folder that contains the playlist

            virtual void playTitleMusic() = 0;
            ///< Start playing title music

            virtual void say(const MWWorld::ConstPtr &reference, const std::string& filename) = 0;
            ///< Make an actor say some text.
            /// \param filename name of a sound file in "Sound/" in the data directory.

            virtual void say(const std::string& filename) = 0;
            ///< Say some text, without an actor ref
            /// \param filename name of a sound file in "Sound/" in the data directory.

            virtual bool sayActive(const MWWorld::ConstPtr &reference=MWWorld::ConstPtr()) const = 0;
            ///< Is actor not speaking?

            virtual bool sayDone(const MWWorld::ConstPtr &reference=MWWorld::ConstPtr()) const = 0;
            ///< For scripting backward compatibility

            virtual void stopSay(const MWWorld::ConstPtr &reference=MWWorld::ConstPtr()) = 0;
            ///< Stop an actor speaking

            virtual float getSaySoundLoudness(const MWWorld::ConstPtr& reference) const = 0;
            ///< Check the currently playing say sound for this actor
            /// and get an average loudness value (scale [0,1]) at the current time position.
            /// If the actor is not saying anything, returns 0.

            virtual SoundStream *playTrack(const MWSound::DecoderPtr& decoder, Type type) = 0;
            ///< Play a 2D audio track, using a custom decoder. The caller is expected to call
            /// stopTrack with the returned handle when done.

            virtual void stopTrack(SoundStream *stream) = 0;
            ///< Stop the given audio track from playing

            virtual double getTrackTimeDelay(SoundStream *stream) = 0;
            ///< Retives the time delay, in seconds, of the audio track (must be a sound
            /// returned by \ref playTrack). Only intended to be called by the track
            /// decoder's read method.

            virtual Sound *playSound(const std::string& soundId, float volume, float pitch,
                                     Type type=Type::Sfx, PlayMode mode=PlayMode::Normal,
                                     float offset=0) = 0;
            ///< Play a sound, independently of 3D-position
            ///< @param offset Number of seconds into the sound to start playback.

            virtual Sound *playSound3D(const MWWorld::ConstPtr &reference, const std::string& soundId,
                                       float volume, float pitch, Type type=Type::Sfx,
                                       PlayMode mode=PlayMode::Normal, float offset=0) = 0;
            ///< Play a 3D sound attached to an MWWorld::Ptr. Will be updated automatically with the Ptr's position, unless Play_NoTrack is specified.
            ///< @param offset Number of seconds into the sound to start playback.

            virtual Sound *playSound3D(const osg::Vec3f& initialPos, const std::string& soundId,
                                       float volume, float pitch, Type type=Type::Sfx,
                                       PlayMode mode=PlayMode::Normal, float offset=0) = 0;
            ///< Play a 3D sound at \a initialPos. If the sound should be moving, it must be updated using Sound::setPosition.

            virtual void stopSound(Sound *sound) = 0;
            ///< Stop the given sound from playing

            virtual void stopSound3D(const MWWorld::ConstPtr &reference, const std::string& soundId) = 0;
            ///< Stop the given object from playing the given sound,

            virtual void stopSound3D(const MWWorld::ConstPtr &reference) = 0;
            ///< Stop the given object from playing all sounds.

            virtual void stopSound(const MWWorld::CellStore *cell) = 0;
            ///< Stop all sounds for the given cell.

            virtual void stopSound(const std::string& soundId) = 0;
            ///< Stop a non-3d looping sound

            virtual void fadeOutSound3D(const MWWorld::ConstPtr &reference, const std::string& soundId, float duration) = 0;
            ///< Fade out given sound (that is already playing) of given object
            ///< @param reference Reference to object, whose sound is faded out
            ///< @param soundId ID of the sound to fade out.
            ///< @param duration Time until volume reaches 0.

            virtual bool getSoundPlaying(const MWWorld::ConstPtr &reference, const std::string& soundId) const = 0;
            ///< Is the given sound currently playing on the given object?
            ///  If you want to check if sound played with playSound is playing, use empty Ptr

            virtual void pauseSounds(MWSound::BlockerType blocker, int types=int(Type::Mask)) = 0;
            ///< Pauses all currently playing sounds, including music.

            virtual void resumeSounds(MWSound::BlockerType blocker) = 0;
            ///< Resumes all previously paused sounds.

            virtual void pausePlayback() = 0;
            virtual void resumePlayback() = 0;

            virtual void update(float duration) = 0;

            virtual void setListenerPosDir(const osg::Vec3f &pos, const osg::Vec3f &dir, const osg::Vec3f &up, bool underwater) = 0;

            virtual void updatePtr(const MWWorld::ConstPtr& old, const MWWorld::ConstPtr& updated) = 0;

            virtual void clear() = 0;
    };
}

#endif
