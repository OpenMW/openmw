#include "lights.hpp"

#include <OgreLight.h>


namespace OEngine {
namespace Render {

Ogre::Real LightFunction::pulseAmplitude(Ogre::Real time)
{
    return std::sin(time);
}

Ogre::Real LightFunction::flickerAmplitude(Ogre::Real time)
{
    static const float fb = 1.17024f;
    static const float f[3] = { 1.5708f,   4.18774f, 5.19934f };
    static const float o[3] = { 0.804248f, 2.11115f, 3.46832f };
    static const float m[3] = { 1.0f,      0.785f,   0.876f   };
    static const float s = 0.394f;

    float v = 0.0f;
    for(int i = 0;i < 3;++i)
        v += std::sin(fb*time*f[i] + o[1])*m[i];
    return v * s;
}

Ogre::Real LightFunction::flickerFrequency(Ogre::Real phase)
{
    static const float fa = 0.785398f;
    static const float tdo = 0.94f;
    static const float tdm = 2.48f;

    return tdo + tdm*std::sin(fa * phase);
}

Ogre::Real LightFunction::calculate(Ogre::Real value)
{
    Ogre::Real brightness = 1.0f;
    float cycle_time;
    float time_distortion;

    if(mType == LT_Pulse || mType == LT_PulseSlow)
    {
        cycle_time = 2.0f * Ogre::Math::PI;
        time_distortion = 20.0f;
    }
    else
    {
        static const float fa = 0.785398f;
        static const float phase_wavelength = 120.0f * 3.14159265359f / fa;

        cycle_time = 500.0f;
        mPhase = std::fmod(mPhase + value, phase_wavelength);
        time_distortion = flickerFrequency(mPhase);
    }

    mDeltaCount += mDirection*value*time_distortion;
    if(mDirection > 0 && mDeltaCount > +cycle_time)
    {
        mDirection = -1.0f;
        mDeltaCount = 2.0f*cycle_time - mDeltaCount;
    }
    if(mDirection < 0 && mDeltaCount < -cycle_time)
    {
        mDirection = +1.0f;
        mDeltaCount = -2.0f*cycle_time - mDeltaCount;
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
        brightness = 1.0f + pulseAmplitude(mDeltaCount*fast)*0.25f;
    else if(mType == LT_PulseSlow)
        brightness = 1.0f + pulseAmplitude(mDeltaCount*slow)*0.25f;

    return brightness;
}

Ogre::Real LightValue::getValue() const
{
    return 0.0f;
}

void LightValue::setValue(Ogre::Real value)
{
    mTarget->setDiffuseColour(mColor * value);
}

}
}
