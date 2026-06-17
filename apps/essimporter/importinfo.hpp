#ifndef OPENMW_ESSIMPORT_IMPORTINFO_H
#define OPENMW_ESSIMPORT_IMPORTINFO_H

#include <string>

#include <components/esm/refid.hpp>

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

    struct INFO
    {
        ESM::RefId mInfo;
        std::string mActorRefId;

        void load(ESM::ESMReader& esm);
    };

}

#endif
