#include "importcellref.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

    void CellRef::load(ESM::ESMReader &esm)
    {
        // (FRMR subrecord name is already read by the loop in ConvertCell)
        esm.getHT(mRefNum.mIndex); // FRMR

        // this is required since openmw supports more than 255 content files
        int pluginIndex = (mRefNum.mIndex & 0xff000000) >> 24;
        mRefNum.mContentFile = pluginIndex-1;
        mRefNum.mIndex &= 0x00ffffff;

        mIndexedRefId = esm.getHNString("NAME");

        if (esm.isNextSub("LVCR"))
        {
            // occurs on leveled creature spawner references
            // probably some identifier for the the creature that has been spawned?
            unsigned char lvcr;
            esm.getHT(lvcr);
            //std::cout << "LVCR: " << (int)lvcr << std::endl;
        }
        mActorData.load(esm);

        mEnabled = true;
        esm.getHNOT(mEnabled, "ZNAM");

        // should occur for all references but not levelled creature spawners
        esm.getHNOT(mPos, "DATA", 24);

        // i've seen DATA record TWICE on a creature record - and with the exact same content too! weird
        // alarmvoi0000.ess
        esm.getHNOT(mPos, "DATA", 24);

        if (esm.isNextSub("MVRF"))
        {
            esm.skipHSub();
            esm.getSubName();
            esm.skipHSub();
        }
    }

}
