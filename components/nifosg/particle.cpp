#include "particle.hpp"

#include <limits>

#include <osg/MatrixTransform>
#include <osg/Geometry>

#include <components/debug/debuglog.hpp>
#include <components/misc/rng.hpp>
#include <components/nif/controlled.hpp>
#include <components/nif/data.hpp>

#include "userdata.hpp"

namespace NifOsg
{

ParticleSystem::ParticleSystem()
    : osgParticle::ParticleSystem()
    , mQuota(std::numeric_limits<int>::max())
{
}

ParticleSystem::ParticleSystem(const ParticleSystem &copy, const osg::CopyOp &copyop)
    : osgParticle::ParticleSystem(copy, copyop)
    , mQuota(copy.mQuota)
{
    // For some reason the osgParticle constructor doesn't copy the particles
    for (int i=0;i<copy.numParticles()-copy.numDeadParticles();++i)
        ParticleSystem::createParticle(copy.getParticle(i));
}

void ParticleSystem::setQuota(int quota)
{
    mQuota = quota;
}

osgParticle::Particle* ParticleSystem::createParticle(const osgParticle::Particle *ptemplate)
{
    if (numParticles()-numDeadParticles() < mQuota)
        return osgParticle::ParticleSystem::createParticle(ptemplate);
    return nullptr;
}

void InverseWorldMatrix::operator()(osg::Node *node, osg::NodeVisitor *nv)
{
    if (nv && nv->getVisitorType() == osg::NodeVisitor::UPDATE_VISITOR)
    {
        osg::NodePath path = nv->getNodePath();
        path.pop_back();

        osg::MatrixTransform* trans = static_cast<osg::MatrixTransform*>(node);

        osg::Matrix mat = osg::computeLocalToWorld( path );
        mat.orthoNormalize(mat); // don't undo the scale
        mat.invert(mat);
        trans->setMatrix(mat);
    }
    traverse(node,nv);
}

ParticleShooter::ParticleShooter(float minSpeed, float maxSpeed, float horizontalDir, float horizontalAngle, float verticalDir, float verticalAngle, float lifetime, float lifetimeRandom)
    : mMinSpeed(minSpeed), mMaxSpeed(maxSpeed), mHorizontalDir(horizontalDir)
    , mHorizontalAngle(horizontalAngle), mVerticalDir(verticalDir), mVerticalAngle(verticalAngle)
    , mLifetime(lifetime), mLifetimeRandom(lifetimeRandom)
{
}

ParticleShooter::ParticleShooter()
    : mMinSpeed(0.f), mMaxSpeed(0.f), mHorizontalDir(0.f)
    , mHorizontalAngle(0.f), mVerticalDir(0.f), mVerticalAngle(0.f)
    , mLifetime(0.f), mLifetimeRandom(0.f)
{
}

ParticleShooter::ParticleShooter(const ParticleShooter &copy, const osg::CopyOp &copyop)
    : osgParticle::Shooter(copy, copyop)
{
    mMinSpeed = copy.mMinSpeed;
    mMaxSpeed = copy.mMaxSpeed;
    mHorizontalDir = copy.mHorizontalDir;
    mHorizontalAngle = copy.mHorizontalAngle;
    mVerticalDir = copy.mVerticalDir;
    mVerticalAngle = copy.mVerticalAngle;
    mLifetime = copy.mLifetime;
    mLifetimeRandom = copy.mLifetimeRandom;
}

void ParticleShooter::shoot(osgParticle::Particle *particle) const
{
    float hdir = mHorizontalDir + mHorizontalAngle * (2.f * Misc::Rng::rollClosedProbability() - 1.f);
    float vdir = mVerticalDir + mVerticalAngle * (2.f * Misc::Rng::rollClosedProbability() - 1.f);

    osg::Vec3f dir = (osg::Quat(vdir, osg::Vec3f(0,1,0)) * osg::Quat(hdir, osg::Vec3f(0,0,1)))
             * osg::Vec3f(0,0,1);

    float vel = mMinSpeed + (mMaxSpeed - mMinSpeed) * Misc::Rng::rollClosedProbability();
    particle->setVelocity(dir * vel);

    // Not supposed to set this here, but there doesn't seem to be a better way of doing it
    particle->setLifeTime(mLifetime + mLifetimeRandom * Misc::Rng::rollClosedProbability());
}

GrowFadeAffector::GrowFadeAffector(float growTime, float fadeTime)
    : mGrowTime(growTime)
    , mFadeTime(fadeTime)
    , mCachedDefaultSize(0.f)
{
}

GrowFadeAffector::GrowFadeAffector()
    : mGrowTime(0.f)
    , mFadeTime(0.f)
    , mCachedDefaultSize(0.f)
{

}

GrowFadeAffector::GrowFadeAffector(const GrowFadeAffector& copy, const osg::CopyOp& copyop)
    : osgParticle::Operator(copy, copyop)
{
    mGrowTime = copy.mGrowTime;
    mFadeTime = copy.mFadeTime;
    mCachedDefaultSize = copy.mCachedDefaultSize;
}

void GrowFadeAffector::beginOperate(osgParticle::Program *program)
{
    mCachedDefaultSize = program->getParticleSystem()->getDefaultParticleTemplate().getSizeRange().minimum;
}

void GrowFadeAffector::operate(osgParticle::Particle* particle, double /* dt */)
{
    float size = mCachedDefaultSize;
    if (particle->getAge() < mGrowTime && mGrowTime != 0.f)
        size *= particle->getAge() / mGrowTime;
    if (particle->getLifeTime() - particle->getAge() < mFadeTime && mFadeTime != 0.f)
        size *= (particle->getLifeTime() - particle->getAge()) / mFadeTime;
    particle->setSizeRange(osgParticle::rangef(size, size));
}

ParticleColorAffector::ParticleColorAffector(const Nif::NiColorData *clrdata)
    : mData(clrdata->mKeyMap, osg::Vec4f(1,1,1,1))
{
}

ParticleColorAffector::ParticleColorAffector()
{

}

ParticleColorAffector::ParticleColorAffector(const ParticleColorAffector &copy, const osg::CopyOp &copyop)
    : osgParticle::Operator(copy, copyop)
{
    mData = copy.mData;
}

void ParticleColorAffector::operate(osgParticle::Particle* particle, double /* dt */)
{
    float time = static_cast<float>(particle->getAge()/particle->getLifeTime());
    osg::Vec4f color = mData.interpKey(time);

    particle->setColorRange(osgParticle::rangev4(color, color));
}

GravityAffector::GravityAffector(const Nif::NiGravity *gravity)
    : mForce(gravity->mForce)
    , mType(static_cast<ForceType>(gravity->mType))
    , mPosition(gravity->mPosition)
    , mDirection(gravity->mDirection)
    , mDecay(gravity->mDecay)
{
}

GravityAffector::GravityAffector()
    : mForce(0), mType(Type_Wind), mDecay(0.f)
{

}

GravityAffector::GravityAffector(const GravityAffector &copy, const osg::CopyOp &copyop)
    : osgParticle::Operator(copy, copyop)
{
    mForce = copy.mForce;
    mType = copy.mType;
    mPosition = copy.mPosition;
    mDirection = copy.mDirection;
    mDecay = copy.mDecay;
    mCachedWorldPosition = copy.mCachedWorldPosition;
    mCachedWorldDirection = copy.mCachedWorldDirection;
}

void GravityAffector::beginOperate(osgParticle::Program* program)
{
    bool absolute = (program->getReferenceFrame() == osgParticle::ParticleProcessor::ABSOLUTE_RF);

    if (mType == Type_Point || mDecay != 0.f) // we don't need the position for Wind gravity, except if decay is being applied
        mCachedWorldPosition = absolute ? program->transformLocalToWorld(mPosition) : mPosition;

    mCachedWorldDirection = absolute ? program->rotateLocalToWorld(mDirection) : mDirection;
    mCachedWorldDirection.normalize();
}

void GravityAffector::operate(osgParticle::Particle *particle, double dt)
{
    const float magic = 1.6f;
    switch (mType)
    {
        case Type_Wind:
        {
            float decayFactor = 1.f;
            if (mDecay != 0.f)
            {
                osg::Plane gravityPlane(mCachedWorldDirection, mCachedWorldPosition);
                float distance = std::abs(gravityPlane.distance(particle->getPosition()));
                decayFactor = std::exp(-1.f * mDecay * distance);
            }

            particle->addVelocity(mCachedWorldDirection * mForce * dt * decayFactor * magic);

            break;
        }
        case Type_Point:
        {
            osg::Vec3f diff = mCachedWorldPosition - particle->getPosition();

            float decayFactor = 1.f;
            if (mDecay != 0.f)
                decayFactor = std::exp(-1.f * mDecay * diff.length());

            diff.normalize();

            particle->addVelocity(diff * mForce * dt * decayFactor * magic);
            break;
        }
    }
}

Emitter::Emitter()
    : osgParticle::Emitter()
{
}

Emitter::Emitter(const Emitter &copy, const osg::CopyOp &copyop)
    : osgParticle::Emitter(copy, copyop)
    , mTargets(copy.mTargets)
    , mPlacer(copy.mPlacer)
    , mShooter(copy.mShooter)
    // need a deep copy because the remainder is stored in the object
    , mCounter(osg::clone(copy.mCounter.get(), osg::CopyOp::DEEP_COPY_ALL))
{
}

Emitter::Emitter(const std::vector<int> &targets)
    : mTargets(targets)
{
}

void Emitter::setShooter(osgParticle::Shooter *shooter)
{
    mShooter = shooter;
}

void Emitter::setPlacer(osgParticle::Placer *placer)
{
    mPlacer = placer;
}

void Emitter::setCounter(osgParticle::Counter *counter)
{
    mCounter = counter;
}

void Emitter::emitParticles(double dt)
{
    int n = mCounter->numParticlesToCreate(dt);
    if (n == 0)
        return;

    osg::Matrix worldToPs;

    // maybe this could be optimized by halting at the lowest common ancestor of the particle and emitter nodes
    osg::NodePathList partsysNodePaths = getParticleSystem()->getParentalNodePaths();
    if (!partsysNodePaths.empty())
    {
        osg::Matrix psToWorld = osg::computeLocalToWorld(partsysNodePaths[0]);
        worldToPs = osg::Matrix::inverse(psToWorld);
    }

    const osg::Matrix& ltw = getLocalToWorldMatrix();
    osg::Matrix emitterToPs = ltw * worldToPs;

    if (!mTargets.empty())
    {
        int randomIndex = Misc::Rng::rollClosedProbability() * (mTargets.size() - 1);
        int randomRecIndex = mTargets[randomIndex];

        // we could use a map here for faster lookup
        FindGroupByRecIndex visitor(randomRecIndex);
        getParent(0)->accept(visitor);

        if (!visitor.mFound)
        {
            Log(Debug::Info) << "Can't find emitter node" << randomRecIndex;
            return;
        }

        osg::NodePath path = visitor.mFoundPath;
        path.erase(path.begin());
        emitterToPs = osg::computeLocalToWorld(path) * emitterToPs;
    }

    emitterToPs.orthoNormalize(emitterToPs);

    for (int i=0; i<n; ++i)
    {
        osgParticle::Particle* P = getParticleSystem()->createParticle(0);
        if (P)
        {
            mPlacer->place(P);

            mShooter->shoot(P);

            P->transformPositionVelocity(emitterToPs);
        }
    }
}

FindGroupByRecIndex::FindGroupByRecIndex(int recIndex)
    : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
    , mFound(nullptr)
    , mRecIndex(recIndex)
{
}

void FindGroupByRecIndex::apply(osg::Node &node)
{
    applyNode(node);
}

void FindGroupByRecIndex::apply(osg::MatrixTransform &node)
{
    applyNode(node);
}

void FindGroupByRecIndex::apply(osg::Geometry &node)
{
    applyNode(node);
}

void FindGroupByRecIndex::applyNode(osg::Node &searchNode)
{
    if (searchNode.getUserDataContainer() && searchNode.getUserDataContainer()->getNumUserObjects())
    {
        NodeUserData* holder = dynamic_cast<NodeUserData*>(searchNode.getUserDataContainer()->getUserObject(0));
        if (holder && holder->mIndex == mRecIndex)
        {
            osg::Group* group = searchNode.asGroup();
            if (!group)
                group = searchNode.getParent(0);

            mFound = group;
            mFoundPath = getNodePath();
            return;
        }
    }
    traverse(searchNode);
}

PlanarCollider::PlanarCollider(const Nif::NiPlanarCollider *collider)
    : mBounceFactor(collider->mBounceFactor)
    , mPlane(-collider->mPlaneNormal, collider->mPlaneDistance)
{
}

PlanarCollider::PlanarCollider()
    : mBounceFactor(0.f)
{
}

PlanarCollider::PlanarCollider(const PlanarCollider &copy, const osg::CopyOp &copyop)
    : osgParticle::Operator(copy, copyop)
    , mBounceFactor(copy.mBounceFactor)
    , mPlane(copy.mPlane)
    , mPlaneInParticleSpace(copy.mPlaneInParticleSpace)
{
}

void PlanarCollider::beginOperate(osgParticle::Program *program)
{
    mPlaneInParticleSpace = mPlane;
    if (program->getReferenceFrame() == osgParticle::ParticleProcessor::ABSOLUTE_RF)
        mPlaneInParticleSpace.transform(program->getLocalToWorldMatrix());
}

void PlanarCollider::operate(osgParticle::Particle *particle, double dt)
{
    float dotproduct = particle->getVelocity() * mPlaneInParticleSpace.getNormal();

    if (dotproduct > 0)
    {
        osg::BoundingSphere bs(particle->getPosition(), 0.f);
        if (mPlaneInParticleSpace.intersect(bs) == 1)
        {
            osg::Vec3 reflectedVelocity = particle->getVelocity() - mPlaneInParticleSpace.getNormal() * (2 * dotproduct);
            reflectedVelocity *= mBounceFactor;
            particle->setVelocity(reflectedVelocity);
        }
    }
}

SphericalCollider::SphericalCollider(const Nif::NiSphericalCollider* collider)
    : mBounceFactor(collider->mBounceFactor),
      mSphere(collider->mCenter, collider->mRadius)
{
}

SphericalCollider::SphericalCollider()
    : mBounceFactor(1.0f)
{

}

SphericalCollider::SphericalCollider(const SphericalCollider& copy, const osg::CopyOp& copyop)
    : osgParticle::Operator(copy, copyop)
    , mBounceFactor(copy.mBounceFactor)
    , mSphere(copy.mSphere)
    , mSphereInParticleSpace(copy.mSphereInParticleSpace)
{

}

void SphericalCollider::beginOperate(osgParticle::Program* program)
{
    mSphereInParticleSpace = mSphere;
    if (program->getReferenceFrame() == osgParticle::ParticleProcessor::ABSOLUTE_RF)
        mSphereInParticleSpace.center() = program->transformLocalToWorld(mSphereInParticleSpace.center());
}

void SphericalCollider::operate(osgParticle::Particle* particle, double dt)
{
    osg::Vec3f cent = (particle->getPosition() - mSphereInParticleSpace.center()); // vector from sphere center to particle

    bool insideSphere = cent.length2() <= mSphereInParticleSpace.radius2();

    if (insideSphere
            || (cent * particle->getVelocity() < 0.0f)) // if outside, make sure the particle is flying towards the sphere
    {
        // Collision test (finding point of contact) is performed by solving a quadratic equation:
        // ||vec(cent) + vec(vel)*k|| = R      /^2
        // k^2 + 2*k*(vec(cent)*vec(vel))/||vec(vel)||^2 + (||vec(cent)||^2 - R^2)/||vec(vel)||^2 = 0

        float b = -(cent * particle->getVelocity()) / particle->getVelocity().length2();

        osg::Vec3f u = cent + particle->getVelocity() * b;

        if (insideSphere
                || (u.length2() < mSphereInParticleSpace.radius2()))
        {
            float d = (mSphereInParticleSpace.radius2() - u.length2()) / particle->getVelocity().length2();
            float k = insideSphere ? (std::sqrt(d) + b) : (b - std::sqrt(d));

            if (k < dt)
            {
                // collision detected; reflect off the tangent plane
                osg::Vec3f contact = particle->getPosition() + particle->getVelocity() * k;

                osg::Vec3 normal = (contact - mSphereInParticleSpace.center());
                normal.normalize();

                float dotproduct = particle->getVelocity() * normal;

                osg::Vec3 reflectedVelocity = particle->getVelocity() - normal * (2 * dotproduct);
                reflectedVelocity *= mBounceFactor;
                particle->setVelocity(reflectedVelocity);
            }
        }
    }
}

}
