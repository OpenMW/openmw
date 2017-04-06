#include <components/openmw-mp/Log.hpp>

#include "LocalActor.hpp"

using namespace mwmp;
using namespace std;

LocalActor::LocalActor()
{

}

LocalActor::~LocalActor()
{

}

void LocalActor::update()
{

}

MWWorld::Ptr LocalActor::getPtr()
{
    return ptr;
}

void LocalActor::setPtr(const MWWorld::Ptr& newPtr)
{
    ptr = newPtr;
}
