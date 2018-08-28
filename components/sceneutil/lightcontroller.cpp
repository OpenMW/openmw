#include "lightcontroller.hpp"

#include <cmath>

#include <osg/NodeVisitor>

#include <components/sceneutil/lightmanager.hpp>

#include <components/misc/rng.hpp>

namespace
{

    float pulseAmplitude(float time)
    {
        return std::sin(time);
    }

    float flickerAmplitude(float time)
    {
        static const float fb = 1.17024f;
        static const float f[3] = { 1.5708f,   4.18774f, 5.19934f };
        static const float o[3] = { 0.804248f, 2.11115f, 3.46832f };
        static const float m[3] = { 1.0f,      0.785f,   0.876f   };
        static const float s = 0.394f;

        float v = 0.0f;
        for(int i = 0;i < 3;++i)
            v += std::sin(fb*time*f[i] + o[i])*m[i];
        return v * s;
    }

    float flickerFrequency(float phase)
    {
        static const float fa = 0.785398f;
        static const float tdo = 0.94f;
        static const float tdm = 2.48f;

        return tdo + tdm*std::sin(fa * phase);
    }

}

namespace SceneUtil
{

    LightController::LightController()
        : mType(LT_Normal)
        , mPhase((Misc::Rng::rollClosedProbability() * 2.f - 1.f) * 500.f)
        , mDeltaCount(0.f)
        , mDirection(1.f)
        , mLastTime(0.0)
    {
    }

    void LightController::setType(LightController::LightType type)
    {
        mType = type;
    }

    void LightController::operator ()(osg::Node* node, osg::NodeVisitor* nv)
    {
        double time = nv->getFrameStamp()->getSimulationTime();

        // disabled early out, light state needs to be set every frame regardless of change, due to the double buffering
        //if (time == mLastTime)
        //    return;

        float dt = static_cast<float>(time - mLastTime);
        mLastTime = time;

        float brightness = 1.0f;
        float cycle_time;
        float time_distortion;

        if(mType == LT_Pulse || mType == LT_PulseSlow)
        {
            cycle_time = 2.0f * osg::PI;
            time_distortion = 3.0f;
        }
        else
        {
            static const float fa = osg::PI / 4.0f;
            static const float phase_wavelength = 120.0f * osg::PI / fa;

            cycle_time = 500.0f;
            mPhase = std::fmod(mPhase + dt, phase_wavelength);
            time_distortion = flickerFrequency(mPhase);
        }

        mDeltaCount += mDirection*dt*time_distortion;
        if(mDirection > 0 && mDeltaCount > +cycle_time)
        {
            mDirection = -1.0f;
            float extra = mDeltaCount - cycle_time;
            mDeltaCount -= 2*extra;
        }
        if(mDirection < 0 && mDeltaCount < -cycle_time)
        {
            mDirection = +1.0f;
            float extra = cycle_time - mDeltaCount;
            mDeltaCount += 2*extra;
        }

        static const float fast = 4.0f/1.0f;
        static const float slow = 1.0f/1.0f;

        // These formulas are just guesswork, but they work pretty well
        if(mType == LT_Normal)
        {
            // Less than 1/255 light modifier for a constant light:
            brightness = 1.0f + flickerAmplitude(mDeltaCount*slow)/255.0f;
        }
        else if(mType == LT_Flicker)
            brightness = 0.75f + flickerAmplitude(mDeltaCount*fast)*0.25f;
        else if(mType == LT_FlickerSlow)
            brightness = 0.75f + flickerAmplitude(mDeltaCount*slow)*0.25f;
        else if(mType == LT_Pulse)
            brightness = 0.7f + pulseAmplitude(mDeltaCount*fast)*0.3f;
        else if(mType == LT_PulseSlow)
            brightness = 0.7f + pulseAmplitude(mDeltaCount*slow)*0.3f;

        static_cast<SceneUtil::LightSource*>(node)->getLight(nv->getTraversalNumber())->setDiffuse(mDiffuseColor * brightness);

        traverse(node, nv);
    }

    void LightController::setDiffuse(const osg::Vec4f& color)
    {
        mDiffuseColor = color;
    }

}
