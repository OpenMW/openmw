#ifndef GAME_SOUND_SOUND_OUTPUT_H
#define GAME_SOUND_SOUND_OUTPUT_H

#include <string>
#include <memory>

namespace MWSound
{
    class SoundManager;
    class Sound_Decoder;
    class Sound;

    class Sound_Output
    {
        SoundManager &mgr;

        virtual bool Initialize(const std::string &devname="") = 0;
        virtual void Deinitialize() = 0;

        virtual Sound *StreamSound(const std::string &fname, std::auto_ptr<Sound_Decoder> decoder) = 0;

        // FIXME: This should take an MWWorld::Ptr that represents the in-world camera
        virtual void UpdateListener(float pos[3], float atdir[3], float updir[3]) = 0;

        Sound_Output(SoundManager &mgr) : mgr(mgr) { }
    public:
        virtual ~Sound_Output() { }

        friend class OpenAL_Output;
        friend class SoundManager;
    };
}

#endif
