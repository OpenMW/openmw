#ifndef OENGINE_OGRE_LIGHTS_H
#define OENGINE_OGRE_LIGHTS_H

#include <OgreController.h>
#include <OgreColourValue.h>

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
        LightFunction(LightType type);
        virtual Ogre::Real calculate(Ogre::Real value);
    };

    class LightValue : public Ogre::ControllerValue<Ogre::Real>
    {
        Ogre::Light *mTarget;
        Ogre::ColourValue mColor;

    public:
        LightValue(Ogre::Light *light, const Ogre::ColourValue &color);

        virtual Ogre::Real getValue() const;
        virtual void setValue(Ogre::Real value);
    };
}
}

#endif
