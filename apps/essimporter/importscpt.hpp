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
    // TODO: test how targeted scripts are saved
    struct SCPT
    {
        ESM::Script::SCHD mSCHD;

        // values of local variables
        SCRI mSCRI;

        int mRNAM; // unknown, seems to be -1 for some scripts, some huge integer for others

        void load(ESM::ESMReader& esm);
    };

}

#endif
