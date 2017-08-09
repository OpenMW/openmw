#include "doorstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void DoorState::load(ESMReader &esm)
    {
        ObjectState::load(esm);

        mDoorState = 0;
        esm.getHNOT (mDoorState, "ANIM");
    }

    void DoorState::save(ESMWriter &esm, bool inInventory) const
    {
        ObjectState::save(esm, inInventory);

        if (mDoorState != 0)
            esm.writeHNT ("ANIM", mDoorState);
    }

}
