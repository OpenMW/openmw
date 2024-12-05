#ifndef OPENMW_ESSIMPORT_NPCC_H
#define OPENMW_ESSIMPORT_NPCC_H

#include <components/esm3/aipackage.hpp>
#include <cstdint>

#include "importinventory.hpp"

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

    struct NPCC
    {
        struct NPDT
        {
            unsigned char mDisposition;
            unsigned char unknown;
            unsigned char mReputation;
            unsigned char unknown2;
            int32_t mIndex;
        } mNPDT;

        Inventory mInventory;
        ESM::AIPackageList mAiPackages;

        void load(ESM::ESMReader& esm);
    };

}

#endif
