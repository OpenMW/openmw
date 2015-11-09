#include "screenfader.hpp"

#include <MyGUI_RenderManager.h>

namespace MWGui
{

    FadeOp::FadeOp(ScreenFader * fader, float time, float targetAlpha)
        : mFader(fader),
          mRemainingTime(time),
          mTargetTime(time),
          mTargetAlpha(targetAlpha),
          mStartAlpha(0.f),
          mRunning(false)
    {
    }

    bool FadeOp::isRunning()
    {
        return mRunning;
    }

    void FadeOp::start()
    {
        if (mRunning)
            return;

        mRemainingTime = mTargetTime;
        mStartAlpha = mFader->getCurrentAlpha();
        mRunning = true;
    }

    void FadeOp::update(float dt)
    {
        if (!mRunning)
            return;

        if (mRemainingTime <= 0 || mStartAlpha == mTargetAlpha)
        {
            finish();
            return;
        }

        float currentAlpha = mFader->getCurrentAlpha();
        if (mStartAlpha > mTargetAlpha)
        {
            currentAlpha -= dt/mTargetTime * (mStartAlpha-mTargetAlpha);
            if (currentAlpha < mTargetAlpha)
                currentAlpha = mTargetAlpha;
        }
        else
        {
            currentAlpha += dt/mTargetTime * (mTargetAlpha-mStartAlpha);
            if (currentAlpha > mTargetAlpha)
                currentAlpha = mTargetAlpha;
        }

        mFader->notifyAlphaChanged(currentAlpha);

        mRemainingTime -= dt;
    }

    void FadeOp::finish()
    {
        mRunning = false;
        mFader->notifyOperationFinished();
    }

    ScreenFader::ScreenFader(const std::string & texturePath, const std::string& layout)
        : WindowBase(layout)
        , mCurrentAlpha(0.f)
        , mFactor(1.f)
        , mRepeat(false)
    {
        mMainWidget->setSize(MyGUI::RenderManager::getInstance().getViewSize());
        setTexture(texturePath);
        setVisible(false);
    }

    void ScreenFader::setTexture(const std::string & texturePath)
    {
        mMainWidget->setProperty("ImageTexture", texturePath);
    }

    void ScreenFader::update(float dt)
    {
        if (!mQueue.empty())
        {
            if (!mQueue.front()->isRunning())
                mQueue.front()->start();
            mQueue.front()->update(dt);
        }
    }

    void ScreenFader::applyAlpha()
    {
        setVisible(true);
        mMainWidget->setAlpha(1.f-((1.f-mCurrentAlpha) * mFactor));
    }

    void ScreenFader::fadeIn(float time)
    {
        queue(time, 1.f);
    }

    void ScreenFader::fadeOut(const float time)
    {
        queue(time, 0.f);
    }

    void ScreenFader::fadeTo(const int percent, const float time)
    {
        queue(time, percent/100.f);
    }

    void ScreenFader::setFactor(float factor)
    {
        mFactor = factor;
        applyAlpha();
    }

    void ScreenFader::setRepeat(bool repeat)
    {
        mRepeat = repeat;
    }

    void ScreenFader::queue(float time, float targetAlpha)
    {
        if (time < 0.f)
            return;

        if (time == 0.f)
        {
            mCurrentAlpha = targetAlpha;
            applyAlpha();
            return;
        }

        mQueue.push_back(FadeOp::Ptr(new FadeOp(this, time, targetAlpha)));
    }

    bool ScreenFader::isEmpty()
    {
        return mQueue.empty();
    }

    void ScreenFader::clearQueue()
    {
        mQueue.clear();
    }

    void ScreenFader::notifyAlphaChanged(float alpha)
    {
        if (mCurrentAlpha == alpha)
            return;

        mCurrentAlpha = alpha;

        if (1.f-((1.f-mCurrentAlpha) * mFactor) == 0.f)
            mMainWidget->setVisible(false);
        else
            applyAlpha();
    }

    void ScreenFader::notifyOperationFinished()
    {
        FadeOp::Ptr op = mQueue.front();
        mQueue.pop_front();

        if (mRepeat)
            mQueue.push_back(op);
    }

    float ScreenFader::getCurrentAlpha()
    {
        return mCurrentAlpha;
    }
}
