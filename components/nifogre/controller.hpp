#ifndef COMPONENTS_NIFOGRE_CONTROLLER_H
#define COMPONENTS_NIFOGRE_CONTROLLER_H

#include <components/nif/niffile.hpp>
#include <components/nif/nifkey.hpp>
#include <OgreController.h>

namespace NifOgre
{

    class ValueInterpolator
    {
    protected:
        float interpKey(const Nif::FloatKeyMap::MapType &keys, float time, float def=0.f) const
        {
            if (keys.size() == 0)
                return def;

            if(time <= keys.begin()->first)
                return keys.begin()->second.mValue;

            Nif::FloatKeyMap::MapType::const_iterator it = keys.lower_bound(time);
            if (it != keys.end())
            {
                float aTime = it->first;
                const Nif::FloatKey* aKey = &it->second;

                assert (it != keys.begin()); // Shouldn't happen, was checked at beginning of this function

                Nif::FloatKeyMap::MapType::const_iterator last = --it;
                float aLastTime = last->first;
                const Nif::FloatKey* aLastKey = &last->second;

                float a = (time - aLastTime) / (aTime - aLastTime);
                return aLastKey->mValue + ((aKey->mValue - aLastKey->mValue) * a);
            }
            else
                return keys.rbegin()->second.mValue;
        }

        Ogre::Vector3 interpKey(const Nif::Vector3KeyMap::MapType &keys, float time) const
        {
            if(time <= keys.begin()->first)
                return keys.begin()->second.mValue;

            Nif::Vector3KeyMap::MapType::const_iterator it = keys.lower_bound(time);
            if (it != keys.end())
            {
                float aTime = it->first;
                const Nif::Vector3Key* aKey = &it->second;

                assert (it != keys.begin()); // Shouldn't happen, was checked at beginning of this function

                Nif::Vector3KeyMap::MapType::const_iterator last = --it;
                float aLastTime = last->first;
                const Nif::Vector3Key* aLastKey = &last->second;

                float a = (time - aLastTime) / (aTime - aLastTime);
                return aLastKey->mValue + ((aKey->mValue - aLastKey->mValue) * a);
            }
            else
                return keys.rbegin()->second.mValue;
        }
    };

    // FIXME: Should not be here.
    class DefaultFunction : public Ogre::ControllerFunction<Ogre::Real>
    {
    private:
        float mFrequency;
        float mPhase;
        float mStartTime;
    public:
        float mStopTime;

    public:
        DefaultFunction(const Nif::Controller *ctrl, bool deltaInput)
            : Ogre::ControllerFunction<Ogre::Real>(deltaInput)
            , mFrequency(ctrl->frequency)
            , mPhase(ctrl->phase)
            , mStartTime(ctrl->timeStart)
            , mStopTime(ctrl->timeStop)
        {
            if(mDeltaInput)
                mDeltaCount = mPhase;
        }

        virtual Ogre::Real calculate(Ogre::Real value)
        {
            if(mDeltaInput)
            {
                if (mStopTime - mStartTime == 0.f)
                    return 0.f;

                mDeltaCount += value*mFrequency;
                if(mDeltaCount < mStartTime)
                    mDeltaCount = mStopTime - std::fmod(mStartTime - mDeltaCount,
                                                        mStopTime - mStartTime);
                mDeltaCount = std::fmod(mDeltaCount - mStartTime,
                                        mStopTime - mStartTime) + mStartTime;
                return mDeltaCount;
            }

            value = std::min(mStopTime, std::max(mStartTime, value+mPhase));
            return value;
        }
    };

}

#endif
