
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

        while (esm.isNextSub ("REAC"))
        {
            std::string faction2 = esm.getHString();
            int reaction;
            esm.getHNT(reaction, "INTV");

            mModFactionReaction[faction][faction2] = reaction;
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

    for (std::map<std::string, std::map<std::string, int> >::const_iterator iter = mModFactionReaction.begin();
         iter != mModFactionReaction.end(); ++iter)
    {
        esm.writeHNString ("FACT", iter->first);

        for (std::map<std::string, int>::const_iterator reactIter = iter->second.begin();
             reactIter != iter->second.end(); ++reactIter)
        {
            esm.writeHNString ("REAC", reactIter->first);
            esm.writeHNT ("INTV", reactIter->second);
        }
    }
}
