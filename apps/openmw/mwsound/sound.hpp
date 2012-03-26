#ifndef GAME_SOUND_SOUND_H
#define GAME_SOUND_SOUND_H

namespace MWSound
{
    class Sound
    {
        virtual void stop() = 0;
        virtual bool isPlaying() = 0;
        virtual void update(const float *pos) = 0;

        Sound& operator=(const Sound &rhs);
        Sound(const Sound &rhs);

    public:
        Sound() { }
        virtual ~Sound() { }

        friend class OpenAL_Output;
        friend class SoundManager;
    };
}

#endif
