#include "environment.hpp"

#include <cassert>

#include <components/resource/resourcesystem.hpp>

#include "world.hpp"
#include "mechanicsmanager.hpp"

MWBase::Environment *MWBase::Environment::sThis = nullptr;

MWBase::Environment::Environment()
{
    assert(sThis == nullptr);
    sThis = this;
}

MWBase::Environment::~Environment()
{
    sThis = nullptr;
}

void MWBase::Environment::reportStats(unsigned int frameNumber, osg::Stats& stats) const
{
    mMechanicsManager->reportStats(frameNumber, stats);
    mWorld->reportStats(frameNumber, stats);
}
