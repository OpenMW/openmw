#ifndef GAME_SOUND_SOUND_H
#define GAME_SOUND_SOUND_H

#include <string>

namespace MWSound
{
    class Sound
    {
        virtual bool Play() = 0;
        virtual void Stop() = 0;
        virtual bool isPlaying() = 0;

    public:
        virtual ~Sound() { }

        friend class OpenAL_Output;
        friend class SoundManager;
    };
}

#endif
