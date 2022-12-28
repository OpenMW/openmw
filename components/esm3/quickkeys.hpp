#ifndef OPENMW_COMPONENTS_ESM_QUICKKEYS_H
#define OPENMW_COMPONENTS_ESM_QUICKKEYS_H

#include "components/esm/refid.hpp"
#include <string>
#include <vector>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    struct QuickKeys
    {
        struct QuickKey
        {
            int mType;
            RefId mId; // Spell or Item ID
        };

        std::vector<QuickKey> mKeys;

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };

}

#endif
