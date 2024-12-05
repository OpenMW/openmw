#ifndef OPENMW_ESSIMPORT_KLST_H
#define OPENMW_ESSIMPORT_KLST_H

#include <cstdint>
#include <map>
#include <string>

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

    /// Kill Stats
    struct KLST
    {
        void load(ESM::ESMReader& esm);

        /// RefId, kill count
        std::map<std::string, int32_t> mKillCounter;

        int32_t mWerewolfKills;
    };

}

#endif
