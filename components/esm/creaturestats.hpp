#ifndef OPENMW_ESM_CREATURESTATS_H
#define OPENMW_ESM_CREATURESTATS_H

#include <string>
#include <vector>
#include <map>

#include "statstate.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only

    struct CreatureStats
    {
        StatState<int> mAttributes[8];
        StatState<float> mDynamic[3];

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };
}

#endif