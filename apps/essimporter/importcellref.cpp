#include "importcellref.hpp"

#include <components/esm3/esmreader.hpp>

namespace ESSImport
{

    void CellRef::load(ESM::ESMReader &esm)
    {
        blank();

        esm.getHNT(mRefNum.mIndex, "FRMR");

        // this is required since openmw supports more than 255 content files
        int pluginIndex = (mRefNum.mIndex & 0xff000000) >> 24;
        mRefNum.mContentFile = pluginIndex-1;
        mRefNum.mIndex &= 0x00ffffff;

        mIndexedRefId = esm.getHNString("NAME");

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

        mActorData.mHasACDT = false;
        if (esm.isNextSub("ACDT"))
        {
            mActorData.mHasACDT = true;
            esm.getHT(mActorData.mACDT);
        }

        mActorData.mHasACSC = false;
        if (esm.isNextSub("ACSC"))
        {
            mActorData.mHasACSC = true;
            esm.getHT(mActorData.mACSC);
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
                mActorData.mSelectedEnchantItem = esm.getHString();
            else
                mActorData.mSelectedSpell = id;

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
            esm.getHExact(mActorData.mSkills, 27*2*sizeof(int));

        if (esm.isNextSub("CRED")) // creature only
            esm.getHExact(mActorData.mCombatStats, 3*2*sizeof(int));

        mActorData.mSCRI.load(esm);

        if (esm.isNextSub("ND3D"))
            esm.skipHSub();

        mActorData.mHasANIS = false;
        if (esm.isNextSub("ANIS"))
        {
            mActorData.mHasANIS = true;
            esm.getHT(mActorData.mANIS);
        }

        if (esm.isNextSub("LVCR"))
        {
            // occurs on levelled creature spawner references
            // probably some identifier for the creature that has been spawned?
            unsigned char lvcr;
            esm.getHT(lvcr);
            //std::cout << "LVCR: " << (int)lvcr << std::endl;
        }

        mEnabled = true;
        esm.getHNOT(mEnabled, "ZNAM");

        // DATA should occur for all references, except levelled creature spawners
        // I've seen DATA *twice* on a creature record, and with the exact same content too! weird
        // alarmvoi0000.ess
        esm.getHNOTSized<24>(mPos, "DATA");
        esm.getHNOTSized<24>(mPos, "DATA");

        mDeleted = 0;
        if (esm.isNextSub("DELE"))
        {
            unsigned int deleted;
            esm.getHT(deleted);
            mDeleted = ((deleted >> 24) & 0x2) != 0; // the other 3 bytes seem to be uninitialized garbage
        }

        if (esm.isNextSub("MVRF"))
        {
            esm.skipHSub();
            esm.getSubName();
            esm.skipHSub();
        }
    }

}
