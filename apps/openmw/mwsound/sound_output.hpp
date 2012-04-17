#ifndef GAME_SOUND_SOUND_OUTPUT_H
#define GAME_SOUND_SOUND_OUTPUT_H

#include <string>
#include <memory>

#include <OgreVector3.h>

#include "soundmanager.hpp"

#include "../mwworld/ptr.hpp"

namespace MWSound
{
    class SoundManager;
    class Sound_Decoder;
    class Sound;

    class Sound_Output
    {
        SoundManager &mManager;

        virtual std::vector<std::string> enumerate() = 0;
        virtual void init(const std::string &devname="") = 0;
        virtual void deinit() = 0;

        virtual SoundPtr playSound(const std::string &fname, float volume, float pitch, int flags) = 0;
        virtual SoundPtr playSound3D(const std::string &fname, const Ogre::Vector3 &pos,
                                     float volume, float pitch, float min, float max, int flags) = 0;
        virtual SoundPtr streamSound(const std::string &fname, float volume, float pitch, int flags) = 0;

        virtual void updateListener(const Ogre::Vector3 &pos, const Ogre::Vector3 &atdir, const Ogre::Vector3 &updir, Environment env) = 0;

        Sound_Output& operator=(const Sound_Output &rhs);
        Sound_Output(const Sound_Output &rhs);

    protected:
        bool mInitialized;
        Ogre::Vector3 mPos;

        Sound_Output(SoundManager &mgr)
          : mManager(mgr)
          , mInitialized(false)
          , mPos(0.0f, 0.0f, 0.0f)
        { }
    public:
        virtual ~Sound_Output() { }

        bool isInitialized() { return mInitialized; }

        friend class OpenAL_Output;
        friend class SoundManager;
    };
}

#endif
