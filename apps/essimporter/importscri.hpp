#ifndef OPENMW_ESSIMPORT_IMPORTSCRI_H
#define OPENMW_ESSIMPORT_IMPORTSCRI_H

#include <components/esm/variant.hpp>

#include <vector>

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

    /// Local variable assignments for a running script
    struct SCRI
    {
        std::string mScript;

        std::vector<short> mShorts;
        std::vector<int> mLongs;
        std::vector<float> mFloats;

        void load(ESM::ESMReader& esm);
    };

}

#endif
