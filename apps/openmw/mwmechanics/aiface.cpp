#include "aiface.hpp"

#include "../mwworld/ptr.hpp"

#include "steering.hpp"

MWMechanics::AiFace::AiFace(float targetX, float targetY)
    : mTargetX(targetX), mTargetY(targetY)
{
}

MWMechanics::AiPackage *MWMechanics::AiFace::clone() const
{
    return new AiFace(*this);
}

bool MWMechanics::AiFace::execute(const MWWorld::Ptr& actor, MWMechanics::CharacterController& /*characterController*/, MWMechanics::AiState& /*state*/, float /*duration*/)
{
    osg::Vec3f dir = osg::Vec3f(mTargetX, mTargetY, 0) - actor.getRefData().getPosition().asVec3();
    return zTurn(actor, std::atan2(dir.x(), dir.y()), osg::DegreesToRadians(3.f));
}

int MWMechanics::AiFace::getTypeId() const
{
    return AiPackage::TypeIdFace;
}

unsigned int MWMechanics::AiFace::getPriority() const
{
    return 2;
}
