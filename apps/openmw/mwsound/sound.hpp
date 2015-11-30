#ifndef GAME_SOUND_SOUND_H
#define GAME_SOUND_SOUND_H

#include "soundmanagerimp.hpp"

namespace MWSound
{
    class Sound
    {
        Sound& operator=(const Sound &rhs);
        Sound(const Sound &rhs);

        osg::Vec3f mPos;
        float mVolume; /* NOTE: Real volume = mVolume*mBaseVolume */
        float mBaseVolume;
        float mPitch;
        float mMinDistance;
        float mMaxDistance;
        int mFlags;

        float mFadeOutTime;

    protected:
        void *mHandle;

        friend class Sound_Output;
        friend class OpenAL_Output;

    public:
        void setPosition(const osg::Vec3f &pos) { mPos = pos; }
        void setVolume(float volume) { mVolume = volume; }
        void setBaseVolume(float volume) { mBaseVolume = volume; }
        void setFadeout(float duration) { mFadeOutTime = duration; }
        void updateFade(float duration)
        {
            if(mFadeOutTime > 0.0f)
            {
                float soundDuration = std::min(duration, mFadeOutTime);
                mVolume *= (mFadeOutTime-soundDuration) / mFadeOutTime;
                mFadeOutTime -= soundDuration;
            }
        }

        MWBase::SoundManager::PlayType getPlayType() const
        { return (MWBase::SoundManager::PlayType)(mFlags&MWBase::SoundManager::Play_TypeMask); }
        bool getDistanceCull() const { return mFlags&MWBase::SoundManager::Play_RemoveAtDistance; }
        bool getIs3D() const { return mFlags&Play_3D; }

        Sound(const osg::Vec3f& pos, float vol, float basevol, float pitch, float mindist, float maxdist, int flags)
          : mPos(pos), mVolume(vol), mBaseVolume(basevol), mPitch(pitch)
          , mMinDistance(mindist), mMaxDistance(maxdist), mFlags(flags)
          , mFadeOutTime(0.0f), mHandle(0)
        { }
        ~Sound() { }
    };
}

#endif
