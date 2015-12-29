#include "idtablebase.hpp"

CSMWorld::IdTableBase::IdTableBase (unsigned int features) : mFeatures (features) {}

unsigned int CSMWorld::IdTableBase::getFeatures() const
{
    return mFeatures;
}
