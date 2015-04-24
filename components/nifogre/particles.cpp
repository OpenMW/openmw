#include "particles.hpp"

#include <OgreStringConverter.h>
#include <OgreParticleSystem.h>
#include <OgreParticleEmitter.h>
#include <OgreParticleAffector.h>
#include <OgreParticle.h>
#include <OgreBone.h>
#include <OgreTagPoint.h>
#include <OgreEntity.h>
#include <OgreSkeletonInstance.h>
#include <OgreSceneNode.h>
#include <OgreSceneManager.h>

#include <openengine/misc/rng.hpp>

/* FIXME: "Nif" isn't really an appropriate emitter name. */
class NifEmitter : public Ogre::ParticleEmitter
{
public:
    std::vector<Ogre::Bone*> mEmitterBones;
    Ogre::Bone* mParticleBone;

    Ogre::ParticleSystem* getPartSys() { return mParent; }

    /** Command object for the emitter width (see Ogre::ParamCommand).*/
    class CmdWidth : public Ogre::ParamCommand
    {
    public:
        Ogre::String doGet(const void *target) const
        {
            return Ogre::StringConverter::toString(static_cast<const NifEmitter*>(target)->getWidth());
        }
        void doSet(void *target, const Ogre::String &val)
        {
            static_cast<NifEmitter*>(target)->setWidth(Ogre::StringConverter::parseReal(val));
        }
    };

    /** Command object for the emitter height (see Ogre::ParamCommand).*/
    class CmdHeight : public Ogre::ParamCommand
    {
    public:
        Ogre::String doGet(const void *target) const
        {
            return Ogre::StringConverter::toString(static_cast<const NifEmitter*>(target)->getHeight());
        }
        void doSet(void *target, const Ogre::String &val)
        {
            static_cast<NifEmitter*>(target)->setHeight(Ogre::StringConverter::parseReal(val));
        }
    };

    /** Command object for the emitter depth (see Ogre::ParamCommand).*/
    class CmdDepth : public Ogre::ParamCommand
    {
    public:
        Ogre::String doGet(const void *target) const
        {
            return Ogre::StringConverter::toString(static_cast<const NifEmitter*>(target)->getDepth());
        }
        void doSet(void *target, const Ogre::String &val)
        {
            static_cast<NifEmitter*>(target)->setDepth(Ogre::StringConverter::parseReal(val));
        }
    };

    /** Command object for the emitter vertical_direction (see Ogre::ParamCommand).*/
    class CmdVerticalDir : public Ogre::ParamCommand
    {
    public:
        Ogre::String doGet(const void *target) const
        {
            const NifEmitter *self = static_cast<const NifEmitter*>(target);
            return Ogre::StringConverter::toString(self->getVerticalDirection().valueDegrees());
        }
        void doSet(void *target, const Ogre::String &val)
        {
            NifEmitter *self = static_cast<NifEmitter*>(target);
            self->setVerticalDirection(Ogre::Degree(Ogre::StringConverter::parseReal(val)));
        }
    };

    /** Command object for the emitter vertical_angle (see Ogre::ParamCommand).*/
    class CmdVerticalAngle : public Ogre::ParamCommand
    {
    public:
        Ogre::String doGet(const void *target) const
        {
            const NifEmitter *self = static_cast<const NifEmitter*>(target);
            return Ogre::StringConverter::toString(self->getVerticalAngle().valueDegrees());
        }
        void doSet(void *target, const Ogre::String &val)
        {
            NifEmitter *self = static_cast<NifEmitter*>(target);
            self->setVerticalAngle(Ogre::Degree(Ogre::StringConverter::parseReal(val)));
        }
    };

    /** Command object for the emitter horizontal_direction (see Ogre::ParamCommand).*/
    class CmdHorizontalDir : public Ogre::ParamCommand
    {
    public:
        Ogre::String doGet(const void *target) const
        {
            const NifEmitter *self = static_cast<const NifEmitter*>(target);
            return Ogre::StringConverter::toString(self->getHorizontalDirection().valueDegrees());
        }
        void doSet(void *target, const Ogre::String &val)
        {
            NifEmitter *self = static_cast<NifEmitter*>(target);
            self->setHorizontalDirection(Ogre::Degree(Ogre::StringConverter::parseReal(val)));
        }
    };

    /** Command object for the emitter horizontal_angle (see Ogre::ParamCommand).*/
    class CmdHorizontalAngle : public Ogre::ParamCommand
    {
    public:
        Ogre::String doGet(const void *target) const
        {
            const NifEmitter *self = static_cast<const NifEmitter*>(target);
            return Ogre::StringConverter::toString(self->getHorizontalAngle().valueDegrees());
        }
        void doSet(void *target, const Ogre::String &val)
        {
            NifEmitter *self = static_cast<NifEmitter*>(target);
            self->setHorizontalAngle(Ogre::Degree(Ogre::StringConverter::parseReal(val)));
        }
    };


