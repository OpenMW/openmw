#include "creaturelevliststate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void CreatureLevListState::load(ESMReader &esm)
    {
        ObjectState::load(esm);

        mSpawnActorId = -1;
        esm.getHNOT (mSpawnActorId, "SPAW");
    }

    void CreatureLevListState::save(ESMWriter &esm, bool inInventory) const
    {
        ObjectState::save(esm, inInventory);

        if (mSpawnActorId != -1)
            esm.writeHNT ("SPAW", mSpawnActorId);
    }

}
