#include "npcstate.hpp"

namespace ESM
{

void NpcState::load (ESMReader &esm)
{
    ObjectState::load (esm);

    if (mHasCustomState)
    {
        mInventory.load (esm);

        mNpcStats.load (esm);

        mCreatureStats.load (esm);
    }
}

void NpcState::save (ESMWriter &esm, bool inInventory) const
{
    ObjectState::save (esm, inInventory);

    if (mHasCustomState)
    {
        mInventory.save (esm);

        mNpcStats.save (esm);

        mCreatureStats.save (esm);
    }
}

void NpcState::blank()
{
    ObjectState::blank();
    mNpcStats.blank();
    mCreatureStats.blank();
    mHasCustomState = true;
}

}
