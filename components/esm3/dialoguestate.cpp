#include "dialoguestate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void DialogueState::load(ESMReader& esm)
    {
        while (esm.isNextSub("TOPI"))
            mKnownTopics.push_back(esm.getRefId());

        while (esm.isNextSub("FACT"))
        {
            ESM::RefId faction = esm.getRefId();

            while (esm.isNextSub("REA2"))
            {
                ESM::RefId faction2 = esm.getRefId();
                int reaction;
                esm.getHNT(reaction, "INTV");
                mChangedFactionReaction[faction][faction2] = reaction;
            }
        }
    }

    void DialogueState::save(ESMWriter& esm) const
    {
        for (auto iter(mKnownTopics.begin()); iter != mKnownTopics.end(); ++iter)
        {
            esm.writeHNRefId("TOPI", *iter);
        }

        for (auto iter = mChangedFactionReaction.begin(); iter != mChangedFactionReaction.end(); ++iter)
        {
            esm.writeHNRefId("FACT", iter->first);

            for (auto reactIter = iter->second.begin(); reactIter != iter->second.end(); ++reactIter)
            {
                esm.writeHNRefId("REA2", reactIter->first);
                esm.writeHNT("INTV", reactIter->second);
            }
        }
    }

}
