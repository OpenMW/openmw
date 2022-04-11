#include "containerstate.hpp"

namespace ESM
{

void ContainerState::load (ESMReader &esm)
{
    ObjectState::load (esm);

    mInventory.load (esm);
}

void ContainerState::save (ESMWriter &esm, bool inInventory) const
{
    ObjectState::save (esm, inInventory);

    mInventory.save (esm);
}

}
