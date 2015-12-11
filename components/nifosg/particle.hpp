#ifndef OPENMW_COMPONENTS_NIFOSG_PARTICLE_H
#define OPENMW_COMPONENTS_NIFOSG_PARTICLE_H

#include <osgParticle/Particle>
#include <osgParticle/Shooter>
#include <osgParticle/Operator>
#include <osgParticle/ModularEmitter>

#include <osg/NodeCallback>
#include <osg/UserDataContainer>

#include <components/nif/nifkey.hpp>
#include <components/nif/data.hpp>

#include "controller.hpp" // ValueInterpolator

namespace Nif
{
    class NiGravity;
    class NiPlanarCollider;
}

namespace NifOsg
{

    // Subclass ParticleSystem to support a limit on the number of active particles.
    class ParticleSystem : public osgParticle::ParticleSystem
    {
    public:
        ParticleSystem();
        ParticleSystem(const ParticleSystem& copy, const osg::CopyOp& copyop);

        META_Object(OpenMW, ParticleSystem)

        virtual osgParticle::Particle* createParticle(const osgParticle::Particle *ptemplate);

        // For serialization.  setQuota used elsewhere as well.
        inline int getQuota() const { return mQuota; }
        inline void setQuota(int quota) { mQuota = quota; }

    private:
        int mQuota;
    };

    // HACK: Particle doesn't allow setting the initial age, but we need this for loading the particle system state
    class ParticleAgeSetter : public osgParticle::Particle
    {
    public:
        ParticleAgeSetter(float age)
            : Particle()
        {
            _t0 = age;
        }
    };

    // Node callback used to set the inverse of the parent's world matrix on the MatrixTransform
    // that the callback is attached to. Used for certain particle systems,
    // so that the particles do not move with the node they are attached to.
    class InverseWorldMatrix : public osg::NodeCallback
    {
    public:
        InverseWorldMatrix()
        {
        }
        InverseWorldMatrix(const InverseWorldMatrix& copy, const osg::CopyOp& op)
            : osg::Object(), osg::NodeCallback()
        {
        }

        META_Object(OpenMW, InverseWorldMatrix)

        void operator()(osg::Node* node, osg::NodeVisitor* nv);
    };

    class ParticleShooter : public osgParticle::Shooter
    {
    public:
        ParticleShooter(float minSpeed, float maxSpeed, float horizontalDir, float horizontalAngle, float verticalDir, float verticalAngle,
                        float lifetime, float lifetimeRandom);
        ParticleShooter();
        ParticleShooter(const ParticleShooter& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        META_Object(OpenMW, ParticleShooter)

        virtual void shoot(osgParticle::Particle* particle) const;

        // For serialization.
        inline float getMinSpeed() const { return mMinSpeed; }
        inline void setMinSpeed(float s) { mMinSpeed = s; }
        inline float getMaxSpeed() const { return mMaxSpeed; }
        inline void setMaxSpeed(float s) { mMaxSpeed = s; }
        inline float getHorizontalDir() const { return mHorizontalDir; }
        inline void setHorizontalDir(float d) { mHorizontalDir = d; }
        inline float getHorizontalAngle() const { return mHorizontalAngle; }
        inline void setHorizontalAngle(float a) { mHorizontalAngle = a; }
        inline float getVerticalDir() const { return mVerticalDir; }
        inline void setVerticalDir(float d) { mVerticalDir = d; }
        inline float getVerticalAngle() const { return mVerticalAngle; }
        inline void setVerticalAngle(float a) { mVerticalAngle = a; }
        inline float getLifetime() const { return mLifetime; }
        inline void setLifetime(float l) { mLifetime = l; }
        inline float getLifetimeRandom() const { return mLifetimeRandom; }
        inline void setLifetimeRandom(float l) { mLifetimeRandom = l; }

    private:
        float mMinSpeed;
        float mMaxSpeed;
        float mHorizontalDir;
        float mHorizontalAngle;
        float mVerticalDir;
        float mVerticalAngle;
        float mLifetime;
        float mLifetimeRandom;
    };

    class PlanarCollider : public osgParticle::Operator
    {
    public:
        PlanarCollider(const Nif::NiPlanarCollider* collider);
        PlanarCollider();
        PlanarCollider(const PlanarCollider& copy, const osg::CopyOp& copyop);

        META_Object(OpenMW, PlanarCollider)

        virtual void beginOperate(osgParticle::Program* program);
        virtual void operate(osgParticle::Particle* particle, double dt);

