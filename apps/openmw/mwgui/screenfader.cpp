#include "screenfader.hpp"

namespace MWGui
{

    ScreenFader::ScreenFader()
        : WindowBase("openmw_screen_fader.layout")
        , mMode(FadingMode_In)
        , mRemainingTime(0.f)
        , mTargetTime(0.f)
        , mTargetAlpha(0.f)
        , mCurrentAlpha(0.f)
        , mStartAlpha(0.f)
        , mFactor(1.f)
    {
        mMainWidget->setSize(MyGUI::RenderManager::getInstance().getViewSize());

        setVisible(false);
    }

    void ScreenFader::update(float dt)
    {
        if (mRemainingTime > 0)
        {
            if (mMode == FadingMode_In)
            {
                mCurrentAlpha -= dt/mTargetTime * (mStartAlpha-mTargetAlpha);
                if (mCurrentAlpha < mTargetAlpha) mCurrentAlpha = mTargetAlpha;
            }
            else if (mMode == FadingMode_Out)
            {
                mCurrentAlpha += dt/mTargetTime * (mTargetAlpha-mStartAlpha);
                if (mCurrentAlpha > mTargetAlpha) mCurrentAlpha = mTargetAlpha;
            }

            mRemainingTime -= dt;
        }

        if (1.f-((1.f-mCurrentAlpha) * mFactor) == 0.f)
            mMainWidget->setVisible(false);
        else
            applyAlpha();
    }

    void ScreenFader::applyAlpha()
    {
        setVisible(true);
        mMainWidget->setAlpha(1.f-((1.f-mCurrentAlpha) * mFactor));
    }

    void ScreenFader::fadeIn(float time)
    {
        if (time<0.f) return;
        if (time==0.f)
        {
            mCurrentAlpha = 0.f;
            applyAlpha();
            return;
        }

        mStartAlpha = mCurrentAlpha;
        mTargetAlpha = 0.f;
        mMode = FadingMode_In;
        mTargetTime = time;
        mRemainingTime = time;
    }

    void ScreenFader::fadeOut(const float time)
    {
        if (time<0.f) return;
        if (time==0.f)
        {
            mCurrentAlpha = 1.f;
            applyAlpha();
            return;
        }

        mStartAlpha = mCurrentAlpha;
        mTargetAlpha = 1.f;
        mMode = FadingMode_Out;
        mTargetTime = time;
        mRemainingTime = time;
    }

    void ScreenFader::fadeTo(const int percent, const float time)
    {
        if (time<0.f) return;
        if (time==0.f)
        {
            mCurrentAlpha = percent/100.f;
            applyAlpha();
            return;
        }

        mStartAlpha = mCurrentAlpha;
        mTargetAlpha = percent/100.f;

        if (mTargetAlpha == mStartAlpha) return;
        else if (mTargetAlpha > mStartAlpha) mMode = FadingMode_Out;
        else mMode = FadingMode_In;

        mTargetTime = time;
        mRemainingTime = time;
    }

    void ScreenFader::setFactor(float factor)
    {
        mFactor = factor;
    }

}
