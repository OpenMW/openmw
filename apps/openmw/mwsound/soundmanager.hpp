#ifndef GAME_SOUND_SOUNDMANAGER_H
#define GAME_SOUND_SOUNDMANAGER_H

#include <string>

#include <boost/filesystem.hpp>
#include <boost/timer.hpp>

#include <mangle/sound/clients/ogre_output_updater.hpp>
#include <mangle/sound/clients/ogre_listener_mover.hpp>

#include <openengine/sound/sndmanager.hpp>
#include <components/files/multidircollection.hpp>
#include <components/file_finder/file_finder.hpp>

#include "../mwworld/ptr.hpp"


namespace Ogre
{
    class Root;
    class Camera;
}

namespace Mangle
{
    namespace Sound
    {
        typedef boost::shared_ptr<Sound> SoundPtr;
        //struct OgreOutputUpdater;
    }
}

typedef OEngine::Sound::SoundManagerPtr OEManagerPtr;

namespace ESMS
{
    struct ESMStore;
}

namespace MWWorld
{
    struct Environment;
}

namespace MWSound
{
    class SoundManager
    {
            Files::PathContainer files;
            bool fsStrict;
            MWWorld::Environment& mEnvironment;

            int total;
            ESM::Region test;
            boost::timer timer;

            void streamMusicFull (const std::string& filename);
            ///< Play a soundifle
            /// \param absolute filename

            /* This is the sound manager. It loades, stores and deletes
            sounds based on the sound factory it is given.
            */
            OEManagerPtr mgr;
            Mangle::Sound::SoundPtr music;

            /* This class calls update() on the sound manager each frame
               using and Ogre::FrameListener
            */
            Mangle::Sound::OgreOutputUpdater updater;

            /* This class tracks the movement of an Ogre::Camera and moves
               a sound listener automatically to follow it.
            */
            Mangle::Sound::OgreListenerMover cameraTracker;

            typedef std::map<std::string,Mangle::Sound::WSoundPtr> IDMap;
            typedef std::map<MWWorld::Ptr,IDMap> PtrMap;
            PtrMap sounds;

            // This is used for case insensitive and slash-type agnostic file
            // finding. It takes DOS paths (any case, \\ slashes or / slashes)
            // relative to the sound dir, and translates them into full paths
            // of existing files in the filesystem, if they exist.
            bool FSstrict;
            FileFinder::LessTreeFileFinder soundfiles;
            FileFinder::StrictTreeFileFinder strict;
            FileFinder::LessTreeFileFinder musicpath;
            FileFinder::StrictTreeFileFinder musicpathStrict;

            static std::string toMp3(std::string str);
            bool hasFile(const std::string &str, bool music = false);
            std::string convertPath(const std::string &str, bool music = false);
            std::string lookup(const std::string &soundId,
                       float &volume, float &min, float &max);
            void add(const std::string &file,
                MWWorld::Ptr ptr, const std::string &id,
                float volume, float pitch, float min, float max,
                bool loop);
            void clearAll(PtrMap::iterator& it);
            void remove(MWWorld::Ptr ptr, const std::string &id = "");
            bool isPlaying(MWWorld::Ptr ptr, const std::string &id) const;
            void removeCell(const MWWorld::Ptr::CellStore *cell);
            void updatePositions(MWWorld::Ptr ptr);

        public:

            SoundManager(Ogre::Root*, Ogre::Camera*,
                   const Files::PathContainer& dataDir, bool useSound, bool fsstrict,
                   MWWorld::Environment& environment);
            ~SoundManager();

            void streamMusic(const std::string& filename);
            ///< Play a soundifle
            /// \param filename name of a sound file in "Music/" in the data directory.

            void startRandomTitle();
            void MP3Lookup(const boost::filesystem::path& dir);

            bool isMusicPlaying();

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

            void update (float duration);
    };
}

#endif
