#include "environment.hpp"

#include <cassert>

#include <components/resource/resourcesystem.hpp>

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