    NifEmitter(Ogre::ParticleSystem *psys)
      : Ogre::ParticleEmitter(psys)
      , mEmitterBones(Ogre::any_cast<NiNodeHolder>(psys->getUserObjectBindings().getUserAny()).mBones)
    {
        assert (!mEmitterBones.empty());
        Ogre::TagPoint* tag = static_cast<Ogre::TagPoint*>(mParent->getParentNode());
        mParticleBone = static_cast<Ogre::Bone*>(tag->getParent());
        initDefaults("Nif");
    }

    /** See Ogre::ParticleEmitter. */
    unsigned short _getEmissionCount(Ogre::Real timeElapsed)
    {
        // Use basic constant emission
        return genConstantEmissionCount(timeElapsed);
    }

    /** See Ogre::ParticleEmitter. */
    void _initParticle(Ogre::Particle *particle)
    {
        Ogre::Vector3 xOff, yOff, zOff;

        // Call superclass
        ParticleEmitter::_initParticle(particle);

        xOff = Ogre::Math::SymmetricRandom() * mXRange;
        yOff = Ogre::Math::SymmetricRandom() * mYRange;
        zOff = Ogre::Math::SymmetricRandom() * mZRange;

#if OGRE_VERSION >= (1 << 16 | 10 << 8 | 0)
        Ogre::Vector3& position = particle->mPosition;
        Ogre::Vector3& direction = particle->mDirection;
        Ogre::ColourValue& colour = particle->mColour;
        Ogre::Real& totalTimeToLive = particle->mTotalTimeToLive;
        Ogre::Real& timeToLive = particle->mTimeToLive;
#else
        Ogre::Vector3& position = particle->position;
        Ogre::Vector3& direction = particle->direction;
        Ogre::ColourValue& colour = particle->colour;
        Ogre::Real& totalTimeToLive = particle->totalTimeToLive;
        Ogre::Real& timeToLive = particle->timeToLive;
#endif

        Ogre::Node* emitterBone = mEmitterBones.at(OEngine::Misc::Rng::rollDice(mEmitterBones.size()));

        position = xOff + yOff + zOff +
                 mParticleBone->_getDerivedOrientation().Inverse() * (emitterBone->_getDerivedPosition()
                - mParticleBone->_getDerivedPosition());

        // Generate complex data by reference
        genEmissionColour(colour);

        // NOTE: We do not use mDirection/mAngle for the initial direction.
        Ogre::Radian hdir = mHorizontalDir + mHorizontalAngle*Ogre::Math::SymmetricRandom();
        Ogre::Radian vdir = mVerticalDir + mVerticalAngle*Ogre::Math::SymmetricRandom();
        direction = (mParticleBone->_getDerivedOrientation().Inverse()
                     * emitterBone->_getDerivedOrientation() *
                                Ogre::Quaternion(hdir, Ogre::Vector3::UNIT_Z) *
                               Ogre::Quaternion(vdir, Ogre::Vector3::UNIT_X)) *
                              Ogre::Vector3::UNIT_Z;

        genEmissionVelocity(direction);

        // Generate simpler data
        timeToLive = totalTimeToLive = genEmissionTTL();
    }

    /** Overloaded to update the trans. matrix */
    void setDirection(const Ogre::Vector3 &dir)
    {
        ParticleEmitter::setDirection(dir);
        genAreaAxes();
    }

    /** Sets the size of the area from which particles are emitted.
    @param
        size Vector describing the size of the area. The area extends
        around the center point by half the x, y and z components of
        this vector. The box is aligned such that it's local Z axis points
        along it's direction (see setDirection)
    */
    void setSize(const Ogre::Vector3 &size)
    {
        mSize = size;
        genAreaAxes();
    }

    /** Sets the size of the area from which particles are emitted.
    @param x,y,z
        Individual axis lengths describing the size of the area. The area
        extends around the center point by half the x, y and z components
        of this vector. The box is aligned such that it's local Z axis
        points along it's direction (see setDirection)
    */
    void setSize(Ogre::Real x, Ogre::Real y, Ogre::Real z)
    {
        mSize.x = x;
        mSize.y = y;
        mSize.z = z;
        genAreaAxes();
    }

