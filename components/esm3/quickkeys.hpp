#ifndef OPENMW_COMPONENTS_ESM_QUICKKEYS_H
#define OPENMW_COMPONENTS_ESM_QUICKKEYS_H

#include <vector>
#include <string>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    struct QuickKeys
    {
        struct QuickKey
        {
            int mType;
            std::string mId; // Spell or Item ID
        };

        std::vector<QuickKey> mKeys;

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };

}

#endif
