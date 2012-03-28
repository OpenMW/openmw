#ifndef GAME_SOUND_SOUND_H
#define GAME_SOUND_SOUND_H

namespace MWSound
{
    class Sound
    {
        virtual void stop() = 0;
        virtual bool isPlaying() = 0;
        virtual void setVolume(float volume) = 0;
        virtual void update(const float *pos) = 0;

        Sound& operator=(const Sound &rhs);
        Sound(const Sound &rhs);

    protected:
        float mVolume; /* NOTE: Real volume = mVolume*mBaseVolume */
        float mBaseVolume;
        float mMinDistance;
        float mMaxDistance;

    public:
        Sound() : mVolume(1.0f)
                , mBaseVolume(1.0f)
                , mMinDistance(20.0f) /* 1 * min_range_scale */
                , mMaxDistance(12750.0f) /* 255 * max_range_scale */
        { }
        virtual ~Sound() { }

        friend class OpenAL_Output;
        friend class SoundManager;
    };
}

#endif
