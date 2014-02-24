#ifndef OENGINE_OGRE_LIGHTS_H
#define OENGINE_OGRE_LIGHTS_H

#include <OgreController.h>
#include <OgreColourValue.h>
#include <OgreMath.h>

/*
 * Controller classes to handle pulsing and flicker lights
 */

namespace OEngine {
namespace Render {
    enum LightType {
        LT_Normal,
        LT_Flicker,
        LT_FlickerSlow,
        LT_Pulse,
        LT_PulseSlow
    };

    class LightFunction : public Ogre::ControllerFunction<Ogre::Real>
    {
        LightType mType;
        Ogre::Real mPhase;
        Ogre::Real mDirection;

        static Ogre::Real pulseAmplitude(Ogre::Real time);

        static Ogre::Real flickerAmplitude(Ogre::Real time);
        static Ogre::Real flickerFrequency(Ogre::Real phase);

    public:
        // MSVC needs the constructor for a class inheriting a template to be defined in header
        LightFunction(LightType type)
          : ControllerFunction<Ogre::Real>(true)
          , mType(type)
          , mPhase(Ogre::Math::RangeRandom(-500.0f, +500.0f))
          , mDirection(1.0f)
        {
        }
        virtual Ogre::Real calculate(Ogre::Real value);
    };

    class LightValue : public Ogre::ControllerValue<Ogre::Real>
    {
        Ogre::Light *mTarget;
        Ogre::ColourValue mColor;

    public:
        // MSVC needs the constructor for a class inheriting a template to be defined in header
        LightValue(Ogre::Light *light, const Ogre::ColourValue &color)
          : mTarget(light)
          , mColor(color)
        {
        }

        virtual Ogre::Real getValue() const;
        virtual void setValue(Ogre::Real value);
    };
}
}

#endif