    /** Sets the width (local x size) of the emitter. */
    void setWidth(Ogre::Real width)
    {
        mSize.x = width;
        genAreaAxes();
    }
    /** Gets the width (local x size) of the emitter. */
    Ogre::Real getWidth(void) const
    { return mSize.x; }
    /** Sets the height (local y size) of the emitter. */
    void setHeight(Ogre::Real height)
    {
        mSize.y = height;
        genAreaAxes();
    }
    /** Gets the height (local y size) of the emitter. */
    Ogre::Real getHeight(void) const
    { return mSize.y; }
    /** Sets the depth (local y size) of the emitter. */
    void setDepth(Ogre::Real depth)
    {
        mSize.z = depth;
        genAreaAxes();
    }
    /** Gets the depth (local y size) of the emitter. */
    Ogre::Real getDepth(void) const
    { return mSize.z; }

    void setVerticalDirection(Ogre::Radian vdir)
    { mVerticalDir = vdir; }
    Ogre::Radian getVerticalDirection(void) const
    { return mVerticalDir; }

    void setVerticalAngle(Ogre::Radian vangle)
    { mVerticalAngle = vangle; }
    Ogre::Radian getVerticalAngle(void) const
    { return mVerticalAngle; }

    void setHorizontalDirection(Ogre::Radian hdir)
    { mHorizontalDir = hdir; }
    Ogre::Radian getHorizontalDirection(void) const
    { return mHorizontalDir; }

    void setHorizontalAngle(Ogre::Radian hangle)
    { mHorizontalAngle = hangle; }
    Ogre::Radian getHorizontalAngle(void) const
    { return mHorizontalAngle; }


protected:
    /// Size of the area
    Ogre::Vector3 mSize;

    Ogre::Radian mVerticalDir;
    Ogre::Radian mVerticalAngle;
    Ogre::Radian mHorizontalDir;
    Ogre::Radian mHorizontalAngle;

    /// Local axes, not normalised, their magnitude reflects area size
    Ogre::Vector3 mXRange, mYRange, mZRange;

    /// Internal method for generating the area axes
    void genAreaAxes(void)
    {
        Ogre::Vector3 mLeft = mUp.crossProduct(mDirection);
        mXRange = mLeft * (mSize.x * 0.5f);
        mYRange = mUp * (mSize.y * 0.5f);
        mZRange = mDirection * (mSize.z * 0.5f);
    }

    /** Internal for initializing some defaults and parameters
    @return True if custom parameters need initialising
    */
    bool initDefaults(const Ogre::String &t)
    {
        // Defaults
        mDirection = Ogre::Vector3::UNIT_Z;
        mUp = Ogre::Vector3::UNIT_Y;
        setSize(100.0f, 100.0f, 100.0f);
        mType = t;

        // Set up parameters
        if(createParamDictionary(mType + "Emitter"))
        {
            addBaseParameters();
            Ogre::ParamDictionary *dict = getParamDictionary();

            // Custom params
            dict->addParameter(Ogre::ParameterDef("width",
                                                  "Width of the shape in world coordinates.",
                                                  Ogre::PT_REAL),
                               &msWidthCmd);
            dict->addParameter(Ogre::ParameterDef("height",
                                                  "Height of the shape in world coordinates.",
                                                  Ogre::PT_REAL),
                               &msHeightCmd);
            dict->addParameter(Ogre::ParameterDef("depth",
                                                  "Depth of the shape in world coordinates.",
                                                  Ogre::PT_REAL),
                               &msDepthCmd);

            dict->addParameter(Ogre::ParameterDef("vertical_direction",
                                                  "Vertical direction of emitted particles (in degrees).",
                                                  Ogre::PT_REAL),
                               &msVerticalDirCmd);
            dict->addParameter(Ogre::ParameterDef("vertical_angle",
                                                  "Vertical direction variance of emitted particles (in degrees).",
                                                  Ogre::PT_REAL),
                               &msVerticalAngleCmd);
            dict->addParameter(Ogre::ParameterDef("horizontal_direction",
                                                  "Horizontal direction of emitted particles (in degrees).",
                                                  Ogre::PT_REAL),
                               &msHorizontalDirCmd);
            dict->addParameter(Ogre::ParameterDef("horizontal_angle",
                                                  "Horizontal direction variance of emitted particles (in degrees).",
                                                  Ogre::PT_REAL),
                               &msHorizontalAngleCmd);

            return true;
        }
        return false;
    }

