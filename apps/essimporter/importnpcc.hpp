#ifndef OPENMW_ESSIMPORT_NPCC_H
#define OPENMW_ESSIMPORT_NPCC_H

#include <components/esm/loadcont.hpp>

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

        struct InventoryItem
        {
            std::string mId;
            int mCondition;
        };
        std::vector<InventoryItem> mInventory;

        int mIndex;

        void load(ESM::ESMReader &esm);
    };

}

#endif
