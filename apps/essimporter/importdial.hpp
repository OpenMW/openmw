#ifndef OPENMW_ESSIMPORT_IMPORTDIAL_H
#define OPENMW_ESSIMPORT_IMPORTDIAL_H
namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

    struct DIAL
    {
        int mIndex; // Journal index

        void load(ESM::ESMReader& esm);
    };

}

#endif
