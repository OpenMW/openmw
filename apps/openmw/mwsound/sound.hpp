#ifndef GAME_SOUND_SOUND_H
#define GAME_SOUND_SOUND_H

#include <string>

#include "../mwworld/ptr.hpp"

namespace MWSound
{
    class Sound
    {
        virtual void Stop() = 0;
        virtual bool isPlaying() = 0;
        virtual void Update(const float *pos) = 0;

    public:
        virtual ~Sound() { }

        friend class OpenAL_Output;
        friend class SoundManager;
    };
}

#endif
