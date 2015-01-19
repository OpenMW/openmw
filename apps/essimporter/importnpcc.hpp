#ifndef OPENMW_ESSIMPORT_NPCC_H
#define OPENMW_ESSIMPORT_NPCC_H

#include <components/esm/loadcont.hpp>

#include <components/esm/aipackage.hpp>

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
            unsigned char unknown[2];
            unsigned char mReputation;
            unsigned char unknown2[5];
        } mNPDT;

        Inventory mInventory;

        void load(ESM::ESMReader &esm);
    };

}

#endif
