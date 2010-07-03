#ifndef GAME_SOUND_SOUNDMANAGER_H
#define GAME_SOUND_SOUNDMANAGER_H

#include <string>
#include <map>

namespace Interpreter
{
    class Context;
}

namespace MWSound
{
    // Note to the sound implementor (can be removed once the implementation is complete):
    //
    // - the dummy implementation allows only one sound effect per object at a time. I am
    // not sure, if that is what Morrowind does. Beyond the dummy code in this class the script
    // system does not make any assumption about the number of sound effects.
    //
    // - the handle argument below is the ogre handle. Since we can assume, that all objects
    // that play a sound are in an active cell, they must have a handle. Sound script instructions
    // that reference objects not in an active cell will be taken care of long before the flow
    // of control reached this class.
    //
    // - all text-output (error messages and such) must be directed through the
    // context.messageBox interface.
    //
    // - the -> script syntax is not implemented yet ( script instructions of the type
    // npc_x -> say "file", "text"
    // aren't working)
    
    class SoundManager
    {
            std::map<std::string, std::string> mSounds; // object, sound (for testing only)
    
        public:
        
            void say (const std::string& handle, const std::string& filename,
                const std::string& text, Interpreter::Context& context);
            ///< Make an actor say some text.
            /// \param handle Handle as returned from render-subsystem
            /// \param filename name of a sound file in "Sound/Vo/" in the data directory.
            /// \param text Subtitle
            
            bool sayDone (const std::string& handle, Interpreter::Context& context) const;
            ///< Is actor not speaking?
            /// \param handle Handle as returned from render-subsystem

            void streamMusic (const std::string& filename, Interpreter::Context& context);
            ///< Play a soundifle
            /// \param filename name of a sound file in "Music/" in the data directory.
                       
            void playSound (const std::string& soundId, float volume, float pitch,
                Interpreter::Context& context);
            ///< Play a sound, independently of 3D-position
            
            void playSound3D (const std::string& handle, const std::string& soundId,
                float volume, float pitch, Interpreter::Context& context);
            ///< Play a sound from an object
            /// \param handle Handle as returned from render-subsystem

            void stopSound3D (const std::string& handle, const std::string& soundId,
                Interpreter::Context& context);
            ///< Stop the given object from playing the given sound.
            /// \param handle Handle as returned from render-subsystem

            bool getSoundPlaying (const std::string& handle, const std::string& soundId,
                Interpreter::Context& context) const;
            ///< Is the given sound currently playing on the given object?
            /// \param handle Handle as returned from render-subsystem
    };
}

#endif


