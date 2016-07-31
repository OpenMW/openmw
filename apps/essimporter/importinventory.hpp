#ifndef OPENMW_ESSIMPORT_IMPORTINVENTORY_H
#define OPENMW_ESSIMPORT_IMPORTINVENTORY_H

#include <vector>
#include <string>

#include <components/esm/cellref.hpp>
#include "importscri.hpp"

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

    struct Inventory
    {
        struct InventoryItem : public ESM::CellRef
        {
            std::string mId;
            int mCount;
            int mRelativeEquipmentSlot;
            SCRI mSCRI;
        };
        std::vector<InventoryItem> mItems;

        void load(ESM::ESMReader& esm);
    };

}

#endif
