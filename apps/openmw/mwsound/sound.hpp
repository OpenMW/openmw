#ifndef GAME_SOUND_SOUND_H
#define GAME_SOUND_SOUND_H

#include <algorithm>

#include "sound_output.hpp"

namespace MWSound
{
    class SoundBase {
        SoundBase& operator=(const SoundBase&) = delete;
        SoundBase(const SoundBase&) = delete;
        SoundBase(SoundBase&&) = delete;

        osg::Vec3f mPos;
        float mVolume; /* NOTE: Real volume = mVolume*mBaseVolume */
        float mBaseVolume;
        float mPitch;
        float mMinDistance;
        float mMaxDistance;
        int mFlags;

        float mFadeOutTime;

    protected:
        Sound_Instance mHandle;

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

        const osg::Vec3f &getPosition() const { return mPos; }
        float getRealVolume() const { return mVolume * mBaseVolume; }
        float getPitch() const { return mPitch; }
        float getMinDistance() const { return mMinDistance; }
        float getMaxDistance() const { return mMaxDistance; }

        MWBase::SoundManager::PlayType getPlayType() const
        { return (MWBase::SoundManager::PlayType)(mFlags&MWBase::SoundManager::Play_TypeMask); }
        bool getUseEnv() const { return !(mFlags&MWBase::SoundManager::Play_NoEnv); }
        bool getIsLooping() const { return mFlags&MWBase::SoundManager::Play_Loop; }
        bool getDistanceCull() const { return mFlags&MWBase::SoundManager::Play_RemoveAtDistance; }
        bool getIs3D() const { return mFlags&Play_3D; }

        void init(const osg::Vec3f& pos, float vol, float basevol, float pitch, float mindist, float maxdist, int flags)
        {
            mPos = pos;
            mVolume = vol;
            mBaseVolume = basevol;
            mPitch = pitch;
            mMinDistance = mindist;
            mMaxDistance = maxdist;
            mFlags = flags;
            mFadeOutTime = 0.0f;
            mHandle = nullptr;
        }

        void init(float vol, float basevol, float pitch, int flags)
        {
            mPos = osg::Vec3f(0.0f, 0.0f, 0.0f);
            mVolume = vol;
            mBaseVolume = basevol;
            mPitch = pitch;
            mMinDistance = 1.0f;
            mMaxDistance = 1000.0f;
            mFlags = flags;
            mFadeOutTime = 0.0f;
            mHandle = nullptr;
        }

        SoundBase()
          : mPos(0.0f, 0.0f, 0.0f), mVolume(1.0f), mBaseVolume(1.0f), mPitch(1.0f)
          , mMinDistance(1.0f), mMaxDistance(1000.0f), mFlags(0), mFadeOutTime(0.0f)
          , mHandle(nullptr)
        { }
    };

    class Sound : public SoundBase {
        Sound& operator=(const Sound&) = delete;
        Sound(const Sound&) = delete;
        Sound(Sound&&) = delete;

    public:
        Sound() { }
    };

    class Stream : public SoundBase {
        Stream& operator=(const Stream&) = delete;
        Stream(const Stream&) = delete;
        Stream(Stream&&) = delete;

    public:
        Stream() { }
    };
}

#endif
