#ifndef OPENMW_ESSIMPORT_CELLREF_H
#define OPENMW_ESSIMPORT_CELLREF_H

#include <string>

#include <components/esm/cellref.hpp>

#include "importacdt.hpp"

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

    // Not sure if we can share any code with ESM::CellRef here
    struct CellRef
    {
        std::string mIndexedRefId;
        ESM::RefNum mRefNum;

        ActorData mActorData;

        ESM::Position mPos;

        std::string mScript;

        bool mEnabled;

        void load(ESM::ESMReader& esm);
    };

}

#endif
