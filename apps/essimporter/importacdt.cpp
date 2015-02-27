#include "importacdt.hpp"

#include <components/esm/esmreader.hpp>

#include <components/esm/cellref.hpp>

namespace ESSImport
{

    void ActorData::load(ESM::ESMReader &esm)
    {
        if (esm.isNextSub("ACTN"))
            esm.skipHSub();

        if (esm.isNextSub("STPR"))
            esm.skipHSub();

        if (esm.isNextSub("MNAM"))
           esm.skipHSub();

        ESM::CellRef::loadData(esm);

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

        // unsure at which point between LSTN and CHRD
        if (esm.isNextSub("APUD"))
            esm.skipHSub(); // 40 bytes, starts with string "ancestor guardian". maybe spellcasting in progress?

        if (esm.isNextSub("WNAM"))
        {
            std::string id = esm.getHString();

            if (esm.isNextSub("XNAM"))
                mSelectedEnchantItem = esm.getHString();
            else
                mSelectedSpell = id;

            if (esm.isNextSub("YNAM"))
                esm.skipHSub(); // 4 byte, 0
        }

        // FIXME: not all actors have this, add flag
        if (esm.isNextSub("CHRD")) // npc only
                esm.getHExact(mSkills, 27*2*sizeof(int));

        if (esm.isNextSub("CRED")) // creature only
            esm.getHExact(mCombatStats, 3*2*sizeof(int));

        mSCRI.load(esm);

        if (esm.isNextSub("ND3D"))
            esm.skipHSub();
        if (esm.isNextSub("ANIS"))
            esm.skipHSub();
    }

}
