#ifndef OPENMW_COMPONENTS_ESM_REGENCREATURES_H
#define OPENMW_COMPONENTS_ESM_REGENCREATURES_H

#include <vector>
#include <string>

#include "defs.hpp"
#include "../../apps/openmw/mwworld/timestamp.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    struct RegenCreature {
        unsigned int mId;
        TimeStamp mTimeStamp;
    };

    // format 0, saved games only
    struct RegenCreatures
    {
        std::vector<RegenCreature> mRegenCreatures;

        void load(ESM::ESMReader& esm);
        void write(ESM::ESMWriter& esm) const;
    };

}

#endif
