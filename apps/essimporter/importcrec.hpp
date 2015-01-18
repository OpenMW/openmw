#ifndef OPENMW_ESSIMPORT_CREC_H
#define OPENMW_ESSIMPORT_CREC_H

#include "importinventory.hpp"

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

    /// Creature changes
    struct CREC
    {
        int mIndex;

        Inventory mInventory;

        void load(ESM::ESMReader& esm);
    };

}

#endif
