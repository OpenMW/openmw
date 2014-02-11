#ifndef GAME_MWBASE_SOUNDMANAGER_H
#define GAME_MWBASE_SOUNDMANAGER_H

#include <string>

#include <boost/shared_ptr.hpp>

#include <components/settings/settings.hpp>

#include "../mwworld/ptr.hpp"

namespace Ogre
{
    class Vector3;
}

namespace MWWorld
{
    class CellStore;
}

namespace MWSound
{
    class Sound;
    class Sound_Decoder;
    typedef boost::shared_ptr<Sound_Decoder> DecoderPtr;
}

namespace MWBase
{
    typedef boost::shared_ptr<MWSound::Sound> SoundPtr;

    /// \brief Interface for sound manager (implemented in MWSound)
    class SoundManager
    {
        public:
            /* These must all fit together */
            enum PlayMode {
                Play_Normal  = 0, /* tracked, non-looping, multi-instance, environment */
                Play_Loop    = 1<<0, /* Sound will continually loop until explicitly stopped */
                Play_NoEnv   = 1<<1, /* Do not apply environment effects (eg, underwater filters) */
                Play_NoTrack = 1<<2, /* (3D only) Play the sound at the given object's position
                                      * but do not keep it updated (the sound will not move with
                                      * the object and will not stop when the object is deleted. */

                Play_LoopNoEnv = Play_Loop | Play_NoEnv
            };
            enum PlayType {
                Play_TypeSfx   = 1<<3, /* Normal SFX sound */
                Play_TypeVoice = 1<<4, /* Voice sound */
                Play_TypeFoot  = 1<<5, /* Footstep sound */
                Play_TypeMusic = 1<<6, /* Music track */
                Play_TypeMovie = 1<<7, /* Movie audio track */
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

            virtual void processChangedSettings(const Settings::CategorySettingVector& settings) = 0;

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

            virtual void say(const MWWorld::Ptr &reference, const std::string& filename) = 0;
            ///< Make an actor say some text.
            /// \param filename name of a sound file in "Sound/" in the data directory.

            virtual void say(const std::string& filename) = 0;
            ///< Say some text, without an actor ref
            /// \param filename name of a sound file in "Sound/" in the data directory.

            virtual bool sayDone(const MWWorld::Ptr &reference=MWWorld::Ptr()) const = 0;
            ///< Is actor not speaking?

            virtual void stopSay(const MWWorld::Ptr &reference=MWWorld::Ptr()) = 0;
            ///< Stop an actor speaking

            virtual SoundPtr playTrack(const MWSound::DecoderPtr& decoder, PlayType type) = 0;
            ///< Play a 2D audio track, using a custom decoder

            virtual SoundPtr playSound(const std::string& soundId, float volume, float pitch,
                                       PlayType type=Play_TypeSfx, PlayMode mode=Play_Normal,
                                       float offset=0) = 0;
            ///< Play a sound, independently of 3D-position
            ///< @param offset Value from [0,1] meaning from which fraction the sound the playback starts.

            virtual SoundPtr playSound3D(const MWWorld::Ptr &reference, const std::string& soundId,
                                         float volume, float pitch, PlayType type=Play_TypeSfx,
                                         PlayMode mode=Play_Normal, float offset=0) = 0;
            ///< Play a sound from an object
            ///< @param offset Value from [0,1] meaning from which fraction the sound the playback starts.

            virtual void stopSound3D(const MWWorld::Ptr &reference, const std::string& soundId) = 0;
            ///< Stop the given object from playing the given sound,

            virtual void stopSound3D(const MWWorld::Ptr &reference) = 0;
            ///< Stop the given object from playing all sounds.

            virtual void stopSound(const MWWorld::CellStore *cell) = 0;
            ///< Stop all sounds for the given cell.

            virtual void stopSound(const std::string& soundId) = 0;
            ///< Stop a non-3d looping sound

            virtual void fadeOutSound3D(const MWWorld::Ptr &reference, const std::string& soundId, float duration) = 0;
            ///< Fade out given sound (that is already playing) of given object
            ///< @param reference Reference to object, whose sound is faded out
            ///< @param soundId ID of the sound to fade out.
            ///< @param duration Time until volume reaches 0.

            virtual bool getSoundPlaying(const MWWorld::Ptr &reference, const std::string& soundId) const = 0;
            ///< Is the given sound currently playing on the given object?
            ///  If you want to check if sound played with playSound is playing, use empty Ptr

            virtual void pauseSounds(int types=Play_TypeMask) = 0;
            ///< Pauses all currently playing sounds, including music.

            virtual void resumeSounds(int types=Play_TypeMask) = 0;
            ///< Resumes all previously paused sounds.

            virtual void update(float duration) = 0;

            virtual void setListenerPosDir(const Ogre::Vector3 &pos, const Ogre::Vector3 &dir, const Ogre::Vector3 &up) = 0;

            virtual void updatePtr (const MWWorld::Ptr& old, const MWWorld::Ptr& updated) = 0;

            virtual void clear() = 0;
    };
}

#endif
