#ifndef COMPONENTS_NIFOGRE_CONTROLLER_H
#define COMPONENTS_NIFOGRE_CONTROLLER_H

#include <components/nif/niffile.hpp>
#include <OgreController.h>

namespace NifOgre
{

    class ValueInterpolator
    {
    protected:
        float interpKey(const Nif::FloatKeyList::VecType &keys, float time, float def=0.f) const
        {
            if (keys.size() == 0)
                return def;

            if(time <= keys.front().mTime)
                return keys.front().mValue;

            const Nif::FloatKey* keyArray = keys.data();
            size_t size = keys.size();

            for (size_t i = 1; i < size; ++i)
            {
                const Nif::FloatKey* aKey = &keyArray[i];

                if(aKey->mTime < time)
                    continue;

                const Nif::FloatKey* aLastKey = &keyArray[i-1];
                float a = (time - aLastKey->mTime) / (aKey->mTime - aLastKey->mTime);
                return aLastKey->mValue + ((aKey->mValue - aLastKey->mValue) * a);
            }

            return keys.back().mValue;
        }

        Ogre::Vector3 interpKey(const Nif::Vector3KeyList::VecType &keys, float time) const
        {
            if(time <= keys.front().mTime)
                return keys.front().mValue;

            const Nif::Vector3Key* keyArray = keys.data();
            size_t size = keys.size();

            for (size_t i = 1; i < size; ++i)
            {
                const Nif::Vector3Key* aKey = &keyArray[i];

                if(aKey->mTime < time)
                    continue;

                const Nif::Vector3Key* aLastKey = &keyArray[i-1];
                float a = (time - aLastKey->mTime) / (aKey->mTime - aLastKey->mTime);
                return aLastKey->mValue + ((aKey->mValue - aLastKey->mValue) * a);
            }

            return keys.back().mValue;
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
