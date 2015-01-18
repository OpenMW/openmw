#include "importacdt.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

    void ActorData::load(ESM::ESMReader &esm)
    {
        esm.getHNT(mACDT, "ACDT");

        ACSC acsc;
        esm.getHNOT(acsc, "ACSC");
        esm.getHNOT(acsc, "ACSL");

        if (esm.isNextSub("CHRD")) // npc only
                esm.getHExact(mSkills, 27*2*sizeof(int));

        if (esm.isNextSub("CRED")) // creature only
            esm.getHExact(mCombatStats, 3*2*sizeof(int));

        mScript = esm.getHNOString("SCRI");

        if (esm.isNextSub("ND3D"))
            esm.skipHSub();
    }

}
