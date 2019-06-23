#include "screenfader.hpp"

#include <MyGUI_RenderManager.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_Gui.h>

namespace MWGui
{

    FadeOp::FadeOp(ScreenFader * fader, float time, float targetAlpha, float delay)
        : mFader(fader),
          mRemainingTime(time+delay),
          mTargetTime(time),
          mTargetAlpha(targetAlpha),
          mStartAlpha(0.f),
          mDelay(delay),
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

        mRemainingTime = mTargetTime + mDelay;
        mStartAlpha = mFader->getCurrentAlpha();
        mRunning = true;
    }

    void FadeOp::update(float dt)
    {
        if (!mRunning)
            return;

        if (mStartAlpha == mTargetAlpha)
        {
            finish();
            return;
        }

        if (mRemainingTime <= 0)
        {
            // Make sure the target alpha is applied
            mFader->notifyAlphaChanged(mTargetAlpha);
            finish();
            return;
        }

        if (mRemainingTime > mTargetTime)
        {
            mRemainingTime -= dt;
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

    ScreenFader::ScreenFader(const std::string & texturePath, const std::string& layout, const MyGUI::FloatCoord& texCoordOverride)
        : WindowBase(layout)
        , mCurrentAlpha(0.f)
        , mFactor(1.f)
        , mRepeat(false)
    {
        MyGUI::Gui::getInstance().eventFrameStart += MyGUI::newDelegate(this, &ScreenFader::onFrameStart);

        mMainWidget->setSize(MyGUI::RenderManager::getInstance().getViewSize());

        MyGUI::ImageBox* imageBox = mMainWidget->castType<MyGUI::ImageBox>(false);
        if (imageBox)
        {
            imageBox->setImageTexture(texturePath);
            const MyGUI::IntSize imageSize = imageBox->getImageSize();
            imageBox->setImageCoord(MyGUI::IntCoord(texCoordOverride.left * imageSize.width,
                                                    texCoordOverride.top * imageSize.height,
                                                    texCoordOverride.width * imageSize.width,
                                                    texCoordOverride.height * imageSize.height));
        }
    }

    ScreenFader::~ScreenFader()
    {
        try
        {
            MyGUI::Gui::getInstance().eventFrameStart -= MyGUI::newDelegate(this, &ScreenFader::onFrameStart);
        }
        catch(const MyGUI::Exception& e)
        {
            Log(Debug::Error) << "Error in the destructor: " << e.what();
        }
    }

    void ScreenFader::onFrameStart(float dt)
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

    void ScreenFader::fadeIn(float time, float delay)
    {
        queue(time, 1.f, delay);
    }

    void ScreenFader::fadeOut(const float time, float delay)
    {
        queue(time, 0.f, delay);
    }

    void ScreenFader::fadeTo(const int percent, const float time, float delay)
    {
        queue(time, percent/100.f, delay);
    }

    void ScreenFader::clear()
    {
        clearQueue();
        notifyAlphaChanged(0.f);
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

    void ScreenFader::queue(float time, float targetAlpha, float delay)
    {
        if (time < 0.f)
            return;

        if (time == 0.f && delay == 0.f)
        {
            mCurrentAlpha = targetAlpha;
            applyAlpha();
            return;
        }

        mQueue.push_back(FadeOp::Ptr(new FadeOp(this, time, targetAlpha, delay)));
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
