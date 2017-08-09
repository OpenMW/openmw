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
    class Sound;
    class Stream;
    struct Sound_Decoder;
    typedef std::shared_ptr<Sound_Decoder> DecoderPtr;
}

namespace MWBase
{
    typedef std::shared_ptr<MWSound::Sound> SoundPtr;
    typedef std::shared_ptr<MWSound::Stream> SoundStreamPtr;

    /// \brief Interface for sound manager (implemented in MWSound)
    class SoundManager
    {
        public:
            /* These must all fit together */
            enum PlayMode {
                Play_Normal  = 0, /* non-looping, affected by environment */
                Play_Loop    = 1<<0, /* Sound will continually loop until explicitly stopped */
                Play_NoEnv   = 1<<1, /* Do not apply environment effects (eg, underwater filters) */
                Play_RemoveAtDistance = 1<<2, /* (3D only) If the listener gets further than 2000 units away
                                                from the sound source, the sound is removed.
                                                This is weird stuff but apparently how vanilla works for sounds
                                                played by the PlayLoopSound family of script functions. Perhaps we
                                                can make this cut off a more subtle fade later, but have to
                                                be careful to not change the overall volume of areas by too much. */
                Play_NoPlayerLocal = 1<<3, /* (3D only) Don't play the sound local to the listener even if the
                                              player is making it. */
                Play_LoopNoEnv = Play_Loop | Play_NoEnv,
                Play_LoopRemoveAtDistance = Play_Loop | Play_RemoveAtDistance
            };
            enum PlayType {
                Play_TypeSfx   = 1<<4, /* Normal SFX sound */
                Play_TypeVoice = 1<<5, /* Voice sound */
                Play_TypeFoot  = 1<<6, /* Footstep sound */
                Play_TypeMusic = 1<<7, /* Music track */
                Play_TypeMovie = 1<<8, /* Movie audio track */
                Play_TypeMask  = Play_TypeSfx|Play_TypeVoice|Play_TypeFoot|Play_TypeMusic|Play_TypeMovie
            };

        private:

            SoundManager (const SoundManager&);
            ///< not implemented

            SoundManager& operator= (const SoundManager&);
            ///< not implemented

        public:

            SoundManager() {}

            virtual ~SoundManager() {}

            virtual void processChangedSettings(const std::set< std::pair<std::string, std::string> >& settings) = 0;

            virtual void stopMusic() = 0;
            ///< Stops music if it's playing

            virtual void streamMusic(const std::string& filename) = 0;
            ///< Play a soundifle
            /// \param filename name of a sound file in "Music/" in the data directory.

            virtual void startRandomTitle() = 0;
            ///< Starts a random track from the current playlist

            virtual bool isMusicPlaying() = 0;
            ///< Returns true if music is playing

            virtual void playPlaylist(const std::string &playlist) = 0;
            ///< Start playing music from the selected folder
            /// \param name of the folder that contains the playlist

            virtual void say(const MWWorld::ConstPtr &reference, const std::string& filename) = 0;
            ///< Make an actor say some text.
            /// \param filename name of a sound file in "Sound/" in the data directory.

            virtual void say(const std::string& filename) = 0;
            ///< Say some text, without an actor ref
            /// \param filename name of a sound file in "Sound/" in the data directory.

            virtual bool sayDone(const MWWorld::ConstPtr &reference=MWWorld::ConstPtr()) const = 0;
            ///< Is actor not speaking?

            virtual void stopSay(const MWWorld::ConstPtr &reference=MWWorld::ConstPtr()) = 0;
            ///< Stop an actor speaking

            virtual float getSaySoundLoudness(const MWWorld::ConstPtr& reference) const = 0;
            ///< Check the currently playing say sound for this actor
            /// and get an average loudness value (scale [0,1]) at the current time position.
            /// If the actor is not saying anything, returns 0.

            virtual SoundStreamPtr playTrack(const MWSound::DecoderPtr& decoder, PlayType type) = 0;
            ///< Play a 2D audio track, using a custom decoder

            virtual void stopTrack(SoundStreamPtr stream) = 0;
            ///< Stop the given audio track from playing

            virtual double getTrackTimeDelay(SoundStreamPtr stream) = 0;
            ///< Retives the time delay, in seconds, of the audio track (must be a sound
            /// returned by \ref playTrack). Only intended to be called by the track
            /// decoder's read method.

            virtual SoundPtr playSound(const std::string& soundId, float volume, float pitch,
                                       PlayType type=Play_TypeSfx, PlayMode mode=Play_Normal,
                                       float offset=0) = 0;
            ///< Play a sound, independently of 3D-position
            ///< @param offset Number of seconds into the sound to start playback.

            virtual MWBase::SoundPtr playSound3D(const MWWorld::ConstPtr &reference, const std::string& soundId,
                                                 float volume, float pitch, PlayType type=Play_TypeSfx,
                                                 PlayMode mode=Play_Normal, float offset=0) = 0;
            ///< Play a 3D sound attached to an MWWorld::Ptr. Will be updated automatically with the Ptr's position, unless Play_NoTrack is specified.
            ///< @param offset Number of seconds into the sound to start playback.

            virtual MWBase::SoundPtr playSound3D(const osg::Vec3f& initialPos, const std::string& soundId,
                                                 float volume, float pitch, PlayType type=Play_TypeSfx, PlayMode mode=Play_Normal, float offset=0) = 0;
            ///< Play a 3D sound at \a initialPos. If the sound should be moving, it must be updated using Sound::setPosition.

            virtual void stopSound(SoundPtr sound) = 0;
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

            virtual void pauseSounds(int types=Play_TypeMask) = 0;
            ///< Pauses all currently playing sounds, including music.

            virtual void resumeSounds(int types=Play_TypeMask) = 0;
            ///< Resumes all previously paused sounds.

            virtual void update(float duration) = 0;

            virtual void setListenerPosDir(const osg::Vec3f &pos, const osg::Vec3f &dir, const osg::Vec3f &up, bool underwater) = 0;

            virtual void updatePtr(const MWWorld::ConstPtr& old, const MWWorld::ConstPtr& updated) = 0;

            virtual void clear() = 0;
    };
}

#endif
