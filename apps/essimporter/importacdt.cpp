#include "importacdt.hpp"

#include <components/esm/esmreader.hpp>

#include <components/esm/cellref.hpp>

namespace ESSImport
{

    void ActorData::load(ESM::ESMReader &esm)
    {
         // unsure at which point between NAME and ESM::CellRef
        if (esm.isNextSub("MNAM"))
            esm.skipHSub();

        if (esm.isNextSub("ACTN"))
            esm.skipHSub();

        if (esm.isNextSub("STPR"))
            esm.skipHSub();

        ESM::CellRef bla;
        bla.ESM::CellRef::loadData(esm);

        // FIXME: not all actors have this, add flag
        esm.getHNOT(mACDT, "ACDT");

        ACSC acsc;
        esm.getHNOT(acsc, "ACSC");
        esm.getHNOT(acsc, "ACSL");

        if (esm.isNextSub("CSTN"))
            esm.skipHSub(); // "PlayerSaveGame", link to some object?

        if (esm.isNextSub("LSTN"))
            esm.skipHSub(); // "PlayerSaveGame", link to some object?

        // unsure at which point between LSTN and TGTN
        if (esm.isNextSub("CSHN"))
            esm.skipHSub(); // "PlayerSaveGame", link to some object?

        // unsure if before or after CSTN/LSTN
        if (esm.isNextSub("LSHN"))
            esm.skipHSub(); // "PlayerSaveGame", link to some object?

        while (esm.isNextSub("TGTN"))
            esm.skipHSub(); // "PlayerSaveGame", link to some object?

        while (esm.isNextSub("FGTN"))
            esm.getHString(); // fight target?

        // unsure at which point between TGTN and CRED
        if (esm.isNextSub("AADT"))
        {
            // occured when a creature was in the middle of its attack, 44 bytes
            esm.skipHSub();
        }

        // unsure at which point between FGTN and CHRD
        if (esm.isNextSub("PWPC"))
            esm.skipHSub();
        if (esm.isNextSub("PWPS"))
            esm.skipHSub();

        if (esm.isNextSub("WNAM"))
        {
            esm.skipHSub(); // seen values: "ancestor guardian", "bound dagger_en". Summoned creature / bound weapons?

            if (esm.isNextSub("XNAM"))
            {
                // "demon tanto", probably the ID of spell/item that created the bound weapon/crature?
                esm.skipHSub();
            }

            if (esm.isNextSub("YNAM"))
                esm.skipHSub(); // 4 byte, 0
        }

        // FIXME: not all actors have this, add flag
        if (esm.isNextSub("CHRD")) // npc only
                esm.getHExact(mSkills, 27*2*sizeof(int));

        if (esm.isNextSub("CRED")) // creature only
            esm.getHExact(mCombatStats, 3*2*sizeof(int));

        mScript = esm.getHNOString("SCRI");

        // script variables?
        if (!mScript.empty())
        {
            if (esm.isNextSub("SLCS"))
                esm.skipHSub();
            if (esm.isNextSub("SLSD")) // Short Data?
                esm.skipHSub();
            if (esm.isNextSub("SLFD")) // Float Data?
                esm.skipHSub();
        }

        if (esm.isNextSub("ND3D"))
            esm.skipHSub();
        if (esm.isNextSub("ANIS"))
            esm.skipHSub();
    }

}
