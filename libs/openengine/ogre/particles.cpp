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


class GravityAffector : public Ogre::ParticleAffector
{
    enum ForceType {
        Type_Wind,
        Type_Point
    };

public:
    /** Command object for force (see Ogre::ParamCommand).*/
    class CmdForce : public Ogre::ParamCommand
    {
    public:
        Ogre::String doGet(const void *target) const
        {
            const GravityAffector *self = static_cast<const GravityAffector*>(target);
            return Ogre::StringConverter::toString(self->getForce());
        }
        void doSet(void *target, const Ogre::String &val)
        {
            GravityAffector *self = static_cast<GravityAffector*>(target);
            self->setForce(Ogre::StringConverter::parseReal(val));
        }
    };

    /** Command object for force_type (see Ogre::ParamCommand).*/
    class CmdForceType : public Ogre::ParamCommand
    {
        static ForceType getTypeFromString(const Ogre::String &type)
        {
            if(type == "wind")
                return Type_Wind;
            if(type == "point")
                return Type_Point;
            OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "Invalid force type string: "+type,
                        "CmdForceType::getTypeFromString");
        }

        static Ogre::String getStringFromType(ForceType type)
        {
            switch(type)
            {
                case Type_Wind: return "wind";
                case Type_Point: return "point";
            }
            OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "Invalid force type enum: "+Ogre::StringConverter::toString(type),
                        "CmdForceType::getStringFromType");
        }

    public:
        Ogre::String doGet(const void *target) const
        {
            const GravityAffector *self = static_cast<const GravityAffector*>(target);
            return getStringFromType(self->getForceType());
        }
        void doSet(void *target, const Ogre::String &val)
        {
            GravityAffector *self = static_cast<GravityAffector*>(target);
            self->setForceType(getTypeFromString(val));
        }
    };

    /** Command object for direction (see Ogre::ParamCommand).*/
    class CmdDirection : public Ogre::ParamCommand
    {
    public:
        Ogre::String doGet(const void *target) const
        {
            const GravityAffector *self = static_cast<const GravityAffector*>(target);
            return Ogre::StringConverter::toString(self->getDirection());
        }
        void doSet(void *target, const Ogre::String &val)
        {
            GravityAffector *self = static_cast<GravityAffector*>(target);
            self->setDirection(Ogre::StringConverter::parseVector3(val));
        }
    };

    /** Command object for position (see Ogre::ParamCommand).*/
    class CmdPosition : public Ogre::ParamCommand
    {
    public:
        Ogre::String doGet(const void *target) const
        {
            const GravityAffector *self = static_cast<const GravityAffector*>(target);
            return Ogre::StringConverter::toString(self->getPosition());
        }
        void doSet(void *target, const Ogre::String &val)
        {
            GravityAffector *self = static_cast<GravityAffector*>(target);
            self->setPosition(Ogre::StringConverter::parseVector3(val));
        }
    };


    /** Default constructor. */
    GravityAffector(Ogre::ParticleSystem *psys)
      : ParticleAffector(psys)
      , mForce(0.0f)
      , mForceType(Type_Wind)
      , mPosition(0.0f)
      , mDirection(0.0f)
    {
        mType = "Gravity";

        // Init parameters
        if(createParamDictionary("GravityAffector"))
        {
            Ogre::ParamDictionary *dict = getParamDictionary();

            Ogre::String force_title("force");
            Ogre::String force_descr("Amount of force applied to particles.");
            Ogre::String force_type_title("force_type");
            Ogre::String force_type_descr("Type of force applied to particles (point or wind).");
            Ogre::String direction_title("direction");
            Ogre::String direction_descr("Direction of wind forces.");
            Ogre::String position_title("position");
            Ogre::String position_descr("Position of point forces.");

            dict->addParameter(Ogre::ParameterDef(force_title, force_descr, Ogre::PT_REAL), &msForceCmd);
            dict->addParameter(Ogre::ParameterDef(force_type_title, force_type_descr, Ogre::PT_STRING), &msForceTypeCmd);
            dict->addParameter(Ogre::ParameterDef(direction_title, direction_descr, Ogre::PT_VECTOR3), &msDirectionCmd);
            dict->addParameter(Ogre::ParameterDef(position_title, position_descr, Ogre::PT_VECTOR3), &msPositionCmd);
        }
    }

    /** See Ogre::ParticleAffector. */
    void _affectParticles(Ogre::ParticleSystem *psys, Ogre::Real timeElapsed)
    {
        switch(mForceType)
        {
            case Type_Wind:
                applyWindForce(psys, timeElapsed);
                break;
            case Type_Point:
                applyPointForce(psys, timeElapsed);
                break;
        }
    }

    void setForce(Ogre::Real force)
    { mForce = force; }
    Ogre::Real getForce() const
    { return mForce; }

    void setForceType(ForceType type)
    { mForceType = type; }
    ForceType getForceType() const
    { return mForceType; }

    void setDirection(const Ogre::Vector3 &dir)
    { mDirection = dir; }
    const Ogre::Vector3 &getDirection() const
    { return mDirection; }

    void setPosition(const Ogre::Vector3 &pos)
    { mPosition = pos; }
    const Ogre::Vector3 &getPosition() const
    { return mPosition; }

    static CmdForce msForceCmd;
    static CmdForceType msForceTypeCmd;
    static CmdDirection msDirectionCmd;
    static CmdPosition msPositionCmd;

protected:
    void applyWindForce(Ogre::ParticleSystem *psys, Ogre::Real timeElapsed)
    {
        const Ogre::Vector3 vec = mDirection * mForce * timeElapsed;
        Ogre::ParticleIterator pi = psys->_getIterator();
        while (!pi.end())
        {
            Ogre::Particle *p = pi.getNext();
            p->direction += vec;
        }
    }

    void applyPointForce(Ogre::ParticleSystem *psys, Ogre::Real timeElapsed)
    {
        const Ogre::Real force = mForce * timeElapsed;
        Ogre::ParticleIterator pi = psys->_getIterator();
        while (!pi.end())
        {
            Ogre::Particle *p = pi.getNext();
            const Ogre::Vector3 vec = (p->position - mPosition).normalisedCopy() * force;
            p->direction += vec;
        }
    }


    float mForce;

    ForceType mForceType;

    Ogre::Vector3 mPosition;
    Ogre::Vector3 mDirection;
};
GravityAffector::CmdForce GravityAffector::msForceCmd;
GravityAffector::CmdForceType GravityAffector::msForceTypeCmd;
GravityAffector::CmdDirection GravityAffector::msDirectionCmd;
GravityAffector::CmdPosition GravityAffector::msPositionCmd;

Ogre::ParticleAffector *GravityAffectorFactory::createAffector(Ogre::ParticleSystem *psys)
{
    Ogre::ParticleAffector *p = new GravityAffector(psys);
    mAffectors.push_back(p);
    return p;
}
