#ifndef OPENMW_COMPONENTS_NIFOSG_PARTICLE_H
#define OPENMW_COMPONENTS_NIFOSG_PARTICLE_H

#include <osgParticle/Particle>
#include <osgParticle/Shooter>
#include <osgParticle/Operator>
#include <osgParticle/Emitter>
#include <osgParticle/Placer>
#include <osgParticle/Counter>

#include <osg/NodeCallback>

#include "controller.hpp" // ValueInterpolator

namespace Nif
{
    struct NiGravity;
    struct NiPlanarCollider;
    struct NiSphericalCollider;
    struct NiColorData;
}

namespace NifOsg
{

    // Subclass ParticleSystem to support a limit on the number of active particles.
    class ParticleSystem : public osgParticle::ParticleSystem
    {
    public:
        ParticleSystem();
        ParticleSystem(const ParticleSystem& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, ParticleSystem)

        osgParticle::Particle* createParticle(const osgParticle::Particle *ptemplate) override;

        void setQuota(int quota);

        void drawImplementation(osg::RenderInfo& renderInfo) const override;

    private:
        int mQuota;
        osg::ref_ptr<osg::Vec3Array> mNormalArray;
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

        META_Object(NifOsg, InverseWorldMatrix)

        void operator()(osg::Node* node, osg::NodeVisitor* nv) override;
    };

    class ParticleShooter : public osgParticle::Shooter
    {
    public:
        ParticleShooter(float minSpeed, float maxSpeed, float horizontalDir, float horizontalAngle, float verticalDir, float verticalAngle,
                        float lifetime, float lifetimeRandom);
        ParticleShooter();
        ParticleShooter(const ParticleShooter& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        ParticleShooter& operator=(const ParticleShooter&) = delete;

        META_Object(NifOsg, ParticleShooter)

        void shoot(osgParticle::Particle* particle) const override;

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

        META_Object(NifOsg, PlanarCollider)

        void beginOperate(osgParticle::Program* program) override;
        void operate(osgParticle::Particle* particle, double dt) override;

    private:
        float mBounceFactor;
        osg::Plane mPlane;
        osg::Plane mPlaneInParticleSpace;
    };

    class SphericalCollider : public osgParticle::Operator
    {
    public:
        SphericalCollider(const Nif::NiSphericalCollider* collider);
        SphericalCollider();
        SphericalCollider(const SphericalCollider& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, SphericalCollider)

        void beginOperate(osgParticle::Program* program) override;
        void operate(osgParticle::Particle* particle, double dt) override;
    private:
        float mBounceFactor;
        osg::BoundingSphere mSphere;
        osg::BoundingSphere mSphereInParticleSpace;
    };

    class GrowFadeAffector : public osgParticle::Operator
    {
    public:
        GrowFadeAffector(float growTime, float fadeTime);
        GrowFadeAffector();
        GrowFadeAffector(const GrowFadeAffector& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        GrowFadeAffector& operator=(const GrowFadeAffector&) = delete;

        META_Object(NifOsg, GrowFadeAffector)

        void beginOperate(osgParticle::Program* program) override;
        void operate(osgParticle::Particle* particle, double dt) override;

    private:
        float mGrowTime;
        float mFadeTime;

        float mCachedDefaultSize;
    };

    class ParticleColorAffector : public osgParticle::Operator
    {
    public:
        ParticleColorAffector(const Nif::NiColorData* clrdata);
        ParticleColorAffector();
        ParticleColorAffector(const ParticleColorAffector& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        ParticleColorAffector& operator=(const ParticleColorAffector&) = delete;

        META_Object(NifOsg, ParticleColorAffector)

        void operate(osgParticle::Particle* particle, double dt) override;

    private:
        Vec4Interpolator mData;
    };

    class GravityAffector : public osgParticle::Operator
    {
    public:
        GravityAffector(const Nif::NiGravity* gravity);
        GravityAffector();
        GravityAffector(const GravityAffector& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        GravityAffector& operator=(const GravityAffector&) = delete;

        META_Object(NifOsg, GravityAffector)

        void operate(osgParticle::Particle* particle, double dt) override;
        void beginOperate(osgParticle::Program *) override ;

    private:
        float mForce;
        enum ForceType {
            Type_Wind,
            Type_Point
        };
        ForceType mType;
        osg::Vec3f mPosition;
        osg::Vec3f mDirection;
        float mDecay;
        osg::Vec3f mCachedWorldPosition;
        osg::Vec3f mCachedWorldDirection;
    };

    // NodeVisitor to find a Group node with the given record index, stored in the node's user data container.
    // Alternatively, returns the node's parent Group if that node is not a Group (i.e. a leaf node).
    class FindGroupByRecIndex : public osg::NodeVisitor
    {
    public:
        FindGroupByRecIndex(unsigned int recIndex);

        void apply(osg::Node &node) override;

        // Technically not required as the default implementation would trickle down to apply(Node&) anyway,
        // but we'll shortcut instead to avoid the chain of virtual function calls
        void apply(osg::MatrixTransform& node) override;
        void apply(osg::Geometry& node) override;

        void applyNode(osg::Node& searchNode);

        osg::Group* mFound;
        osg::NodePath mFoundPath;
    private:
        unsigned int mRecIndex;
    };

    // Subclass emitter to support randomly choosing one of the child node's transforms for the emit position of new particles.
    class Emitter : public osgParticle::Emitter
    {
    public:
        Emitter(const std::vector<int>& targets);
        Emitter();
        Emitter(const Emitter& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, Emitter)

        void emitParticles(double dt) override;

        void setShooter(osgParticle::Shooter* shooter);
        void setPlacer(osgParticle::Placer* placer);
        void setCounter(osgParticle::Counter* counter);

    private:
        // NIF Record indices
        std::vector<int> mTargets;

        osg::ref_ptr<osgParticle::Placer> mPlacer;
        osg::ref_ptr<osgParticle::Shooter> mShooter;
        osg::ref_ptr<osgParticle::Counter> mCounter;
    };

}

#endif