    /// Command objects
    static CmdWidth msWidthCmd;
    static CmdHeight msHeightCmd;
    static CmdDepth msDepthCmd;
    static CmdVerticalDir msVerticalDirCmd;
    static CmdVerticalAngle msVerticalAngleCmd;
    static CmdHorizontalDir msHorizontalDirCmd;
    static CmdHorizontalAngle msHorizontalAngleCmd;
};
NifEmitter::CmdWidth NifEmitter::msWidthCmd;
NifEmitter::CmdHeight NifEmitter::msHeightCmd;
NifEmitter::CmdDepth NifEmitter::msDepthCmd;
NifEmitter::CmdVerticalDir NifEmitter::msVerticalDirCmd;
NifEmitter::CmdVerticalAngle NifEmitter::msVerticalAngleCmd;
NifEmitter::CmdHorizontalDir NifEmitter::msHorizontalDirCmd;
NifEmitter::CmdHorizontalAngle NifEmitter::msHorizontalAngleCmd;

Ogre::ParticleEmitter* NifEmitterFactory::createEmitter(Ogre::ParticleSystem *psys)
{
    Ogre::ParticleEmitter *emitter = OGRE_NEW NifEmitter(psys);
    mEmitters.push_back(emitter);
    return emitter;
}


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
#if OGRE_VERSION >= (1 << 16 | 10 << 8 | 0)
        const Ogre::Real life_time     = particle->mTotalTimeToLive;
        Ogre::Real       particle_time = particle->mTimeToLive;
#else
        const Ogre::Real life_time     = particle->totalTimeToLive;
        Ogre::Real       particle_time = particle->timeToLive;
#endif
        Ogre::Real width = mParent->getDefaultWidth();
        Ogre::Real height = mParent->getDefaultHeight();
        if(life_time-particle_time < mGrowTime)
        {
            Ogre::Real scale = (life_time-particle_time) / mGrowTime;
            assert (scale >= 0);
            // HACK: don't allow zero-sized particles which can rarely cause an AABB assertion in Ogre to fail
            scale = std::max(scale, 0.00001f);
            width *= scale;
            height *= scale;
        }
        if(particle_time < mFadeTime)
        {
            Ogre::Real scale = particle_time / mFadeTime;
            assert (scale >= 0);
            // HACK: don't allow zero-sized particles which can rarely cause an AABB assertion in Ogre to fail
            scale = std::max(scale, 0.00001f);
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
#if OGRE_VERSION >= (1 << 16 | 10 << 8 | 0)
        const Ogre::Real life_time     = p->mTotalTimeToLive;
        Ogre::Real       particle_time = p->mTimeToLive;
#else
        const Ogre::Real life_time     = p->totalTimeToLive;
        Ogre::Real       particle_time = p->timeToLive;
#endif
            Ogre::Real width = mParent->getDefaultWidth();
            Ogre::Real height = mParent->getDefaultHeight();
            if(life_time-particle_time < mGrowTime)
            {
                Ogre::Real scale = (life_time-particle_time) / mGrowTime;
                assert (scale >= 0);
                // HACK: don't allow zero-sized particles which can rarely cause an AABB assertion in Ogre to fail
                scale = std::max(scale, 0.00001f);
                width *= scale;
                height *= scale;
            }
            if(particle_time < mFadeTime)
            {
                Ogre::Real scale = particle_time / mFadeTime;
                assert (scale >= 0);
                // HACK: don't allow zero-sized particles which can rarely cause an AABB assertion in Ogre to fail
                scale = std::max(scale, 0.00001f);
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
    Ogre::Bone* mEmitterBone;
    Ogre::Bone* mParticleBone;

    Ogre::ParticleSystem* getPartSys() { return mParent; }

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
        std::vector<Ogre::Bone*> bones = Ogre::any_cast<NiNodeHolder>(psys->getUserObjectBindings().getUserAny()).mBones;
        assert (!bones.empty());
        mEmitterBone = bones[0];
        Ogre::TagPoint* tag = static_cast<Ogre::TagPoint*>(mParent->getParentNode());
        mParticleBone = static_cast<Ogre::Bone*>(tag->getParent());

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
#if OGRE_VERSION >= (1 << 16 | 10 << 8 | 0)
            p->mDirection += vec;
#else
            p->direction += vec;
#endif
        }
    }

    void applyPointForce(Ogre::ParticleSystem *psys, Ogre::Real timeElapsed)
    {
        const Ogre::Real force = mForce * timeElapsed;
        Ogre::ParticleIterator pi = psys->_getIterator();
        while (!pi.end())
        {
            Ogre::Particle *p = pi.getNext();
#if OGRE_VERSION >= (1 << 16 | 10 << 8 | 0)
            Ogre::Vector3 position = p->mPosition;
#else
            Ogre::Vector3 position = p->position;
#endif

            Ogre::Vector3 vec = (mPosition - position).normalisedCopy() * force;
#if OGRE_VERSION >= (1 << 16 | 10 << 8 | 0)
            p->mDirection += vec;
#else
            p->direction += vec;
#endif
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
