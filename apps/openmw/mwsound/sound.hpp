#ifndef GAME_SOUND_SOUND_H
#define GAME_SOUND_SOUND_H

#include <OgreVector3.h>

#include "soundmanagerimp.hpp"

namespace MWSound
{
    class Sound
    {
        virtual void update() = 0;

        Sound& operator=(const Sound &rhs);
        Sound(const Sound &rhs);

    protected:
        Ogre::Vector3 mPos;
        float mVolume; /* NOTE: Real volume = mVolume*mBaseVolume */
        float mBaseVolume;
        float mPitch;
        float mMinDistance;
        float mMaxDistance;
        int mFlags;
        float mFadeOutTime;

        std::vector<float> mLoudnessVector;
        float mLoudnessFPS;

    public:
        virtual void stop() = 0;
        virtual bool isPlaying() = 0;
        virtual double getTimeOffset() = 0;
        void setPosition(const Ogre::Vector3 &pos) { mPos = pos; }
        void setVolume(float volume) { mVolume = volume; }
        void setFadeout(float duration) { mFadeOutTime=duration; }
        void setLoudnessVector(const std::vector<float>& loudnessVector, float loudnessFPS);

        /// Get loudness at the current time position on a [0,1] scale.
        /// Requires that loudnessVector was filled in by the user.
        float getCurrentLoudness();

        MWBase::SoundManager::PlayType getPlayType() const
        { return (MWBase::SoundManager::PlayType)(mFlags&MWBase::SoundManager::Play_TypeMask); }


        Sound(const Ogre::Vector3& pos, float vol, float basevol, float pitch, float mindist, float maxdist, int flags)
          : mPos(pos)
          , mVolume(vol)
          , mBaseVolume(basevol)
          , mPitch(pitch)
          , mMinDistance(mindist)
          , mMaxDistance(maxdist)
          , mFlags(flags)
          , mFadeOutTime(0)
          , mLoudnessFPS(20)
        { }
        virtual ~Sound() { }

        friend class OpenAL_Output;
        friend class SoundManager;
    };
}

#endif
