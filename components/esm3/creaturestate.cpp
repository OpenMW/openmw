#include "creaturestate.hpp"

void ESM::CreatureState::load (ESMReader &esm)
{
    ObjectState::load (esm);

    if (mHasCustomState)
    {
        mInventory.load (esm);

        mCreatureStats.load (esm);
    }
}

void ESM::CreatureState::save (ESMWriter &esm, bool inInventory) const
{
    ObjectState::save (esm, inInventory);

    if (mHasCustomState)
    {
        mInventory.save (esm);

        mCreatureStats.save (esm);
    }
}

void ESM::CreatureState::blank()
{
    ObjectState::blank();
    mCreatureStats.blank();
}
