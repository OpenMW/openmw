#ifndef OPENMW_MWGUI_SCREENFADER_H
#define OPENMW_MWGUI_SCREENFADER_H

#include <deque>
#include <memory>

#include "windowbase.hpp"

namespace MWGui
{
    class ScreenFader;

    class FadeOp
    {
    public:
        typedef std::shared_ptr<FadeOp> Ptr;

        FadeOp(ScreenFader * fader, float time, float targetAlpha, float delay);

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
        float mDelay;
        bool mRunning;
    };

    class ScreenFader : public WindowBase
    {
    public:
        ScreenFader(const std::string & texturePath, const std::string& layout = "openmw_screen_fader.layout", const MyGUI::FloatCoord& texCoordOverride = MyGUI::FloatCoord(0,0,1,1));
        ~ScreenFader();

        void onFrameStart(float dt);

        void fadeIn(const float time, float delay=0);
        void fadeOut(const float time, float delay=0);
        void fadeTo(const int percent, const float time, float delay=0);

        void clear();

        void setFactor (float factor);
        void setRepeat(bool repeat);

        void queue(float time, float targetAlpha, float delay);
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
