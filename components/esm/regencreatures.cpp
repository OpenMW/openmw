#include "regencreatures.hpp"

#include "defs.hpp"

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>

namespace ESM
{


    void RegenCreatures::write(ESMWriter &esm) const
    {
        for (std::vector<RegenCreature>::const_iterator it = mRegenCreatures.begin(); it != mRegenCreatures.end(); ++it)
        {
            RegenCreature item = *it;
            esm.writeHNT("IDDD", item.mId);
            esm.writeHNT("TIME", item.mTimeStamp);
        }
    }

    void RegenCreatures::load(ESMReader &esm)
    {
        std::vector<RegenCreature> tRegenCreatures;
        while (esm.isNextSub("IDDD"))
        {
            RegenCreature rItem;
            esm.getHNT(rItem.mId, "IDDD");
            esm.getHNT(rItem.mTimeStamp, "TIME");
            tRegenCreatures.push_back(rItem);
        }
    }

}
