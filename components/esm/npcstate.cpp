#include "npcstate.hpp"

void ESM::NpcState::load (ESMReader &esm)
{
    ObjectState::load (esm);

    if (mHasCustomState)
    {
        mInventory.load (esm);

        mNpcStats.load (esm);

        mCreatureStats.load (esm);
    }
}

void ESM::NpcState::save (ESMWriter &esm, bool inInventory) const
{
    ObjectState::save (esm, inInventory);

    if (mHasCustomState)
    {
        mInventory.save (esm);

        mNpcStats.save (esm);

        mCreatureStats.save (esm);
    }
}

void ESM::NpcState::blank()
{
    ObjectState::blank();
    mNpcStats.blank();
    mCreatureStats.blank();
    mHasCustomState = true;
}
