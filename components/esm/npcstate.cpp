
#include "npcstate.hpp"

void ESM::NpcState::load (ESMReader &esm)
{
    ObjectState::load (esm);

    mInventory.load (esm);

    mNpcStats.load (esm);

    mCreatureStats.load (esm);
}

void ESM::NpcState::save (ESMWriter &esm, bool inInventory) const
{
    ObjectState::save (esm, inInventory);

    mInventory.save (esm);

    mNpcStats.save (esm);

    mCreatureStats.save (esm);
}