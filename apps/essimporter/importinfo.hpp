#ifndef OPENMW_ESSIMPORT_IMPORTINFO_H
#define OPENMW_ESSIMPORT_IMPORTINFO_H

#include <string>

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

    struct INFO
    {
        std::string mInfo;
        std::string mActorRefId;

        void load(ESM::ESMReader& esm);
    };

}

#endif
