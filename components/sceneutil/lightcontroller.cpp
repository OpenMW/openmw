#include "lightcontroller.hpp"

#include <cmath>

#include <osg/NodeVisitor>

#include <components/sceneutil/lightmanager.hpp>

#include <components/misc/rng.hpp>

namespace SceneUtil
{

    LightController::LightController()
        : mType(LT_Normal)
        , mPhase(0.25f + Misc::Rng::rollClosedProbability() * 0.75f)
        , mBrightness(0.675f)
        , mStartTime(0.0)
        , mLastTime(0.0)
        , mTicksToAdvance(0.f)
    {
    }

    void LightController::setType(LightController::LightType type)
    {
        mType = type;
    }

    void LightController::operator ()(osg::Node* node, osg::NodeVisitor* nv)
    {
        double time = nv->getFrameStamp()->getSimulationTime();
        if (mStartTime == 0)
            mStartTime = time;

        // disabled early out, light state needs to be set every frame regardless of change, due to the double buffering
        //if (time == mLastTime)
        //    return;

        if (mType == LT_Normal)
        {
            static_cast<SceneUtil::LightSource*>(node)->getLight(nv->getTraversalNumber())->setDiffuse(mDiffuseColor);
            traverse(node, nv);
            return;
        }

        // Updating flickering at 15 FPS like vanilla.
        constexpr float updateRate = 15.f;
        mTicksToAdvance = static_cast<float>(time - mStartTime - mLastTime) * updateRate * 0.25f + mTicksToAdvance * 0.75f;
        mLastTime = time - mStartTime;

        float speed = (mType == LT_Flicker || mType == LT_Pulse) ? 0.1f : 0.05f;
        if (mBrightness >= mPhase)
            mBrightness -= mTicksToAdvance * speed;
        else
            mBrightness += mTicksToAdvance * speed;

        if (std::abs(mBrightness - mPhase) < speed)
        {
            if (mType == LT_Flicker || mType == LT_FlickerSlow)
                mPhase = 0.25f + Misc::Rng::rollClosedProbability() * 0.75f;
            else // if (mType == LT_Pulse || mType == LT_PulseSlow)
                mPhase = mPhase <= 0.5f ? 1.f : 0.25f;
        }

        static_cast<SceneUtil::LightSource*>(node)->getLight(nv->getTraversalNumber())->setDiffuse(mDiffuseColor * mBrightness);

        traverse(node, nv);
    }

    void LightController::setDiffuse(const osg::Vec4f& color)
    {
        mDiffuseColor = color;
    }

}
