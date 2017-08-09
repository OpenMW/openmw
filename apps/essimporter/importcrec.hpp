#ifndef OPENMW_ESSIMPORT_CREC_H
#define OPENMW_ESSIMPORT_CREC_H

#include "importinventory.hpp"
#include <components/esm/aipackage.hpp>

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
        ESM::AIPackageList mAiPackages;

        void load(ESM::ESMReader& esm);
    };

}

#endif
