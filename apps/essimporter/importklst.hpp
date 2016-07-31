#ifndef OPENMW_ESSIMPORT_KLST_H
#define OPENMW_ESSIMPORT_KLST_H

#include <string>
#include <map>

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
        std::map<std::string, int> mKillCounter;

        int mWerewolfKills;
    };

}

#endif
