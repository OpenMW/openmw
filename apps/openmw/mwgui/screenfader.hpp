#ifndef OPENMW_MWGUI_SCREENFADER_H
#define OPENMW_MWGUI_SCREENFADER_H

#include <deque>

#include <boost/shared_ptr.hpp>

#include "windowbase.hpp"

namespace MWGui
{
    class ScreenFader;

    class FadeOp
    {
    public:
        typedef boost::shared_ptr<FadeOp> Ptr;

        FadeOp(ScreenFader * fader, float time, float targetAlpha);

        bool isRunning();

        void start();
        void update(float dt);
        void finish();

    private:
        ScreenFader * mFader;
        float mRemainingTime;
        float mTargetTime;
        float mTargetAlpha;
        float mStartAlpha;
        bool mRunning;
    };

    class ScreenFader : public WindowBase
    {
    public:
        ScreenFader(const std::string & texturePath);

        void setTexture(const std::string & texturePath);

        void update(float dt);

        void fadeIn(const float time);
        void fadeOut(const float time);
        void fadeTo(const int percent, const float time);

        void setFactor (float factor);
        void setRepeat(bool repeat);

        void queue(float time, float targetAlpha);
        bool isEmpty();
        void clearQueue();

        void notifyAlphaChanged(float alpha);
        void notifyOperationFinished();
        float getCurrentAlpha();

    private:
        void applyAlpha();

        float mCurrentAlpha;
        float mFactor;

        bool mRepeat; // repeat queued operations without removing them
        std::deque<FadeOp::Ptr> mQueue;
    };
}

#endif
