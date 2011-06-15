#ifndef GAME_SOUND_SOUNDMANAGER_H
#define GAME_SOUND_SOUNDMANAGER_H

#include <string>
#include <map>

#include <boost/filesystem.hpp>
#include "../mwworld/ptr.hpp"
#include <openengine/sound/sndmanager.hpp>

namespace Ogre
{
    class Root;
    class Camera;
}

namespace ESMS
{
    struct ESMStore;
}

namespace MWSound
{
    //SoundPtr *music;
    class SoundManager
    {
            // Hide implementation details - engine.cpp is compiling
            // enough as it is.
            struct SoundImpl;

            SoundImpl *mData;
            std::vector<boost::filesystem::path> files;
            bool fsStrict;

            void streamMusicFull (const std::string& filename);
            ///< Play a soundifle
            /// \param absolute filename

        public:

      SoundManager(Ogre::Root*, Ogre::Camera*, const ESMS::ESMStore &store,
                   boost::filesystem::path dataDir, bool useSound, bool fsstrict);
            ~SoundManager();

            void streamMusic(const std::string& filename);
            ///< Play a soundifle
            /// \param filename name of a sound file in "Music/" in the data directory.

            void startRandomTitle();
            void MP3Lookup(boost::filesystem::path dir);

            bool isMusicPlaying();

            SoundImpl getMData();

            void say (MWWorld::Ptr reference, const std::string& filename);
            ///< Make an actor say some text.
            /// \param filename name of a sound file in "Sound/Vo/" in the data directory.

            bool sayDone (MWWorld::Ptr reference) const;
            ///< Is actor not speaking?



            void playSound (const std::string& soundId, float volume, float pitch);
            ///< Play a sound, independently of 3D-position

            void playSound3D (MWWorld::Ptr reference, const std::string& soundId,
                float volume, float pitch, bool loop);
            ///< Play a sound from an object

            void stopSound3D (MWWorld::Ptr reference, const std::string& soundId = "");
            ///< Stop the given object from playing the given sound, If no soundId is given,
            /// all sounds for this reference will stop.

            void stopSound (MWWorld::Ptr::CellStore *cell);
            ///< Stop all sounds for the given cell.

            bool getSoundPlaying (MWWorld::Ptr reference, const std::string& soundId) const;
            ///< Is the given sound currently playing on the given object?

            void updateObject(MWWorld::Ptr reference);
            ///< Update the position of all sounds connected to the given object.
    };
}

#endif
