#include "importacdt.hpp"

#include <components/esm/esmreader.hpp>

#include <components/esm/cellref.hpp>

namespace ESSImport
{

    void ActorData::load(ESM::ESMReader &esm)
    {
        if (esm.isNextSub("ACTN"))
        {
            /*
            Activation flags:
            ActivationFlag_UseEnabled  = 1
            ActivationFlag_OnActivate  = 2
            ActivationFlag_OnDeath  = 10h
            ActivationFlag_OnKnockout  = 20h
            ActivationFlag_OnMurder  = 40h
            ActivationFlag_DoorOpening  = 100h
            ActivationFlag_DoorClosing  = 200h
            ActivationFlag_DoorJammedOpening  = 400h
            ActivationFlag_DoorJammedClosing  = 800h
            */
            esm.skipHSub();
        }

        if (esm.isNextSub("STPR"))
            esm.skipHSub();

        if (esm.isNextSub("MNAM"))
           esm.skipHSub();

        bool isDeleted = false;
        ESM::CellRef::loadData(esm, isDeleted);

        mHasACDT = false;
        if (esm.isNextSub("ACDT"))
        {
            mHasACDT = true;
            esm.getHT(mACDT);
        }

        mHasACSC = false;
        if (esm.isNextSub("ACSC"))
        {
            mHasACSC = true;
            esm.getHT(mACSC);
        }

        if (esm.isNextSub("ACSL"))
            esm.skipHSubSize(112);

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
            // occurred when a creature was in the middle of its attack, 44 bytes
            esm.skipHSub();
        }

        // unsure at which point between FGTN and CHRD
        if (esm.isNextSub("PWPC"))
            esm.skipHSub();
        if (esm.isNextSub("PWPS"))
            esm.skipHSub();

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

        while (esm.isNextSub("APUD"))
        {
            // used power
            esm.getSubHeader();
            std::string id = esm.getString(32);
            (void)id;
            // timestamp can't be used: this is the total hours passed, calculated by
            // timestamp = 24 * (365 * year + cumulativeDays[month] + day)
            // unfortunately cumulativeDays[month] is not clearly defined,
            // in the (non-MCP) vanilla version the first month was missing, but MCP added it.
            double timestamp;
            esm.getT(timestamp);
        }

        // FIXME: not all actors have this, add flag
        if (esm.isNextSub("CHRD")) // npc only
                esm.getHExact(mSkills, 27*2*sizeof(int));

        if (esm.isNextSub("CRED")) // creature only
            esm.getHExact(mCombatStats, 3*2*sizeof(int));

        mSCRI.load(esm);

        if (esm.isNextSub("ND3D"))
            esm.skipHSub();

        mHasANIS = false;
        if (esm.isNextSub("ANIS"))
        {
            mHasANIS = true;
            esm.getHT(mANIS);
        }
    }

}
