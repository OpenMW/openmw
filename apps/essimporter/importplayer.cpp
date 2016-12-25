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

        mHasMark = false;
        if (esm.isNextSub("MNAM"))
        {
            mHasMark = true;
            mMNAM = esm.getHString();
        }

        esm.getHNT(mPNAM, "PNAM");

        if (esm.isNextSub("SNAM"))
            esm.skipHSub();
        if (esm.isNextSub("NAM9"))
            esm.skipHSub();

        // Rest state. You shouldn't even be able to save during rest, but skip just in case.
        if (esm.isNextSub("RNAM"))
            /*
                int hoursLeft;
                float x, y, z; // resting position
            */
            esm.skipHSub(); // 16 bytes

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

        mHasENAM = false;
        if (esm.isNextSub("ENAM"))
        {
            mHasENAM = true;
            esm.getHT(mENAM);
        }

        if (esm.isNextSub("LNAM"))
            esm.skipHSub();

        while (esm.isNextSub("FNAM"))
        {
            FNAM fnam;
            esm.getHT(fnam);
            mFactions.push_back(fnam);
        }

        mHasAADT = false;
        if (esm.isNextSub("AADT")) // Attack animation data?
        {
            mHasAADT = true;
            esm.getHT(mAADT);
        }

        if (esm.isNextSub("KNAM"))
            esm.skipHSub(); // assigned Quick Keys, I think

        if (esm.isNextSub("ANIS"))
            esm.skipHSub(); // 16 bytes

        if (esm.isNextSub("WERE"))
        {
            // some werewolf data, 152 bytes
            // maybe current skills and attributes for werewolf form
            esm.getSubHeader();
            esm.skip(152);
        }
    }

}
