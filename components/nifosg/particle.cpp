#include "particle.hpp"

#include <limits>

#include <osg/MatrixTransform>

#include <components/nif/controlled.hpp>

#include <osg/io_utils>

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

        osg::MatrixTransform* trans = dynamic_cast<osg::MatrixTransform*>(node);

        osg::Matrix worldMat = osg::computeLocalToWorld( path );
        trans->setMatrix(osg::Matrix::inverse(worldMat));
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

ParticleShooter::ParticleShooter(const osgParticle::Shooter &copy, const osg::CopyOp &copyop)
    : osgParticle::Shooter(copy, copyop)
{
    *this = copy;
}

void ParticleShooter::shoot(osgParticle::Particle *particle) const
{
    float hdir = mHorizontalDir + mHorizontalAngle * (2.f * (std::rand() / static_cast<double>(RAND_MAX)) - 1.f);
    float vdir = mVerticalDir + mVerticalAngle * (2.f * (std::rand() / static_cast<double>(RAND_MAX)) - 1.f);
    float vdir2 = mVerticalDir + mVerticalAngle * (2.f * (std::rand() / static_cast<double>(RAND_MAX)) - 1.f);
    osg::Vec3f dir = osg::Quat(hdir, osg::Vec3f(0,0,1)) * osg::Quat(vdir, osg::Vec3f(0,1,0)) * osg::Quat(vdir2, osg::Vec3f(1,0,0))
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
    osg::Vec4f color = interpKey(mData.mKeyMap.mKeys, time, osg::Vec4f(1,1,1,1));

    particle->setColorRange(osgParticle::rangev4(color, color));
}

GravityAffector::GravityAffector(const Nif::NiGravity *gravity)
    : mForce(gravity->mForce)
    , mType(static_cast<ForceType>(gravity->mType))
    , mPosition(gravity->mPosition)
    , mDirection(gravity->mDirection)
{
}

GravityAffector::GravityAffector()
    : mForce(0), mType(Type_Wind)
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
    if (mType == Type_Wind)
        mCachedWorldPositionDirection = absolute ? program->rotateLocalToWorld(mDirection) : mDirection;
    else // Type_Point
        mCachedWorldPositionDirection = absolute ? program->transformLocalToWorld(mPosition) : mPosition;
}

void GravityAffector::operate(osgParticle::Particle *particle, double dt)
{
    switch (mType)
    {
        case Type_Wind:
            particle->addVelocity(mCachedWorldPositionDirection * mForce * dt);
            break;
        case Type_Point:
        {
            osg::Vec3f diff = mCachedWorldPositionDirection - particle->getPosition();
            diff.normalize();
            particle->addVelocity(diff * mForce * dt);
            break;
        }
    }
}

}
