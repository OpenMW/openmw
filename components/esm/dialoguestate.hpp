#ifndef OPENMW_ESM_DIALOGUESTATE_H
#define OPENMW_ESM_DIALOGUESTATE_H

#include <string>
#include <vector>
#include <map>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only

    struct DialogueState
    {
        std::vector<std::string> mKnownTopics;

        std::map<std::string, std::map<std::string, int> > mModFactionReaction;

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };
}

#endif
