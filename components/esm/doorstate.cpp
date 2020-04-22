#include "doorstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/debug/debuglog.hpp>

namespace ESM
{

    void DoorState::load(ESMReader &esm)
    {
        ObjectState::load(esm);

        mDoorState = 0;
        esm.getHNOT (mDoorState, "ANIM");
        if (mDoorState < 0 || mDoorState > 2)
            Log(Debug::Warning) << "Dropping invalid door state (" << mDoorState << ") for door \"" << mRef.mRefID << "\"";
    }

    void DoorState::save(ESMWriter &esm, bool inInventory) const
    {
        ObjectState::save(esm, inInventory);

        if (mDoorState < 0 || mDoorState > 2)
        {
            Log(Debug::Warning) << "Dropping invalid door state (" << mDoorState << ") for door \"" << mRef.mRefID << "\"";
            return;
        }

        if (mDoorState != 0)
            esm.writeHNT ("ANIM", mDoorState);
    }

}
