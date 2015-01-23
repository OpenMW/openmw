#include "importplayer.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

    void REFR::load(ESM::ESMReader &esm)
    {
        esm.getHNT(mRefNum.mIndex, "FRMR");

        mRefID = esm.getHNString("NAME");

        mActorData.load(esm);

        esm.getHNOT(mPos, "DATA", 24);
    }

    void PCDT::load(ESM::ESMReader &esm)
    {
        while (esm.isNextSub("DNAM"))
        {
            mKnownDialogueTopics.push_back(esm.getHString());
        }

        if (esm.isNextSub("MNAM"))
            esm.skipHSub(); // If this field is here it seems to specify the interior cell the player is in,
                            // but it's not always here, so it's kinda useless

        esm.getHNT(mPNAM, "PNAM");

        if (esm.isNextSub("SNAM"))
            esm.skipHSub();
        if (esm.isNextSub("NAM9"))
            esm.skipHSub();

        mBounty = 0;
        esm.getHNOT(mBounty, "CNAM");

        mBirthsign = esm.getHNOString("BNAM");

        // Holds the names of the last used Alchemy apparatus. Don't need to import this ATM,
        // because our GUI auto-selects the best apparatus.
        if (esm.isNextSub("NAM0"))
            esm.skipHSub();
        if (esm.isNextSub("NAM1"))
            esm.skipHSub();
        if (esm.isNextSub("NAM2"))
            esm.skipHSub();
        if (esm.isNextSub("NAM3"))
            esm.skipHSub();

        if (esm.isNextSub("ENAM"))
            esm.skipHSub();

        if (esm.isNextSub("LNAM"))
            esm.skipHSub();

        while (esm.isNextSub("FNAM"))
        {
            FNAM fnam;
            esm.getHT(fnam);
            mFactions.push_back(fnam);
        }

        if (esm.isNextSub("AADT"))
            esm.skipHSub(); // 44 bytes, no clue

        if (esm.isNextSub("KNAM"))
            esm.skipHSub(); // assigned Quick Keys, I think

        if (esm.isNextSub("WERE"))
        {
            // some werewolf data, 152 bytes
            // maybe current skills and attributes for werewolf form
            esm.getSubHeader();
            esm.skip(152);
        }

        // unsure if before or after WERE
        if (esm.isNextSub("ANIS"))
            esm.skipHSub();
    }

}
