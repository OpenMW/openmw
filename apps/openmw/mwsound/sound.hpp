#ifndef GAME_SOUND_SOUND_H
#define GAME_SOUND_SOUND_H

#include <algorithm>

#include "soundoutput.hpp"

namespace MWSound
{
    // Extra play flags, not intended for caller use
    enum PlayModeEx
    {
        Play_2D = 0,
        Play_StopAtFadeEnd = 1 << 28,
        Play_FadeExponential = 1 << 29,
        Play_InFade = 1 << 30,
        Play_3D = 1 << 31,
        Play_FadeFlagsMask = (Play_StopAtFadeEnd | Play_FadeExponential),
    };

    // For testing individual PlayMode flags
    inline int operator&(int a, PlayMode b)
    {
        return a & static_cast<int>(b);
    }
    inline int operator&(PlayMode a, PlayMode b)
    {
        return static_cast<int>(a) & static_cast<int>(b);
    }

    struct SoundParams
    {
        osg::Vec3f mPos;
        osg::Vec3f mLastPos;
        osg::Vec3f mVel;
        float mVolume = 1.0f;
        float mBaseVolume = 1.0f;
        float mPitch = 1.0f;
        float mMinDistance = 1.0f;
        float mMaxDistance = 1000.0f;
        int mFlags = 0;
        float mFadeVolume = 1.0f;
        float mFadeTarget = 0.0f;
        float mFadeStep = 0.0f;
    };

    class SoundBase
    {
        SoundBase& operator=(const SoundBase&) = delete;
        SoundBase(const SoundBase&) = delete;
        SoundBase(SoundBase&&) = delete;

        SoundParams mParams;

    protected:
        Sound_Instance mHandle = nullptr;

        friend class OpenALOutput;

    public:
        void setPosition(const osg::Vec3f& pos) { mParams.mPos = pos; }
        void setLastPosition(const osg::Vec3f& lastpos) { mParams.mLastPos = lastpos; }
        void setVelocity(const osg::Vec3f& vel) { mParams.mVel = vel; }
        void setVolume(float volume) { mParams.mVolume = volume; }
        void setBaseVolume(float volume) { mParams.mBaseVolume = volume; }
        void setFadeout(float duration) { setFade(duration, 0.0, Play_StopAtFadeEnd); }

        /// Fade to the given linear gain within the specified amount of time.
        /// Note that the fade gain is independent of the sound volume.
        ///
        /// \param duration specifies the duration of the fade. For *linear*
        /// fades (default) this will be exactly the time at which the desired
        /// volume is reached. Let v0 be the initial volume, v1 be the target
        /// volume, and t0 be the initial time. Then the volume over time is
        /// given as
        ///
        ///   v(t) =  v0 + (v1 - v0) * (t - t0) / duration  if t <= t0 + duration
        ///   v(t) =  v1                                    if t >  t0 + duration
        ///
        /// For *exponential* fades this determines the time-constant of the
        /// exponential process describing the fade. In particular, we guarantee
        /// that we reach v0 + 0.99 * (v1 - v0) within the given duration.
        ///
        ///   v(t) = v1 + (v0 - v1) * exp(-4.6 * (t0 - t) / duration)
        ///
        /// where -4.6 is approximately log(1%) (i.e., -40 dB).
        ///
        /// This interpolation mode is meant for environmental sound effects to
        /// achieve less jarring transitions.
        ///
        /// \param targetVolume is the linear gain that should be reached at
        /// the end of the fade.
        ///
        /// \param flags may be a combination of Play_FadeExponential and
        /// Play_StopAtFadeEnd. If Play_StopAtFadeEnd is set, stops the sound
        /// once the fade duration has passed or the target volume has been
        /// reached. If Play_FadeExponential is set, enables the exponential
        /// fade mode (see above).
        void setFade(float duration, float targetVolume, int flags = 0)
        {
            // Approximation of log(1%) (i.e., -40 dB).
            constexpr float minus40Decibel = -4.6f;

            // Do nothing if already at the target, unless we need to trigger a stop event
            if ((mParams.mFadeVolume == targetVolume) && !(flags & Play_StopAtFadeEnd))
                return;

            mParams.mFadeTarget = targetVolume;
            mParams.mFlags = (mParams.mFlags & ~Play_FadeFlagsMask) | (flags & Play_FadeFlagsMask) | Play_InFade;
            if (duration > 0.0f)
            {
                if (mParams.mFlags & Play_FadeExponential)
                    mParams.mFadeStep = -minus40Decibel / duration;
                else
                    mParams.mFadeStep = (mParams.mFadeTarget - mParams.mFadeVolume) / duration;
            }
            else
            {
                mParams.mFadeVolume = mParams.mFadeTarget;
                mParams.mFadeStep = 0.0f;
            }
        }

