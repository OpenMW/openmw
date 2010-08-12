#ifndef GAME_SOUND_SOUNDMANAGER_H
#define GAME_SOUND_SOUNDMANAGER_H

#include <string>
#include <map>

#include "../mwworld/ptr.hpp"

namespace Interpreter
{
    class Context;
}

namespace Ogre
{
    class Root;
    class Camera;
}

namespace MWSound
{
    // Note: the -> script syntax is not implemented yet ( script
    // instructions of the type npc_x -> say "file", "text" aren't
    // working)
    
    class SoundManager
    {
            // Hide implementation details - engine.cpp is compiling
            // enough as it is.
            struct SoundImpl;
            SoundImpl *mData;
    
        public:
            SoundManager(Ogre::Root*, Ogre::Camera*);
            ~SoundManager();
        
            void say (MWWorld::Ptr reference, const std::string& filename,
                const std::string& text, Interpreter::Context& context);
            ///< Make an actor say some text.
            /// \param filename name of a sound file in "Sound/Vo/" in the data directory.
            /// \param text Subtitle
            
            bool sayDone (MWWorld::Ptr reference, Interpreter::Context& context) const;
            ///< Is actor not speaking?

            void streamMusic (const std::string& filename, Interpreter::Context& context);
            ///< Play a soundifle
            /// \param filename name of a sound file in "Music/" in the data directory.
                       
            void playSound (const std::string& soundId, float volume, float pitch,
                Interpreter::Context& context);
            ///< Play a sound, independently of 3D-position
            
            void playSound3D (MWWorld::Ptr reference, const std::string& soundId,
                float volume, float pitch, bool loop, Interpreter::Context& context);
            ///< Play a sound from an object

            void stopSound3D (MWWorld::Ptr reference, const std::string& soundId,
                Interpreter::Context& context);
            ///< Stop the given object from playing the given sound.

            bool getSoundPlaying (MWWorld::Ptr reference, const std::string& soundId,
                Interpreter::Context& context) const;
            ///< Is the given sound currently playing on the given object?
    };
}

#endif


