#ifndef OPENMW_ESSIMPORT_IMPORTCNTC_H
#define OPENMW_ESSIMPORT_IMPORTCNTC_H

#include "importinventory.hpp"

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

    /// Changed container contents
    struct CNTC
    {
        int mIndex;

        Inventory mInventory;

        void load(ESM::ESMReader& esm);
    };

}
#endif
