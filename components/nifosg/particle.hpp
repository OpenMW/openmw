#ifndef OPENMW_COMPONENTS_NIFOSG_PARTICLE_H
#define OPENMW_COMPONENTS_NIFOSG_PARTICLE_H

#include <optional>

#include <osgParticle/Counter>
#include <osgParticle/Emitter>
#include <osgParticle/Operator>
#include <osgParticle/Particle>
#include <osgParticle/Placer>
#include <osgParticle/Shooter>

#include <components/nif/particle.hpp> // NiGravity::ForceType

#include <components/sceneutil/nodecallback.hpp>

#include "controller.hpp" // ValueInterpolator

namespace Nif
{
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

        osgParticle::Particle* createParticle(const osgParticle::Particle* ptemplate) override;

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
    class InverseWorldMatrix : public SceneUtil::NodeCallback<InverseWorldMatrix, osg::MatrixTransform*>
    {
    public:
        InverseWorldMatrix() {}
        InverseWorldMatrix(const InverseWorldMatrix& copy, const osg::CopyOp& copyop)
            : osg::Object(copy, copyop)
            , SceneUtil::NodeCallback<InverseWorldMatrix, osg::MatrixTransform*>(copy, copyop)
        {
        }

        META_Object(NifOsg, InverseWorldMatrix)

        void operator()(osg::MatrixTransform* node, osg::NodeVisitor* nv);
    };

    class ParticleShooter : public osgParticle::Shooter
    {
    public:
        ParticleShooter(float minSpeed, float maxSpeed, float horizontalDir, float horizontalAngle, float verticalDir,
            float verticalAngle, float lifetime, float lifetimeRandom);
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
        PlanarCollider() = default;
        PlanarCollider(const PlanarCollider& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, PlanarCollider)

        void beginOperate(osgParticle::Program* program) override;
        void operate(osgParticle::Particle* particle, double dt) override;

    private:
        float mBounceFactor{ 0.f };
        osg::Vec2f mExtents;
        osg::Vec3f mPosition, mPositionInParticleSpace;
        osg::Vec3f mXVector, mXVectorInParticleSpace;
        osg::Vec3f mYVector, mYVectorInParticleSpace;
        osg::Plane mPlane, mPlaneInParticleSpace;
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
        GravityAffector() = default;
        GravityAffector(const GravityAffector& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        GravityAffector& operator=(const GravityAffector&) = delete;

        META_Object(NifOsg, GravityAffector)

        void operate(osgParticle::Particle* particle, double dt) override;
        void beginOperate(osgParticle::Program*) override;

    private:
        float mForce{ 0.f };
        Nif::ForceType mType{ Nif::ForceType::Wind };
        osg::Vec3f mPosition;
        osg::Vec3f mDirection;
        float mDecay{ 0.f };
        osg::Vec3f mCachedWorldPosition;
        osg::Vec3f mCachedWorldDirection;
    };

    class ParticleBomb : public osgParticle::Operator
    {
    public:
        ParticleBomb(const Nif::NiParticleBomb* bomb);
        ParticleBomb() = default;
        ParticleBomb(const ParticleBomb& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        ParticleBomb& operator=(const ParticleBomb&) = delete;

        META_Object(NifOsg, ParticleBomb)

        void operate(osgParticle::Particle* particle, double dt) override;
        void beginOperate(osgParticle::Program*) override;

    private:
        float mRange{ 0.f };
        float mStrength{ 0.f };
        Nif::DecayType mDecayType{ Nif::DecayType::None };
        Nif::SymmetryType mSymmetryType{ Nif::SymmetryType::Spherical };
        osg::Vec3f mPosition;
        osg::Vec3f mDirection;
        osg::Vec3f mCachedWorldPosition;
        osg::Vec3f mCachedWorldDirection;
    };

    // NodeVisitor to find a Group node with the given record index, stored in the node's user data container.
    // Alternatively, returns the node's parent Group if that node is not a Group (i.e. a leaf node).
    class FindGroupByRecordIndex : public osg::NodeVisitor
    {
    public:
        FindGroupByRecordIndex(unsigned int recordIndex);

        void apply(osg::Node& node) override;

        // Technically not required as the default implementation would trickle down to apply(Node&) anyway,
        // but we'll shortcut instead to avoid the chain of virtual function calls
        void apply(osg::MatrixTransform& node) override;
        void apply(osg::Geometry& node) override;

        void applyNode(osg::Node& searchNode);

        osg::Group* mFound;
        osg::NodePath mFoundPath;

    private:
        unsigned int mRecordIndex;
    };

    // Subclass emitter to support randomly choosing one of the child node's transforms for the emit position of new
    // particles.
    class Emitter : public osgParticle::Emitter
    {
    public:
        Emitter(const std::vector<int>& targets);
        Emitter();
        Emitter(const Emitter& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, Emitter)

        void emitParticles(double dt) override;

        void setShooter(osgParticle::Shooter* shooter) { mShooter = shooter; }
        void setPlacer(osgParticle::Placer* placer) { mPlacer = placer; }
        void setCounter(osgParticle::Counter* counter) { mCounter = counter; }
        void setGeometryEmitterTarget(std::optional<int> recordIndex) { mGeometryEmitterTarget = recordIndex; }
        void setFlags(int flags) { mFlags = flags; }

    private:
        // NIF Record indices
        std::vector<int> mTargets;

        osg::ref_ptr<osgParticle::Placer> mPlacer;
        osg::ref_ptr<osgParticle::Shooter> mShooter;
        osg::ref_ptr<osgParticle::Counter> mCounter;

        int mFlags;

        std::optional<int> mGeometryEmitterTarget;
        osg::observer_ptr<osg::Vec3Array> mCachedGeometryEmitter;
    };

}

#endif
