#include "importcellref.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

    void CellRef::load(ESM::ESMReader &esm)
    {
        blank();

        // (FRMR subrecord name is already read by the loop in ConvertCell)
        esm.getHT(mRefNum.mIndex); // FRMR

        // this is required since openmw supports more than 255 content files
        int pluginIndex = (mRefNum.mIndex & 0xff000000) >> 24;
        mRefNum.mContentFile = pluginIndex-1;
        mRefNum.mIndex &= 0x00ffffff;

        mIndexedRefId = esm.getHNString("NAME");

        ActorData::load(esm);
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
        esm.getHNOT(mPos, "DATA", 24);
        esm.getHNOT(mPos, "DATA", 24);

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
