#include "particles.hpp"

#include <OgreStringConverter.h>
#include <OgreParticleSystem.h>
#include <OgreParticleAffector.h>
#include <OgreParticle.h>

class GrowFadeAffector : public Ogre::ParticleAffector
{
public:
    /** Command object for grow_time (see Ogre::ParamCommand).*/
    class CmdGrowTime : public Ogre::ParamCommand
    {
    public:
        Ogre::String doGet(const void *target) const
        {
            const GrowFadeAffector *self = static_cast<const GrowFadeAffector*>(target);
            return Ogre::StringConverter::toString(self->getGrowTime());
        }
        void doSet(void *target, const Ogre::String &val)
        {
            GrowFadeAffector *self = static_cast<GrowFadeAffector*>(target);
            self->setGrowTime(Ogre::StringConverter::parseReal(val));
        }
    };

    /** Command object for fade_time (see Ogre::ParamCommand).*/
    class CmdFadeTime : public Ogre::ParamCommand
    {
    public:
        Ogre::String doGet(const void *target) const
        {
            const GrowFadeAffector *self = static_cast<const GrowFadeAffector*>(target);
            return Ogre::StringConverter::toString(self->getFadeTime());
        }
        void doSet(void *target, const Ogre::String &val)
        {
            GrowFadeAffector *self = static_cast<GrowFadeAffector*>(target);
            self->setFadeTime(Ogre::StringConverter::parseReal(val));
        }
    };

    /** Default constructor. */
    GrowFadeAffector(Ogre::ParticleSystem *psys) : ParticleAffector(psys)
    {
        mGrowTime = 0.0f;
        mFadeTime = 0.0f;

        mType = "GrowFade";

        // Init parameters
        if(createParamDictionary("GrowFadeAffector"))
        {
            Ogre::ParamDictionary *dict = getParamDictionary();

            Ogre::String grow_title("grow_time");
            Ogre::String fade_title("fade_time");
            Ogre::String grow_descr("Time from begin to reach full size.");
            Ogre::String fade_descr("Time from end to shrink.");

            dict->addParameter(Ogre::ParameterDef(grow_title, grow_descr, Ogre::PT_REAL), &msGrowCmd);
            dict->addParameter(Ogre::ParameterDef(fade_title, fade_descr, Ogre::PT_REAL), &msFadeCmd);
        }
    }

    /** See Ogre::ParticleAffector. */
    void _initParticle(Ogre::Particle *particle)
    {
        const Ogre::Real life_time     = particle->totalTimeToLive;
        Ogre::Real       particle_time = particle->timeToLive;

        Ogre::Real width = mParent->getDefaultWidth();
        Ogre::Real height = mParent->getDefaultHeight();
        if(life_time-particle_time < mGrowTime)
        {
            Ogre::Real scale = (life_time-particle_time) / mGrowTime;
            width *= scale;
            height *= scale;
        }
        if(particle_time < mFadeTime)
        {
            Ogre::Real scale = particle_time / mFadeTime;
            width *= scale;
            height *= scale;
        }
        particle->setDimensions(width, height);
    }

    /** See Ogre::ParticleAffector. */
    void _affectParticles(Ogre::ParticleSystem *psys, Ogre::Real timeElapsed)
    {
        Ogre::ParticleIterator pi = psys->_getIterator();
        while (!pi.end())
        {
            Ogre::Particle *p = pi.getNext();
            const Ogre::Real life_time     = p->totalTimeToLive;
            Ogre::Real       particle_time = p->timeToLive;

            Ogre::Real width = mParent->getDefaultWidth();
            Ogre::Real height = mParent->getDefaultHeight();
            if(life_time-particle_time < mGrowTime)
            {
                Ogre::Real scale = (life_time-particle_time) / mGrowTime;
                width *= scale;
                height *= scale;
            }
            if(particle_time < mFadeTime)
            {
                Ogre::Real scale = particle_time / mFadeTime;
                width *= scale;
                height *= scale;
            }
            p->setDimensions(width, height);
        }
    }

    void setGrowTime(Ogre::Real time)
    {
        mGrowTime = time;
    }
    Ogre::Real getGrowTime() const
    { return mGrowTime; }

    void setFadeTime(Ogre::Real time)
    {
        mFadeTime = time;
    }
    Ogre::Real getFadeTime() const
    { return mFadeTime; }

    static CmdGrowTime msGrowCmd;
    static CmdFadeTime msFadeCmd;

protected:
    Ogre::Real mGrowTime;
    Ogre::Real mFadeTime;
};
GrowFadeAffector::CmdGrowTime GrowFadeAffector::msGrowCmd;
GrowFadeAffector::CmdFadeTime GrowFadeAffector::msFadeCmd;

Ogre::ParticleAffector *GrowFadeAffectorFactory::createAffector(Ogre::ParticleSystem *psys)
{
    Ogre::ParticleAffector *p = new GrowFadeAffector(psys);
    mAffectors.push_back(p);
    return p;
}
