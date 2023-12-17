#ifndef OPENMW_ESSIMPORT_IMPORTDIAL_H
#define OPENMW_ESSIMPORT_IMPORTDIAL_H

#include <cstdint>

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

    struct DIAL
    {
        int32_t mIndex; // Journal index

        void load(ESM::ESMReader& esm);
    };

}

#endif
