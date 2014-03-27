
#include "dialoguestate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::DialogueState::load (ESMReader &esm)
{
    while (esm.isNextSub ("TOPI"))
        mKnownTopics.push_back (esm.getHString());
}

void ESM::DialogueState::save (ESMWriter &esm) const
{
    for (std::vector<std::string>::const_iterator iter (mKnownTopics.begin());
        iter!=mKnownTopics.end(); ++iter)
    {
        esm.writeHNString ("TOPI", *iter);

    }
}