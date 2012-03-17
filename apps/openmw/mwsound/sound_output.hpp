#ifndef GAME_SOUND_SOUND_OUTPUT_H
#define GAME_SOUND_SOUND_OUTPUT_H

#include <string>

namespace MWSound
{
    class SoundManager;

    class Sound_Output
    {
        SoundManager &mgr;

        virtual bool Initialize(const std::string &devname="") = 0;
        virtual void Deinitialize() = 0;

        Sound_Output(SoundManager &mgr) : mgr(mgr) { }
    public:
        virtual ~Sound_Output() { }

        friend class OpenAL_Output;
        friend class SoundManager;
    };
}

#endif
