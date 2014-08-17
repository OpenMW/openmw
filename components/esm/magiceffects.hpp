#ifndef COMPONENTS_ESM_MAGICEFFECTS_H
#define COMPONENTS_ESM_MAGICEFFECTS_H

#include <map>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only
    struct MagicEffects
    {
        // <Effect Id, Base value>
        std::map<int, int> mEffects;

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };

}

#endif
