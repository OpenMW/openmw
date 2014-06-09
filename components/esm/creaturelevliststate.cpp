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

        mSpawn = false;
        esm.getHNOT (mSpawn, "RESP");
    }

    void CreatureLevListState::save(ESMWriter &esm, bool inInventory) const
    {
        ObjectState::save(esm, inInventory);

        if (mSpawnActorId != -1)
            esm.writeHNT ("SPAW", mSpawnActorId);

        if (mSpawn)
            esm.writeHNT ("RESP", mSpawn);
    }

}
