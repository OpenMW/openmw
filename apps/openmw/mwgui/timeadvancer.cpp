#include "timeadvancer.hpp"

namespace MWGui
{
    TimeAdvancer::TimeAdvancer(float delay)
        : mRunning(false),
          mCurHour(0),
          mHours(1),
          mInterruptAt(-1),
          mDelay(delay),
          mRemainingTime(delay)
    {
    }

    void TimeAdvancer::run(int hours, int interruptAt)
    {
        mHours = hours;
        mCurHour = 0;
        mInterruptAt = interruptAt;
        mRemainingTime = mDelay;

        mRunning = true;
    }

    void TimeAdvancer::stop()
    {
        mRunning = false;
    }

    void TimeAdvancer::onFrame(float dt)
    {
        if (!mRunning)
            return;

        if (mCurHour == mInterruptAt)
        {
            stop();
            eventInterrupted();
            return;
        }

        mRemainingTime -= dt;

        while (mRemainingTime <= 0)
        {
            mRemainingTime += mDelay;
            ++mCurHour;

            if (mCurHour <= mHours)
                eventProgressChanged(mCurHour, mHours);
            else
            {
                stop();
                eventFinished();
                return;
            }

        }
    }

    int TimeAdvancer::getHours()
    {
        return mHours;
    }

    bool TimeAdvancer::isRunning()
    {
        return mRunning;
    }
}
