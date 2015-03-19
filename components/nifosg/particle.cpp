#include "particle.hpp"

#include <osg/MatrixTransform>

#include <components/nif/controlled.hpp>

#include <osg/io_utils>

namespace NifOsg
{

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
{
    *this = copy;
}

void ParticleShooter::shoot(osgParticle::Particle *particle) const
{
    // NOTE: We do not use mDirection/mAngle for the initial direction.
    float hdir = mHorizontalDir + mHorizontalAngle * (2.f * (std::rand() / static_cast<double>(RAND_MAX)) - 1.f);
    float vdir = mVerticalDir + mVerticalAngle * (2.f * (std::rand() / static_cast<double>(RAND_MAX)) - 1.f);
    osg::Vec3f dir = osg::Quat(hdir, osg::Vec3f(0,0,1)) * osg::Quat(vdir, osg::Vec3f(1,0,0))
                                                                        // ^ Vec3f(0,1,0) according to nifskope, TODO: test in mw
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

osg::Vec4f ParticleColorAffector::interpolate(const float time, const Nif::Vector4KeyMap::MapType &keys)
{
    if(time <= keys.begin()->first)
        return keys.begin()->second.mValue;

    Nif::Vector4KeyMap::MapType::const_iterator it = keys.lower_bound(time);
    if (it != keys.end())
    {
        float aTime = it->first;
        const Nif::KeyT<osg::Vec4f>* aKey = &it->second;

        assert (it != keys.begin()); // Shouldn't happen, was checked at beginning of this function

        Nif::Vector4KeyMap::MapType::const_iterator last = --it;
        float aLastTime = last->first;
        const Nif::KeyT<osg::Vec4f>* aLastKey = &last->second;

        float a = (time - aLastTime) / (aTime - aLastTime);
        return aLastKey->mValue + ((aKey->mValue - aLastKey->mValue) * a);
    }
    else
        return keys.rbegin()->second.mValue;
}

void ParticleColorAffector::operate(osgParticle::Particle* particle, double /* dt */)
{
    float time = static_cast<float>(particle->getAge()/particle->getLifeTime());
    osg::Vec4f color = interpolate(time, mData.mKeyMap.mKeys);

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
