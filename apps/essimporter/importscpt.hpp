#ifndef OPENMW_ESSIMPORT_IMPORTSCPT_H
#define OPENMW_ESSIMPORT_IMPORTSCPT_H

#include "importscri.hpp"

#include <components/esm/loadscpt.hpp>

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

    // A running global script
    struct SCPT
    {
        ESM::Script::SCHD mSCHD;

        // values of local variables
        SCRI mSCRI;

        bool mRunning;
        int mRefNum; // Targeted reference, -1: no reference

        void load(ESM::ESMReader& esm);
    };

}

#endif
