#include "creaturelevliststate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void CreatureLevListState::load(ESMReader& esm)
    {
        ObjectState::load(esm);

        if (esm.getFormatVersion() <= MaxActorIdSaveGameFormatVersion)
        {
            mSpawnedActor.mIndex = -1;
            esm.getHNOT(mSpawnedActor.mIndex, "SPAW");
        }
        else if (esm.peekNextSub("SPAW"))
            esm.getFormId(true, "SPAW");

        mSpawn = false;
        esm.getHNOT(mSpawn, "RESP");
    }

    void CreatureLevListState::save(ESMWriter& esm, bool inInventory) const
    {
        ObjectState::save(esm, inInventory);

        if (mSpawnedActor.isSet())
            esm.writeFormId(mSpawnedActor, true, "SPAW");

        if (mSpawn)
            esm.writeHNT("RESP", mSpawn);
    }

}
