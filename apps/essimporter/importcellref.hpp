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

    struct CellRef : public ActorData
    {
        std::string mIndexedRefId;

        std::string mScript;

        bool mEnabled;

        bool mDeleted;

        void load(ESM::ESMReader& esm) override;

        ~CellRef() override = default;
    };

}

#endif
