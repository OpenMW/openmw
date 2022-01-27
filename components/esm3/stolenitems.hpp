#ifndef OPENMW_COMPONENTS_ESM_STOLENITEMS_H
#define OPENMW_COMPONENTS_ESM_STOLENITEMS_H

#include <map>
#include <string>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only
    struct StolenItems
    {
        typedef std::map<std::string, std::map<std::pair<std::string, bool>, int> > StolenItemsMap;
        StolenItemsMap mStolenItems;

        void load(ESM::ESMReader& esm);
        void write(ESM::ESMWriter& esm) const;
    };

}

#endif
