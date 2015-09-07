#include "containerstate.hpp"

void ESM::ContainerState::load (ESMReader &esm)
{
    ObjectState::load (esm);

    mInventory.load (esm);
}

void ESM::ContainerState::save (ESMWriter &esm, bool inInventory) const
{
    ObjectState::save (esm, inInventory);

    mInventory.save (esm);
}
