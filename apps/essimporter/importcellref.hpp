#ifndef OPENMW_ESSIMPORT_CELLREF_H
#define OPENMW_ESSIMPORT_CELLREF_H

#include <string>

#include <components/esm3/cellref.hpp>

#include "importacdt.hpp"

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

    struct CellRef : public ESM::CellRef
    {
        std::string mIndexedRefId;

        std::string mScript;

        bool mEnabled;

        bool mDeleted;

        ActorData mActorData;

        void load(ESM::ESMReader& esm);

        ~CellRef() = default;
    };

}

#endif
