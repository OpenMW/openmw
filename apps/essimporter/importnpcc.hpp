#ifndef OPENMW_ESSIMPORT_NPCC_H
#define OPENMW_ESSIMPORT_NPCC_H

#include <components/esm/loadnpc.hpp>
#include <components/esm/loadcont.hpp>

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

    struct NPCC : public ESM::NPC
    {
        struct NPDT
        {
            unsigned char unknown[2];
            unsigned char mReputation;
            unsigned char unknown2[5];
        } mNPDT;

        void load(ESM::ESMReader &esm);
    };

}

#endif
