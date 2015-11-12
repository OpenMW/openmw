#include "particle.hpp"

#include <limits>

#include <osg/MatrixTransform>

#include <components/nif/controlled.hpp>

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
        _particles.push_back(*copy.getParticle(i));
}

void ParticleSystem::setQuota(int quota)
{
    mQuota = quota;
}

osgParticle::Particle* ParticleSystem::createParticle(const osgParticle::Particle *ptemplate)
{
    if (numParticles()-numDeadParticles() < mQuota)
        return osgParticle::ParticleSystem::createParticle(ptemplate);
    return NULL;
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
        mat = osg::Matrix::inverse(mat);
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
    *this = copy;
}

void ParticleShooter::shoot(osgParticle::Particle *particle) const
{
    float hdir = mHorizontalDir + mHorizontalAngle * (2.f * (std::rand() / static_cast<double>(RAND_MAX)) - 1.f);
    float vdir = mVerticalDir + mVerticalAngle * (2.f * (std::rand() / static_cast<double>(RAND_MAX)) - 1.f);

    osg::Vec3f dir = (osg::Quat(vdir, osg::Vec3f(0,1,0)) * osg::Quat(hdir, osg::Vec3f(0,0,1)))
             * osg::Vec3f(0,0,1);

    float vel = mMinSpeed + (mMaxSpeed - mMinSpeed) * std::rand() / static_cast<float>(RAND_MAX);
    particle->setVelocity(dir * vel);

    // Not supposed to set this here, but there doesn't seem to be a better way of doing it
    particle->setLifeTime(mLifetime + mLifetimeRandom * std::rand() / static_cast<float>(RAND_MAX));
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
    *this = copy;
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
    : mData(*clrdata)
{
}

ParticleColorAffector::ParticleColorAffector()
{

}

ParticleColorAffector::ParticleColorAffector(const ParticleColorAffector &copy, const osg::CopyOp &copyop)
    : osgParticle::Operator(copy, copyop)
{
    *this = copy;
}

void ParticleColorAffector::operate(osgParticle::Particle* particle, double /* dt */)
{
    float time = static_cast<float>(particle->getAge()/particle->getLifeTime());
    osg::Vec4f color = interpKey(mData.mKeyMap->mKeys, time, osg::Vec4f(1,1,1,1));

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
    *this = copy;
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
    osg::MatrixList worldMats = getParticleSystem()->getWorldMatrices();
    if (!worldMats.empty())
    {
        osg::Matrix psToWorld = worldMats[0];
        // ignore scales in particlesystem world matrix. this seems wrong, but have to do so for MW compatibility.
        psToWorld.orthoNormalize(psToWorld);
        worldToPs = osg::Matrix::inverse(psToWorld);
    }

    const osg::Matrix& ltw = getLocalToWorldMatrix();
    osg::Matrix emitterToPs = ltw * worldToPs;

    if (!mTargets.empty())
    {
        int randomRecIndex = mTargets[(std::rand() / (static_cast<double>(RAND_MAX)+1.0)) * mTargets.size()];

        // we could use a map here for faster lookup
        FindRecIndexVisitor visitor(randomRecIndex);
        getParent(0)->accept(visitor);

        if (!visitor.mFound)
        {
            std::cerr << "Emitter: Can't find emitter node" << randomRecIndex << std::endl;
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

FindRecIndexVisitor::FindRecIndexVisitor(int recIndex)
    : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
    , mFound(NULL)
    , mRecIndex(recIndex)
{
}

void FindRecIndexVisitor::apply(osg::Node &searchNode)
{
    if (searchNode.getUserDataContainer() && searchNode.getUserDataContainer()->getNumUserObjects())
    {
        NodeUserData* holder = dynamic_cast<NodeUserData*>(searchNode.getUserDataContainer()->getUserObject(0));
        if (holder && holder->mIndex == mRecIndex)
        {
            mFound = static_cast<osg::Group*>(&searchNode);
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

}
