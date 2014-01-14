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

            Nif::FloatKeyList::VecType::const_iterator iter(keys.begin()+1);
            for(;iter != keys.end();iter++)
            {
                if(iter->mTime < time)
                    continue;

                Nif::FloatKeyList::VecType::const_iterator last(iter-1);
                float a = (time-last->mTime) / (iter->mTime-last->mTime);
                return last->mValue + ((iter->mValue - last->mValue)*a);
            }
            return keys.back().mValue;
        }

        Ogre::Vector3 interpKey(const Nif::Vector3KeyList::VecType &keys, float time) const
        {
            if(time <= keys.front().mTime)
                return keys.front().mValue;

            Nif::Vector3KeyList::VecType::const_iterator iter(keys.begin()+1);
            for(;iter != keys.end();iter++)
            {
                if(iter->mTime < time)
                    continue;

                Nif::Vector3KeyList::VecType::const_iterator last(iter-1);
                float a = (time-last->mTime) / (iter->mTime-last->mTime);
                return last->mValue + ((iter->mValue - last->mValue)*a);
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