        // For serialization.
        inline float getBounceFactor() const { return mBounceFactor; }
        inline void setBounceFactor(float f) { mBounceFactor = f; }
        inline const osg::Plane& getPlane() const { return mPlane; }
        inline void setPlane(const osg::Plane& p) { mPlane = p; }
        inline const osg::Plane& getPlaneInParticleSpace() const { return mPlaneInParticleSpace; }
        inline void setPlaneInParticleSpace(const osg::Plane& p) { mPlaneInParticleSpace = p; }

    private:
        float mBounceFactor;
        osg::Plane mPlane;
        osg::Plane mPlaneInParticleSpace;
    };

    class GrowFadeAffector : public osgParticle::Operator
    {
    public:
        GrowFadeAffector(float growTime, float fadeTime);
        GrowFadeAffector();
        GrowFadeAffector(const GrowFadeAffector& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        META_Object(OpenMW, GrowFadeAffector)

        virtual void beginOperate(osgParticle::Program* program);
        virtual void operate(osgParticle::Particle* particle, double dt);

        // For serialization.
        inline float getGrow() const { return mGrowTime; }
        inline void setGrow(float g) { mGrowTime = g; }
        inline float getFade() const { return mFadeTime; }
        inline void setFade(float f) { mFadeTime = f; }

    private:
        float mGrowTime;
        float mFadeTime;

        float mCachedDefaultSize;
    };

    typedef ValueInterpolator<Nif::Vector4KeyMap, LerpFunc> Vec4Interpolator;
    class ParticleColorAffector : public osgParticle::Operator
    {
    public:
        ParticleColorAffector(const Nif::NiColorData* clrdata);
        ParticleColorAffector();
        ParticleColorAffector(const ParticleColorAffector& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        META_Object(OpenMW, ParticleColorAffector)

        virtual void operate(osgParticle::Particle* particle, double dt);

        Vec4Interpolator mData;
    };

    class GravityAffector : public osgParticle::Operator
    {
    public:
        GravityAffector(const Nif::NiGravity* gravity);
        GravityAffector();
        GravityAffector(const GravityAffector& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        META_Object(OpenMW, GravityAffector)

        virtual void operate(osgParticle::Particle* particle, double dt);
        virtual void beginOperate(osgParticle::Program *);

        enum ForceType {
            Type_Wind,
            Type_Point
        };

        // For serialization.
        inline float getForce() const { return mForce; }
        inline void setForce(float f) { mForce = f; }
        inline ForceType getType() const { return mType; }
        inline void setType(ForceType t) { mType = t; }
        inline float getDecay() const { return mDecay; }
        inline void setDecay(float d) { mDecay = d; }
        inline const osg::Vec3f& getPosition() const { return mPosition; }
        inline void setPosition(const osg::Vec3f& p) { mPosition = p; }
        inline const osg::Vec3f& getDirection() const { return mDirection; }
        inline void setDirection(const osg::Vec3f& d) { mDirection = d; }

    private:
        float mForce;
        ForceType mType;
        osg::Vec3f mPosition;
        osg::Vec3f mDirection;
        float mDecay;
        osg::Vec3f mCachedWorldPosition;
        osg::Vec3f mCachedWorldDirection;
    };

    // NodeVisitor to find a child node with the given record index, stored in the node's user data container.
    class FindRecIndexVisitor : public osg::NodeVisitor
    {
    public:
        FindRecIndexVisitor(int recIndex);

        virtual void apply(osg::Node &searchNode);

        osg::Group* mFound;
        osg::NodePath mFoundPath;
    private:
        int mRecIndex;
    };

    // Subclass emitter to support randomly choosing one of the child node's transforms for the emit position of new particles.
    class Emitter : public osgParticle::Emitter
    {
    public:
        Emitter(const std::vector<int>& targets);
        Emitter();
        Emitter(const Emitter& copy, const osg::CopyOp& copyop);

        META_Object(OpenMW, Emitter)

        virtual void emitParticles(double dt);

        // For serialization.   setShooter(), setPlacer() and setCounter() used elsewhere.
        const std::vector<int>& getTargets() const { return mTargets; }
        void setTargets(const std::vector<int>& targets) { mTargets = targets; }
        const osgParticle::Shooter* getShooter() const { return mShooter; }
        void setShooter(osgParticle::Shooter* shooter) { mShooter = shooter; }
        const osgParticle::Placer* getPlacer() const { return mPlacer; }
        void setPlacer(osgParticle::Placer* placer) { mPlacer = placer; }
        const osgParticle::Counter* getCounter() const { return mCounter; }
        void setCounter(osgParticle::Counter* counter) { mCounter = counter; }

    private:
        // NIF Record indices
        std::vector<int> mTargets;

        osg::ref_ptr<osgParticle::Placer> mPlacer;
        osg::ref_ptr<osgParticle::Shooter> mShooter;
        osg::ref_ptr<osgParticle::Counter> mCounter;
    };

}

#endif
