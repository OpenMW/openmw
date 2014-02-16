
#include "creaturestate.hpp"

void ESM::CreatureState::load (ESMReader &esm)
{
    ObjectState::load (esm);

    mInventory.load (esm);

    mCreatureStats.load (esm);
}

void ESM::CreatureState::save (ESMWriter &esm, bool inInventory) const
{
    ObjectState::save (esm, inInventory);

    mInventory.save (esm);

    mCreatureStats.save (esm);
}