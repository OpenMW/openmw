#ifndef OPENMW_ESSIMPORT_CREC_H
#define OPENMW_ESSIMPORT_CREC_H

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

        void load(ESM::ESMReader& esm);
    };

}

#endif
