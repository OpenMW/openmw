#include "dialoguestate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::DialogueState::load (ESMReader &esm)
{
    while (esm.isNextSub ("TOPI"))
        mKnownTopics.push_back (esm.getHString());

    while (esm.isNextSub ("FACT"))
    {
        std::string faction = esm.getHString();

        while (esm.isNextSub("REA2"))
        {
            std::string faction2 = esm.getHString();
            int reaction;
            esm.getHNT(reaction, "INTV");
            mChangedFactionReaction[faction][faction2] = reaction;
        }

        // no longer used
        while (esm.isNextSub ("REAC"))
        {
            esm.skipHSub();
            esm.getSubName();
            esm.skipHSub();
        }
    }
}

void ESM::DialogueState::save (ESMWriter &esm) const
{
    for (std::vector<std::string>::const_iterator iter (mKnownTopics.begin());
        iter!=mKnownTopics.end(); ++iter)
    {
        esm.writeHNString ("TOPI", *iter);
    }

    for (std::map<std::string, std::map<std::string, int> >::const_iterator iter = mChangedFactionReaction.begin();
         iter != mChangedFactionReaction.end(); ++iter)
    {
        esm.writeHNString ("FACT", iter->first);

        for (std::map<std::string, int>::const_iterator reactIter = iter->second.begin();
             reactIter != iter->second.end(); ++reactIter)
        {
            esm.writeHNString ("REA2", reactIter->first);
            esm.writeHNT ("INTV", reactIter->second);
        }
    }
}