        /// Updates the internal fading logic.
        ///
        /// \param dt is the time in seconds since the last call to update.
        ///
        /// \return true if the sound is still active, false if the sound has
        /// reached a fading destination that was marked with Play_StopAtFadeEnd.
        bool updateFade(float dt)
        {
            // Mark fade as done at this volume difference (-80dB when fading to zero)
            constexpr float minVolumeDifference = 1e-4f;

            if (!getInFade())
                return true;

            // Perform the actual fade operation
            const float deltaBefore = mParams.mFadeTarget - mParams.mFadeVolume;
            if (mParams.mFlags & Play_FadeExponential)
                mParams.mFadeVolume += mParams.mFadeStep * deltaBefore * dt;
            else
                mParams.mFadeVolume += mParams.mFadeStep * dt;
            const float deltaAfter = mParams.mFadeTarget - mParams.mFadeVolume;

            // Abort fade if we overshot or reached the minimum difference
            if ((std::signbit(deltaBefore) != std::signbit(deltaAfter)) || (std::abs(deltaAfter) < minVolumeDifference))
            {
                mParams.mFadeVolume = mParams.mFadeTarget;
                mParams.mFlags &= ~Play_InFade;
            }

            return getInFade() || !(mParams.mFlags & Play_StopAtFadeEnd);
        }

        const osg::Vec3f& getPosition() const { return mParams.mPos; }
        const osg::Vec3f& getLastPosition() const { return mParams.mLastPos; }
        const osg::Vec3f& getVelocity() const {return mParams.mVel; }
        float getRealVolume() const { return mParams.mVolume * mParams.mBaseVolume * mParams.mFadeVolume; }
        float getPitch() const { return mParams.mPitch; }
        float getMinDistance() const { return mParams.mMinDistance; }
        float getMaxDistance() const { return mParams.mMaxDistance; }

        MWSound::Type getPlayType() const { return static_cast<MWSound::Type>(mParams.mFlags & MWSound::Type::Mask); }
        bool getUseEnv() const { return !(mParams.mFlags & MWSound::PlayMode::NoEnv); }
        bool getIsLooping() const { return mParams.mFlags & MWSound::PlayMode::Loop; }
        bool getDistanceCull() const { return mParams.mFlags & MWSound::PlayMode::RemoveAtDistance; }
        bool getIs3D() const { return mParams.mFlags & Play_3D; }
        bool getInFade() const { return mParams.mFlags & Play_InFade; }

        void init(const SoundParams& params)
        {
            mParams = params;
            mHandle = nullptr;
        }

        SoundBase() = default;
    };

    class Sound : public SoundBase
    {
        Sound& operator=(const Sound&) = delete;
        Sound(const Sound&) = delete;
        Sound(Sound&&) = delete;

    public:
        Sound() = default;
    };

    class Stream : public SoundBase
    {
        Stream& operator=(const Stream&) = delete;
        Stream(const Stream&) = delete;
        Stream(Stream&&) = delete;

    public:
        Stream() = default;
    };
}

#endif
