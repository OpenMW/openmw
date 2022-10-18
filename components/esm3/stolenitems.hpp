#ifndef OPENMW_COMPONENTS_ESM_STOLENITEMS_H
#define OPENMW_COMPONENTS_ESM_STOLENITEMS_H

#include <components/esm/refid.hpp>
#include <map>
#include <string>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only
    struct StolenItems
    {
        typedef std::map<ESM::RefId, std::map<std::pair<ESM::RefId, bool>, int>> StolenItemsMap;
        StolenItemsMap mStolenItems;

        void load(ESMReader& esm);
        void write(ESMWriter& esm) const;
    };

}

#endif
