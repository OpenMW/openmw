
#include "lightstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::LightState::load (ESMReader &esm)
{
    ObjectState::load (esm);

    mTime = 0;
    esm.getHNOT (mTime, "LTIM");
}

void ESM::LightState::save (ESMWriter &esm, bool inInventory) const
{
    ObjectState::save (esm, inInventory);

    if (mTime)
        esm.writeHNT ("LTIM", mTime);
}