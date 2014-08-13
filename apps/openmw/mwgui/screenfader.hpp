#ifndef OPENMW_MWGUI_SCREENFADER_H
#define OPENMW_MWGUI_SCREENFADER_H

#include "windowbase.hpp"

namespace MWGui
{

    class ScreenFader : public WindowBase
    {
    public:
        ScreenFader();

        void update(float dt);

        void fadeIn(const float time);
        void fadeOut(const float time);
        void fadeTo(const int percent, const float time);

        void setFactor (float factor);

    private:
        enum FadingMode
        {
            FadingMode_In,
            FadingMode_Out
        };

        void applyAlpha();

        FadingMode mMode;

        float mRemainingTime;
        float mTargetTime;
        float mTargetAlpha;
        float mCurrentAlpha;
        float mStartAlpha;

        float mFactor;
    };

}

#endif
